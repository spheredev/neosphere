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

#include "encoding.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "lstring.h"
#include "unicode.h"

struct decoder
{
	unsigned int   refcount;
	bool           bom_seen;
	bool           ignore_bom;
	bool           fatal;
	utf8_decode_t* stream;
};

struct encoder
{
	unsigned int refcount;
};

decoder_t*
decoder_new(bool fatal, bool ignore_bom)
{
	decoder_t* decoder;

	if (!(decoder = calloc(1, sizeof(decoder_t))))
		return NULL;
	decoder->fatal = fatal;
	decoder->ignore_bom = ignore_bom;
	return decoder_ref(decoder);
}

decoder_t*
decoder_ref(decoder_t* decoder)
{
	++decoder->refcount;
	return decoder;
}

void
decoder_free(decoder_t* decoder)
{
	if (--decoder->refcount > 0)
		return;
	free(decoder);
}

bool
decoder_fatal(const decoder_t* decoder)
{
	return decoder->fatal;
}

bool
decoder_ignore_bom(const decoder_t* decoder)
{
	return decoder->ignore_bom;
}

lstring_t*
decoder_finish(decoder_t* decoder)
{
	uint8_t    output[6];
	utf8_ret_t state;
	uint8_t    *p_out;

	p_out = output;
	state = utf8_decode_end(decoder->stream);
	decoder->stream = NULL;
	decoder->bom_seen = false;
	if (state >= UTF8_ERROR) {
		if (!decoder->fatal)
			utf8_emit(0xFFFD, &p_out);
		else
			return NULL;
	}
	return lstr_from_utf8((char*)output, p_out - output, false);
}

lstring_t*
decoder_run(decoder_t* decoder, const uint8_t* buffer, size_t size)
{
	uint32_t      codepoint;
	uint8_t*      output;
	utf8_ret_t    state;
	lstring_t*    string;
	const uint8_t *p_in;
	uint8_t       *p_out;

	output = malloc(size * 3);
	p_in = buffer;
	p_out = output;

	if (decoder->stream == NULL)
		decoder->stream = utf8_decode_start(true);
	while (p_in < buffer + size) {
		state = utf8_decode_next(decoder->stream, *p_in++, &codepoint);
		if (state == UTF8_CONTINUE)
			continue;
		if (state == UTF8_RETRY)
			--p_in;
		if (state >= UTF8_ERROR) {
			if (!decoder->fatal)
				codepoint = 0xFFFD;
			else
				goto on_error;
		}
		if (codepoint != 0xFEFF || decoder->ignore_bom || decoder->bom_seen)
			utf8_emit(codepoint, &p_out);
		decoder->bom_seen = true;
	}
	string = lstr_from_utf8((char*)output, p_out - output, false);
	free(output);
	return string;

on_error:
	free(output);
	return NULL;
}

encoder_t*
encoder_new(void)
{
	encoder_t* encoder;

	encoder = calloc(1, sizeof(encoder_t));
	return encoder_ref(encoder);
}

encoder_t*
encoder_ref(encoder_t* encoder)
{
	++encoder->refcount;
	return encoder;
}

void
encoder_free(encoder_t* encoder)
{
	if (--encoder->refcount > 0)
		return;
	free(encoder);
}

uint8_t*
encoder_run(encoder_t* encoder, const lstring_t* string, size_t *out_size)
{
	uint8_t*       output;
	uint32_t       codepoint;
	utf8_decode_t* cx;
	size_t         length;
	uint8_t*       text;
	const uint8_t* p_in;
	uint8_t*       p_out;

	text = (uint8_t*)lstr_cstr(string);
	length = lstr_len(string);
	if (!(output = malloc(length)))
		return NULL;
	p_in = (uint8_t*)lstr_cstr(string);
	p_out = output;

	cx = utf8_decode_start(false);
	while (p_in < text + length) {
		// no error check is needed because lstring_t is well-formed by definition.
		// barring memory corruption or some such shenanigans, a decoding error here
		// can't possibly happen.  I know, famous last words and all...
		while (utf8_decode_next(cx, *p_in++, &codepoint) == UTF8_CONTINUE);
		utf8_emit(codepoint, &p_out);
	}
	utf8_decode_end(cx);
	*out_size = p_out - output;
	return output;
}
