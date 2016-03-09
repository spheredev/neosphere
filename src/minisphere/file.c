#include "minisphere.h"
#include "api.h"
#include "bytearray.h"

#include "file.h"

static duk_ret_t js_DoesFileExist    (duk_context* ctx);
static duk_ret_t js_GetDirectoryList (duk_context* ctx);
static duk_ret_t js_GetFileList      (duk_context* ctx);
static duk_ret_t js_CreateDirectory  (duk_context* ctx);
static duk_ret_t js_HashRawFile      (duk_context* ctx);
static duk_ret_t js_RemoveDirectory  (duk_context* ctx);
static duk_ret_t js_RemoveFile       (duk_context* ctx);
static duk_ret_t js_Rename           (duk_context* ctx);

static duk_ret_t js_OpenFile            (duk_context* ctx);
static duk_ret_t js_new_KevFile         (duk_context* ctx);
static duk_ret_t js_KevFile_finalize    (duk_context* ctx);
static duk_ret_t js_KevFile_toString    (duk_context* ctx);
static duk_ret_t js_KevFile_get_numKeys (duk_context* ctx);
static duk_ret_t js_KevFile_getKey      (duk_context* ctx);
static duk_ret_t js_KevFile_close       (duk_context* ctx);
static duk_ret_t js_KevFile_flush       (duk_context* ctx);
static duk_ret_t js_KevFile_read        (duk_context* ctx);
static duk_ret_t js_KevFile_write       (duk_context* ctx);

static duk_ret_t js_OpenRawFile          (duk_context* ctx);
static duk_ret_t js_new_RawFile          (duk_context* ctx);
static duk_ret_t js_RawFile_finalize     (duk_context* ctx);
static duk_ret_t js_RawFile_toString     (duk_context* ctx);
static duk_ret_t js_RawFile_get_position (duk_context* ctx);
static duk_ret_t js_RawFile_set_position (duk_context* ctx);
static duk_ret_t js_RawFile_get_size     (duk_context* ctx);
static duk_ret_t js_RawFile_close        (duk_context* ctx);
static duk_ret_t js_RawFile_read         (duk_context* ctx);
static duk_ret_t js_RawFile_readString   (duk_context* ctx);
static duk_ret_t js_RawFile_write        (duk_context* ctx);

static duk_ret_t js_new_FileStream          (duk_context* ctx);
static duk_ret_t js_FileStream_finalize     (duk_context* ctx);
static duk_ret_t js_FileStream_get_position (duk_context* ctx);
static duk_ret_t js_FileStream_set_position (duk_context* ctx);
static duk_ret_t js_FileStream_get_length   (duk_context* ctx);
static duk_ret_t js_FileStream_close        (duk_context* ctx);
static duk_ret_t js_FileStream_read         (duk_context* ctx);
static duk_ret_t js_FileStream_write        (duk_context* ctx);

struct kev_file
{
	unsigned int    id;
	sandbox_t*      fs;
	ALLEGRO_CONFIG* conf;
	char*           filename;
	bool            is_dirty;
};

static unsigned int s_next_file_id = 0;

kev_file_t*
open_kev_file(sandbox_t* fs, const char* filename)
{
	kev_file_t*   file;
	ALLEGRO_FILE* memfile = NULL;
	void*         slurp;
	size_t        slurp_size;
	
	console_log(2, "opening kevfile #%u as '%s'", s_next_file_id, filename);
	file = calloc(1, sizeof(kev_file_t));
	if (slurp = sfs_fslurp(fs, filename, NULL, &slurp_size)) {
		memfile = al_open_memfile(slurp, slurp_size, "rb");
		if (!(file->conf = al_load_config_file_f(memfile)))
			goto on_error;
		al_fclose(memfile);
		free(slurp);
	}
	else {
		console_log(3, "    '%s' doesn't exist", filename);
		if (!(file->conf = al_create_config()))
			goto on_error;
	}
	file->fs = ref_sandbox(fs);
	file->filename = strdup(filename);
	file->id = s_next_file_id++;
	return file;

on_error:
	console_log(2, "    failed to open kevfile #%u", s_next_file_id++);
	if (memfile != NULL) al_fclose(memfile);
	if (file->conf != NULL)
		al_destroy_config(file->conf);
	free(file);
	return NULL;
}

