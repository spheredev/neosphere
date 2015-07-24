#ifndef MINISPHERE__LSTRING_H__INCLUDED
#define MINISPHERE__LSTRING_H__INCLUDED

typedef struct lstring lstring_t;

extern lstring_t*  lstr_newf     (const char* fmt, ...);
extern lstring_t*  lstr_vnewf    (const char* fmt, va_list args);
extern lstring_t*  lstr_from_buf (const char* buffer, size_t length);
extern void        lstr_free     (lstring_t* string);
extern const char* lstr_cstr     (const lstring_t* string);
extern int         lstr_cmp      (const lstring_t* string1, const lstring_t* string2);
extern lstring_t*  lstr_dup      (const lstring_t* string);
extern size_t      lstr_len      (const lstring_t* string);

extern lstring_t* read_lstring     (sfs_file_t* file, bool trim_null);
extern lstring_t* read_lstring_raw (sfs_file_t* file, size_t length, bool trim_null);

extern void       duk_push_lstring_t    (duk_context* ctx, const lstring_t* string);
extern lstring_t* duk_require_lstring_t (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__LSTRING_H__INCLUDED
