#include "read_dct.h"
#include <cstdint>
#include <array>
#include <algorithm>

struct ComponentInfo
{
  int resWidth;
  int resHeight;
  JCOEF *resData;
  uint8_t resQuantTableNum;
};

struct ReadDCTProps
{
  jpeg_decompress_struct* cinfo;
  std::array<ComponentInfo, 4> components;
  std::array<uint16_t*, 4> resQuantTables;
};

Napi::Object ReadDCTResult(const Napi::Env &env,
std::vector<Napi::Int16Array>&& dstBuffers,
std::vector<Napi::Uint16Array>&& quantBuffers,
const ReadDCTProps &props)
{
  Napi::Object res = Napi::Object::New(env);
  const char* componentNames[] = {"Y", "Cb", "Cr", "K"};

  for (int i = 0; i < dstBuffers.size(); ++i)
  {
    auto comp = Napi::Object::New(env);
    comp.Set("width", props.components[i].resWidth);
    comp.Set("height", props.components[i].resHeight);
    comp.Set("qt_no", props.components[i].resQuantTableNum);
    comp.Set("data", std::move(dstBuffers[i]));

    res.Set(componentNames[i], std::move(comp));
  }

  std::size_t length = 4;
  while(length > 0 && props.resQuantTables[length - 1] == nullptr)
  {
    length--;
  }
  auto quant = Napi::Array::New(env, length);
  for(std::size_t i = 0; i < length; i++)
  {
    quant[i] = std::move(quantBuffers[i]);
  }
  res.Set("qts", std::move(quant));

  return res;
}

void DoReadDCT(ReadDCTProps &props)
{
  jvirt_barray_ptr *coeffsArray = jpeg_read_coefficients(props.cinfo);

  for (int chan = 0; chan < props.cinfo->num_components; ++chan)
  {
    ComponentInfo& comp = props.components[chan];

    comp.resQuantTableNum = props.cinfo->comp_info[chan].quant_tbl_no;

    if (comp.resData == nullptr)
    {
      continue;
    }

    unsigned char* outputBuf = reinterpret_cast<unsigned char*>(comp.resData);

    for (int row = 0; row < comp.resHeight; ++row)
    {
      // It's possible to access multiple rows at once, but there doesn't seem
      // to be any way to read the maxaccess field that tells us how many we
      // can read.
      JBLOCK* buf = *props.cinfo->mem->access_virt_barray(
        reinterpret_cast<j_common_ptr>(props.cinfo),
        coeffsArray[chan],
        row, 1, false);

      for (int col = 0; col < comp.resWidth; ++col)
      {
        JCOEF* block = buf[col];
        constexpr std::size_t readSize = sizeof(JCOEF) * DCTSIZE2;
        memcpy(outputBuf, block, readSize);
        outputBuf += readSize;
      }
    }
  }

  for (int i = 0; i < 4; ++i)
  {
    auto* tablePtr = props.cinfo->quant_tbl_ptrs[i];
    if (tablePtr == nullptr || props.resQuantTables[i] == nullptr)
    {
      tablePtr == nullptr;
      continue;
    }
    memcpy(props.resQuantTables[i], tablePtr->quantval, DCTSIZE2);
  }

  jpeg_finish_decompress(props.cinfo);
  jpeg_destroy_decompress(props.cinfo);
}

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
  Napi::Buffer<unsigned char> srcBuffer = info[0].As<Napi::Buffer<unsigned char>>();

  std::vector<Napi::Int16Array> dataBuffers{};
  std::vector<Napi::Uint16Array> quantBuffers{};

  auto cinfo = std::make_unique<jpeg_decompress_struct>();
  auto jerr = std::make_unique<jpeg_error_mgr>();
  jpeg_std_error(jerr.get());
  cinfo->err = jerr.get();

  jpeg_create_decompress(cinfo.get());

  jpeg_mem_src(cinfo.get(), srcBuffer.Data(), srcBuffer.Length());
  jpeg_read_header(cinfo.get(), true);

  ReadDCTProps props = {};
  props.cinfo = cinfo.get();

  for (int i = 0; i < 4; ++i)
  {
    quantBuffers.push_back(Napi::TypedArrayOf<UINT16>::New(info.Env(), DCTSIZE2));
  }

  for (int chan = 0; chan < cinfo->num_components; ++chan)
  {
    ComponentInfo& comp = props.components[chan];
    jpeg_component_info& compInfo = cinfo->comp_info[chan];

    comp.resHeight = compInfo.height_in_blocks;
    comp.resWidth = compInfo.width_in_blocks;

    dataBuffers.push_back(Napi::TypedArrayOf<JCOEF>::New(
      info.Env(),
      comp.resHeight * comp.resWidth * DCTSIZE2));

    comp.resData = dataBuffers.back().Data();
  }

  if (async)
  {
    // TODO
    return info.Env().Null();
  }
  else
  {
    DoReadDCT(props);
    return ReadDCTResult(info.Env(),std::move(dataBuffers), std::move(quantBuffers), props);
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
