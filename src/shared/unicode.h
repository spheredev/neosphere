#ifndef MINISPHERE__UNICODE_H__INCLUDED
#define MINISPHERE__UNICODE_H__INCLUDED

#include <stddef.h>
#include <stdint.h>

#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

uint32_t utf8_decode (uint32_t* state, uint32_t* codep, uint8_t byte);
size_t   utf8_encode (uint32_t codep, uint8_t* *p_ptr);

#endif // MINISPHERE__UNICODE_H__INCLUDED
