#ifndef MINISPHERE__LSTRING_H__INCLUDED
#define MINISPHERE__LSTRING_H__INCLUDED

typedef struct lstring lstring_t;
struct lstring
{
	size_t      length;
	const char* cstr;
};

extern lstring_t* new_lstring       (const char* fmt, ...);
extern lstring_t* lstring_from_buf  (size_t length, const char* buffer);
extern lstring_t* lstring_from_cstr (const char* cstr);
extern lstring_t* clone_lstring     (const lstring_t* string);
extern lstring_t* read_lstring      (FILE* file, bool trim_null);
extern lstring_t* read_lstring_raw  (FILE* file, size_t length, bool trim_null);
extern void       free_lstring      (lstring_t* string);

extern lstring_t* duk_require_lstring_t (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__LSTRING_H__INCLUDED
