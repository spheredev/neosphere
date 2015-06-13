#include "minisphere.h"

#include "lstring.h"

lstring_t*
new_lstring(const char* fmt, ...)
{
	va_list ap;

	int        buf_size;
	lstring_t* lstring;
	char*      output_str = NULL;
	
	if ((lstring = malloc(sizeof(lstring_t))) == NULL)
		goto on_error;
	va_start(ap, fmt);
	buf_size = vsnprintf(NULL, 0, fmt, ap) + 1;
	if (!(output_str = malloc(buf_size))) goto on_error;
	va_start(ap, fmt);
	vsnprintf(output_str, buf_size, fmt, ap);
	output_str[buf_size - 1] = '\0';
	lstring->cstr = output_str;
	lstring->length = buf_size - 1;
	return lstring;

on_error:
	free(output_str);
	return NULL;
}

lstring_t*
lstring_from_buf(size_t length, const char* buffer)
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
lstring_from_cstr(const char* cstr)
{
	return lstring_from_buf(strlen(cstr), cstr);
}

lstring_t*
clone_lstring(const lstring_t* string)
{
	return lstring_from_buf(string->length, string->cstr);
}

lstring_t*
read_lstring(FILE* file, bool trim_null)
{
	long     file_pos;
	uint16_t length;

	file_pos = ftell(file);
	if (fread(&length, 2, 1, file) != 1) goto on_error;
	return read_lstring_raw(file, length, trim_null);

on_error:
	fseek(file, file_pos, SEEK_CUR);
	return NULL;
}

lstring_t*
read_lstring_raw(FILE* file, size_t length, bool trim_null)
{
	long       file_pos;
	lstring_t* string = NULL;

	file_pos = ftell(file);
	if ((string = calloc(1, sizeof(lstring_t))) == NULL)
		goto on_error;
	string->length = length;
	if ((string->cstr = calloc(length + 1, sizeof(char))) == NULL) goto on_error;
	if (fread((char*)string->cstr, 1, length, file) != length) goto on_error;
	if (trim_null) string->length = strchr(string->cstr, '\0') - string->cstr;
	return string;

on_error:
	fseek(file, file_pos, SEEK_CUR);
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

const char*
lstr_cstr(const lstring_t* string)
{
	return string->cstr;
}

int
lstr_cmp(const lstring_t* string1, const lstring_t* string2)
{
	// TODO: make this work with strings containing NULs
	// that is, assuming I don't end up replacing this with something like bstrlib...
	return strcmp(string1->cstr, string2->cstr);
}


lstring_t*
duk_require_lstring_t(duk_context* ctx, duk_idx_t index)
{
	const char* buffer;
	size_t      length;

	buffer = duk_require_lstring(ctx, index, &length);
	return lstring_from_buf(length, buffer);
}