void
close_kev_file(kev_file_t* file)
{
	if (file == NULL)
		return;
	
	console_log(3, "disposing kevfile #%u no longer in use", file->id);
	if (file->is_dirty)
		save_kev_file(file);
	al_destroy_config(file->conf);
	free_sandbox(file->fs);
	free(file);
}

int
get_record_count(kev_file_t* file)
{
	ALLEGRO_CONFIG_ENTRY* iter;
	const char*           key;
	int                   sum;

	sum = 0;
	key = al_get_first_config_entry(file->conf, NULL, &iter);
	while (key != NULL) {
		++sum;
		key = al_get_next_config_entry(&iter);
	}
	return sum;
}

const char*
get_record_name(kev_file_t* file, int index)
{
	ALLEGRO_CONFIG_ENTRY* iter;
	const char*           name;

	int i = 0;

	name = al_get_first_config_entry(file->conf, NULL, &iter);
	while (name != NULL) {
		if (i == index)
			return name;
		++i;
		name = al_get_next_config_entry(&iter);
	}
	return NULL;
}

bool
read_bool_rec(kev_file_t* file, const char* key, bool def_value)
{
	const char* string;
	bool        value;

	string = read_string_rec(file, key, def_value ? "true" : "false");
	value = strcasecmp(string, "true") == 0;
	return value;
}

double
read_number_rec(kev_file_t* file, const char* key, double def_value)
{
	char        def_string[500];
	const char* string;
	double      value;

	sprintf(def_string, "%f", def_value);
	string = read_string_rec(file, key, def_string);
	value = atof(string);
	return value;
}

const char*
read_string_rec(kev_file_t* file, const char* key, const char* def_value)
{
	const char* value;
	
	console_log(2, "reading key '%s' from kevfile #%u", key, file->id);
	if (!(value = al_get_config_value(file->conf, NULL, key)))
		value = def_value;
	return value;
}

bool
save_kev_file(kev_file_t* file)
{
	void*         buffer = NULL;
	size_t        file_size;
	bool          is_aok = false;
	ALLEGRO_FILE* memfile;
	size_t        next_buf_size;
	sfs_file_t*   sfs_file = NULL;

	console_log(3, "saving kevfile #%u as '%s'", file->id, file->filename);
	next_buf_size = 4096;
	while (!is_aok) {
		buffer = realloc(buffer, next_buf_size);
		memfile = al_open_memfile(buffer, next_buf_size, "wb");
		next_buf_size *= 2;
		al_save_config_file_f(memfile, file->conf);
		is_aok = !al_feof(memfile);
		file_size = al_ftell(memfile);
		al_fclose(memfile); memfile = NULL;
	}
	if (!(sfs_file = sfs_fopen(file->fs, file->filename, NULL, "wt")))
		goto on_error;
	sfs_fwrite(buffer, file_size, 1, sfs_file);
	sfs_fclose(sfs_file);
	free(buffer);
	return true;

on_error:
	if (memfile != NULL)
		al_fclose(memfile);
	sfs_fclose(sfs_file);
	free(buffer);
	return false;
}

void
write_bool_rec(kev_file_t* file, const char* key, bool value)
{
	console_log(3, "writing boolean to kevfile #%u, key '%s'", file->id, key);
	al_set_config_value(file->conf, NULL, key, value ? "true" : "false");
	file->is_dirty = true;
}

void
write_number_rec(kev_file_t* file, const char* key, double value)
{
	char string[500];

	console_log(3, "writing number to kevfile #%u, key '%s'", file->id, key);
	sprintf(string, "%f", value);
	al_set_config_value(file->conf, NULL, key, string);
	file->is_dirty = true;
}

void
write_string_rec(kev_file_t* file, const char* key, const char* value)
{
	console_log(3, "writing string to kevfile #%u, key '%s'", file->id, key);
	al_set_config_value(file->conf, NULL, key, value);
	file->is_dirty = true;
}

