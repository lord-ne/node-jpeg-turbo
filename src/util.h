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

// A catch block that rethrows non-Napi excceptions as Napi exceptions
#define RETHROW_EXCEPTIONS_AS_JS_EXCEPTIONS(Env)      \
  catch (Napi::Error const&)                          \
  {                                                   \
    throw;                                            \
  }                                                   \
  catch(std::exception const& e)                      \
  {                                                   \
    throw Napi::Error::New(Env, e.what());            \
  }

namespace internal
{
  template<typename COMPRESS_OR_DECOMPRESS_STRUCT>
  class JHandle
  {
    struct DataHolder
    {
      COMPRESS_OR_DECOMPRESS_STRUCT cinfo;
      jpeg_error_mgr jerr;

      DataHolder();
      ~DataHolder();

      DataHolder(DataHolder&&) = delete;
      DataHolder& operator=(DataHolder&&) = delete;
      DataHolder(DataHolder const&) = delete;
      DataHolder& operator=(DataHolder const&) = delete;
    };

    std::unique_ptr<DataHolder> data;
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

// Call jpeg_abort and jpeg_destroy on cinfo, if they have not already been called
void abortAndDestroy(j_common_ptr cinfo);

// Hold a unique_ptr to a jpeg_(de)compress_struct and jpeg_error_mgr.
// These templates are explicitly instantiated in util.cc
using JCompressHandle = internal::JHandle<jpeg_compress_struct>;
using JDecompressHandle = internal::JHandle<jpeg_decompress_struct>;

inline jpeg_common_struct * asJCommon(jpeg_compress_struct * in) {
  return reinterpret_cast<jpeg_common_struct *>(in);
}

inline jpeg_common_struct * asJCommon(jpeg_decompress_struct * in) {
  return reinterpret_cast<jpeg_common_struct *>(in);
}

inline jpeg_common_struct const * asJCommon(jpeg_compress_struct const * in) {
  return reinterpret_cast<jpeg_common_struct const *>(in);
}

inline jpeg_common_struct const * asJCommon(jpeg_decompress_struct const * in) {
  return reinterpret_cast<jpeg_common_struct const *>(in);
}

#endif
