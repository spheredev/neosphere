#include "minisphere.h"

#include "lstring.h"

lstring_t*
new_lstring(size_t length, const char* buffer)
{
	lstring_t* lstring = NULL;

	if ((lstring = malloc(sizeof(lstring_t))) == NULL) goto on_error;
	if ((lstring->cstr = malloc(length + 1)) == NULL) goto on_error;
	lstring->length = length;
	memcpy((char*)lstring->cstr, buffer, length);
	((char*)lstring->cstr)[length] = '\0';
	return lstring;

on_error:
	if (lstring != NULL) {
		free((char*)lstring->cstr);
		free(lstring);
	}
	return NULL;
}

lstring_t*
read_lstring(ALLEGRO_FILE* file)
{
	uint16_t   length;
	int64_t    old_file_pos;

	old_file_pos = al_ftell(file);
	if (al_fread(file, &length, 2) != 2) goto on_error;
	return read_lstring_s(file, length);

on_error:
	al_fseek(file, old_file_pos, ALLEGRO_SEEK_CUR);
	return NULL;
}

lstring_t*
read_lstring_s(ALLEGRO_FILE* file, size_t length)
{
	int64_t    old_file_pos;
	lstring_t* string = NULL;

	old_file_pos = al_ftell(file);
	if ((string = calloc(1, sizeof(lstring_t))) == NULL)
		goto on_error;
	string->length = length;
	if ((string->cstr = calloc(length + 1, sizeof(char))) == NULL) goto on_error;
	if (al_fread(file, (char*)string->cstr, length) != length) goto on_error;
	return string;

on_error:
	al_fseek(file, old_file_pos, ALLEGRO_SEEK_CUR);
	if (string != NULL) {
		free((char*)string->cstr);
		free(string);
	}
	return NULL;
}

void
free_lstring(lstring_t* string)
{
	if (string != NULL) free((char*)string->cstr);
	free(string);
}

lstring_t*
duk_require_lstring_t(duk_context* ctx, duk_idx_t index)
{
	const char* buffer;
	size_t      length;

	buffer = duk_require_lstring(ctx, index, &length);
	return new_lstring(length, buffer);
}
