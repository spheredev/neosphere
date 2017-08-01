#include "minisphere.h"
#include "utility.h"

#include "api.h"
#include "md5.h"

static duk_ret_t do_decode_json (duk_context* ctx, void* udata);

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
			// for OS X, we want the directory containing miniSphere.app and NOT the
			// executable directory that's buried inside.  to get there, we need to
			// go 3 levels up from the executable:
			//     miniSphere.app/Contents/MacOS/minisphere
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
md5sum(const void* data, size_t size)
{
	// note: a static buffer is used here to store the last generated hash, so
	//       only one output can be used at a time.  be careful.

	static char output[33];

	MD5_CTX ctx;
	uint8_t hash_bytes[16];
	char    *p;

	int i;

	MD5_Init(&ctx);
	MD5_Update(&ctx, data, (unsigned long)size);
	MD5_Final(hash_bytes, &ctx);
	p = &output[0];
	for (i = 0; i < 16; ++i) {
		sprintf(p, "%.2x", (int)hash_bytes[i]);
		p += 2;
	}
	return output;
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

bool
write_lstring(sfs_file_t* file, const lstring_t* string, bool include_nul)
{
	uint16_t length;

	length = (uint16_t)lstr_len(string);
	if (include_nul)
		++length;
	if (sfs_fwrite(&length, 2, 1, file) != 1)
		return false;
	if (sfs_fwrite(lstr_cstr(string), length, 1, file) != 1)
		return false;
	return true;
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
	return duk_safe_call(ctx, do_decode_json, NULL, 1, 1);
}

void
duk_push_lstring_t(duk_context* ctx, const lstring_t* string)
{
	duk_push_lstring(ctx, lstr_cstr(string), lstr_len(string));
}

void*
duk_ref_heapptr(duk_context* ctx, duk_idx_t idx)
{
	void* heapptr;

	heapptr = duk_require_heapptr(ctx, idx);
	
	duk_push_global_stash(ctx);
	if (!duk_get_prop_string(ctx, -1, "refs")) {
		duk_push_bare_object(ctx);
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
		duk_push_bare_object(ctx);
		duk_push_number(ctx, 1.0);
		duk_put_prop_string(ctx, -2, "refcount");
		duk_push_heapptr(ctx, heapptr);
		duk_put_prop_string(ctx, -2, "value");

		/* [ stash refs undefined key ref_obj ] */

		duk_put_prop(ctx, -4);
		duk_pop_3(ctx);
	}

	return heapptr;
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
duk_require_path(duk_context* ctx, duk_idx_t index, const char* origin_name, bool legacy, bool need_write)
{
	// note: for compatibility with Sphere 1.x, if `legacy` is true, then the game package
	//       is treated as writable.
	
	static int     s_index = 0;
	static path_t* s_paths[10];
	
	const char* first_hop = "";
	const char* pathname;
	const char* prefix;
	path_t*     path;

	pathname = duk_require_string(ctx, index);
	path = fs_make_path(pathname, origin_name, legacy);
	prefix = path_hop(path, 0);  // note: fs_make_path() *always* prefixes
	if (path_num_hops(path) > 1)
		first_hop = path_hop(path, 1);
	if (strcmp(first_hop, "..") == 0 || path_is_rooted(path))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "FS sandboxing violation");
	if (strcmp(prefix, "~") == 0 && fs_save_id(g_fs) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no save ID defined");
	if (need_write && !legacy && strcmp(prefix, "~") != 0)
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "directory is read-only");
	if (need_write && strcmp(prefix, "#") == 0)  // `system/` is always read-only
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "directory is read-only");
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
		duk_push_bare_object(ctx);
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
do_decode_json(duk_context* ctx, void* udata)
{
	duk_json_decode(ctx, -1);
	return 1;
}
