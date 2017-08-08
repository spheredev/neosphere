#ifndef MINISPHERE__UTILITY_H__INCLUDED
#define MINISPHERE__UTILITY_H__INCLUDED

const path_t* assets_path (void);
const path_t* engine_path (void);
const path_t* home_path   (void);

duk_int_t   duk_json_pdecode      (duk_context* ctx);
void        duk_push_lstring_t    (duk_context* ctx, const lstring_t* string);
void*       duk_ref_heapptr       (duk_context* ctx, duk_idx_t idx);
lstring_t*  duk_require_lstring_t (duk_context* ctx, duk_idx_t index);
const char* duk_require_path      (duk_context* ctx, duk_idx_t index, const char* origin_name, bool legacy, bool need_write);
void        duk_unref_heapptr     (duk_context* ctx, void* heapptr);
bool        fread_rect16          (file_t* file, rect_t* out_rect);
bool        fread_rect32          (file_t* file, rect_t* out_rect);
const char* md5sum                (const void* data, size_t size);
lstring_t*  read_lstring          (file_t* file, bool trim_null);
lstring_t*  read_lstring_raw      (file_t* file, size_t length, bool trim_null);
bool        write_lstring         (file_t* file, const lstring_t* string, bool include_nul);
char*       strnewf               (const char* fmt, ...);

#endif // MINISPHERE__UTILITY_H__INCLUDED
