#include "read_dct.h"
#include <cstdint>
#include <array>
#include <algorithm>

#define TRACE_PRINT() printf("%s:%d\n\n\n\n\n", __FILE__, __LINE__);

struct JHandle
{
  jpeg_decompress_struct cinfo;
  jpeg_error_mgr jerr;
};

struct ComponentInfo
{
  bool componentExists;
  std::size_t dataOffsetBytes;
  std::size_t dataLengthElements;
  int width;
  int height;

  uint8_t resQuantTableNum;
};

struct QuantTableInfo
{
  std::size_t dataOffsetBytes;
  std::size_t dataLengthElements;

  bool resTableExists;
};

struct ReadDCTProps
{
  std::unique_ptr<JHandle> handle;
  uint8_t* resData;
  std::array<ComponentInfo, MAX_COMPS_IN_SCAN> components;
  std::array<QuantTableInfo, NUM_QUANT_TBLS> quantTables;
};

Napi::Object ReadDCTResult(const Napi::Env &env,
Napi::Buffer<uint8_t> const& buffer,
ReadDCTProps const& props)
{
  Napi::Object res = Napi::Object::New(env);
  const char* componentNames[] = {"Y", "Cb", "Cr", "K"};

  for (int i = 0; i < MAX_COMPS_IN_SCAN; ++i)
  {
    res.Set("buffer", buffer);

    auto const& propComp = props.components[i];
    if (!propComp.componentExists)
    {
      continue;
    }

    auto comp = Napi::Object::New(env);
    comp.Set("width", propComp.width);
    comp.Set("height", propComp.height);
    comp.Set("qt_no", propComp.resQuantTableNum);
    comp.Set("data_offset_bytes", propComp.dataOffsetBytes);
    comp.Set("data_length_elements", propComp.dataLengthElements);

    res.Set(componentNames[i], comp);
  }

  std::size_t length = NUM_QUANT_TBLS;
  while(length > 0 && !props.quantTables[length - 1].resTableExists)
  {
    length--;
  }
  auto quants = Napi::Array::New(env, length);
  for(std::size_t i = 0; i < length; i++)
  {
    auto const& propQuant = props.quantTables[i];
    if (!propQuant.resTableExists)
    {
      quants[i] = env.Null();
      continue;
    }

    auto quant = Napi::Object::New(env);
    quant.Set("data_offset_bytes", propQuant.dataOffsetBytes);
    quant.Set("data_length_elements", propQuant.dataLengthElements);
    quants[i] = quant;
  }
  res.Set("qts", quants);

  return res;
}

