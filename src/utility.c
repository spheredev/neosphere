#include "minisphere.h"

#include "utility.h"

const char*
syspath(const char* filename)
{
	static char retval[SPHERE_PATH_MAX];

	retval[SPHERE_PATH_MAX - 1] = '\0';
	snprintf(retval, SPHERE_PATH_MAX - 1, "~sys/%s", filename);
	return retval;
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
	duk_push_lstring(ctx, lstr_cstr(string), lstr_len(string));
}

lstring_t*
duk_require_lstring_t(duk_context* ctx, duk_idx_t index)
{
	const char* buffer;
	size_t      length;

	buffer = duk_require_lstring(ctx, index, &length);
	return lstr_from_buf(buffer, length);
}
