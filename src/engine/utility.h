#ifndef MINISPHERE__UTILITY_H__INCLUDED
#define MINISPHERE__UTILITY_H__INCLUDED

const path_t* enginepath (void);
const path_t* homepath   (void);
const char*   systempath (const char* filename);

bool        is_cpu_little_endian  (void);
duk_int_t   duk_json_pdecode      (duk_context* ctx);
void        duk_push_lstring_t    (duk_context* ctx, const lstring_t* string);
void        duk_ref_heapptr       (duk_context* ctx, void* heapptr);
lstring_t*  duk_require_lstring_t (duk_context* ctx, duk_idx_t index);
const char* duk_require_path      (duk_context* ctx, duk_idx_t index, const char* origin_name, bool legacy);
void        duk_unref_heapptr     (duk_context* ctx, void* heapptr);
lstring_t*  read_lstring          (sfs_file_t* file, bool trim_null);
lstring_t*  read_lstring_raw      (sfs_file_t* file, size_t length, bool trim_null);
char*       strnewf               (const char* fmt, ...);

#endif // MINISPHERE__UTILITY_H__INCLUDED
