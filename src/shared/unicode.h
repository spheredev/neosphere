#ifndef FATCERBERUS__UNICODE_H__INCLUDED
#define FATCERBERUS__UNICODE_H__INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef
enum utf8_ret
{
	UTF8_OK,
	UTF8_CODEPOINT,
	UTF8_CONTINUE,
	UTF8_ERROR,
	UTF8_REPLACEMENT,
	UTF8_RETRY,
} utf8_ret_t;

typedef struct utf8_decode utf8_decode_t;

size_t         cesu8_emit        (uint32_t codep, uint8_t* *p_ptr);
utf8_decode_t* utf8_decode_start (bool strict);
utf8_ret_t     utf8_decode_end   (utf8_decode_t* cx);
utf8_ret_t     utf8_decode_next  (utf8_decode_t* cx, uint8_t byte, uint32_t *out_codepoint);
size_t         utf8_emit         (uint32_t codep, uint8_t* *p_ptr);

#endif // FATCERBERUS__UNICODE_H__INCLUDED
