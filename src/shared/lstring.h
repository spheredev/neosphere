#ifndef MINISPHERE__LSTRING_H__INCLUDED
#define MINISPHERE__LSTRING_H__INCLUDED

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct lstring lstring_t;

lstring_t*  lstr_new         (const char* cstr);
lstring_t*  lstr_newf        (const char* fmt, ...);
lstring_t*  lstr_vnewf       (const char* fmt, va_list args);
lstring_t*  lstr_from_cp1252 (const char* text, size_t length);
void        lstr_free        (lstring_t* string);
const char* lstr_cstr        (const lstring_t* string);
int         lstr_cmp         (const lstring_t* string1, const lstring_t* string2);
lstring_t*  lstr_dup         (const lstring_t* string);
size_t      lstr_len         (const lstring_t* string);

#endif // MINISPHERE__LSTRING_H__INCLUDED
