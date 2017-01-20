#ifndef CELL__UTILITY_H__INCLUDED
#define CELL__UTILITY_H__INCLUDED

#include <stdbool.h>
#include <stddef.h>

void*       duk_get_heap_udata (duk_context* ctx);
duk_int_t   duk_json_pdecode   (duk_context* ctx);
void        duk_push_lstring_t (duk_context* ctx, const lstring_t* string);
void*       duk_ref_heapptr    (duk_context* ctx, duk_idx_t idx);
const char* duk_require_path   (duk_context* ctx, duk_idx_t index);
void        duk_unref_heapptr  (duk_context* ctx, void* heapptr);
bool        fexist             (const char* filename);
void*       fslurp             (const char* filename, size_t *out_size);
bool        fspew              (const void* buffer, size_t size, const char* filename);
char*       strnewf            (const char* fmt, ...);
bool        wildcmp            (const char* filename, const char* pattern);

#endif
