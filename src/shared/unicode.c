// utf8_decode function (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

#include "unicode.h"

#include <stddef.h>
#include <stdint.h>

static const uint8_t utf8d[] =
{
	// The first part of the table maps bytes to character classes that
	// to reduce the size of the transition table and create bitmasks.
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	 8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

	// The second part is a transition table that maps a combination
	// of a state of the automaton and a character class to a state.
	 0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
	12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
	12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
	12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
	12,36,12,12,12,12,12,12,12,12,12,12, 
};

uint32_t
utf8_decode(uint32_t* state, uint32_t* codep, uint8_t byte)
{
	uint32_t type = utf8d[byte];

	*codep = (*state != UTF8_ACCEPT)
		? (byte & 0x3fu) | (*codep << 6)
		: (0xff >> type) & (byte);

	*state = utf8d[256 + *state + type];
	return *state;
}

size_t
utf8_encode(uint32_t codep, uint8_t* buffer)
{
	// canonical UTF-8: code points above U+FFFF are encoded directly (i.e. not as
	// surrogate pairs).  May emit up to 4 bytes per code point.  Surrogate code
	// points and code points beyond U+10FFFF are not legal and will not be encoded.
	
	if (codep <= 0x007f) {
		buffer[0] = (uint8_t)codep;
		return 1;
	}
	else if (codep <= 0x07ff) {
		buffer[0] = (codep >> 6) + 0xc0;
		buffer[1] = 0x80 + (codep & 0x3f);
		return 2;
	}
	else if (codep <= 0xffff) {
		if (codep >= 0xd800 && codep <= 0xdfff)
			return 0;  // don't encode surrogates
		buffer[0] = (codep >> 12) + 0xe0;
		buffer[1] = 0x80 + (codep >> 6 & 0x3f);
		buffer[2] = 0x80 + (codep & 0x3f);
		return 3;
	}
	else if (codep <= 0x10ffff) {
		buffer[0] = (codep >> 18) + 0xf0;
		buffer[1] = 0x80 + (codep >> 12 & 0x3f);
		buffer[2] = 0x80 + (codep >> 6 & 0x3f);
		buffer[3] = 0x80 + (codep & 0x3f);
		return 4;
	}
	else {
		// not a legal Unicode code point, don't encode
		return 0;
	}
}
