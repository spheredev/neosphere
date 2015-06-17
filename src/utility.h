#ifndef MINISPHERE__UTILITY_H__INCLUDED
#define MINISPHERE__UTILITY_H__INCLUDED

extern void duk_push_cstr_to_utf8 (duk_context *ctx, const char* string, size_t length);

extern const char* syspath (const char* filename);

#endif // MINISPHERE__UTILITY_H__INCLUDED
