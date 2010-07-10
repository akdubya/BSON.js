#include "decode.h"
#include "types.h"

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <string.h>
#include <stdlib.h>
#include <bson.h>

using namespace v8;
using namespace node;
using namespace std;

static Persistent<String> regex_sym;
static Persistent<String> bson_type_sym;
static Persistent<String> scope_sym;

typedef struct {
    Local<Object> stack[32];
    int stackPos;
} bson_context;

int OnDocumentStart(void *ctx, const char *e_name) {
    bson_context *foo = (bson_context *)ctx;
    Local<Object> obj = Object::New();
    foo->stack[foo->stackPos]->Set(String::New(e_name), obj);
    foo->stackPos += 1;
    foo->stack[foo->stackPos] = obj;
    return 1;
}

int OnDocumentEnd(void *ctx) {
    bson_context *foo = (bson_context *)ctx;
    if (foo->stackPos > 0) foo-> stackPos -= 1;
    return 1;
}

int OnArrayStart(void *ctx, const char *e_name) {
    bson_context *foo = (bson_context *)ctx;
    Local<Array> arr = Array::New();
    foo->stack[foo->stackPos]->Set(String::New(e_name), arr);
    foo->stackPos += 1;
    foo->stack[foo->stackPos] = arr;
    return 1;
}

int OnArrayEnd(void *ctx) {
    bson_context *foo = (bson_context *)ctx;
    if (foo->stackPos > 0) foo-> stackPos -= 1;
    return 1;
}

int OnFloat(void *ctx, const char *e_name, double val) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), Number::New(val));
    return 1;
}

int OnString(void *ctx, const char *e_name, const char *str, int len) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), String::New(str, len));
    return 1;
}

int OnBinary(void *ctx, const char *e_name, const char *data, unsigned char subtype, int len) {
    bson_context *foo = (bson_context *)ctx;
    node::Buffer *obj = node::Buffer::New(len);
    memcpy((void *)obj->data(), (const void *)data, obj->length());
    obj->handle_->Set(bson_type_sym, Integer::New(subtype));
    foo->stack[foo->stackPos]->Set(String::New(e_name), obj->handle_);
    return 1;
}

int OnUndefined(void *ctx, const char *e_name) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), Undefined());
    return 1;
}

int OnObjectID(void *ctx, const char *e_name, bson_oid_t *oid) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), ObjectID::New(oid));
    return 1;
}

int OnBoolean(void *ctx, const char *e_name, int val) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), Boolean::New(val));
    return 1;
}

int OnDatetime(void *ctx, const char *e_name, int64_t date) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), Date::New(date));
    return 1;
}

int OnNull(void *ctx, const char *e_name) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), Null());
    return 1;
}

int OnRegex(void *ctx, const char *e_name, const char *pattern, const char *options) {
    bson_context *foo = (bson_context *)ctx;
    Handle<Value> val = Context::GetCurrent()->Global()->Get(regex_sym);
    Handle<Function> fn = Handle<Function>::Cast(val);
    Handle<Value> args[] = {
        String::New(pattern),
        String::New(options)
    };
    Local<Object> obj = fn->NewInstance(2, args);
    foo->stack[foo->stackPos]->Set(String::New(e_name), obj);
    return 1;
}

int OnCode(void *ctx, const char *e_name, const char *code, int len) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), Code::New(code));
    return 1;
}

int OnSymbol(void *ctx, const char *e_name, const char *str, int len) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), Symbol::New(str, len));
    return 1;
}

int OnCodeScope(void *ctx, const char *e_name, const char *code, int len) {
    bson_context *foo = (bson_context *)ctx;
    Local<Object> scope = Object::New();
    foo->stack[foo->stackPos]->Set(String::New(e_name), Code::New(code, scope));
    foo->stackPos += 1;
    foo->stack[foo->stackPos] = scope;
    return 1;
}

int OnInteger32(void *ctx, const char *e_name, int val) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), Integer::New(val));
    return 1;
}

int OnTimestamp(void *ctx, const char *e_name, int incr, int ts) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), Timestamp::New(incr, ts));
    return 1;
}

int OnInteger64(void *ctx, const char *e_name, int64_t val) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), Number::New((double)val));
    return 1;
}

int OnMinKey(void *ctx, const char *e_name) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), MinKey::GetFunction());
    return 1;
}

int OnMaxKey(void *ctx, const char *e_name) {
    bson_context *foo = (bson_context *)ctx;
    foo->stack[foo->stackPos]->Set(String::New(e_name), MaxKey::GetFunction());
    return 1;
}

static bson_parser_callbacks cbs = {
    OnDocumentStart,
    OnDocumentEnd,
    OnArrayStart,
    OnArrayEnd,
    OnFloat,
    OnString,
    OnBinary,
    OnUndefined,
    OnObjectID,
    OnBoolean,
    OnDatetime,
    OnNull,
    OnRegex,
    OnCode,
    OnSymbol,
    OnCodeScope,
    OnInteger32,
    OnTimestamp,
    OnInteger64,
    OnMinKey,
    OnMaxKey
};

Handle<Value> decode(const Arguments &args) {
    HandleScope scope;
    if (!args[0]->IsString()) {
        return ThrowException(Exception::TypeError(String::New("Value to decode must be a string")));
    }
    Local<String> str = args[0]->ToString();
    size_t buflen = str->Length();
    char *buf = (char *)malloc(buflen);
    node::DecodeWrite(buf, buflen, str, node::BINARY);

    bson_context ctx;
    ctx.stackPos = 0;
    ctx.stack[0] = Object::New();
    bson_parser parser = bson_init_parser(buf, buflen, &cbs, &ctx);

    Handle<Value> retval;
    if (!bson_parse(&parser)) {
        retval = ThrowException(Exception::Error(String::New("BSON Parse Error")));
    } else {
        retval = ctx.stack[ctx.stackPos];
    }
    free(buf);

    return scope.Close(retval);
}

void InitDecoder(Handle<Object> target) {
    HandleScope scope;

    regex_sym = Persistent<String>::New(String::NewSymbol("RegExp"));
    bson_type_sym = Persistent<String>::New(String::NewSymbol("bsonType"));
    scope_sym = Persistent<String>::New(String::NewSymbol("scope"));

    target->Set(String::NewSymbol("decode"),
        FunctionTemplate::New(decode)->GetFunction());
}