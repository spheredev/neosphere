#include "lstring.h"
#include "unicode.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct lstring
{
	size_t length;
	char*  cstr;
};

// map CP-1252 codepage to Unicode
static uint16_t cp1252[256] =
{
	(uint32_t) 0x0000,
	(uint32_t) 0x0001,
	(uint32_t) 0x0002,
	(uint32_t) 0x0003,
	(uint32_t) 0x0004,
	(uint32_t) 0x0005,
	(uint32_t) 0x0006,
	(uint32_t) 0x0007,
	(uint32_t) 0x0008,
	(uint32_t) 0x0009,
	(uint32_t) 0x000A,
	(uint32_t) 0x000B,
	(uint32_t) 0x000C,
	(uint32_t) 0x000D,
	(uint32_t) 0x000E,
	(uint32_t) 0x000F,
	(uint32_t) 0x0010,
	(uint32_t) 0x0011,
	(uint32_t) 0x0012,
	(uint32_t) 0x0013,
	(uint32_t) 0x0014,
	(uint32_t) 0x0015,
	(uint32_t) 0x0016,
	(uint32_t) 0x0017,
	(uint32_t) 0x0018,
	(uint32_t) 0x0019,
	(uint32_t) 0x001A,
	(uint32_t) 0x001B,
	(uint32_t) 0x001C,
	(uint32_t) 0x001D,
	(uint32_t) 0x001E,
	(uint32_t) 0x001F,
	(uint32_t) 0x0020,
	(uint32_t) 0x0021,
	(uint32_t) 0x0022,
	(uint32_t) 0x0023,
	(uint32_t) 0x0024,
	(uint32_t) 0x0025,
	(uint32_t) 0x0026,
	(uint32_t) 0x0027,
	(uint32_t) 0x0028,
	(uint32_t) 0x0029,
	(uint32_t) 0x002A,
	(uint32_t) 0x002B,
	(uint32_t) 0x002C,
	(uint32_t) 0x002D,
	(uint32_t) 0x002E,
	(uint32_t) 0x002F,
	(uint32_t) 0x0030,
	(uint32_t) 0x0031,
	(uint32_t) 0x0032,
	(uint32_t) 0x0033,
	(uint32_t) 0x0034,
	(uint32_t) 0x0035,
	(uint32_t) 0x0036,
	(uint32_t) 0x0037,
	(uint32_t) 0x0038,
	(uint32_t) 0x0039,
	(uint32_t) 0x003A,
	(uint32_t) 0x003B,
	(uint32_t) 0x003C,
	(uint32_t) 0x003D,
	(uint32_t) 0x003E,
	(uint32_t) 0x003F,
	(uint32_t) 0x0040,
	(uint32_t) 0x0041,
	(uint32_t) 0x0042,
	(uint32_t) 0x0043,
	(uint32_t) 0x0044,
	(uint32_t) 0x0045,
	(uint32_t) 0x0046,
	(uint32_t) 0x0047,
	(uint32_t) 0x0048,
	(uint32_t) 0x0049,
	(uint32_t) 0x004A,
	(uint32_t) 0x004B,
	(uint32_t) 0x004C,
	(uint32_t) 0x004D,
	(uint32_t) 0x004E,
	(uint32_t) 0x004F,
	(uint32_t) 0x0050,
	(uint32_t) 0x0051,
	(uint32_t) 0x0052,
	(uint32_t) 0x0053,
	(uint32_t) 0x0054,
	(uint32_t) 0x0055,
	(uint32_t) 0x0056,
	(uint32_t) 0x0057,
	(uint32_t) 0x0058,
	(uint32_t) 0x0059,
	(uint32_t) 0x005A,
	(uint32_t) 0x005B,
	(uint32_t) 0x005C,
	(uint32_t) 0x005D,
	(uint32_t) 0x005E,
	(uint32_t) 0x005F,
	(uint32_t) 0x0060,
	(uint32_t) 0x0061,
	(uint32_t) 0x0062,
	(uint32_t) 0x0063,
	(uint32_t) 0x0064,
	(uint32_t) 0x0065,
	(uint32_t) 0x0066,
	(uint32_t) 0x0067,
	(uint32_t) 0x0068,
	(uint32_t) 0x0069,
	(uint32_t) 0x006A,
	(uint32_t) 0x006B,
	(uint32_t) 0x006C,
	(uint32_t) 0x006D,
	(uint32_t) 0x006E,
	(uint32_t) 0x006F,
	(uint32_t) 0x0070,
	(uint32_t) 0x0071,
	(uint32_t) 0x0072,
	(uint32_t) 0x0073,
	(uint32_t) 0x0074,
	(uint32_t) 0x0075,
	(uint32_t) 0x0076,
	(uint32_t) 0x0077,
	(uint32_t) 0x0078,
	(uint32_t) 0x0079,
	(uint32_t) 0x007A,
	(uint32_t) 0x007B,
	(uint32_t) 0x007C,
	(uint32_t) 0x007D,
	(uint32_t) 0x007E,
	(uint32_t) 0x007F,
	(uint32_t) 0x20AC,
	(uint32_t) 0xFFFD,  /* undefined */
	(uint32_t) 0x201A,
	(uint32_t) 0x0192,
	(uint32_t) 0x201E,
	(uint32_t) 0x2026,
	(uint32_t) 0x2020,
	(uint32_t) 0x2021,
	(uint32_t) 0x02C6,
	(uint32_t) 0x2030,
	(uint32_t) 0x0160,
	(uint32_t) 0x2039,
	(uint32_t) 0x0152,
	(uint32_t) 0xFFFD,  /* undefined */
	(uint32_t) 0x017D,
	(uint32_t) 0xFFFD,  /* undefined */
	(uint32_t) 0xFFFD,  /* undefined */
	(uint32_t) 0x2018,
	(uint32_t) 0x2019,
	(uint32_t) 0x201C,
	(uint32_t) 0x201D,
	(uint32_t) 0x2022,
	(uint32_t) 0x2013,
	(uint32_t) 0x2014,
	(uint32_t) 0x02DC,
	(uint32_t) 0x2122,
	(uint32_t) 0x0161,
	(uint32_t) 0x203A,
	(uint32_t) 0x0153,
	(uint32_t) 0xFFFD,  /* undefined */
	(uint32_t) 0x017E,
	(uint32_t) 0x0178,
	(uint32_t) 0x00A0,
	(uint32_t) 0x00A1,
	(uint32_t) 0x00A2,
	(uint32_t) 0x00A3,
	(uint32_t) 0x00A4,
	(uint32_t) 0x00A5,
	(uint32_t) 0x00A6,
	(uint32_t) 0x00A7,
	(uint32_t) 0x00A8,
	(uint32_t) 0x00A9,
	(uint32_t) 0x00AA,
	(uint32_t) 0x00AB,
	(uint32_t) 0x00AC,
	(uint32_t) 0x00AD,
	(uint32_t) 0x00AE,
	(uint32_t) 0x00AF,
	(uint32_t) 0x00B0,
	(uint32_t) 0x00B1,
	(uint32_t) 0x00B2,
	(uint32_t) 0x00B3,
	(uint32_t) 0x00B4,
	(uint32_t) 0x00B5,
	(uint32_t) 0x00B6,
	(uint32_t) 0x00B7,
	(uint32_t) 0x00B8,
	(uint32_t) 0x00B9,
	(uint32_t) 0x00BA,
	(uint32_t) 0x00BB,
	(uint32_t) 0x00BC,
	(uint32_t) 0x00BD,
	(uint32_t) 0x00BE,
	(uint32_t) 0x00BF,
	(uint32_t) 0x00C0,
	(uint32_t) 0x00C1,
	(uint32_t) 0x00C2,
	(uint32_t) 0x00C3,
	(uint32_t) 0x00C4,
	(uint32_t) 0x00C5,
	(uint32_t) 0x00C6,
	(uint32_t) 0x00C7,
	(uint32_t) 0x00C8,
	(uint32_t) 0x00C9,
	(uint32_t) 0x00CA,
	(uint32_t) 0x00CB,
	(uint32_t) 0x00CC,
	(uint32_t) 0x00CD,
	(uint32_t) 0x00CE,
	(uint32_t) 0x00CF,
	(uint32_t) 0x00D0,
	(uint32_t) 0x00D1,
	(uint32_t) 0x00D2,
	(uint32_t) 0x00D3,
	(uint32_t) 0x00D4,
	(uint32_t) 0x00D5,
	(uint32_t) 0x00D6,
	(uint32_t) 0x00D7,
	(uint32_t) 0x00D8,
	(uint32_t) 0x00D9,
	(uint32_t) 0x00DA,
	(uint32_t) 0x00DB,
	(uint32_t) 0x00DC,
	(uint32_t) 0x00DD,
	(uint32_t) 0x00DE,
	(uint32_t) 0x00DF,
	(uint32_t) 0x00E0,
	(uint32_t) 0x00E1,
	(uint32_t) 0x00E2,
	(uint32_t) 0x00E3,
	(uint32_t) 0x00E4,
	(uint32_t) 0x00E5,
	(uint32_t) 0x00E6,
	(uint32_t) 0x00E7,
	(uint32_t) 0x00E8,
	(uint32_t) 0x00E9,
	(uint32_t) 0x00EA,
	(uint32_t) 0x00EB,
	(uint32_t) 0x00EC,
	(uint32_t) 0x00ED,
	(uint32_t) 0x00EE,
	(uint32_t) 0x00EF,
	(uint32_t) 0x00F0,
	(uint32_t) 0x00F1,
	(uint32_t) 0x00F2,
	(uint32_t) 0x00F3,
	(uint32_t) 0x00F4,
	(uint32_t) 0x00F5,
	(uint32_t) 0x00F6,
	(uint32_t) 0x00F7,
	(uint32_t) 0x00F8,
	(uint32_t) 0x00F9,
	(uint32_t) 0x00FA,
	(uint32_t) 0x00FB,
	(uint32_t) 0x00FC,
	(uint32_t) 0x00FD,
	(uint32_t) 0x00FE,
	(uint32_t) 0x00FF
};

