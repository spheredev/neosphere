#ifndef MINISPHERE__UNICODE_H__INCLUDED
#define MINISPHERE__UNICODE_H__INCLUDED

#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

uint32_t utf8decode (uint32_t* state, uint32_t* codep, uint8_t byte);
int      utf8cmp    (const char* string1, const char* string2);

#endif // MINISPHERE__UNICODE_H__INCLUDED
