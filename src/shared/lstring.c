/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

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
static const
uint16_t CP1252_TO_UNICODE[256] =
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
	return lstr_from_utf8(cstr, strlen(cstr), false);
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
lstr_from_cp1252(const char* text, size_t length)
{
	// create an lstring from plain text.  CP-1252 is assumed.  as Duktape
	// expects JS code to be CESU-8 encoded, this functionality is needed for
	// full compatibility with Sphere v1 scripts.

	uint8_t*       buffer;
	uint32_t       codepoint;
	uint8_t*       input;
	bool           is_utf8 = true;
	lstring_t*     string;
	utf8_decode_t* utf8;
	uint8_t        *p_out;
	const uint8_t  *p_in;

	size_t i;

	input = (uint8_t*)text;

	// check that the string isn't already properly encoded
	utf8 = utf8_decode_start(true);
	p_in = input;
	while (p_in < input + length && is_utf8) {
		if (utf8_decode_next(utf8, *p_in++, NULL) >= UTF8_ERROR)
			is_utf8 = false;
	}
	if (utf8_decode_end(utf8) >= UTF8_ERROR)
		is_utf8 = false;

	if (!is_utf8) {
		// note: UTF-8 conversion may expand the string by up to 3x
		if (!(string = malloc(sizeof(lstring_t) + length * 3 + 1)))
			return NULL;
		buffer = (uint8_t*)string + sizeof(lstring_t);
		p_out = buffer;
		p_in = input;
		for (i = 0; i < length; ++i) {
			codepoint = CP1252_TO_UNICODE[*p_in++];
			utf8_emit(codepoint, &p_out);
		}
		*p_out = '\0';  // NUL terminator
		length = p_out - buffer;
	}
	else {
		// string is already UTF-8/CESU-8, copy buffer as-is
		if (!(string = malloc(sizeof(lstring_t) + length + 1)))
			return NULL;
		buffer = (uint8_t*)string + sizeof(lstring_t);
		memcpy(buffer, text, length);
		buffer[length] = '\0';  // NUL terminator
	}

	string->cstr = (char*)buffer;
	string->length = length;
	return string;
}

lstring_t*
lstr_from_utf8(const char* text, size_t length, bool strip_bom)
{
	// create an lstring from plain text.  CP-1252 is assumed.  as Duktape
	// expects JS code to be CESU-8 encoded, this functionality is needed for
	// full compatibility with Sphere v1 scripts.

	uint8_t*       buffer;
	utf8_decode_t* codec;
	uint32_t       codepoint;
	uint8_t*       input;
	utf8_ret_t     state;
	lstring_t*     string;
	uint8_t        *p_out;
	const uint8_t  *p_in;

	input = (uint8_t*)text;

	// in the worst case, conversion can expand the string by up to 4x.
	if (!(string = malloc(sizeof(lstring_t) + length * 4 + 1)))
		return NULL;

	// decode the string; emit U+FFFD after any stream error
	buffer = ((uint8_t*)string) + sizeof(lstring_t);
	p_out = &buffer[0];
	p_in = input;
	codec = utf8_decode_start(true);
	while (p_in < input + length) {
		state = utf8_decode_next(codec, *p_in++, &codepoint);
		if (state == UTF8_CONTINUE)
			continue;
		if (state == UTF8_CODEPOINT) {
			if (codepoint != 0xfeff || !strip_bom)
				utf8_emit(codepoint, &p_out);
			strip_bom = false;
		}
		else {
			utf8_emit(0xfffd, &p_out);
			if (state == UTF8_RETRY)
				--p_in;
		}
	}
	if (utf8_decode_end(codec) >= UTF8_ERROR)
		utf8_emit(0xfffd, &p_out);
	*p_out = '\0';  // NUL terminator

	length = p_out - buffer;

	string->cstr = (char*)buffer;
	string->length = length;
	return string;
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

lstring_t*
lstr_cat(const lstring_t* string1, const lstring_t* string2)
{
	char*      buffer;
	size_t     length;
	lstring_t* string;

	length = string1->length + string2->length;
	if (!(string = malloc(sizeof(lstring_t) + length + 1)))
		return NULL;
	buffer = (char*)string + sizeof(lstring_t);
	memcpy(buffer, string1->cstr, string1->length);
	memcpy(buffer + string1->length, string2->cstr, string2->length);
	buffer[length] = '\0';  // NUL terminator

	string->cstr = buffer;
	string->length = length;
	return string;
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
