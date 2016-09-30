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
	size_t         length;
	const uint8_t* ptr;
	const uint8_t* text;
	uint8_t        utf8_high;
	uint8_t        utf8_low;
};

size_t
cesu8_emit(uint32_t codep, uint8_t* *p_ptr)
{
	// CESU-8: code points above U+FFFF are encoded as a surrogate pair, like UTF-16.
	// may emit up to 6 bytes.  surrogate code points themselves and code points beyond
	// U+10FFFF are not legal and will not be encoded.

	uint32_t utf16_hi;
	uint32_t utf16_lo;

	if (codep <= 0xffff)
		return utf8_emit(codep, p_ptr);
	else if (codep <= 0x10ffff) {
		utf16_hi = 0xd800 + ((codep - 0x010000) >> 10);
		utf16_lo = 0xdc00 + ((codep - 0x010000) & 0x3ff);
		*(*p_ptr)++ = (utf16_hi >> 12) + 0xe0;
		*(*p_ptr)++ = 0x80 + (utf16_hi >> 6 & 0x3f);
		*(*p_ptr)++ = 0x80 + (utf16_hi & 0x3f);
		*(*p_ptr)++ = (utf16_lo >> 12) + 0xe0;
		*(*p_ptr)++ = 0x80 + (utf16_lo >> 6 & 0x3f);
		*(*p_ptr)++ = 0x80 + (utf16_lo & 0x3f);
		return 4;
	}
	else {
		// not a legal Unicode code point, don't encode
		return 0;
	}
}

utf8ctx_t*
utf8_decode(uint8_t* text, size_t length)
{
	utf8ctx_t* ctx;

	ctx = calloc(1, sizeof(utf8ctx_t));
	ctx->utf8_low = 0x80;
	ctx->utf8_high = 0xbf;
	ctx->text = text;
	ctx->length = length;
	ctx->ptr = ctx->text;
	return ctx;
}

size_t
utf8_emit(uint32_t codep, uint8_t* *p_ptr)
{
	// canonical UTF-8: code points above U+FFFF are encoded directly (i.e. not as
	// surrogate pairs).  may emit up to 4 bytes.  surrogate code points and code points
	// beyond U+10FFFF are not legal and will not be encoded.

	if (codep <= 0x007f) {
		*(*p_ptr)++ = (uint8_t)codep;
		return 1;
	}
	else if (codep <= 0x07ff) {
		*(*p_ptr)++ = (codep >> 6) + 0xc0;
		*(*p_ptr)++ = 0x80 + (codep & 0x3f);
		return 2;
	}
	else if (codep <= 0xffff) {
		if (codep >= 0xd800 && codep <= 0xdfff)
			return 0;  // don't encode surrogates
		*(*p_ptr)++ = (codep >> 12) + 0xe0;
		*(*p_ptr)++ = 0x80 + (codep >> 6 & 0x3f);
		*(*p_ptr)++ = 0x80 + (codep & 0x3f);
		return 3;
	}
	else if (codep <= 0x10ffff) {
		*(*p_ptr)++ = (codep >> 18) + 0xf0;
		*(*p_ptr)++ = 0x80 + (codep >> 12 & 0x3f);
		*(*p_ptr)++ = 0x80 + (codep >> 6 & 0x3f);
		*(*p_ptr)++ = 0x80 + (codep & 0x3f);
		return 4;
	}
	else {
		// not a legal Unicode code point, don't encode
		return 0;
	}
}

utf8_ret_t
utf8_next(utf8ctx_t* ctx, uint32_t *out_codepoint)
{
	// UTF-8 streaming decoder based on WHATWG Encoding specification:
	// https://encoding.spec.whatwg.org/#utf-8-decoder
	
	uint8_t byte;

	// check for end-of-stream.  if there's nothing else to do, free the decoding
	// context and return OK.
	if (ctx->ptr >= ctx->text + ctx->length) {
		if (ctx->bytes_needed == 0) {
			free(ctx);
			return UTF8_OK;
		}
		else {
			ctx->bytes_needed = 0;
			return UTF8_ERROR;
		}
	}
	
	// process the next byte in the stream
	byte = *ctx->ptr++;
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
			if (byte == 0xe0)
				ctx->utf8_low = 0xa0;
			if (byte == 0xed)
				ctx->utf8_high = 0x9f;
			ctx->bytes_needed = 2;
			ctx->codepoint = byte & 0xf;
			return UTF8_CONTINUE;
		}
		else if (byte >= 0xf0 && byte <= 0xf4) {
			if (byte == 0xf0)
				ctx->utf8_low = 0x90;
			if (byte == 0xf4)
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
			else {
				if (out_codepoint)
					*out_codepoint = ctx->codepoint;
				ctx->codepoint = 0x0000;
				ctx->bytes_needed = 0;
				ctx->bytes_seen = 0;
				return UTF8_CODEPOINT;
			}

		}
		else {
			ctx->codepoint = 0x0000;
			ctx->bytes_needed = 0;
			ctx->bytes_seen = 0;
			ctx->utf8_low = 0x80;
			ctx->utf8_high = 0xbf;
			--ctx->ptr;  // prevent masking
			return UTF8_ERROR;
		}
	}
}
