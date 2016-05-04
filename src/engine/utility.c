#include "minisphere.h"
#include "api.h"

#include "utility.h"

const path_t*
enginepath(void)
{
	static path_t* retval = NULL;

	ALLEGRO_PATH* al_path;

	if (retval == NULL) {
		al_path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
		retval = path_new_dir(al_path_cstr(al_path, '/'));
		al_destroy_path(al_path);
	}
	return retval;
}

const path_t*
homepath(void)
{
	static path_t* retval = NULL;

	ALLEGRO_PATH* al_path;

	if (retval == NULL) {
		al_path = al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
		retval = path_new_dir(al_path_cstr(al_path, '/'));
		al_destroy_path(al_path);
	}
	path_mkdir(retval);
	return retval;
}

const char*
systempath(const char* filename)
{
	static char retval[SPHERE_PATH_MAX];

	retval[SPHERE_PATH_MAX - 1] = '\0';
	snprintf(retval, SPHERE_PATH_MAX - 1, "#/%s", filename);
	return retval;
}

bool
is_cpu_little_endian(void)
{
	uint8_t  lead_byte;
	uint16_t value;

	value = 812;
	lead_byte = *(uint8_t*)&value;
	return lead_byte == 44;
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

char*
strnewf(const char* fmt, ...)
{
	va_list ap, apc;
	char* buffer;
	int   buf_size;

	va_start(ap, fmt);
	va_copy(apc, ap);
	buf_size = vsnprintf(NULL, 0, fmt, apc) + 1;
	va_end(apc);
	buffer = malloc(buf_size);
	va_copy(apc, ap);
	vsnprintf(buffer, buf_size, fmt, apc);
	va_end(apc);
	va_end(ap);
	return buffer;
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

const char*
duk_require_path(duk_context* ctx, duk_idx_t index, const char* origin_name, bool allow_absolute)
{
	static int     s_index = 0;
	static path_t* s_paths[10];
	
	const char* pathname;
	path_t*     path;

	pathname = duk_require_string(ctx, index);
	path = make_sfs_path(pathname, origin_name);
	if ((path_num_hops(path) > 0 && path_hop_cmp(path, 0, ".."))
	    || (path_is_rooted(path) && !allow_absolute))
	{
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "FS sandbox violation (`%s`)", pathname);
	}
	if (s_paths[s_index] != NULL)
		path_free(s_paths[s_index]);
	s_paths[s_index] = path;
	s_index = (s_index + 1) % 10;
	return path_cstr(path);
}
