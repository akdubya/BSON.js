#include "encode.h"
#include "types.h"

#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <math.h>
#include <sstream>
#include <bson.h>
#include <string.h>

using namespace v8;
using namespace node;
using namespace std;

static Persistent<String> as_bson_sym;
static Persistent<String> id_sym;
static Persistent<String> regex_sym;
static Persistent<String> bson_type_sym;
static Persistent<String> scope_sym;
static Persistent<String> source_sym;
static Persistent<String> code_sym;
static Persistent<String> constructor_sym;
static Persistent<String> name_sym;
static Persistent<String> global_sym;
static Persistent<String> ignore_case_sym;
static Persistent<String> multiline_sym;
static Persistent<String> ordered_keys_sym;

bson encodeObject(const Local<Object> object);
void encodeArray(bson_generator *bb, const char *name, const Local<Value> element);
inline void encodeToken(bson_generator *bb, const char *name, const Local<Value> element);
Handle<Value> encode(const Arguments &args);
Handle<Value> decode(const Arguments &args);

const char * ToCString(const String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

// TODO: pass return values
inline void encodeNull(bson_generator *bb, const char *name) {
    bson_append_null(bb, name);
}

inline void encodeString(bson_generator *bb, const char *name, const Local<Value> element) {
    HandleScope scope;
    String::Utf8Value v(element);
    const char *value(ToCString(v));
    bson_append_string(bb, name, value);
}

inline void encodeSymbol(bson_generator *bb, const char *name, const Local<Object> obj) {
    HandleScope scope;
    String::Utf8Value v(obj->Get(String::NewSymbol("string"))->ToString());
    const char *value(ToCString(v));
    bson_append_symbol(bb, name, value);
}

inline void encodeNumber(bson_generator *bb, const char *name, const Local<Value> element) {
    HandleScope scope;
    double value(element->NumberValue());
    if (floor(value) == value) {
        bson_append_long(bb, name, element->IntegerValue());
    } else {
        bson_append_double(bb, name, value);
    }
}

inline void encodeTimestamp(bson_generator *bb, const char *name, const Local<Object> obj) {
    HandleScope scope;
    int incr(obj->Get(String::NewSymbol("increment"))->NumberValue());
    int ts(obj->Get(String::NewSymbol("timestamp"))->NumberValue());
    bson_append_timestamp(bb, name, incr, ts);
}

inline void encodeInteger(bson_generator *bb, const char *name, const Local<Value> element) {
    int value(element->NumberValue());
    bson_append_int(bb, name, value);
}

inline void encodeBoolean(bson_generator *bb, const char *name, const Local<Value> element) {
    bool value(element->IsTrue());
    bson_append_bool(bb, name, value);
}

inline void encodeDate(bson_generator *bb, const char *name, const Local<Value> element) {
    double value(element->NumberValue());
    bson_append_date(bb, name, value);
}

inline void encodeCode(bson_generator *bb, const char *name, const Local<Object> obj) {
    Local<Value> code = obj->Get(code_sym);
    String::Utf8Value code_utf(code);
    const char *bson_code(ToCString(code_utf));
    if (obj->Has(scope_sym)) {
        Local<Value> scope = obj->Get(scope_sym);
        bson bson_scope(encodeObject(scope->ToObject()));
        bson_append_code_w_scope(bb, name, bson_code, &bson_scope);
        bson_destroy(&bson_scope);
    } else {
        bson_append_code(bb, name, bson_code);
    }
}

inline void encodeFunction(bson_generator *bb, const char *name, const Local<Value> element) {
    Local<Function> fn = Function::Cast(*element);
    String::Utf8Value nv(fn->GetName()->ToString());
    const char *cls(ToCString(nv));
    if (strncmp(cls, "MinKey", 6) == 0) {
        bson_append_min_key(bb, name);
    } else if (strncmp(cls, "MaxKey", 6) == 0) {
        bson_append_max_key(bb, name);
    }
}

inline void encodeRegex(bson_generator *bb, const char *name, const Local<Object> obj) {
    Local<Value> source = obj->Get(source_sym);
    String::Utf8Value v(source);
    const char *value(ToCString(v));
    char opts[10] = "";
    if (obj->Get(global_sym)->IsTrue()) {
        strcat(opts, "g");
    }
    if (obj->Get(ignore_case_sym)->IsTrue()) {
        strcat(opts, "i");
    }
    if (obj->Get(multiline_sym)->IsTrue()) {
        strcat(opts, "m");
    }
    bson_append_regex(bb, name, value, opts);
}

inline void encodeBinary(bson_generator *bb, const char *name, const Local<Object> obj) {
    int type;

    if (obj->Has(bson_type_sym)) {
        type = (int)obj->Get(bson_type_sym)->NumberValue();
    } else {
        type = 0;
    }

    Buffer *buf = ObjectWrap::Unwrap<Buffer>(obj);
    const char *data = buf->data();
    int len = buf->length();
    bson_append_binary(bb, name, (char)type, data, len);
}

inline void encodeObjectID(bson_generator *bb, const char *name, const Local<Object> obj) {
    bson_oid_t oid;
    node::DecodeWrite(oid.bytes, 12, obj->Get(id_sym), node::BINARY);
    bson_append_oid(bb, name, &oid);
}

inline void encodeUndefined(bson_generator *bb, const char *name) {
    bson_append_undefined(bb, name);
}

inline void encodeObjectToken(bson_generator *bb, const char *name, const Local<Value> element) {
    HandleScope scope;
    Local<Object> obj = element->ToObject();

    if (ObjectID::HasInstance(element)) {
        encodeObjectID(bb, name, obj);
    } else if (Buffer::HasInstance(element)) {
        encodeBinary(bb, name, obj);
    } else if (Code::HasInstance(element)) {
        encodeCode(bb, name, obj);
    } else if (Symbol::HasInstance(element)) {
        encodeSymbol(bb, name, obj);
    } else if (obj->Get(constructor_sym)
            ->ToObject()->Get(name_sym)
            ->Equals(regex_sym)) {
        encodeRegex(bb, name, obj);
    } else if (Timestamp::HasInstance(element)) {
        encodeTimestamp(bb, name, obj);
    } else if (obj->Has(as_bson_sym)) {
        Local<Value> prop = obj->Get(as_bson_sym);
        Local<Value> elem;
        if (prop->IsFunction()) {
            Handle<Function> fn = Handle<Function>::Cast(prop);
            Local<Value> argv[0];
            elem = fn->Call(obj, 0, argv);
        } else {
            elem = prop;
        }
        encodeToken(bb, name, elem);
    } else {
        bson bson(encodeObject(obj));
        bson_append_bson(bb, name, &bson);
        bson_destroy(&bson);
    }
}

inline void encodeToken(bson_generator *bb, const char *name, const Local<Value> element) {
    if (element->IsNull()) {
        encodeNull(bb, name);
    } else if (element->IsString()) {
        encodeString(bb, name, element);
    } else if (element->IsInt32()) {
        encodeInteger(bb, name, element);
    } else if (element->IsNumber()) {
        encodeNumber(bb, name, element);
    } else if (element->IsBoolean()) {
        encodeBoolean(bb, name, element);
    } else if (element->IsDate()) {
        encodeDate(bb, name, element);
    } else if (element->IsArray()) {
        encodeArray(bb, name, element);
    } else if (element->IsFunction()) {
        encodeFunction(bb, name, element);
    } else if(element->IsObject()) {
        encodeObjectToken(bb, name, element);
    } else if (element->IsUndefined()) {
        encodeUndefined(bb, name);
    }
}

void encodeArray(bson_generator *bb, const char *name, const Local<Value> element) {
    Local<Array> a = Array::Cast(*element);
    bson_append_start_array(bb, name);

    for (int i = 0, l=a->Length(); i < l; i++) {
        Local<Value> val = a->Get(Number::New(i));
        stringstream keybuf;
        string keyval;
        keybuf << i << endl;
        keybuf >> keyval;

        encodeToken(bb, keyval.c_str(), val);
    }
    bson_append_finish_object(bb);
}

// TODO: Needs to be integrated
inline void checkKey(const char *name) {
    int i;
    if (name[0] != '\0' && name[0] == '$') {
        throw(Exception::TypeError(String::New("Key must not start with '$'")));
    }
    for (i = 0; i < strlen(name); i++) {
        if (name[i] == '.') {
            throw(Exception::TypeError(String::New("Key must not contain '.'")));
        }
    }
}

bson encodeObject(const Local<Object> object) {
    bson_generator bb = bson_init_generator();
    
    Local<Array> properties;
    Local<Object> values;

    if (object->Has(ordered_keys_sym)) {
        properties = Array::Cast(*object->Get(ordered_keys_sym));
        values = object->Get(String::NewSymbol("values"))->ToObject();
        for (int i = 0; i < properties->Length(); i++) {
            Local<String> prop_name = properties->Get(i)->ToString();
            Local<Value> prop_val = values->Get(prop_name);
            String::Utf8Value n(prop_name);
            const char *pname = ToCString(n);
            encodeToken(&bb, pname, prop_val);
        }
    } else {
        properties = object->GetPropertyNames();
        values = object;
        for (int i = 0; i < properties->Length(); i++) {
            Local<String> prop_name = properties->Get(i)->ToString();
            if (!values->IsArray() && !values->HasRealNamedProperty(prop_name)) continue;
            Local<Value> prop_val = values->Get(prop_name);
            String::Utf8Value n(prop_name);
            const char *pname = ToCString(n);
            encodeToken(&bb, pname, prop_val);
        }
    }

    return bson_from_buffer(&bb);
}

Handle<Value> encode(const Arguments &args) {
    HandleScope scope;
    if (!args[0]->IsObject()) {
        return ThrowException(Exception::TypeError(String::New("Value to encode must be an object")));
    }
    try {
        bson bson(encodeObject(args[0]->ToObject()));
        Handle<Value> ret = node::Encode(bson.data, bson_size(&bson), node::BINARY);
        bson_destroy(&bson);
        return scope.Close(ret);
    } catch (Local<Value> err) {
        return ThrowException(err);
    }
}

void InitEncoder(Handle<Object> target) {
    HandleScope scope;

    as_bson_sym = Persistent<String>::New(String::NewSymbol("asBSON"));
    id_sym = Persistent<String>::New(String::NewSymbol("id"));
    regex_sym = Persistent<String>::New(String::NewSymbol("RegExp"));
    bson_type_sym = Persistent<String>::New(String::NewSymbol("bsonType"));
    scope_sym = Persistent<String>::New(String::NewSymbol("scope"));
    source_sym = Persistent<String>::New(String::NewSymbol("source"));
    code_sym = Persistent<String>::New(String::NewSymbol("code"));
    constructor_sym = Persistent<String>::New(String::NewSymbol("constructor"));
    name_sym = Persistent<String>::New(String::NewSymbol("name"));
    global_sym = Persistent<String>::New(String::NewSymbol("global"));
    ignore_case_sym = Persistent<String>::New(String::NewSymbol("ignoreCase"));
    multiline_sym = Persistent<String>::New(String::NewSymbol("multiline"));
    ordered_keys_sym = Persistent<String>::New(String::NewSymbol("ordered_keys"));

    target->Set(String::NewSymbol("encode"),
        FunctionTemplate::New(encode)->GetFunction());
}