void DoReadDCT(ReadDCTProps &props)
{
  auto& cinfo = props.handle->cinfo;
  jvirt_barray_ptr *coeffsArray = jpeg_read_coefficients(&cinfo);
  for (int chan = 0; chan < cinfo.num_components; ++chan)
  {
    ComponentInfo& comp = props.components[chan];
    if (!comp.componentExists)
    {
      continue;
    }

    comp.resQuantTableNum = cinfo.comp_info[chan].quant_tbl_no;
    uint8_t* outputBuf = props.resData + comp.dataOffsetBytes;

    for (int row = 0; row < comp.height; ++row)
    {
      // It's possible to access multiple rows at once, but there doesn't seem
      // to be any way to read the maxaccess field that tells us how many we
      // can read.
      JBLOCK* buf = *cinfo.mem->access_virt_barray(
        reinterpret_cast<j_common_ptr>(&cinfo),
        coeffsArray[chan],
        row, 1, false);

      for (int col = 0; col < comp.width; ++col)
      {
        JCOEF* block = buf[col];
        constexpr std::size_t readSize = sizeof(JCOEF) * DCTSIZE2;
        memcpy(outputBuf, block, readSize);
        outputBuf += readSize;
      }
    }
  }

  for (int i = 0; i < NUM_QUANT_TBLS; ++i)
  {
    auto* tablePtr = cinfo.quant_tbl_ptrs[i];
    if (tablePtr == nullptr)
    {
      props.quantTables[i].resTableExists = false;
      continue;
    }

    memcpy(props.resData + props.quantTables[i].dataOffsetBytes, tablePtr->quantval, DCTSIZE2 * sizeof(UINT16));
    props.quantTables[i].resTableExists = true;
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
}

class ReadDCTWorker : public Napi::AsyncWorker
{
public:
  ReadDCTWorker(
      Napi::Env& env,
      Napi::Buffer<uint8_t>& srcBuffer,
      Napi::Buffer<uint8_t>& dstBuffer,
      ReadDCTProps&& props)
      : AsyncWorker(env),
        deferred(Napi::Promise::Deferred::New(env)),
        srcBuffer(Napi::Reference<Napi::Buffer<uint8_t>>::New(srcBuffer, 1)),
        dstBuffer(Napi::Reference<Napi::Buffer<uint8_t>>::New(dstBuffer, 1)),
        props(std::move(props))
  {
  }

  ~ReadDCTWorker()
  {
    this->srcBuffer.Reset();
    this->dstBuffer.Reset();
  }

  void Execute()
  {
    DoReadDCT(this->props);
  }

  void OnOK()
  {
    deferred.Resolve(ReadDCTResult(Env(), this->dstBuffer.Value(), this->props));
  }

  void OnError(Napi::Error const &error)
  {
    deferred.Reject(error.Value());
  }

  Napi::Promise GetPromise() const
  {
    return deferred.Promise();
  }

private:
  Napi::Promise::Deferred deferred;
  Napi::Reference<Napi::Buffer<uint8_t>> srcBuffer;
  Napi::Reference<Napi::Buffer<uint8_t>> dstBuffer;
  ReadDCTProps props;
};

Napi::Value ReadDCTInner(const Napi::CallbackInfo &info, bool async)
{
  if (info.Length() < 1)
  {
    Napi::TypeError::New(info.Env(), "Not enough arguments")
        .ThrowAsJavaScriptException();
    return info.Env().Null();
  }

  if (!info[0].IsBuffer())
  {
    Napi::TypeError::New(info.Env(), "Invalid source buffer")
        .ThrowAsJavaScriptException();
    return info.Env().Null();
  }
  Napi::Buffer<uint8_t> srcBuffer = info[0].As<Napi::Buffer<uint8_t>>();

  bool bufferProvided = ((info.Length() > 1) && (info[1].IsBuffer()));

  auto handle = std::make_unique<JHandle>();
  jpeg_std_error(&handle->jerr);
  handle->cinfo.err = &handle->jerr;

  jpeg_create_decompress(&handle->cinfo);

  jpeg_mem_src(&handle->cinfo, srcBuffer.Data(), srcBuffer.ByteLength());
  jpeg_read_header(&handle->cinfo, true);

  ReadDCTProps props = {};

  std::size_t bufLengthBytes = 0;

  for (int chan = 0; chan < handle->cinfo.num_components; ++chan)
  {
    ComponentInfo& comp = props.components[chan];
    jpeg_component_info& compInfo = handle->cinfo.comp_info[chan];

    comp.componentExists = true;
    comp.width = compInfo.width_in_blocks;
    comp.height = compInfo.height_in_blocks;
    comp.dataOffsetBytes = bufLengthBytes;
    comp.dataLengthElements = comp.height * comp.width * DCTSIZE2;

    bufLengthBytes += comp.dataLengthElements * sizeof(JCOEF);
  }

  for (int i = 0; i < NUM_QUANT_TBLS; ++i)
  {
    QuantTableInfo& qt = props.quantTables[i];
    qt.dataOffsetBytes = bufLengthBytes;
    qt.dataLengthElements = DCTSIZE2;
    bufLengthBytes += qt.dataLengthElements * sizeof(UINT16);
  }

  Napi::Buffer<uint8_t> dstBuffer = bufferProvided ?
    info[1].As<Napi::Buffer<uint8_t>>()
    : Napi::Buffer<uint8_t>::New(info.Env(), bufLengthBytes);

  if (bufferProvided && dstBuffer.ByteLength() < bufLengthBytes)
  {
    Napi::TypeError::New(info.Env(), "Insufficient output buffer")
      .ThrowAsJavaScriptException();
    return info.Env().Null();
  }

  props.resData = dstBuffer.Data();

  props.handle = std::move(handle);

  if (async)
  {
    auto* wk = new ReadDCTWorker(info.Env(),
      srcBuffer, dstBuffer, std::move(props));
    wk->Queue();
    return wk->GetPromise();
  }
  else
  {
    DoReadDCT(props);
    return ReadDCTResult(info.Env(), dstBuffer, props);
  }
}

Napi::Value ReadDCTAsync(const Napi::CallbackInfo &info)
{
  return ReadDCTInner(info, true);
}

Napi::Value ReadDCTSync(const Napi::CallbackInfo &info)
{
  return ReadDCTInner(info, false);
}
