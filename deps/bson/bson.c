#include "bson.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __GNUC__
#define INLINE static __inline__
#else
#define INLINE static
#endif

static const int initialBufferSize = 128;
static const int zero = 0;

/** Utilities **/

bson bson_empty() {
    static char *data = "\005\0\0\0\0";
    return bson_init(data, 0);
}

void bson_copy(bson* out, const bson* in) {
    if (!out) return;
    out->data = malloc(bson_size(in));
    out->owned = 1;
    memcpy(out->data, in->data, bson_size(in));
}

bson bson_init(char *data, int mine) {
    bson b;
    b.data = data;
    b.owned = mine;
    return b;
}

int bson_size(const bson *b) {
    int i;
    if (!b || !b->data)
        return 0;
    bson_little_endian32(&i, b->data);
    return i;
}

void bson_destroy(bson *b) {
    if (b->owned && b->data)
        free(b->data);
    b->data = 0;
    b->owned = 0;
}

INLINE void bson_swap_endian64(void *outp, const void *inp) {
    const char *in = (const char *)inp;
    char *out = (char *)outp;
    out[0] = in[7];
    out[1] = in[6];
    out[2] = in[5];
    out[3] = in[4];
    out[4] = in[3];
    out[5] = in[2];
    out[6] = in[1];
    out[7] = in[0];

}

INLINE void bson_swap_endian32(void *outp, const void *inp) {
    const char *in = (const char *)inp;
    char *out = (char *)outp;
    out[0] = in[3];
    out[1] = in[2];
    out[2] = in[1];
    out[3] = in[0];
}

/** ObjectID **/

static char hexbyte(char hex) {
    switch (hex) {
        case '0': return 0x0;
        case '1': return 0x1;
        case '2': return 0x2;
        case '3': return 0x3;
        case '4': return 0x4;
        case '5': return 0x5;
        case '6': return 0x6;
        case '7': return 0x7;
        case '8': return 0x8;
        case '9': return 0x9;
        case 'a':
        case 'A': return 0xa;
        case 'b':
        case 'B': return 0xb;
        case 'c':
        case 'C': return 0xc;
        case 'd':
        case 'D': return 0xd;
        case 'e':
        case 'E': return 0xe;
        case 'f':
        case 'F': return 0xf;
        default: return 0x0; /* something smarter? */
    }
}

void bson_oid_from_string(bson_oid_t *oid, const char *str) {
    int i;
    for (i=0; i<12; i++) {
        oid->bytes[i] = (hexbyte(str[2*i]) << 4) | hexbyte(str[2*i + 1]);
    }
}
void bson_oid_to_string(const bson_oid_t *oid, char *str) {
    static const char hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    int i;
    for (i=0; i<12; i++) {
        str[2*i]     = hex[(oid->bytes[i] & 0xf0) >> 4];
        str[2*i + 1] = hex[ oid->bytes[i] & 0x0f      ];
    }
    str[24] = '\0';
}
void bson_oid_gen(bson_oid_t *oid) {
    static int incr = 0;
    static int fuzz = 0;
    int i = incr++; /* TODO make atomic */
    int t = time(NULL);

    /* TODO rand sucks. find something better */
    if (!fuzz) {
        srand(t);
        fuzz = rand();
    }

    bson_big_endian32(&oid->ints[0], &t);
    oid->ints[1] = fuzz;
    bson_big_endian32(&oid->ints[2], &i);
}

time_t bson_oid_generated_time(bson_oid_t* oid) {
    time_t out;
    bson_big_endian32(&out, &oid->ints[0]);
    return out;
}

/** Parser **/

typedef enum {
    bson_state_document_start = 0,
    bson_state_array_start,
    bson_state_element_list,
    bson_state_parse_complete,
    bson_state_parse_error
} bson_parser_state;

bson_parser bson_init_parser(const char *buf, int buflen, const bson_parser_callbacks *callbacks, void *ctx) {
    bson_parser parser;
    parser.stackPos = 0;
    parser.stack[0] = bson_state_document_start;
    parser.cur = buf;
    parser.remain = buflen;
    parser.callbacks = callbacks;
    parser.ctx = ctx;
    return parser;
}

int bson_parse_integer_32(const char *cur) {
    int out;
    bson_little_endian32(&out, cur);
    return out;
}

