#ifndef MINISPHERE__LSTRING_H__INCLUDED
#define MINISPHERE__LSTRING_H__INCLUDED

typedef struct lstring lstring_t;
struct lstring
{
	size_t      length;
	const char* cstr;
};

extern lstring_t* new_lstring    (size_t length, const char* buffer);
extern lstring_t* read_lstring   (ALLEGRO_FILE* file);
extern lstring_t* read_lstring_s (ALLEGRO_FILE* file, size_t length);
extern void       free_lstring   (lstring_t* string);

extern lstring_t* duk_require_lstring_t (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__LSTRING_H__INCLUDED