void
init_file_api(void)
{
	// File API functions
	register_api_method(g_duk, NULL, "DoesFileExist", js_DoesFileExist);
	register_api_method(g_duk, NULL, "GetDirectoryList", js_GetDirectoryList);
	register_api_method(g_duk, NULL, "GetFileList", js_GetFileList);
	register_api_method(g_duk, NULL, "CreateDirectory", js_CreateDirectory);
	register_api_method(g_duk, NULL, "HashRawFile", js_HashRawFile);
	register_api_method(g_duk, NULL, "RemoveDirectory", js_RemoveDirectory);
	register_api_method(g_duk, NULL, "RemoveFile", js_RemoveFile);
	register_api_method(g_duk, NULL, "Rename", js_Rename);

	// File object
	register_api_method(g_duk, NULL, "OpenFile", js_OpenFile);
	register_api_ctor(g_duk, "KevFile", js_new_KevFile, js_KevFile_finalize);
	register_api_prop(g_duk, "KevFile", "numKeys", js_KevFile_get_numKeys, NULL);
	register_api_method(g_duk, "KevFile", "toString", js_KevFile_toString);
	register_api_method(g_duk, "KevFile", "getKey", js_KevFile_getKey);
	register_api_method(g_duk, "KevFile", "getNumKeys", js_KevFile_get_numKeys);
	register_api_method(g_duk, "KevFile", "close", js_KevFile_close);
	register_api_method(g_duk, "KevFile", "flush", js_KevFile_flush);
	register_api_method(g_duk, "KevFile", "read", js_KevFile_read);
	register_api_method(g_duk, "KevFile", "write", js_KevFile_write);

	// RawFile object
	register_api_method(g_duk, NULL, "OpenRawFile", js_OpenRawFile);
	register_api_ctor(g_duk, "RawFile", js_new_RawFile, js_RawFile_finalize);
	register_api_method(g_duk, "RawFile", "toString", js_RawFile_toString);
	register_api_prop(g_duk, "RawFile", "length", js_RawFile_get_size, NULL);
	register_api_prop(g_duk, "RawFile", "position", js_RawFile_get_position, js_RawFile_set_position);
	register_api_prop(g_duk, "RawFile", "size", js_RawFile_get_size, NULL);
	register_api_method(g_duk, "RawFile", "getPosition", js_RawFile_get_position);
	register_api_method(g_duk, "RawFile", "setPosition", js_RawFile_set_position);
	register_api_method(g_duk, "RawFile", "getSize", js_RawFile_get_size);
	register_api_method(g_duk, "RawFile", "close", js_RawFile_close);
	register_api_method(g_duk, "RawFile", "read", js_RawFile_read);
	register_api_method(g_duk, "RawFile", "readString", js_RawFile_readString);
	register_api_method(g_duk, "RawFile", "write", js_RawFile_write);
	
	// FileStream object
	register_api_ctor(g_duk, "FileStream", js_new_FileStream, js_FileStream_finalize);
	register_api_prop(g_duk, "FileStream", "length", js_FileStream_get_length, NULL);
	register_api_prop(g_duk, "FileStream", "position", js_FileStream_get_position, js_FileStream_set_position);
	register_api_method(g_duk, "FileStream", "close", js_FileStream_close);
	register_api_method(g_duk, "FileStream", "read", js_FileStream_read);
	register_api_method(g_duk, "FileStream", "write", js_FileStream_write);
}

static duk_ret_t
js_DoesFileExist(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, false);
	duk_push_boolean(ctx, sfs_fexist(g_fs, filename, NULL));
	return 1;
}

static duk_ret_t
js_GetDirectoryList(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* dirname = n_args >= 1
		? duk_require_path(ctx, 0, NULL, false)
		: "";

	vector_t*  list;
	lstring_t* *p_filename;

	iter_t iter;

	list = list_filenames(g_fs, dirname, NULL, true);
	duk_push_array(ctx);
	iter = vector_enum(list);
	while (p_filename = vector_next(&iter)) {
		duk_push_string(ctx, lstr_cstr(*p_filename));
		duk_put_prop_index(ctx, -2, (duk_uarridx_t)iter.index);
		lstr_free(*p_filename);
	}
	vector_free(list);
	return 1;
}

static duk_ret_t
js_GetFileList(duk_context* ctx)
{
	const char* directory_name;
	vector_t*   list;
	int         num_args;

	iter_t iter;
	lstring_t* *p_filename;

	num_args = duk_get_top(ctx);
	directory_name = num_args >= 1
		? duk_require_path(ctx, 0, NULL, false)
		: "save";
	
	list = list_filenames(g_fs, directory_name, NULL, false);
	duk_push_array(ctx);
	iter = vector_enum(list);
	while (p_filename = vector_next(&iter)) {
		duk_push_string(ctx, lstr_cstr(*p_filename));
		duk_put_prop_index(ctx, -2, (duk_uarridx_t)iter.index);
		lstr_free(*p_filename);
	}
	vector_free(list);
	return 1;
}

