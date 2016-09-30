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
	0x0000,
	0x0001,
	0x0002,
	0x0003,
	0x0004,
	0x0005,
	0x0006,
	0x0007,
	0x0008,
	0x0009,
	0x000A,
	0x000B,
	0x000C,
	0x000D,
	0x000E,
	0x000F,
	0x0010,
	0x0011,
	0x0012,
	0x0013,
	0x0014,
	0x0015,
	0x0016,
	0x0017,
	0x0018,
	0x0019,
	0x001A,
	0x001B,
	0x001C,
	0x001D,
	0x001E,
	0x001F,
	0x0020,
	0x0021,
	0x0022,
	0x0023,
	0x0024,
	0x0025,
	0x0026,
	0x0027,
	0x0028,
	0x0029,
	0x002A,
	0x002B,
	0x002C,
	0x002D,
	0x002E,
	0x002F,
	0x0030,
	0x0031,
	0x0032,
	0x0033,
	0x0034,
	0x0035,
	0x0036,
	0x0037,
	0x0038,
	0x0039,
	0x003A,
	0x003B,
	0x003C,
	0x003D,
	0x003E,
	0x003F,
	0x0040,
	0x0041,
	0x0042,
	0x0043,
	0x0044,
	0x0045,
	0x0046,
	0x0047,
	0x0048,
	0x0049,
	0x004A,
	0x004B,
	0x004C,
	0x004D,
	0x004E,
	0x004F,
	0x0050,
	0x0051,
	0x0052,
	0x0053,
	0x0054,
	0x0055,
	0x0056,
	0x0057,
	0x0058,
	0x0059,
	0x005A,
	0x005B,
	0x005C,
	0x005D,
	0x005E,
	0x005F,
	0x0060,
	0x0061,
	0x0062,
	0x0063,
	0x0064,
	0x0065,
	0x0066,
	0x0067,
	0x0068,
	0x0069,
	0x006A,
	0x006B,
	0x006C,
	0x006D,
	0x006E,
	0x006F,
	0x0070,
	0x0071,
	0x0072,
	0x0073,
	0x0074,
	0x0075,
	0x0076,
	0x0077,
	0x0078,
	0x0079,
	0x007A,
	0x007B,
	0x007C,
	0x007D,
	0x007E,
	0x007F,
	0x20AC,
	0xFFFD,  /* undefined */
	0x201A,
	0x0192,
	0x201E,
	0x2026,
	0x2020,
	0x2021,
	0x02C6,
	0x2030,
	0x0160,
	0x2039,
	0x0152,
	0xFFFD,  /* undefined */
	0x017D,
	0xFFFD,  /* undefined */
	0xFFFD,  /* undefined */
	0x2018,
	0x2019,
	0x201C,
	0x201D,
	0x2022,
	0x2013,
	0x2014,
	0x02DC,
	0x2122,
	0x0161,
	0x203A,
	0x0153,
	0xFFFD,  /* undefined */
	0x017E,
	0x0178,
	0x00A0,
	0x00A1,
	0x00A2,
	0x00A3,
	0x00A4,
	0x00A5,
	0x00A6,
	0x00A7,
	0x00A8,
	0x00A9,
	0x00AA,
	0x00AB,
	0x00AC,
	0x00AD,
	0x00AE,
	0x00AF,
	0x00B0,
	0x00B1,
	0x00B2,
	0x00B3,
	0x00B4,
	0x00B5,
	0x00B6,
	0x00B7,
	0x00B8,
	0x00B9,
	0x00BA,
	0x00BB,
	0x00BC,
	0x00BD,
	0x00BE,
	0x00BF,
	0x00C0,
	0x00C1,
	0x00C2,
	0x00C3,
	0x00C4,
	0x00C5,
	0x00C6,
	0x00C7,
	0x00C8,
	0x00C9,
	0x00CA,
	0x00CB,
	0x00CC,
	0x00CD,
	0x00CE,
	0x00CF,
	0x00D0,
	0x00D1,
	0x00D2,
	0x00D3,
	0x00D4,
	0x00D5,
	0x00D6,
	0x00D7,
	0x00D8,
	0x00D9,
	0x00DA,
	0x00DB,
	0x00DC,
	0x00DD,
	0x00DE,
	0x00DF,
	0x00E0,
	0x00E1,
	0x00E2,
	0x00E3,
	0x00E4,
	0x00E5,
	0x00E6,
	0x00E7,
	0x00E8,
	0x00E9,
	0x00EA,
	0x00EB,
	0x00EC,
	0x00ED,
	0x00EE,
	0x00EF,
	0x00F0,
	0x00F1,
	0x00F2,
	0x00F3,
	0x00F4,
	0x00F5,
	0x00F6,
	0x00F7,
	0x00F8,
	0x00F9,
	0x00FA,
	0x00FB,
	0x00FC,
	0x00FD,
	0x00FE,
	0x00FF
};

lstring_t*
lstr_new(const char* cstr)
{
	return lstr_from_cp1252(cstr, strlen(cstr));
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
	string = lstr_from_cp1252(buffer, buf_size - 1);
	free(buffer);
	return string;
}

lstring_t*
lstr_from_cesu8(const uint8_t* text, size_t length)
{
	// create an lstring from CESU-8-encoded text.
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
lstr_from_cp1252(const char* text, size_t length)
{
	// create an lstring from plain text.  CP-1252 is assumed.  as Duktape
	// expects JS code to be CESU-8 encoded, this functionality is needed for
	// full compatibility with Sphere v1 scripts.

	uint32_t            cp;
	unsigned char*      out_buf;
	lstring_t*          string;
	uint32_t            utf8state = UTF8_ACCEPT;
	unsigned char       *p;
	const unsigned char *p_src;

	size_t i;

	// check that the string isn't actually already UTF-8
	p_src = text;
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
		p_src = text;
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
		memcpy(out_buf, text, length);
		out_buf[length] = '\0';  // NUL terminator
	}

	string->cstr = out_buf;
	string->length = length;
	return string;
}

lstring_t*
lstr_from_utf8(const uint8_t* text, size_t length, bool fatal_mode)
{
	// create an lstring from UTF-8 text which may be malformed.
	// when an encoding error is encountered, it is handled according to the value of `fatal_mode`:
	//     - if `fatal_mode` is true, conversion is aborted and the function returns NULL.
	//     - otherwise the invalid sequence of bytes is replaced with FFFD.

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
	// requires 3 bytes to encode.
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
					num_bytes += utf8_encode(byte, &p_out);
				else
					goto abort;
			}
		}
		else {
			codep = (codep << 6) | (byte & 0x3f);
			if (++seen == needed) {
				if (codep >= 0xd800 && codep <= 0xdfff) {
					if (!fatal_mode)
						num_bytes += utf8_encode(REPLACEMENT, &p_out);
					else
						goto abort;
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
	return lstr_from_cp1252(string->cstr, string->length);
}

size_t
lstr_len(const lstring_t* string)
{
	return string->length;
}