lstring_t*
lstr_new(const char* cstr)
{
	return lstr_from_buf(cstr, strlen(cstr));
}

lstring_t*
lstr_newf(const char* fmt, ...)
{
	va_list ap;

	lstring_t* string = NULL;

	va_start(ap, fmt);
	string = lstr_vnewf(fmt, ap);
	va_end(ap);
	return string;
}

lstring_t*
lstr_vnewf(const char* fmt, va_list args)
{
	va_list ap;

	char*      buffer;
	int        buf_size;
	lstring_t* string = NULL;

	va_copy(ap, args);
	buf_size = vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);
	buffer = malloc(buf_size);
	va_copy(ap, args);
	vsnprintf(buffer, buf_size, fmt, ap);
	va_end(ap);
	string = lstr_from_buf(buffer, buf_size - 1);
	free(buffer);
	return string;
}

lstring_t*
lstr_from_buf(const char* buffer, size_t length)
{
	// this function converts plain text in CP-1252 to UTF-8.  as the Duktape API
	// will only accept UTF-8 strings, this preprocessing is necessary for full
	// compatibility with Sphere v1 scripts.

	uint32_t            cp;
	unsigned char*      out_buf;
	lstring_t*          string;
	uint32_t            utf8state = UTF8_ACCEPT;
	unsigned char       *p;
	const unsigned char *p_src;

	size_t i;

	// check that the string isn't already UTF-8-encoded
	p_src = buffer;
	for (i = 0; i < length; ++i) {
		if (utf8_decode(&utf8state, &cp, *p_src++) == UTF8_REJECT)
			break;
	}

	if (utf8state != UTF8_ACCEPT) {
		// note: UTF-8 conversion may expand the string by up to 3x
		if (!(string = malloc(sizeof(lstring_t) + length * 3 + 1)))
			return NULL;
		out_buf = (char*)string + sizeof(lstring_t);
		p = out_buf;
		p_src = buffer;
		for (i = 0; i < length; ++i) {
			cp = cp1252[*p_src++];
			utf8_encode(cp, &p);
		}
		*p = '\0';  // NUL terminator
		length = p - out_buf;
	}
	else {
		// string is already UTF-8, copy buffer as-is
		if (!(string = malloc(sizeof(lstring_t) + length + 1)))
			return NULL;
		out_buf = (char*)string + sizeof(lstring_t);
		memcpy(out_buf, buffer, length);
		out_buf[length] = '\0';  // NUL terminator
	}

	string->cstr = out_buf;
	string->length = length;
	return string;
}

