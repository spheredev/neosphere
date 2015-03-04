#include "minisphere.h"

#include "lstring.h"

lstring_t*
new_lstring(size_t length, const char* buffer)
{
	lstring_t* lstring = NULL;

	if ((lstring = malloc(sizeof(lstring_t))) == NULL) goto on_error;
	if ((lstring->cstr = malloc(length + 1)) == NULL) goto on_error;
	lstring->length = length;
	memcpy(lstring->cstr, buffer, length);
	lstring->cstr[length] = '\0';
	return lstring;

on_error:
	if (lstring != NULL) {
		free(lstring->cstr);
		free(lstring);
	}
	return NULL;
}

void
free_lstring(lstring_t* string)
{
	if (string != NULL) free(string->cstr);
	free(string);
}

lstring_t*
al_fread_lstring(ALLEGRO_FILE* file)
{
	lstring_t* string = NULL;
	uint16_t   length;

	if ((string = calloc(1, sizeof(lstring_t))) == NULL)
		goto on_error;
	if (al_fread(file, &length, 2) != 2) goto on_error;
	string->length = length;
	if ((string->cstr = calloc(length + 1, sizeof(char))) == NULL) goto on_error;
	if (al_fread(file, string->cstr, length) != length) goto on_error;
	return string;

on_error:
	if (string != NULL) {
		free(string->cstr);
		free(string);
	}
	return NULL;
}

lstring_t*
duk_require_lstring_t(duk_context* ctx, duk_idx_t index)
{
	const char* buffer;
	size_t      length;

	buffer = duk_require_lstring(ctx, index, &length);
	return new_lstring(length, buffer);
}
