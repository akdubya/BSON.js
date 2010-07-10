#ifndef _TYPES_H
#define	_TYPES_H

#include <v8.h>
#include <bson.h>

void InitTypes(v8::Handle<v8::Object> target);

namespace ObjectID {
    bool HasInstance(v8::Handle<v8::Value> obj);
    v8::Handle<v8::Value> New(bson_oid_t *oid);
}
namespace Code {
    bool HasInstance(v8::Handle<v8::Value> obj);
    v8::Handle<v8::Value> New(const char *code);
    v8::Handle<v8::Value> New(const char *code, v8::Handle<v8::Value> obj);
}
namespace Symbol {
    bool HasInstance(v8::Handle<v8::Value> obj);
    v8::Handle<v8::Value> New(const char *sym, int length = -1);
}
namespace MinKey { 
    v8::Handle<v8::Function> GetFunction();
}
namespace MaxKey {
    v8::Handle<v8::Function> GetFunction();
}
namespace Timestamp {
    bool HasInstance(v8::Handle<v8::Value> obj);
    v8::Handle<v8::Value> New(uint32_t incr, uint32_t ts);
}

#endif	/* _TYPES_H */