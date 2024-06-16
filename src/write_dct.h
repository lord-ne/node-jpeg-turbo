#ifndef NODE_JPEGTURBO_WRITE_DCT_H
#define NODE_JPEGTURBO_WRITE_DCT_H

#include "util.h"

Napi::Value WriteDCTAsync(const Napi::CallbackInfo &info);
Napi::Value WriteDCTSync(const Napi::CallbackInfo &info);

#endif
