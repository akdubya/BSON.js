#include <node.h>
#include <v8.h>
#include <bson.h>
#include "types.h"

using namespace node;
using namespace v8;

#define PERSIST_TEMPLATE \
Persistent<FunctionTemplate> constructor_template; \
bool HasInstance(Handle<Value> val) { \
    if (!val->IsObject()) return false; \
    Local<Object> obj = val->ToObject(); \
    return constructor_template->HasInstance(obj); \
}

static Persistent<String> id_sym;
static Persistent<String> scope_sym;
static Persistent<String> code_sym;

namespace ObjectID {
    PERSIST_TEMPLATE

    Handle<Value> New(bson_oid_t *oid) {
        HandleScope scope;

        Local<Value> arg = External::New((void *)oid);
        Local<Object> b = constructor_template->GetFunction()->NewInstance(1, &arg);

        return scope.Close(b);
    }

    Handle<Value> ToUtf8String(const Arguments &args) {
        HandleScope scope;
        Local<Value> id = args.This()->Get(id_sym);
        return scope.Close(id);
    }
    
    Handle<Value> ToHexString(const Arguments &args) {
        HandleScope scope;
        Local<Value> id = args.This()->Get(id_sym);
        bson_oid_t oid;
        node::DecodeWrite(oid.bytes, 12, id, node::BINARY);
        char hex[25];
        bson_oid_to_string(&oid, hex);
        return scope.Close(String::New(hex));
    }
    
    Handle<Value> ToTimestamp(const Arguments &args) {
        HandleScope scope;
        Local<Value> id = args.This()->Get(id_sym);
        char buf[12];
        node::DecodeWrite(buf, 12, id, node::BINARY);
        time_t foo = bson_oid_generated_time((bson_oid_t *)buf);
        return Integer::New(foo);
    }

    Handle<Value> Generate() {
        bson_oid_t oid;
        bson_oid_gen(&oid);
        return node::Encode(oid.bytes, 12, node::BINARY);
    }

    Handle<Value> FromHexString(Local<String> hex) {
        bson_oid_t oid;
        bson_oid_from_string(&oid, (const char *) *String::Utf8Value(hex));
        return node::Encode(oid.bytes, 12, node::BINARY);
    }

    Handle<Value> FromExternal(Local<Value> val) {
        bson_oid_t *oid = (bson_oid_t *)External::Unwrap(val);
        return node::Encode(oid->bytes, 12, node::BINARY);
    }

    Handle<Value> New(const Arguments &args) {
        HandleScope scope;

        if (args.Length() == 0) {
            args.This()->Set(id_sym, Generate());
        } else if (args[0]->IsString() && args[0]->ToString()->Length() == 24) {
            args.This()->Set(id_sym, FromHexString(args[0]->ToString()));
        } else if (args[0]->IsExternal()) {
            args.This()->Set(id_sym, FromExternal(args[0]));
        } else {
            return ThrowException(Exception::TypeError(String::New("Invalid hex string")));
        }

        return args.This();
    }

    void Setup(Handle<Object> target) {
        HandleScope scope;

        Local<FunctionTemplate> t = FunctionTemplate::New(ObjectID::New);
        constructor_template = Persistent<FunctionTemplate>::New(t);
        constructor_template->SetClassName(String::NewSymbol("ObjectID"));

        NODE_SET_PROTOTYPE_METHOD(constructor_template, "inspect", ObjectID::ToHexString);
        NODE_SET_PROTOTYPE_METHOD(constructor_template, "toString", ObjectID::ToHexString);
        NODE_SET_PROTOTYPE_METHOD(constructor_template, "toUtf8String", ObjectID::ToUtf8String);
        NODE_SET_PROTOTYPE_METHOD(constructor_template, "toHexString", ObjectID::ToHexString);
        NODE_SET_PROTOTYPE_METHOD(constructor_template, "toTimestamp", ObjectID::ToTimestamp);

        target->Set(String::NewSymbol("ObjectID"), constructor_template->GetFunction());
    }
}

namespace Code {
    PERSIST_TEMPLATE

    Handle<Value> New(const char *code) {
        HandleScope scope;

        Local<Value> arg = String::New(code);
        Local<Object> b = constructor_template->GetFunction()->NewInstance(1, &arg);

        return scope.Close(b);
    }

    Handle<Value> New(const char *code, Handle<Value> obj) {
        HandleScope scope;

        Handle<Value> argv[2] = { String::New(code), obj };
        Local<Object> b = constructor_template->GetFunction()->NewInstance(2, argv);

        return scope.Close(b);
    }

