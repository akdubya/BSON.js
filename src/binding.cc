#include "binding.h"
#include "encode.h"
#include "decode.h"
#include "types.h"

#include <v8.h>
#include <node.h>

using namespace v8;

extern "C" void
init (Handle<Object> target) {
    HandleScope scope;
    InitEncoder(target);
    InitDecoder(target);
    InitTypes(target);
}