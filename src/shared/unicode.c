#include "unicode.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct utf8ctx
{
	int            bytes_needed;
	int            bytes_seen;
	uint32_t       codepoint;
	uint32_t       lead;
	bool           strict;
	uint8_t        utf8_high;
	uint8_t        utf8_low;
};

size_t
cesu8_emit(uint32_t codepoint, uint8_t* *p_ptr)
{
	// CESU-8: code points above U+FFFF are encoded as a surrogate pair, like UTF-16.
	// may emit up to 6 bytes.  surrogate code points themselves and code points beyond
	// U+10FFFF are not legal and will not be encoded.

	uint32_t utf16_hi;
	uint32_t utf16_lo;

	if (codepoint <= 0xffff)
		return utf8_emit(codepoint, p_ptr);
	else if (codepoint <= 0x10ffff) {
		utf16_hi = 0xd800 + ((codepoint - 0x010000) >> 10);
		utf16_lo = 0xdc00 + ((codepoint - 0x010000) & 0x3ff);
		*(*p_ptr)++ = (utf16_hi >> 12) + 0xe0;
		*(*p_ptr)++ = 0x80 + (utf16_hi >> 6 & 0x3f);
		*(*p_ptr)++ = 0x80 + (utf16_hi & 0x3f);
		*(*p_ptr)++ = (utf16_lo >> 12) + 0xe0;
		*(*p_ptr)++ = 0x80 + (utf16_lo >> 6 & 0x3f);
		*(*p_ptr)++ = 0x80 + (utf16_lo & 0x3f);
		return 6;
	}
	else {
		// not a legal Unicode code point, don't encode
		return 0;
	}
}

utf8ctx_t*
utf8_decode_start(bool strict)
{
	utf8ctx_t* ctx;

	ctx = calloc(1, sizeof(utf8ctx_t));
	ctx->strict = strict;
	ctx->utf8_low = 0x80;
	ctx->utf8_high = 0xbf;
	return ctx;
}

utf8_ret_t
utf8_decode_end(utf8ctx_t* ctx)
{
	bool is_ok;

	is_ok = ctx->bytes_needed == 0;
	free(ctx);
	return is_ok
		? UTF8_OK
		: UTF8_ERROR;
}