    Handle<Value> New(const Arguments &args) {
        HandleScope scope;
        
        args.This()->Set(code_sym, args[0]);
        if (args.Length() > 1) {
            args.This()->Set(scope_sym, args[1]);
        }

        return args.This();
    }

    void Setup(Handle<Object> target) {
        HandleScope scope;

        Local<FunctionTemplate> t = FunctionTemplate::New(Code::New);
        constructor_template = Persistent<FunctionTemplate>::New(t);
        constructor_template->SetClassName(String::NewSymbol("Code"));

        target->Set(String::NewSymbol("Code"), constructor_template->GetFunction());
    }
}

namespace Symbol {
    PERSIST_TEMPLATE

    Handle<Value> New(const char *sym, int length) {
        HandleScope scope;

        Local<Value> arg = String::New(sym, length);
        Local<Object> b = constructor_template->GetFunction()->NewInstance(1, &arg);

        return scope.Close(b);
    }

    Handle<Value> New(const Arguments &args) {
        HandleScope scope;

        args.This()->Set(String::NewSymbol("string"), args[0]);

        return args.This();
    }

    void Setup(Handle<Object> target) {
        HandleScope scope;

        Local<FunctionTemplate> t = FunctionTemplate::New(Symbol::New);
        constructor_template = Persistent<FunctionTemplate>::New(t);
        constructor_template->SetClassName(String::NewSymbol("Symbol"));

        target->Set(String::NewSymbol("Symbol"), constructor_template->GetFunction());
    }
}

namespace Timestamp {
    PERSIST_TEMPLATE

    Handle<Value> New(uint32_t incr, uint32_t ts) {
        HandleScope scope;

        Handle<Value> argv[2] = { Integer::NewFromUnsigned(incr), Integer::NewFromUnsigned(ts) };
        Local<Object> t = constructor_template->GetFunction()->NewInstance(2, argv);

        return scope.Close(t);
    }

    Handle<Value> New(const Arguments &args) {
        HandleScope scope;

        args.This()->Set(String::NewSymbol("increment"), args[0]);
        args.This()->Set(String::NewSymbol("timestamp"), args[1]);

        return args.This();
    }

    void Setup(Handle<Object> target) {
        HandleScope scope;

        Local<FunctionTemplate> t = FunctionTemplate::New(Timestamp::New);
        constructor_template = Persistent<FunctionTemplate>::New(t);
        constructor_template->SetClassName(String::NewSymbol("Timestamp"));

        target->Set(String::NewSymbol("Timestamp"), constructor_template->GetFunction());
    }
}

namespace MinKey {
    Persistent<FunctionTemplate> constructor_template;

    Handle<Function> GetFunction() {
        return constructor_template->GetFunction();
    }

    Handle<Value> New(const Arguments &args) {
        HandleScope scope;
        return ThrowException(Exception::TypeError(String::New("MinKey cannot be instantiated")));
        return args.This();
    }

    void Setup(Handle<Object> target) {
        HandleScope scope;

        Local<FunctionTemplate> t = FunctionTemplate::New(MinKey::New);
        constructor_template = Persistent<FunctionTemplate>::New(t);
        constructor_template->SetClassName(String::NewSymbol("MinKey"));

        target->Set(String::NewSymbol("MinKey"), constructor_template->GetFunction());
    }
}

namespace MaxKey {
    Persistent<FunctionTemplate> constructor_template;

    Handle<Function> GetFunction() {
        return constructor_template->GetFunction();
    }

    Handle<Value> New(const Arguments &args) {
        HandleScope scope;
        return ThrowException(Exception::TypeError(String::New("MaxKey cannot be instantiated")));
        return args.This();
    }

    void Setup(Handle<Object> target) {
        HandleScope scope;

        Local<FunctionTemplate> t = FunctionTemplate::New(MaxKey::New);
        constructor_template = Persistent<FunctionTemplate>::New(t);
        constructor_template->SetClassName(String::NewSymbol("MaxKey"));

        target->Set(String::NewSymbol("MaxKey"), constructor_template->GetFunction());
    }
}

void InitTypes(Handle<Object> target) {
    HandleScope scope;
    
    id_sym = Persistent<String>::New(String::NewSymbol("id"));
    code_sym = Persistent<String>::New(String::NewSymbol("code"));
    scope_sym = Persistent<String>::New(String::NewSymbol("scope"));

    ObjectID::Setup(target);
    Code::Setup(target);
    Symbol::Setup(target);
    MinKey::Setup(target);
    MaxKey::Setup(target);
    Timestamp::Setup(target);
}