#ifndef MINISPHERE__UNICODE_H__INCLUDED
#define MINISPHERE__UNICODE_H__INCLUDED

#include <stddef.h>
#include <stdint.h>

typedef
enum utf8_ret
{
	UTF8_OK,
	UTF8_CODEPOINT,
	UTF8_CONTINUE,
	UTF8_ERROR,
} utf8_ret_t;

typedef struct utf8ctx utf8ctx_t;

size_t     cesu8_emit  (uint32_t codep, uint8_t* *p_ptr);
utf8ctx_t* utf8_decode (uint8_t* text, size_t length);
size_t     utf8_emit   (uint32_t codep, uint8_t* *p_ptr);
utf8_ret_t utf8_next   (utf8ctx_t* ctx, uint32_t *out_codepoint);

#endif // MINISPHERE__UNICODE_H__INCLUDED
