#include "util.h"
#include "enums.h"
#include "buffersize.h"
#include "compress.h"
#include "decompress.h"
#include "read_dct.h"
#include "write_dct.h"

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  // FT_Init_FreeType(&library);

  // sprintf(version, "%i.%i.%i", FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH);
  // exports.Set("FreeTypeVersion", version);

  exports.Set("bufferSize", Napi::Function::New(env, BufferSize));
  exports.Set("compress", Napi::Function::New(env, CompressAsync));
  exports.Set("compressSync", Napi::Function::New(env, CompressSync));
  exports.Set("decompress", Napi::Function::New(env, DecompressAsync));
  exports.Set("decompressSync", Napi::Function::New(env, DecompressSync));
  exports.Set("readDCT", Napi::Function::New(env, ReadDCTAsync));
  exports.Set("readDCTSync", Napi::Function::New(env, ReadDCTSync));
  exports.Set("writeDCT", Napi::Function::New(env, WriteDCTAsync));
  exports.Set("writeDCTSync", Napi::Function::New(env, WriteDCTSync));

  InitializeEnums(env, exports);

  return exports;
}

NODE_API_MODULE(jpegturbo, Init)