int64_t bson_parse_integer_64(const char *cur) {
    int64_t out;
    bson_little_endian64(&out, cur);
    return out;
}

double bson_parse_double(const char *cur) {
    double out;
    bson_little_endian64(&out, cur);
    return out;
}

int64_t bson_parse_date(const char *cur) {
    return bson_parse_integer_64(cur);
}

bson_oid_t *bson_parse_oid(const char *cur) {
    return (bson_oid_t*)cur;
}

#define PARSE_ELEMENT_NAME \
    klen = strnlen(parser->cur, parser->remain); \
    DECR_REMAIN(klen + 1); \
    ename = parser->cur; \
    parser->cur += klen + 1;

#define DECR_REMAIN(x) \
    parser->remain -= x; \
    if (parser->remain < 0) { \
        parser->stack[parser->stackPos] = bson_state_parse_error; \
        break; \
    }

#define CHECK_CB(x) \
    if (!(x)) { \
        parser->stack[parser->stackPos] = bson_state_parse_error; \
        return 0; \
    }

int bson_parse(bson_parser *parser) {
    start_token:
    switch (parser->stack[parser->stackPos]) {
        bson_type etype;
        
        case bson_state_parse_error:
            return 0;
        case bson_state_parse_complete:
            return 1;
        case bson_state_document_start:
        case bson_state_array_start:
            DECR_REMAIN(4);
            parser->cur += 4;
            parser->stack[parser->stackPos] = bson_state_element_list;
        case bson_state_element_list:
            DECR_REMAIN(1);
            etype = (bson_type)parser->cur[0];
            parser->cur += 1;
            switch (etype) {
                char subtype;
                const char *ename, *data, *supl;
                int klen, elen, rlen;

                case bson_eoo:
                    if (parser->callbacks->bson_end_document) {
                        CHECK_CB(parser->callbacks->bson_end_document(parser->ctx));
                    }
                    if (parser->stackPos > 0) {
                        parser->stackPos -= 1;
                    } else {
                        parser->stack[0] = bson_state_parse_complete;
                    }
                    break;
                case bson_null:
                    PARSE_ELEMENT_NAME;
                    if (parser->callbacks->bson_null) {
                        CHECK_CB(parser->callbacks->bson_null(parser->ctx, ename));
                    }
                    break;
                case bson_bool:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(1);
                    if (parser->callbacks->bson_boolean) {
                        CHECK_CB(parser->callbacks->bson_boolean(parser->ctx, ename, (int)parser->cur[0]));
                    }
                    parser->cur += 1;
                    break;
                case bson_int:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(4);
                    if (parser->callbacks->bson_integer_32) {
                        CHECK_CB(parser->callbacks->bson_integer_32(parser->ctx, ename, bson_parse_integer_32(parser->cur)));
                    }
                    parser->cur += 4;
                    break;
                case bson_long:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(8);
                    if (parser->callbacks->bson_integer_64) {
                        CHECK_CB(parser->callbacks->bson_integer_64(parser->ctx, ename, bson_parse_integer_64(parser->cur)));
                    }
                    parser->cur += 8;
                    break;
                case bson_double:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(8);
                    if (parser->callbacks->bson_float) {
                        CHECK_CB(parser->callbacks->bson_float(parser->ctx, ename, bson_parse_double(parser->cur)));
                    }
                    parser->cur += 8;
                    break;
                case bson_date:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(8);
                    if (parser->callbacks->bson_datetime) {
                        CHECK_CB(parser->callbacks->bson_datetime(parser->ctx, ename, bson_parse_date(parser->cur)));
                    }
                    parser->cur += 8;
                    break;
                case bson_oid:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(12);
                    if (parser->callbacks->bson_objectid) {
                        CHECK_CB(parser->callbacks->bson_objectid(parser->ctx, ename, bson_parse_oid(parser->cur)));
                    }
                    parser->cur += 12;
                    break;
                case bson_string:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(4);
                    elen = bson_parse_integer_32(parser->cur);
                    parser->cur += 4;
                    DECR_REMAIN(elen);
                    if (parser->callbacks->bson_string) {
                        CHECK_CB(parser->callbacks->bson_string(parser->ctx, ename, parser->cur, elen - 1));
                    }
                    parser->cur += elen;
                    break;
                case bson_code:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(4);
                    elen = bson_parse_integer_32(parser->cur);
                    parser->cur += 4;
                    DECR_REMAIN(elen);
                    if (parser->callbacks->bson_code) {
                        CHECK_CB(parser->callbacks->bson_code(parser->ctx, ename, parser->cur, elen - 1));
                    }
                    parser->cur += elen;
                    break;
                case bson_bindata:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(5);
                    elen = bson_parse_integer_32(parser->cur);
                    parser->cur += 4;
                    subtype = parser->cur[0];
                    parser->cur += 1;
                    DECR_REMAIN(elen);

                    /* Handle binary subtype 2 */
                    rlen = elen;
                    data = parser->cur;
                    if (subtype == 2) {
                        rlen = bson_parse_integer_32(parser->cur);
                        data = parser->cur + 4;
                    }

                    if (parser->callbacks->bson_binary) {
                        CHECK_CB(parser->callbacks->bson_binary(parser->ctx, ename, data, subtype, rlen));
                    }
                    parser->cur += elen;
                    break;
                case bson_object:
                    PARSE_ELEMENT_NAME;
                    if (parser->callbacks->bson_start_document) {
                        CHECK_CB(parser->callbacks->bson_start_document(parser->ctx, ename));
                    }
                    parser->stackPos += 1;
                    parser->stack[parser->stackPos] = bson_state_document_start;
                    break;
                case bson_array:
                    PARSE_ELEMENT_NAME;
                    if (parser->callbacks->bson_start_array) {
                        CHECK_CB(parser->callbacks->bson_start_array(parser->ctx, ename));
                    }
                    parser->stackPos += 1;
                    parser->stack[parser->stackPos] = bson_state_array_start;
                    break;
                case bson_codewscope:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(8);
                    parser->cur += 4;
                    elen = bson_parse_integer_32(parser->cur);
                    parser->cur += 4;
                    DECR_REMAIN(elen);
                    if (parser->callbacks->bson_code_w_scope) {
                        CHECK_CB(parser->callbacks->bson_code_w_scope(parser->ctx, ename, parser->cur, elen - 1));
                    }
                    parser->cur += elen;
                    parser->stackPos += 1;
                    parser->stack[parser->stackPos] = bson_state_document_start;
                    break;
                case bson_regex:
                    PARSE_ELEMENT_NAME;

                    elen = strnlen(parser->cur, parser->remain);
                    DECR_REMAIN(elen + 1);
                    data = parser->cur;
                    parser->cur += elen + 1;

                    elen = strnlen(parser->cur, parser->remain);
                    DECR_REMAIN(elen + 1);
                    supl = parser->cur;
                    parser->cur += elen + 1;

                    if (parser->callbacks->bson_regex) {
                        CHECK_CB(parser->callbacks->bson_regex(parser->ctx, ename, data, supl));
                    }
                    break;
                case bson_symbol:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(4);
                    elen = bson_parse_integer_32(parser->cur);
                    parser->cur += 4;
                    DECR_REMAIN(elen);
                    if (parser->callbacks->bson_symbol) {
                        CHECK_CB(parser->callbacks->bson_symbol(parser->ctx, ename, parser->cur, elen - 1));
                    }
                    parser->cur += elen;
                    break;
                case bson_timestamp:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(8);
                    elen = bson_parse_integer_32(parser->cur);
                    parser->cur += 4;
                    rlen = bson_parse_integer_32(parser->cur);
                    parser->cur += 4;
                    if (parser->callbacks->bson_timestamp) {
                        CHECK_CB(parser->callbacks->bson_timestamp(parser->ctx, ename, elen, rlen));
                    }
                    break;
                case bson_undefined:
                    PARSE_ELEMENT_NAME;
                    if (parser->callbacks->bson_undefined) {
                        CHECK_CB(parser->callbacks->bson_undefined(parser->ctx, ename));
                    }
                    break;
                case bson_min_key:
                    PARSE_ELEMENT_NAME;
                    if (parser->callbacks->bson_min_key) {
                        CHECK_CB(parser->callbacks->bson_min_key(parser->ctx, ename));
                    }
                    break;
                case bson_max_key:
                    PARSE_ELEMENT_NAME;
                    if (parser->callbacks->bson_max_key) {
                        CHECK_CB(parser->callbacks->bson_max_key(parser->ctx, ename));
                    }
                    break;
                case bson_dbref:
                    PARSE_ELEMENT_NAME;
                    DECR_REMAIN(4);
                    elen = bson_parse_integer_32(parser->cur);
                    DECR_REMAIN(elen + 12);
                    parser->cur += elen + 16;
                    break;
                default:
                    parser->stack[parser->stackPos] = bson_state_parse_error;
            }
            break;
        default:
            parser->stack[parser->stackPos] = bson_state_parse_error;
    }
    goto start_token;
}

