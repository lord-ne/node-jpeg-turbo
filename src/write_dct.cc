#include "write_dct.h"
#include <cstdint>
#include <array>

Napi::Value WriteDCTInner(Napi::CallbackInfo const& info, bool async)
{
  return info.Env().Null();
}

Napi::Value WriteDCTAsync(Napi::CallbackInfo const& info)
{
  try {
    return WriteDCTInner(info, true);
  } RETHROW_EXCEPTIONS_AS_JS_EXCEPTIONS(info.Env())
}

Napi::Value WriteDCTSync(Napi::CallbackInfo const& info)
{
  try {
    return WriteDCTInner(info, false);
  } RETHROW_EXCEPTIONS_AS_JS_EXCEPTIONS(info.Env())
}
