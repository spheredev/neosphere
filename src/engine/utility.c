#include "minisphere.h"
#include "utility.h"

#include "api.h"

static duk_ret_t do_decode_json (duk_context* ctx);

const path_t*
assets_path(void)
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
engine_path(void)
{
	static path_t* retval = NULL;

	ALLEGRO_PATH* al_path;

	int i;

	if (retval == NULL) {
		al_path = al_get_standard_path(ALLEGRO_EXENAME_PATH);
		al_set_path_filename(al_path, NULL);
		// FIXME: how do we detect if we're running from an app bundle?
		if (false) {
			// for OS X, we want the directory containing minisphere.app and NOT the
			// executable directory that's buried inside.  to get there, we need to
			// go 3 levels up from the executable:
			//     minisphere.app/Contents/MacOS/minisphere
			//     ^3             ^2       ^1    ^0
			for (i = 0; i < 3; ++i)
				al_drop_path_tail(al_path);
		}
		retval = path_new_dir(al_path_cstr(al_path, '/'));
		al_destroy_path(al_path);
	}
	return retval;
}

const path_t*
home_path(void)
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
system_path(const char* filename)
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
	string = lstr_from_cp1252(buffer, length);
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

duk_int_t
duk_json_pdecode(duk_context* ctx)
{
	return dukrub_safe_call(ctx, do_decode_json, 1, 1);
}

void
duk_push_lstring_t(duk_context* ctx, const lstring_t* string)
{
	duk_push_lstring(ctx, lstr_cstr(string), lstr_len(string));
}

void
duk_ref_heapptr(duk_context* ctx, void* heapptr)
{
	duk_push_global_stash(ctx);
	if (!duk_get_prop_string(ctx, -1, "refs")) {
		dukrub_push_bare_object(ctx);
		duk_put_prop_string(ctx, -3, "refs");
		duk_get_prop_string(ctx, -2, "refs");
		duk_replace(ctx, -2);
	}

	/* [ ... stash refs ] */

	duk_push_sprintf(ctx, "%p", heapptr);
	if (duk_get_prop(ctx, -2)) {
		/* [ stash refs ref_obj ] */

		duk_get_prop_string(ctx, -1, "refcount");
		duk_push_number(ctx, duk_get_number(ctx, -1) + 1);
		duk_put_prop_string(ctx, -3, "refcount");
		duk_pop_n(ctx, 4);
	}
	else {
		/* [ stash refs undefined ] */

		duk_push_sprintf(ctx, "%p", heapptr);
		dukrub_push_bare_object(ctx);
		duk_push_number(ctx, 1.0);
		duk_put_prop_string(ctx, -2, "refcount");
		duk_push_heapptr(ctx, heapptr);
		duk_put_prop_string(ctx, -2, "value");

		/* [ stash refs undefined key ref_obj ] */

		duk_put_prop(ctx, -4);
		duk_pop_3(ctx);
	}
}

lstring_t*
duk_require_lstring_t(duk_context* ctx, duk_idx_t index)
{
	const char* buffer;
	size_t      length;

	buffer = duk_require_lstring(ctx, index, &length);
	return lstr_from_cp1252(buffer, length);
}

const char*
duk_require_path(duk_context* ctx, duk_idx_t index, const char* origin_name, bool legacy)
{
	static int     s_index = 0;
	static path_t* s_paths[10];
	
	const char* pathname;
	path_t*     path;

	pathname = duk_require_string(ctx, index);
	path = fs_make_path(pathname, origin_name, legacy);
	if ((path_num_hops(path) > 0 && path_hop_cmp(path, 0, ".."))
	    || path_is_rooted(path))
	{
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "FS sandboxing violation");
	}
	if (s_paths[s_index] != NULL)
		path_free(s_paths[s_index]);
	s_paths[s_index] = path;
	s_index = (s_index + 1) % 10;
	return path_cstr(path);
}

void
duk_unref_heapptr(duk_context* ctx, void* heapptr)
{
	double refcount;

	duk_push_global_stash(ctx);
	if (!duk_get_prop_string(ctx, -1, "refs")) {
		dukrub_push_bare_object(ctx);
		duk_put_prop_string(ctx, -3, "refs");
		duk_get_prop_string(ctx, -2, "refs");
		duk_replace(ctx, -2);
	}

	/* [ ... stash refs ] */

	duk_push_sprintf(ctx, "%p", heapptr);
	if (duk_get_prop(ctx, -2)) {
		/* [ stash refs ref_obj ] */

		duk_get_prop_string(ctx, -1, "refcount");
		refcount = duk_get_number(ctx, -1) - 1.0;
		if (refcount > 0.0) {
			duk_push_number(ctx, refcount);
			duk_put_prop_string(ctx, -3, "refcount");
		}
		else {
			duk_push_sprintf(ctx, "%p", heapptr);
			duk_del_prop(ctx, -4);
		}
		duk_pop_n(ctx, 4);
	}
	else {
		/* [ stash refs undefined ] */

		duk_pop_3(ctx);
	}
}

static duk_ret_t
do_decode_json(duk_context* ctx)
{
	duk_json_decode(ctx, -1);
	return 1;
}
