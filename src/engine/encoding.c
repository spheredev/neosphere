#include "minisphere.h"
#include "encoding.h"

#include "unicode.h"

struct decoder
{
	unsigned int refcount;
	bool         ignoreBOM;
	bool         fatal;
	utf8ctx_t*   utf8ctx;
};

struct encoder
{
	unsigned int refcount;
};

decoder_t*
decoder_new(bool fatal, bool ignoreBOM)
{
	decoder_t* decoder;

	decoder = calloc(1, sizeof(decoder_t));
	decoder->ignoreBOM = ignoreBOM;
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
	return decoder->ignoreBOM;
}

lstring_t*
decoder_decode(decoder_t* decoder, const uint8_t* buffer, size_t size)
{
	uint32_t       codepoint;
	utf8ctx_t*     ctx;
	utf8_ret_t     ret;
	lstring_t*     string;
	uint8_t*       text;
	const uint8_t* p_in;
	uint8_t*       p_out;

	text = malloc(size * 3);
	p_in = buffer;
	p_out = text;

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
		cesu8_emit(codepoint, &p_out);
	}
	ret = utf8_decode_end(ctx);
	if (ret >= UTF8_ERROR) {
		if (!decoder->fatal)
			cesu8_emit(0xFFFD, &p_out);
		else
			goto on_error;
	}
	string = lstr_from_cp1252(text, p_out - text);
	return string;

on_error:
	free(text);
	return NULL;
}