static duk_ret_t
js_CreateDirectory(duk_context* ctx)
{
	const char* name;

	name = duk_require_path(ctx, 0, "save", false);
	if (!sfs_mkdir(g_fs, name, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateDirectory(): unable to create directory '%s'", name);
	return 0;
}

static duk_ret_t
js_HashRawFile(duk_context* ctx)
{
	sfs_file_t* file;
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, false);
	file = sfs_fopen(g_fs, filename, "other", "rb");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "HashRawFile(): unable to open file '%s' for reading");
	sfs_fclose(file);
	// TODO: implement raw file hashing
	duk_error_ni(ctx, -1, DUK_ERR_ERROR, "HashRawFile(): function is not yet implemented");
}

static duk_ret_t
js_RemoveDirectory(duk_context* ctx)
{
	const char* name;

	name = duk_require_path(ctx, 0, "save", false);
	if (!sfs_rmdir(g_fs, name, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateDirectory(): unable to remove directory '%s'", name);
	return 0;
}

static duk_ret_t
js_RemoveFile(duk_context* ctx)
{
	const char* filename;
	
	filename = duk_require_path(ctx, 0, "save", false);
	if (!sfs_unlink(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RemoveFile(): unable to delete file '%s'", filename);
	return 0;
}

static duk_ret_t
js_Rename(duk_context* ctx)
{
	const char* name1; 
	const char* name2;

	name1 = duk_require_path(ctx, 0, "save", false);
	name2 = duk_require_path(ctx, 1, "save", false);
	if (!sfs_rename(g_fs, name1, name2, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Rename(): unable to rename file '%s' to '%s'", name1, name2);
	return 0;
}

static duk_ret_t
js_OpenFile(duk_context* ctx)
{
	kev_file_t* file;
	const char* filename;

	filename = duk_require_path(ctx, 0, "save", false);
	if (!(file = open_kev_file(g_fs, filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenFile(): unable to create or open file '%s'", filename);
	duk_push_sphere_obj(ctx, "KevFile", file);
	return 1;
}

static duk_ret_t
js_new_KevFile(duk_context* ctx)
{
	kev_file_t* file;
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, false);
	if (!(file = open_kev_file(g_fs, filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File(): unable to create or open file '%s'", filename);
	duk_push_sphere_obj(ctx, "KevFile", file);
	return 1;
}

static duk_ret_t
js_KevFile_finalize(duk_context* ctx)
{
	kev_file_t* file;

	file = duk_require_sphere_obj(ctx, 0, "KevFile");
	close_kev_file(file);
	return 0;
}

static duk_ret_t
js_KevFile_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object file]");
	return 1;
}

static duk_ret_t
js_KevFile_get_numKeys(duk_context* ctx)
{
	kev_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "KevFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getNumKeys(): file has been closed");
	duk_push_int(ctx, get_record_count(file));
	return 1;
}

static duk_ret_t
js_KevFile_getKey(duk_context* ctx)
{
	int index = duk_require_int(ctx, 0);
	
	kev_file_t*  file;
	const char* key;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "KevFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getKey(): file has been closed");
	if (key = get_record_name(file, index))
		duk_push_string(ctx, key);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_KevFile_flush(duk_context* ctx)
{
	kev_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "KevFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:flush(): file has been closed");
	save_kev_file(file);
	return 0;
}

static duk_ret_t
js_KevFile_close(duk_context* ctx)
{
	kev_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "KevFile");
	duk_pop(ctx);
	close_kev_file(file);
	duk_push_this(ctx);
	duk_push_pointer(ctx, NULL); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_KevFile_read(duk_context* ctx)
{
	const char* key = duk_to_string(ctx, 0);
	
	bool        def_bool;
	double      def_num;
	const char* def_string;
	kev_file_t*  file;
	const char* value;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "KevFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:read(): file has been closed");
	switch (duk_get_type(ctx, 1)) {
	case DUK_TYPE_BOOLEAN:
		def_bool = duk_get_boolean(ctx, 1);
		duk_push_boolean(ctx, read_bool_rec(file, key, def_bool));
		break;
	case DUK_TYPE_NUMBER:
		def_num = duk_get_number(ctx, 1);
		duk_push_number(ctx, read_number_rec(file, key, def_num));
		break;
	default:
		def_string = duk_to_string(ctx, 1);
		value = read_string_rec(file, key, def_string);
		duk_push_string(ctx, value);
		break;
	}
	return 1;
}

static duk_ret_t
js_KevFile_write(duk_context* ctx)
{
	const char* key = duk_to_string(ctx, 0);
	
	kev_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "KevFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:write(): file has been closed");
	write_string_rec(file, key, duk_to_string(ctx, 1));
	return 0;
}

static duk_ret_t
js_OpenRawFile(duk_context* ctx)
{
	// OpenRawFile(filename);
	// Arguments:
	//     filename: The name of the file to open, relative to ~sgm/other.
	//     writable: Optional. If true, the file is truncated to zero size and
	//               opened for writing. (default: false)

	const char* filename;
	bool        writable;

	sfs_file_t* file;

	int n_args = duk_get_top(ctx);
	filename = duk_require_path(ctx, 0, "other", false);
	writable = n_args >= 2
		? duk_require_boolean(ctx, 1)
		: false;

	file = sfs_fopen(g_fs, filename, NULL, writable ? "w+b" : "rb");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenRawFile(): unable to open file '%s' for %s",
			filename, writable ? "writing" : "reading");
	duk_push_sphere_obj(ctx, "RawFile", file);
	return 1;
}

static duk_ret_t
js_new_RawFile(duk_context* ctx)
{
	// new RawFile(filename);
	// Arguments:
	//     filename: The name of the file to open. SphereFS-compliant.
	//     writable: Optional. If true, the file is truncated to zero size and
	//               opened for writing. (default: false)
	
	const char* filename;
	bool        writable;

	sfs_file_t* file;

	int n_args = duk_get_top(ctx);
	filename = duk_require_path(ctx, 0, NULL, false);
	writable = n_args >= 2
		? duk_require_boolean(ctx, 1)
		: false;

	file = sfs_fopen(g_fs, filename, NULL, writable ? "w+b" : "rb");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenRawFile(): unable to open file '%s' for %s",
			filename, writable ? "writing" : "reading");
	duk_push_sphere_obj(ctx, "RawFile", file);
	return 1;
}

static duk_ret_t
js_RawFile_finalize(duk_context* ctx)
{
	sfs_file_t* file;

	file = duk_require_sphere_obj(ctx, 0, "RawFile");
	if (file != NULL) sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_RawFile_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object rawfile]");
	return 1;
}

static duk_ret_t
js_RawFile_get_position(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position: file has been closed");
	duk_push_int(ctx, sfs_ftell(file));
	return 1;
}

static duk_ret_t
js_RawFile_set_position(duk_context* ctx)
{
	int new_pos = duk_require_int(ctx, 0);

	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position: file has been closed");
	if (!sfs_fseek(file, new_pos, SEEK_SET))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position: Failed to set read/write position");
	return 0;
}

static duk_ret_t
js_RawFile_get_size(duk_context* ctx)
{
	sfs_file_t* file;
	long  file_pos;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:size: file has been closed");
	file_pos = sfs_ftell(file);
	sfs_fseek(file, 0, SEEK_END);
	duk_push_int(ctx, sfs_ftell(file));
	sfs_fseek(file, file_pos, SEEK_SET);
	return 1;
}

static duk_ret_t
js_RawFile_close(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_push_pointer(ctx, NULL);
	duk_put_prop_string(ctx, -2, "\xFF" "udata");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:close(): file has been closed");
	sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_RawFile_read(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	long num_bytes = n_args >= 1 ? duk_require_int(ctx, 0) : 0;

	bytearray_t* array;
	sfs_file_t*        file;
	long         pos;
	void*        read_buffer;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): file has been closed");
	if (n_args < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes <= 0 || num_bytes > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RawFile:read(): read size out of range (%u)", num_bytes);
	if (!(read_buffer = malloc(num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): unable to allocate buffer for file read");
	num_bytes = (long)sfs_fread(read_buffer, 1, num_bytes, file);
	if (n_args < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	if (!(array = bytearray_from_buffer(read_buffer, (int)num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): unable to create byte array");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_RawFile_readString(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	long num_bytes = n_args >= 1 ? duk_require_int(ctx, 0) : 0;

	sfs_file_t* file;
	long  pos;
	void* read_buffer;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): file has been closed");
	if (n_args < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes <= 0 || num_bytes > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RawFile:read(): read size out of range (%i)", num_bytes);
	if (!(read_buffer = malloc(num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): unable to allocate buffer for file read");
	num_bytes = (long)sfs_fread(read_buffer, 1, num_bytes, file);
	if (n_args < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	duk_push_lstring(ctx, read_buffer, num_bytes);
	return 1;
}

static duk_ret_t
js_RawFile_write(duk_context* ctx)
{
	bytearray_t* array;
	const void*  data;
	sfs_file_t*  file;
	duk_size_t   write_size;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:write(): file has been closed");
	if (duk_is_string(ctx, 0))
		data = duk_get_lstring(ctx, 0, &write_size);
	else if (duk_is_sphere_obj(ctx, 0, "ByteArray")) {
		array = duk_require_sphere_obj(ctx, 0, "ByteArray");
		data = get_bytearray_buffer(array);
		write_size = get_bytearray_size(array);
	}
	else {
		data = duk_require_buffer_data(ctx, 0, &write_size);
	}
	if (sfs_fwrite(data, 1, write_size, file) != write_size)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:write(): error writing to file");
	return 0;
}

static duk_ret_t
js_new_FileStream(duk_context* ctx)
{
	// new FileStream(filename);
	// Arguments:
	//     filename: The name of the file to open, relative to ~sgm/.
	//     mode:     A string specifying how to open the file, in the same format as
	//               would be passed to C fopen().

	sfs_file_t* file;
	const char* filename;
	const char* mode;

	filename = duk_require_path(ctx, 0, NULL, false);
	mode = duk_require_string(ctx, 1);
	file = sfs_fopen(g_fs, filename, NULL, mode);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream(): unable to open file '%s' with mode '%s'",
			filename, mode);
	duk_push_sphere_obj(ctx, "FileStream", file);
	return 1;
}

static duk_ret_t
js_FileStream_finalize(duk_context* ctx)
{
	sfs_file_t* file;

	file = duk_require_sphere_obj(ctx, 0, "FileStream");
	if (file != NULL) sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_FileStream_get_position(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_push_number(ctx, sfs_ftell(file));
	return 1;
}

static duk_ret_t
js_FileStream_set_position(duk_context* ctx)
{
	sfs_file_t* file;
	long long   new_pos;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	new_pos = duk_require_number(ctx, 0);
	sfs_fseek(file, new_pos, SFS_SEEK_SET);
	return 0;
}

static duk_ret_t
js_FileStream_get_length(duk_context* ctx)
{
	sfs_file_t* file;
	long        file_pos;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream:length: file has been closed");
	file_pos = sfs_ftell(file);
	sfs_fseek(file, 0, SEEK_END);
	duk_push_number(ctx, sfs_ftell(file));
	sfs_fseek(file, file_pos, SEEK_SET);
	return 1;
}

static duk_ret_t
js_FileStream_close(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_push_pointer(ctx, NULL);
	duk_put_prop_string(ctx, -2, "\xFF" "udata");
	sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_FileStream_read(duk_context* ctx)
{
	// FileStream:read([numBytes]);
	// Reads data from the stream, returning it in an ArrayBuffer.
	// Arguments:
	//     numBytes: Optional. The number of bytes to read. If not provided, the
	//               entire file is read.

	int          argc;
	void*        buffer;
	sfs_file_t*  file;
	long         pos;

	argc = duk_get_top(ctx);
	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	long num_bytes = argc >= 1 ? duk_require_int(ctx, 0) : 0;
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream:read(): file has been closed");
	if (argc < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes <= 0 || num_bytes > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "FileStream:read(): read size out of range (%u)", num_bytes);
	buffer = duk_push_fixed_buffer(ctx, num_bytes);
	num_bytes = (long)sfs_fread(buffer, 1, num_bytes, file);
	if (argc < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	duk_push_buffer_object(ctx, -1, 0, num_bytes, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_FileStream_write(duk_context* ctx)
{
	const void* data;
	sfs_file_t* file;
	duk_size_t  write_size;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream:write(): file has been closed");
	data = duk_require_buffer_data(ctx, 0, &write_size);
	if (sfs_fwrite(data, 1, write_size, file) != write_size)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream:write(): error writing to file");
	return 0;
}
