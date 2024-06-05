#ifndef NODE_JPEGTURBO_READ_DCT_H
#define NODE_JPEGTURBO_READ_DCT_H

#include "util.h"

Napi::Value ReadDCTAsync(const Napi::CallbackInfo &info);
Napi::Value ReadDCTSync(const Napi::CallbackInfo &info);

#endif