lstring_t*
lstr_from_cesu8(const uint8_t* text, size_t length)
{
	// create an lstring (UTF-8) from CESU-8-encoded text.
	// the input is assumed to be mostly well-formed.  unpaired surrogates are replaced
	// with U+FFFD.

	const uint32_t REPLACEMENT = 0xFFFD;
	
	uint8_t        byte;
	uint32_t       codep;
	int            needed = 0;
	size_t         num_bytes = 0;
	int            seen = 0;
	lstring_t*     string;
	uint32_t       utf16_hi = 0x0000;
	const uint8_t* p_in;
	uint8_t*       p_out;

	// conveniently for us, UTF-8 output size <= CESU-8 input size.
	string = malloc(sizeof(lstring_t) + length + 1);
	
	p_in = text;
	p_out = (uint8_t*)string + sizeof(lstring_t);
	while (p_in < text + length) {
		byte = *p_in++;
		if (needed == 0) {
			if (byte <= 0x7f)
				num_bytes += utf8_encode(byte, &p_out);
			else if (byte >= 0xc2 && byte <= 0xdf) {
				needed = 1;
				codep = byte & 0x1f;
			}
			else if (byte >= 0xe0 && byte <= 0xef) {
				needed = 2;
				codep = byte & 0xf;
			}
			else if (byte >= 0xf0 && byte <= 0xf4) {
				needed = 3;
				codep = byte & 0x7;
			}
		}
		else {
			codep = (codep << 6) | (byte & 0x3f);
			if (++seen == needed) {
				if (codep >= 0xd800 && codep <= 0xdbff) {
					if (utf16_hi == 0x0000)
						utf16_hi = codep;
					else {
						// consecutive high surrogates, emit U+FFFD in their place
						num_bytes += utf8_encode(REPLACEMENT, &p_out);
						num_bytes += utf8_encode(REPLACEMENT, &p_out);
						utf16_hi = 0x0000;
					}
				}
				else if (codep >= 0xdc00 && codep <= 0xdfff) {
					// if we find a low surrogate without a corresponding high surrogate,
					// replace it with U+FFFD.  otherwise we now have a surrogate pair and can
					// proceed to decode it.
					codep = utf16_hi != 0x0000
						? 0x010000 + ((utf16_hi - 0xd800) << 10) + (codep - 0xdc00)
						: REPLACEMENT;
					num_bytes += utf8_encode(codep, &p_out);
					utf16_hi = 0x0000;
				}
				else {
					// in case of an outstanding surrogate, clear it and emit U+FFFD.
					if (utf16_hi != 0x0000)
						num_bytes += utf8_encode(REPLACEMENT, &p_out);
					num_bytes += utf8_encode(codep, &p_out);
					utf16_hi = 0x0000;
				}
				codep = 0x0000;
				needed = 0;
				seen = 0;
			}
		}
	}
	if (utf16_hi != 0x0000)  // outstanding surrogate?
		num_bytes += utf8_encode(REPLACEMENT, &p_out);
	*p_out = '\0';  // NUL terminator

	string->cstr = (char*)((uint8_t*)string + sizeof(lstring_t));
	string->length = num_bytes;
	return string;
}