/** Generator **/

bson_generator bson_init_generator() {
    bson_generator g;
    g.buf = (char *)malloc(initialBufferSize);
    g.bufSize = initialBufferSize;
    g.cur = g.buf + 4;
    g.finished = 0;
    g.stackPos = 0;
    return g;
}

bson bson_from_buffer(bson_generator *buf) {
    return bson_init(bson_generator_finish(buf), 1);
}

void bson_append_byte(bson_generator *b, char c) {
    b->cur[0] = c;
    b->cur++;
}
void bson_append(bson_generator *b, const void *data, int len) {
    memcpy(b->cur, data, len);
    b->cur += len;
}
void bson_append32(bson_generator * b, const void *data) {
    bson_little_endian32(b->cur, data);
    b->cur += 4;
}
void bson_append64(bson_generator *b, const void *data) {
    bson_little_endian64(b->cur, data);
    b->cur += 8;
}

int bson_ensure_space(bson_generator *b, const int bytesNeeded) {
    int pos = b->cur - b->buf;
    char *orig = b->buf;
    int new_size;

    if (b->finished) return 0;
    if (pos + bytesNeeded <= b->bufSize) return 1;
    new_size = 1.5 * (b->bufSize + bytesNeeded);
    b->buf = realloc(b->buf, new_size);
    if (!b->buf) return 0;
    b->bufSize = new_size;
    b->cur += b->buf - orig;
    return 1;
}

