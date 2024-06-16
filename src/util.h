#ifndef NODE_JPEGTURBO_UTIL_H
#define NODE_JPEGTURBO_UTIL_H

#include <napi.h>
#include <stdexcept>

#include <turbojpeg.h>
extern "C" {
  #include <jpeglib.h>
}
#include "consts.h"

// Napi::Value getProperty(const Napi::Object &obj, const char *name);

struct BufferSizeOptions
{
  bool valid;
  uint32_t width;
  uint32_t height;
  uint32_t subsampling;
};

BufferSizeOptions ParseBufferSizeOptions(const Napi::Env &env, const Napi::Object &obj);

#ifndef NAPI_CPP_EXCEPTIONS
#error "NAPI C++ exception support must be enabled"
#endif
class JPEGLibError : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};

void SetupThrowingErrorManager(jpeg_error_mgr * err);

#define RETHROW_EXCEPTIONS_AS_JS_EXCEPTIONS(Env, ...) \
{                                                     \
  try                                                 \
  {                                                   \
    ;__VA_ARGS__;                                     \
  }                                                   \
  catch (Napi::Error const&)                          \
  {                                                   \
    throw;                                            \
  }                                                   \
  catch(std::exception const& e)                      \
  {                                                   \
    throw Napi::Error::New(Env, e.what());            \
  }                                                   \
}

namespace internal
{
  template<typename COMPRESS_OR_DECOMPRESS_STRUCT>
  class JHandle
  {
    struct JHandleDataHolder
    {
      COMPRESS_OR_DECOMPRESS_STRUCT cinfo;
      jpeg_error_mgr jerr;
    };

    std::unique_ptr<JHandleDataHolder> data;
  public:
    JHandle();
    JHandle(JHandle&&) = default;
    JHandle& operator=(JHandle&&) = default;
    ~JHandle() = default;
    COMPRESS_OR_DECOMPRESS_STRUCT * cinfo();
    COMPRESS_OR_DECOMPRESS_STRUCT const * cinfo() const;
    jpeg_error_mgr * jerr();
    jpeg_error_mgr const * jerr() const;
  };
}

// Hold a unique_ptr to a jpeg_(de)compress_struct and jpeg_error_mgr.
// These templates are explicitly instantiated in util.cc
using JCompressHandle = internal::JHandle<jpeg_compress_struct>;
using JDecompressHandle = internal::JHandle<jpeg_decompress_struct>;

#endif