lstring_t*
lstr_from_utf8(const uint8_t* text, size_t length, bool fatal_mode)
{
	// create a well-formed lstring from UTF-8 text which may be malformed.
	// when an encoding error is encountered, it is handled according to the value of `fatal_mode`:
	//     - if `fatal_mode` is true, conversion is aborted and the function returns NULL.
	//     - otherwise the invalid sequence is replaced with FFFD.

	const uint32_t REPLACEMENT = 0xFFFD;

	uint8_t        byte;
	uint32_t       codep;
	int            needed = 0;
	size_t         num_bytes = 0;
	int            seen = 0;
	lstring_t*     string;
	const uint8_t* p_in;
	uint8_t*       p_out;

	// worst case scenario, every byte in the source is replaced with U+FFFD, which
	// requires 3 bytes in UTF-8.
	if (!(string = malloc(sizeof(lstring_t) + length * 3 + 1)))
		return NULL;

	p_in = text;
	p_out = (uint8_t*)string + sizeof(lstring_t);
	while (p_in < text + length) {
		byte = *p_in++;
		if (needed == 0) {
			if (byte <= 0x7f)
				num_bytes += utf8_encode(byte, &p_out);
			else if (byte >= 0xc2 && byte <= 0xdf) {
				needed = 1;
				codep = byte & 0x1f;
			}
			else if (byte >= 0xe0 && byte <= 0xef) {
				needed = 2;
				codep = byte & 0xf;
			}
			else if (byte >= 0xf0 && byte <= 0xf4) {
				needed = 3;
				codep = byte & 0x7;
			}
			else {
				if (fatal_mode)
					goto abort;
				else
					num_bytes += utf8_encode(byte, &p_out);
			}
		}
		else {
			codep = (codep << 6) | (byte & 0x3f);
			if (++seen == needed) {
				if (codep >= 0xd800 && codep <= 0xdfff) {
					if (fatal_mode)
						goto abort;
					else	
						num_bytes += utf8_encode(REPLACEMENT, &p_out);
				}
				else
					num_bytes += utf8_encode(codep, &p_out);
				codep = 0x0000;
				needed = 0;
				seen = 0;
			}
		}
	}
	*p_out = '\0';  // NUL terminator

	string->cstr = (char*)((uint8_t*)string + sizeof(lstring_t));
	string->length = num_bytes;
	return string;

abort:
	free(string);
	return NULL;
}

void
lstr_free(lstring_t* string)
{
	free(string);
}

const char*
lstr_cstr(const lstring_t* string)
{
	return string->cstr;
}

int
lstr_cmp(const lstring_t* string1, const lstring_t* string2)
{
	size_t length;
	
	// the fake NUL terminator comes in REALLY handy here, as we can just
	// include it in the comparison, saving us an extra check at the end.
	length = string1->length < string2->length
		? string1->length + 1
		: string2->length + 1;
	return memcmp(string1->cstr, string2->cstr, length);
}

lstring_t*
lstr_dup(const lstring_t* string)
{
	return lstr_from_buf(string->cstr, string->length);
}

size_t
lstr_len(const lstring_t* string)
{
	return string->length;
}