char *bson_generator_finish(bson_generator *b) {
    int i;
    if (!b->finished) {
        if (!bson_ensure_space(b, 1)) return 0;
        bson_append_byte(b, 0);
        i = b->cur - b->buf;
        bson_little_endian32(b->buf, &i);
        b->finished = 1;
    }
    return b->buf;
}

void bson_generator_destroy(bson_generator *b) {
    free(b->buf);
    b->buf = 0;
    b->cur = 0;
    b->finished = 1;
}

INLINE int bson_append_estart(bson_generator *b, int type, const char *name, const int dataSize) {
    const int sl = strlen(name) + 1;
    if (!bson_ensure_space(b, 1 + sl + dataSize)) return 0;
    bson_append_byte(b, (char)type);
    bson_append(b, name, sl);
    return 1;
}

int bson_append_int(bson_generator *b, const char *name, const int i) {
    if (!bson_append_estart(b, bson_int, name, 4)) return 0;
    bson_append32(b, &i);
    return 1;
}

int bson_append_long(bson_generator *b, const char *name, const int64_t i) {
    if (!bson_append_estart(b, bson_long, name, 8)) return 0;
    bson_append64(b, &i);
    return 1;
}

int bson_append_double(bson_generator *b, const char *name, const double d) {
    if (!bson_append_estart(b, bson_double, name, 8)) return 0;
    bson_append64(b, &d);
    return 1;
}

int bson_append_bool(bson_generator *b, const char *name, const int i) {
    if (!bson_append_estart(b, bson_bool, name, 1)) return 0;
    bson_append_byte(b, i != 0);
    return 1;
}

int bson_append_null(bson_generator * b, const char * name) {
    if (!bson_append_estart(b, bson_null, name, 0)) return 0;
    return 1;
}

int bson_append_undefined(bson_generator * b, const char * name) {
    if (!bson_append_estart(b, bson_undefined, name, 0)) return 0;
    return 1;
}

int bson_append_min_key(bson_generator * b, const char * name) {
    if (!bson_append_estart(b, bson_min_key, name, 0)) return 0;
    return 1;
}

int bson_append_max_key(bson_generator *b, const char *name) {
    if (!bson_append_estart(b, bson_max_key, name, 0)) return 0;
    return 1;
}

