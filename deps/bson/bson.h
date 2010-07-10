#ifndef _BSON_H_
#define _BSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

#if defined (HAVE_STDINT_H) || __STDC_VERSION__ >= 199901L || __GNUC__
#include <stdint.h>
#elif defined HAVE_UNISTD_H
#include <unistd.h>
#elif defined USE__INT64
typedef __int64 int64_t;
#elif defined USE_LONG_LONG_INT
typedef long long int int64_t;
#else
  #error "Requires 64bit integer type"
#endif

#ifdef USE_BIG_ENDIAN
#define bson_little_endian64(out, in) (bson_swap_endian64(out, in))
#define bson_little_endian32(out, in) (bson_swap_endian32(out, in))
#define bson_big_endian64(out, in) (memcpy(out, in, 8))
#define bson_big_endian32(out, in) (memcpy(out, in, 4))
#else
#define bson_little_endian64(out, in) (memcpy(out, in, 8))
#define bson_little_endian32(out, in) (memcpy(out, in, 4))
#define bson_big_endian64(out, in) (bson_swap_endian64(out, in))
#define bson_big_endian32(out, in) (bson_swap_endian32(out, in))
#endif

typedef enum {
    bson_min_key=-1,
    bson_eoo=0,
    bson_double=1,
    bson_string=2,
    bson_object=3,
    bson_array=4,
    bson_bindata=5,
    bson_undefined=6,
    bson_oid=7,
    bson_bool=8,
    bson_date=9,
    bson_null=10,
    bson_regex=11,
    bson_dbref=12, /* deprecated */
    bson_code=13,
    bson_symbol=14,
    bson_codewscope=15,
    bson_int = 16,
    bson_timestamp = 17,
    bson_long = 18,
    bson_max_key = 127
} bson_type;

typedef struct {
    char * data;
    int owned;
} bson;

/* Utilities */

/* returns static empty bson object */
bson bson_empty();

/* puts data in new buffer. NOOP if out==NULL */
void bson_copy(bson *out, const bson *in);

bson bson_init(char *data, int mine);
int bson_size(const bson *b);
void bson_destroy(bson *b);

/** ObjectID **/

#pragma pack(1)
typedef union{
    char bytes[12];
    int ints[3];
} bson_oid_t;
#pragma pack()

/* str must be at least 24 hex chars + null byte */
void bson_oid_from_string(bson_oid_t* oid, const char* str);

void bson_oid_to_string(const bson_oid_t* oid, char* str);
void bson_oid_gen(bson_oid_t* oid);
time_t bson_oid_generated_time(bson_oid_t* oid);

/** Parser **/

typedef struct {
    int (*bson_start_document)(void *ctx, const char *e_name);
    int (*bson_end_document)(void *ctx);
    int (*bson_start_array)(void *ctx, const char *e_name);
    int (*bson_end_array)(void *ctx);
    int (*bson_float)(void *ctx, const char *e_name, double val);
    int (*bson_string)(void *ctx, const char *e_name, const char *string, int length);
    int (*bson_binary)(void *ctx, const char *e_name, const char *data, unsigned char subtype, int length);
    int (*bson_undefined)(void *ctx, const char *e_name);
    int (*bson_objectid)(void *ctx, const char *e_name, bson_oid_t *oid);
    int (*bson_boolean)(void *ctx,const char *e_name, int val);
    int (*bson_datetime)(void *ctx, const char *e_name, int64_t date);
    int (*bson_null)(void *ctx, const char *e_name);
    int (*bson_regex)(void *ctx, const char *e_name, const char *pattern, const char *options);
    int (*bson_code)(void *ctx, const char *e_name, const char *code, int length);
    int (*bson_symbol)(void *ctx, const char *e_name, const char *symbol, int length);
    int (*bson_code_w_scope)(void *ctx, const char *e_name, const char *code, int length);
    int (*bson_integer_32)(void * ctx, const char *e_name, int val);
    int (*bson_timestamp)(void * ctx, const char *e_name, int increment, int timestamp);
    int (*bson_integer_64)(void * ctx, const char *e_name, int64_t val);
    int (*bson_min_key)(void *ctx, const char *e_name);
    int (*bson_max_key)(void *ctx, const char *e_name);
} bson_parser_callbacks;

typedef struct {
    const bson_parser_callbacks *callbacks;
    const char *cur;
    int remain;
    void *ctx;
    int stack[32];
    int stackPos;
} bson_parser;

bson_parser bson_init_parser(const char *buf, int buflen, const bson_parser_callbacks *callbacks, void *ctx);
int bson_parse(bson_parser *parser);

/** Generator **/

typedef struct {
    char * buf;
    char * cur;
    int bufSize;
    int finished;
    int stack[32];
    int stackPos;
} bson_generator;

bson_generator bson_init_generator();
char *bson_generator_finish(bson_generator *b);
void bson_generator_destroy(bson_generator *b);
bson bson_from_buffer(bson_generator *buf);

int bson_append_oid(bson_generator *b, const char *name, const bson_oid_t *oid);
int bson_append_new_oid(bson_generator *b, const char *name);
int bson_append_int(bson_generator *b, const char *name, const int i);
int bson_append_long(bson_generator *b, const char *name, const int64_t i);
int bson_append_double(bson_generator *b, const char *name, const double d);
int bson_append_string(bson_generator *b, const char *name, const char *str);
int bson_append_symbol(bson_generator *b, const char *name, const char *str);
int bson_append_code(bson_generator *b, const char *name, const char *str);
int bson_append_code_w_scope(bson_generator *b, const char *name, const char *code, const bson *scope);
int bson_append_binary(bson_generator *b, const char *name, char type, const char *str, int len);
int bson_append_bool(bson_generator *b, const char *name, const int v);
int bson_append_null(bson_generator *b, const char *name);
int bson_append_undefined(bson_generator *b, const char *name);
int bson_append_min_key(bson_generator *b, const char *name);
int bson_append_max_key(bson_generator *b, const char *name);
int bson_append_regex(bson_generator *b, const char *name, const char *pattern, const char *opts);
int bson_append_bson(bson_generator *b, const char *name, const bson *bson);
int bson_append_date(bson_generator *b, const char *name, int64_t millis);
int bson_append_time_t(bson_generator *b, const char *name, time_t secs);
int bson_append_timestamp(bson_generator *b, const char *name, const int incr, const int ts);
int bson_append_start_object(bson_generator *b, const char *name);
int bson_append_start_array(bson_generator *b, const char *name);
int bson_append_finish_object(bson_generator *b);

#ifdef __cplusplus
}
#endif
#endif