utf8_ret_t
utf8_decode_next(utf8ctx_t* ctx, uint8_t byte, uint32_t *out_codepoint)
{
	// UTF-8 streaming decoder based on WHATWG Encoding specification:
	// https://encoding.spec.whatwg.org/#utf-8-decoder

	// note: this decoder supports both strict UTF-8 as well as CESU-8.  CESU-8 is enabled
	//       by passing false for the `strict` argument of utf8_decode_start().

	if (ctx->bytes_needed == 0) {
		if (byte <= 0x7f) {
			if (out_codepoint)
				*out_codepoint = byte;
			return UTF8_CODEPOINT;
		}
		else if (byte >= 0xc2 && byte <= 0xdf) {
			ctx->bytes_needed = 2;
			ctx->codepoint = byte & 0x1f;
			return UTF8_CONTINUE;
		}
		else if (byte >= 0xe0 && byte <= 0xef) {
			if (byte == 0xe0 && ctx->strict)
				ctx->utf8_low = 0xa0;
			if (byte == 0xed && ctx->strict)
				ctx->utf8_high = 0x9f;
			ctx->bytes_needed = 2;
			ctx->codepoint = byte & 0xf;
			return UTF8_CONTINUE;
		}
		else if (byte >= 0xf0 && byte <= 0xf4) {
			if (byte == 0xf0 && ctx->strict)
				ctx->utf8_low = 0x90;
			if (byte == 0xf4 && ctx->strict)
				ctx->utf8_high = 0x8f;
			ctx->bytes_needed = 3;
			ctx->codepoint = byte & 0x7;
			return UTF8_CONTINUE;
		}
		else {
			return UTF8_ERROR;
		}
	}
	else {
		if (byte >= ctx->utf8_low && byte <= ctx->utf8_high) {
			ctx->utf8_low = 0x80;
			ctx->utf8_high = 0xbf;
			ctx->codepoint = (ctx->codepoint << 6) | (byte & 0x3f);
			if (++ctx->bytes_seen < ctx->bytes_needed)
				return UTF8_CONTINUE;
			else if (ctx->codepoint < 0xd800 || ctx->codepoint > 0xdfff) {
				if (out_codepoint)
					*out_codepoint = ctx->codepoint;
				ctx->codepoint = 0x0000;
				ctx->bytes_needed = 0;
				ctx->bytes_seen = 0;
				return UTF8_CODEPOINT;
			}
			else if (ctx->codepoint >= 0xd800 && ctx->codepoint <= 0xdbff && ctx->lead == 0x0000) {
				ctx->lead = ctx->codepoint;
				ctx->codepoint = 0x0000;
				ctx->bytes_needed = 0;
				ctx->bytes_seen = 0;
				return UTF8_CONTINUE;
			}
			else if (ctx->codepoint >= 0xdc00 && ctx->codepoint <= 0xdfff && ctx->lead != 0x0000) {
				if (out_codepoint) {
					*out_codepoint = 0x010000
						| ((ctx->lead - 0xd800) << 10)
						| (ctx->codepoint - 0xdc00);
				}
				ctx->lead = 0x0000;
				ctx->codepoint = 0x0000;
				ctx->bytes_needed = 0;
				ctx->bytes_seen = 0;
				return UTF8_CODEPOINT;
			}
			else {
				ctx->codepoint = 0x0000;
				ctx->bytes_needed = 0;
				ctx->bytes_seen = 0;
				return UTF8_ERROR;
			}
		}
		else {
			// this is a dangerous situation.  we've just encountered an invalid continuation
			// byte which may in fact be the initial byte of another character; if we signal
			// an encoding error here, that character would be masked.  to try to avoid that, we signal
			// "retry" so the caller knows to re-send the last byte.
			ctx->codepoint = 0x0000;
			ctx->bytes_needed = 0;
			ctx->bytes_seen = 0;
			ctx->utf8_low = 0x80;
			ctx->utf8_high = 0xbf;
			return UTF8_RETRY;
		}
	}
}

size_t
utf8_emit(uint32_t codepoint, uint8_t* *p_ptr)
{
	// canonical UTF-8: code points above U+FFFF are encoded directly (i.e. not as
	// surrogate pairs).  may emit up to 4 bytes.  surrogate code points and code points
	// beyond U+10FFFF are not legal and will not be encoded.

	if (codepoint <= 0x007f) {
		*(*p_ptr)++ = (uint8_t)codepoint;
		return 1;
	}
	else if (codepoint <= 0x07ff) {
		*(*p_ptr)++ = (codepoint >> 6) + 0xc0;
		*(*p_ptr)++ = 0x80 + (codepoint & 0x3f);
		return 2;
	}
	else if (codepoint <= 0xffff) {
		if (codepoint >= 0xd800 && codepoint <= 0xdfff)
			return 0;  // don't encode surrogates
		*(*p_ptr)++ = (codepoint >> 12) + 0xe0;
		*(*p_ptr)++ = 0x80 + (codepoint >> 6 & 0x3f);
		*(*p_ptr)++ = 0x80 + (codepoint & 0x3f);
		return 3;
	}
	else if (codepoint <= 0x10ffff) {
		*(*p_ptr)++ = (codepoint >> 18) + 0xf0;
		*(*p_ptr)++ = 0x80 + (codepoint >> 12 & 0x3f);
		*(*p_ptr)++ = 0x80 + (codepoint >> 6 & 0x3f);
		*(*p_ptr)++ = 0x80 + (codepoint & 0x3f);
		return 4;
	}
	else {
		// not a legal Unicode code point, don't encode
		return 0;
	}
}
