#include "read_dct.h"
#include <array>

struct ComponentInfo
{
  int resWidth;
  int resHeight;
  JCOEF *resData;
};

struct ReadDCTProps
{
  jpeg_decompress_struct* cinfo;
  std::array<ComponentInfo, 4> components;
};

Napi::Object ReadDCTResult(const Napi::Env &env, std::vector<Napi::Int16Array>&& dstBuffers, const ReadDCTProps &props)
{
  Napi::Object res = Napi::Object::New(env);
  const char* componentNames[] = {"Y", "Cb", "Cr", "K"};

  for (int i = 0; i < dstBuffers.size(); ++i)
  {
    auto comp = Napi::Object::New(env);
    comp.Set("width", props.components[i].resWidth);
    comp.Set("height", props.components[i].resHeight);
    comp.Set("data", std::move(dstBuffers[i]));
    res.Set(componentNames[i], std::move(comp));
  }

  return res;
}

void DoReadDCT(ReadDCTProps &props)
{
  jvirt_barray_ptr *coeffsArray = jpeg_read_coefficients(props.cinfo);

  for (int chan = 0; chan < props.cinfo->num_components; ++chan)
  {
    ComponentInfo& comp = props.components[chan];
    if (comp.resData == nullptr)
    {
      continue;
    }

    JBLOCK** buf = props.cinfo->mem->access_virt_barray(
      reinterpret_cast<j_common_ptr>(props.cinfo),
      coeffsArray[chan],
      0, comp.resHeight, false);

    for (int row = 0; row < comp.resHeight; ++row)
    {
      for (int col = 0; col < comp.resWidth; ++col)
      {
        JCOEF* block = buf[row][col];
        memcpy(comp.resData, block, sizeof(JCOEF) * DCTSIZE2);
      }
    }
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

  ReadDCTProps props = {};
  std::vector<Napi::Int16Array> buffers{};

  auto cinfo = std::make_unique<jpeg_decompress_struct>();
  auto jerr = std::make_unique<jpeg_error_mgr>();
  jpeg_std_error(jerr.get());
  cinfo->err = jerr.get();

  jpeg_create_decompress(cinfo.get());

  jpeg_mem_src(cinfo.get(), srcBuffer.Data(), srcBuffer.Length());
  jpeg_read_header(cinfo.get(), true);

  for (int chan = 0; chan < cinfo->num_components; ++chan)
  {
    ComponentInfo& comp = props.components[chan];
    jpeg_component_info& compInfo = cinfo->comp_info[chan];

    comp.resHeight = compInfo.height_in_blocks;
    comp.resWidth = compInfo.width_in_blocks;

    buffers.push_back(Napi::TypedArrayOf<JCOEF>::New(
      info.Env(),
      comp.resHeight * comp.resWidth * DCTSIZE2));

    comp.resData = buffers.back().Data();
  }

  if (async)
  {
    // TODO
    return info.Env().Null();
  }
  else
  {
    DoReadDCT(props);
    return ReadDCTResult(info.Env(), std::move(buffers), props);
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
