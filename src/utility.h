#ifndef MINISPHERE__UTILITY_H__INCLUDED
#define MINISPHERE__UTILITY_H__INCLUDED

extern const char* syspath (const char* filename);

extern void       duk_push_lstring_t    (duk_context* ctx, const lstring_t* string);
extern lstring_t* duk_require_lstring_t (duk_context* ctx, duk_idx_t index);
extern path_t*    duk_require_path      (duk_context* ctx, duk_idx_t index, const char* origin_name);
extern lstring_t* read_lstring          (sfs_file_t* file, bool trim_null);
extern lstring_t* read_lstring_raw      (sfs_file_t* file, size_t length, bool trim_null);

#endif // MINISPHERE__UTILITY_H__INCLUDED
