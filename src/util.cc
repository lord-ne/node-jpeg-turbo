#include "util.h"
#include <type_traits>

extern "C" {
  #include <jpegint.h> // CSTATE_START, DSTATE_START
}

BufferSizeOptions ParseBufferSizeOptions(const Napi::Env &env, const Napi::Object &options)
{
  Napi::Value tmpWidth = options.Get("width");
  if (!tmpWidth.IsNumber())
  {
    Napi::TypeError::New(env, "Invalid width").ThrowAsJavaScriptException();
    return BufferSizeOptions{false};
  }
  uint32_t width = tmpWidth.As<Napi::Number>().Uint32Value();

  Napi::Value tmpHeight = options.Get("height");
  if (!tmpHeight.IsNumber())
  {
    Napi::TypeError::New(env, "Invalid height").ThrowAsJavaScriptException();
    return BufferSizeOptions{false};
  }
  uint32_t height = tmpHeight.As<Napi::Number>().Uint32Value();

  uint32_t subsampling = NJT_DEFAULT_SUBSAMPLING;
  Napi::Value tmpSubsampling = options.Get("subsampling");
  if (!tmpSubsampling.IsUndefined())
  {
    if (!tmpSubsampling.IsNumber())
    {
      Napi::TypeError::New(env, "Invalid subsampling").ThrowAsJavaScriptException();
      return BufferSizeOptions{false};
    }
    subsampling = tmpSubsampling.As<Napi::Number>().Uint32Value();
  }

  switch (subsampling)
  {
  case TJSAMP_444:
  case TJSAMP_422:
  case TJSAMP_420:
  case TJSAMP_GRAY:
  case TJSAMP_440:
    break;
  default:
    Napi::TypeError::New(env, "Invalid subsampling").ThrowAsJavaScriptException();
    return BufferSizeOptions{false};
  }

  return BufferSizeOptions{true, width, height, subsampling};
}

#define ADDITIONAL_MESSAGE "jpeglib exited with an error: "
static constexpr std::size_t ADDITIONAL_MESSAGE_LENGTH = sizeof(ADDITIONAL_MESSAGE) - 1;

NAPI_NO_RETURN void ErrorExitThrow(j_common_ptr cinfo)
{
  char buffer[ADDITIONAL_MESSAGE_LENGTH + JMSG_LENGTH_MAX] = ADDITIONAL_MESSAGE;
  (*cinfo->err->format_message) (cinfo, buffer + ADDITIONAL_MESSAGE_LENGTH);

  jpeg_destroy(cinfo);

  throw JPEGLibError{buffer};
}

void SetupThrowingErrorManager(jpeg_error_mgr * err)
{
  jpeg_std_error(err);
  err->error_exit = ErrorExitThrow;
}

namespace internal
{
  template<typename C_OR_D>
  JHandle<C_OR_D>::DataHolder::DataHolder()
    : cinfo{}, jerr{}
  {}

  template<typename C_OR_D>
  JHandle<C_OR_D>::DataHolder::~DataHolder()
  {
    // If cinfo has already been destroyed, return
    if (this->cinfo.mem == nullptr) {
      return;
    }

    // We do this check to avoid calling abort if it has already been called on
    // cinfo, which may be unsafe
    if ((!this->cinfo.is_decompressor && this->cinfo.global_state != CSTATE_START)
      || (this->cinfo.is_decompressor && this->cinfo.global_state != DSTATE_START))
    {
      jpeg_abort(asJCommon(&this->cinfo));
    }

    jpeg_destroy(asJCommon(&this->cinfo));
  }

  template<typename C_OR_D>
  JHandle<C_OR_D>::JHandle()
    : data(std::make_unique<JHandle<C_OR_D>::DataHolder>())
  {}

  template<typename C_OR_D>
  C_OR_D * JHandle<C_OR_D>::cinfo()
  {
    return &this->data->cinfo;
  }

  template<typename C_OR_D>
  C_OR_D const * JHandle<C_OR_D>::cinfo() const
  {
    return &this->data->cinfo;
  }

  template<typename C_OR_D>
  jpeg_error_mgr * JHandle<C_OR_D>::jerr()
  {
    return &this->data->jerr;
  }

  template<typename C_OR_D>
  jpeg_error_mgr const * JHandle<C_OR_D>::jerr() const
  {
    return &this->data->jerr;
  }

  // Explicit instantiation
  template class JHandle<jpeg_compress_struct>;
  template class JHandle<jpeg_decompress_struct>;
} // namespace internal
