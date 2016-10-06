#include "minisphere.h"
#include "encoding.h"

#include "unicode.h"

struct decoder
{
	unsigned int refcount;
	bool         bom_seen;
	utf8ctx_t*   context;
	bool         ignore_bom;
	bool         fatal;
};

struct encoder
{
	unsigned int refcount;
};

decoder_t*
decoder_new(bool fatal, bool ignore_bom)
{
	decoder_t* decoder;

	decoder = calloc(1, sizeof(decoder_t));
	decoder->ignore_bom = ignore_bom;
	decoder->fatal = fatal;
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
decoder_ignoreBOM(const decoder_t* decoder)
{
	return decoder->ignore_bom;
}

void
decoder_reset(decoder_t* decoder)
{
	decoder->bom_seen = false;
}

lstring_t*
decoder_run(decoder_t* decoder, const uint8_t* buffer, size_t size)
{
	uint32_t       codepoint;
	utf8ctx_t*     ctx;
	utf8_ret_t     ret;
	uint8_t*       output;
	lstring_t*     string;
	const uint8_t* p_in;
	uint8_t*       p_out;

	output = malloc(size * 3);
	p_in = buffer;
	p_out = output;

	ctx = utf8_decode_start(true);
	while (p_in < buffer + size) {
		while ((ret = utf8_decode_next(ctx, *p_in++, &codepoint)) == UTF8_CONTINUE);
		if (ret == UTF8_RETRY)
			--p_in;
		if (ret >= UTF8_ERROR) {
			if (!decoder->fatal)
				codepoint = 0xFFFD;
			else
				goto on_error;
		}
		if (codepoint != 0xFEFF || decoder->ignore_bom || decoder->bom_seen)
			cesu8_emit(codepoint, &p_out);
		decoder->bom_seen = true;
	}
	ret = utf8_decode_end(ctx);
	if (ret >= UTF8_ERROR) {
		if (!decoder->fatal)
			cesu8_emit(0xFFFD, &p_out);
		else
			goto on_error;
	}
	string = lstr_from_cp1252(output, p_out - output);
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
	utf8ctx_t*     ctx;
	size_t         length;
	uint8_t*       text;
	const uint8_t* p_in;
	uint8_t*       p_out;

	text = (uint8_t*)lstr_cstr(string);
	length = lstr_len(string);
	if (!(output = malloc(length)))
		return NULL;
	p_in = lstr_cstr(string);
	p_out = output;

	ctx = utf8_decode_start(false);
	while (p_in < text + length) {
		while (utf8_decode_next(ctx, *p_in++, &codepoint) == UTF8_CONTINUE);
		utf8_emit(codepoint, &p_out);
	}
	utf8_decode_end(ctx);
	*out_size = p_out - output;
	return output;
}