int bson_append_string_base(bson_generator * b, const char * name, const char * value, bson_type type) {
    int sl = strlen(value) + 1;
    if (!bson_append_estart(b, type, name, 4 + sl)) return 0;
    bson_append32(b, &sl);
    bson_append(b, value, sl);
    return 1;
}

int bson_append_string(bson_generator * b, const char * name, const char * value) {
    return bson_append_string_base(b, name, value, bson_string);
}

int bson_append_symbol(bson_generator * b, const char * name, const char * value) {
    return bson_append_string_base(b, name, value, bson_symbol);
}

int bson_append_code(bson_generator * b, const char * name, const char * value) {
    return bson_append_string_base(b, name, value, bson_code);
}

int bson_append_code_w_scope(bson_generator * b, const char * name, const char * code, const bson * scope) {
    int sl = strlen(code) + 1;
    int size = 4 + 4 + sl + bson_size(scope);
    if (!bson_append_estart(b, bson_codewscope, name, size)) return 0;
    bson_append32(b, &size);
    bson_append32(b, &sl);
    bson_append(b, code, sl);
    bson_append(b, scope->data, bson_size(scope));
    return 1;
}

int bson_append_binary(bson_generator * b, const char * name, char type, const char * str, int len) {
    int subtwolen = len + 4;
    if (type == 2) {
        if (!bson_append_estart(b, bson_bindata, name, 4 + 1 + 4 + len)) return 0;
        bson_append32(b, &subtwolen);
        bson_append_byte(b, type);
        bson_append32(b, &len);
        bson_append(b, str, len);
    } else {
        if (!bson_append_estart(b, bson_bindata, name, 4 + 1 + len)) return 0;
        bson_append32(b, &len);
        bson_append_byte(b, type);
        bson_append(b, str, len);
    }
    return 1;
}

int bson_append_oid(bson_generator *b, const char *name, const bson_oid_t *oid) {
    if (!bson_append_estart(b, bson_oid, name, 12)) return 0;
    bson_append(b, oid, 12);
    return 1;
}

int bson_append_new_oid(bson_generator *b, const char * name) {
    bson_oid_t oid;
    bson_oid_gen(&oid);
    return bson_append_oid(b, name, &oid);
}

int bson_append_regex(bson_generator *b, const char *name, const char *pattern, const char *opts) {
    const int plen = strlen(pattern)+1;
    const int olen = strlen(opts)+1;
    if (!bson_append_estart(b, bson_regex, name, plen + olen)) return 0;
    bson_append(b, pattern, plen);
    bson_append(b, opts, olen);
    return 1;
}

int bson_append_bson(bson_generator * b, const char * name, const bson* bson) {
    if (!bson_append_estart(b, bson_object, name, bson_size(bson))) return 0;
    bson_append(b, bson->data, bson_size(bson));
    return 1;
}

int bson_append_date(bson_generator *b, const char *name, int64_t millis) {
    if (!bson_append_estart(b, bson_date, name, 8)) return 0;
    bson_append64(b, &millis);
    return 1;
}

int bson_append_time_t(bson_generator * b, const char *name, time_t secs) {
    return bson_append_date(b, name, (int64_t)secs * 1000);
}

int bson_append_timestamp(bson_generator * b, const char * name, const int incr, const int ts) {
    if (!bson_append_estart(b, bson_timestamp, name, 8)) return 0;
    bson_append32(b, &incr);
    bson_append32(b, &ts);
    return 1;
}

int bson_append_start_object(bson_generator *b, const char *name) {
    if (!bson_append_estart(b, bson_object, name, 5)) return 0;
    b->stack[b->stackPos++] = b->cur - b->buf;
    bson_append32(b, &zero);
    return 1;
}

int bson_append_start_array(bson_generator *b, const char * name) {
    if (!bson_append_estart(b, bson_array, name, 5)) return 0;
    b->stack[b->stackPos++] = b->cur - b->buf;
    bson_append32(b, &zero);
    return 1;
}

int bson_append_finish_object(bson_generator *b) {
    char * start;
    int i;
    if (!bson_ensure_space(b, 1)) return 0;
    bson_append_byte(b, 0);
    
    start = b->buf + b->stack[--b->stackPos];
    i = b->cur - start;
    bson_little_endian32(start, &i);

    return 1;
}