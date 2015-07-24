#include "minisphere.h"
#include "unicode.h"

#include "lstring.h"

struct lstring
{
	size_t length;
	char*  cstr;
};

// the Windows-1252 codepage
static uint32_t cp1252[256] =
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
	// courtesy of Sami Vaarala, adapted for use in minisphere

	uint32_t            cp;
	unsigned char*      out_buf;
	lstring_t*          string;
	uint32_t            utf8state = UTF8_ACCEPT;
	unsigned char       *p;
	const unsigned char *p_src;

	size_t i;

	// check that the string isn't already UTF-8
	p_src = buffer;
	for (i = 0; i < length; ++i) {
		if (utf8decode(&utf8state, &cp, *p_src++) == UTF8_REJECT)
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
			cp = cp1252[*p_src++] & 0xffffUL;
			if (cp < 0x80)
				*p++ = cp;
			else if (cp < 0x800UL) {
				*p++ = (unsigned char)(0xc0 + ((cp >> 6) & 0x1f));
				*p++ = (unsigned char)(0x80 + (cp & 0x3f));
			}
			else {
				*p++ = (unsigned char)(0xe0 + ((cp >> 12) & 0x0f));
				*p++ = (unsigned char)(0x80 + ((cp >> 6) & 0x3f));
				*p++ = (unsigned char)(0x80 + (cp & 0x3f));
			}
		}
		*p = '\0';  // tack on NUL terminator
		length = p - out_buf;
	}
	else {
		// string is already UTF-8, copy buffer as-is
		if (!(string = malloc(sizeof(lstring_t) + length + 1)))
			return NULL;
		out_buf = (char*)string + sizeof(lstring_t);
		memcpy(out_buf, buffer, length);
		out_buf[length] = '\0';  // tack on NUL terminator
	}

	string->cstr = out_buf;
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

lstring_t*
read_lstring(sfs_file_t* file, bool trim_null)
{
	long     file_pos;
	uint16_t length;

	file_pos = sfs_ftell(file);
	if (sfs_fread(&length, 2, 1, file) != 1) goto on_error;
	return read_lstring_raw(file, length, trim_null);

on_error:
	sfs_fseek(file, file_pos, SEEK_CUR);
	return NULL;
}

lstring_t*
read_lstring_raw(sfs_file_t* file, size_t length, bool trim_null)
{
	char*      buffer = NULL;
	long       file_pos;
	lstring_t* string;

	file_pos = sfs_ftell(file);
	if (!(buffer = malloc(length + 1))) goto on_error;
	if (sfs_fread(buffer, 1, length, file) != length) goto on_error;
	buffer[length] = '\0';
	if (trim_null)
		length = strchr(buffer, '\0') - buffer;
	string = lstr_from_buf(buffer, length);
	free(buffer);
	return string;

on_error:
	free(buffer);
	sfs_fseek(file, file_pos, SEEK_CUR);
	return NULL;
}

void
duk_push_lstring_t(duk_context* ctx, const lstring_t* string)
{
	duk_push_lstring(ctx, string->cstr, string->length);
}

lstring_t*
duk_require_lstring_t(duk_context* ctx, duk_idx_t index)
{
	const char* buffer;
	size_t      length;

	buffer = duk_require_lstring(ctx, index, &length);
	return lstr_from_buf(buffer, length);
}
