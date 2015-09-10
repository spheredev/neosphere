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

static duk_ret_t js_OpenFile         (duk_context* ctx);
static duk_ret_t js_new_File         (duk_context* ctx);
static duk_ret_t js_File_finalize    (duk_context* ctx);
static duk_ret_t js_File_toString    (duk_context* ctx);
static duk_ret_t js_File_getKey      (duk_context* ctx);
static duk_ret_t js_File_getNumKeys  (duk_context* ctx);
static duk_ret_t js_File_close       (duk_context* ctx);
static duk_ret_t js_File_flush       (duk_context* ctx);
static duk_ret_t js_File_read        (duk_context* ctx);
static duk_ret_t js_File_write       (duk_context* ctx);

static duk_ret_t js_OpenRawFile(duk_context* ctx);
static duk_ret_t js_new_RawFile(duk_context* ctx);
static duk_ret_t js_RawFile_finalize(duk_context* ctx);
static duk_ret_t js_RawFile_toString(duk_context* ctx);
static duk_ret_t js_RawFile_get_position(duk_context* ctx);
static duk_ret_t js_RawFile_set_position(duk_context* ctx);
static duk_ret_t js_RawFile_get_size(duk_context* ctx);
static duk_ret_t js_RawFile_close(duk_context* ctx);
static duk_ret_t js_RawFile_read(duk_context* ctx);
static duk_ret_t js_RawFile_readString(duk_context* ctx);
static duk_ret_t js_RawFile_write(duk_context* ctx);

struct kv_file
{
	unsigned int    id;
	ALLEGRO_CONFIG* conf;
	char*           filename;
	bool            is_dirty;
};

static unsigned int s_next_file_id = 0;

kv_file_t*
open_file(const char* filename)
{
	kv_file_t*    file;
	ALLEGRO_FILE* memfile = NULL;
	void*         slurp;
	size_t        slurp_size;
	
	console_log(2, "Opening File %u as '%s'", s_next_file_id, filename);
	file = calloc(1, sizeof(kv_file_t));
	if (slurp = sfs_fslurp(g_fs, filename, "save", &slurp_size)) {
		memfile = al_open_memfile(slurp, slurp_size, "rb");
		if (!(file->conf = al_load_config_file_f(memfile)))
			goto on_error;
		al_fclose(memfile);
	}
	else {
		console_log(3, "  '%s' doesn't exist", filename);
		if (!(file->conf = al_create_config()))
			goto on_error;
	}
	file->filename = strdup(filename);
	file->id = s_next_file_id++;
	return file;

on_error:
	console_log(2, "  Failed to open File %u", s_next_file_id++);
	if (memfile != NULL) al_fclose(memfile);
	if (file->conf != NULL)
		al_destroy_config(file->conf);
	free(file);
	return NULL;
}

void
close_file(kv_file_t* file)
{
	if (file == NULL)
		return;
	
	console_log(3, "Disposing File %u as it is no longer in use", file->id);
	if (file->is_dirty)
		save_file(file);
	al_destroy_config(file->conf);
	free(file);
}

int
get_record_count(kv_file_t* file)
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
get_record_name(kv_file_t* file, int index)
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
read_bool_rec(kv_file_t* file, const char* key, bool def_value)
{
	const char* string;
	bool        value;

	string = read_string_rec(file, key, def_value ? "true" : "false");
	value = strcasecmp(string, "true") == 0;
	return value;
}

double
read_number_rec(kv_file_t* file, const char* key, double def_value)
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
read_string_rec(kv_file_t* file, const char* key, const char* def_value)
{
	const char* value;
	
	console_log(2, "Reading key '%s' from File %u", key, file->id);
	if (!(value = al_get_config_value(file->conf, NULL, key)))
		value = def_value;
	return value;
}

bool
save_file(kv_file_t* file)
{
	void*         buffer = NULL;
	size_t        file_size;
	bool          is_aok = false;
	ALLEGRO_FILE* memfile;
	size_t        next_buf_size;
	sfs_file_t*   sfs_file = NULL;

	console_log(3, "Saving File %u as '%s'", file->id, file->filename);
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
	if (!(sfs_file = sfs_fopen(g_fs, file->filename, "save", "wt")))
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
write_bool_rec(kv_file_t* file, const char* key, bool value)
{
	console_log(3, "Writing boolean to File %u, key '%s'", file->id, key);
	al_set_config_value(file->conf, NULL, key, value ? "true" : "false");
	file->is_dirty = true;
}

void
write_number_rec(kv_file_t* file, const char* key, double value)
{
	char string[500];

	console_log(3, "Writing number to File %u, key '%s'", file->id, key);
	sprintf(string, "%f", value);
	al_set_config_value(file->conf, NULL, key, string);
	file->is_dirty = true;
}

void
write_string_rec(kv_file_t* file, const char* key, const char* value)
{
	console_log(3, "Writing string to File %u, key '%s'", file->id, key);
	al_set_config_value(file->conf, NULL, key, value);
	file->is_dirty = true;
}

void
init_file_api(void)
{
	// File API functions
	register_api_function(g_duk, NULL, "DoesFileExist", js_DoesFileExist);
	register_api_function(g_duk, NULL, "GetDirectoryList", js_GetDirectoryList);
	register_api_function(g_duk, NULL, "GetFileList", js_GetFileList);
	register_api_function(g_duk, NULL, "CreateDirectory", js_CreateDirectory);
	register_api_function(g_duk, NULL, "HashRawFile", js_HashRawFile);
	register_api_function(g_duk, NULL, "RemoveDirectory", js_RemoveDirectory);
	register_api_function(g_duk, NULL, "RemoveFile", js_RemoveFile);
	register_api_function(g_duk, NULL, "Rename", js_Rename);

	// File object
	register_api_function(g_duk, NULL, "OpenFile", js_OpenFile);
	register_api_ctor(g_duk, "File", js_new_File, js_File_finalize);
	register_api_function(g_duk, "File", "toString", js_File_toString);
	register_api_function(g_duk, "File", "getKey", js_File_getKey);
	register_api_function(g_duk, "File", "getNumKeys", js_File_getNumKeys);
	register_api_function(g_duk, "File", "close", js_File_close);
	register_api_function(g_duk, "File", "flush", js_File_flush);
	register_api_function(g_duk, "File", "read", js_File_read);
	register_api_function(g_duk, "File", "write", js_File_write);

	// RawFile object
	register_api_function(g_duk, NULL, "OpenRawFile", js_OpenRawFile);
	register_api_ctor(g_duk, "RawFile", js_new_RawFile, js_RawFile_finalize);
	register_api_function(g_duk, "RawFile", "toString", js_RawFile_toString);
	register_api_prop(g_duk, "RawFile", "length", js_RawFile_get_size, NULL);
	register_api_prop(g_duk, "RawFile", "position", js_RawFile_get_position, js_RawFile_set_position);
	register_api_prop(g_duk, "RawFile", "size", js_RawFile_get_size, NULL);
	register_api_function(g_duk, "RawFile", "getPosition", js_RawFile_get_position);
	register_api_function(g_duk, "RawFile", "setPosition", js_RawFile_set_position);
	register_api_function(g_duk, "RawFile", "getSize", js_RawFile_get_size);
	register_api_function(g_duk, "RawFile", "close", js_RawFile_close);
	register_api_function(g_duk, "RawFile", "read", js_RawFile_read);
	register_api_function(g_duk, "RawFile", "readString", js_RawFile_readString);
	register_api_function(g_duk, "RawFile", "write", js_RawFile_write);
}

static duk_ret_t
js_DoesFileExist(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	duk_push_boolean(ctx, sfs_fexist(g_fs, filename, NULL));
	return 1;
}

static duk_ret_t
js_GetDirectoryList(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* dirname = n_args >= 1 ? duk_require_string(ctx, 0) : "";

	vector_t*  list;
	lstring_t* *p_filename;

	iter_t iter;

	list = list_filenames(g_fs, dirname, NULL, true);
	duk_push_array(ctx);
	iter = iterate_vector(list);
	while (p_filename = next_vector_item(&iter)) {
		duk_push_string(ctx, lstr_cstr(*p_filename));
		duk_put_prop_index(ctx, -2, (duk_uarridx_t)iter.index);
		lstr_free(*p_filename);
	}
	free_vector(list);
	return 1;
}

static duk_ret_t
js_GetFileList(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* directory_name = n_args >= 1 ? duk_require_string(ctx, 0) : "save";

	vector_t*  list;
	lstring_t* *p_filename;

	iter_t iter;

	list = list_filenames(g_fs, directory_name, NULL, false);
	duk_push_array(ctx);
	iter = iterate_vector(list);
	while (p_filename = next_vector_item(&iter)) {
		duk_push_string(ctx, lstr_cstr(*p_filename));
		duk_put_prop_index(ctx, -2, (duk_uarridx_t)iter.index);
		lstr_free(*p_filename);
	}
	free_vector(list);
	return 1;
}

static duk_ret_t
js_CreateDirectory(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	if (!sfs_mkdir(g_fs, name, "save"))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateDirectory(): Failed to create directory '%s'", name);
	return 0;
}

static duk_ret_t
js_HashRawFile(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	sfs_file_t* file;

	file = sfs_fopen(g_fs, filename, "other", "rb");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "HashRawFile(): Failed to open file '%s' for reading");
	sfs_fclose(file);
	// TODO: implement raw file hashing
	duk_error_ni(ctx, -1, DUK_ERR_ERROR, "HashRawFile(): Function is not yet implemented");
}

static duk_ret_t
js_RemoveDirectory(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	if (!sfs_rmdir(g_fs, name, "save"))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateDirectory(): Failed to create directory '%s'", name);
	return 0;
}

static duk_ret_t
js_RemoveFile(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	
	if (!sfs_unlink(g_fs, filename, "save"))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RemoveFile(): Failed to delete file '%s'", filename);
	return 0;
}

static duk_ret_t
js_Rename(duk_context* ctx)
{
	const char* filename1 = duk_require_string(ctx, 0);
	const char* filename2 = duk_require_string(ctx, 1);

	if (!sfs_rename(g_fs, filename1, filename2, "save"))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Rename(): Failed to rename file '%s' to '%s'", filename1, filename2);
	return 0;
}

static duk_ret_t
js_OpenFile(duk_context* ctx)
{
	duk_require_string(ctx, 0);
	
	js_new_File(ctx);
	return 1;
}

static duk_ret_t
js_new_File(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	
	kv_file_t* file;

	if (!(file = open_file(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenFile(): Failed to create or open file '%s'", filename);
	duk_push_sphere_obj(ctx, "File", file);
	return 1;
}

static duk_ret_t
js_File_finalize(duk_context* ctx)
{
	kv_file_t* file;

	file = duk_require_sphere_obj(ctx, 0, "File");
	close_file(file);
	return 0;
}

static duk_ret_t
js_File_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object file]");
	return 1;
}

static duk_ret_t
js_File_getKey(duk_context* ctx)
{
	int index = duk_require_int(ctx, 0);
	
	kv_file_t*  file;
	const char* key;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getKey(): File has been closed");
	if (key = get_record_name(file, index))
		duk_push_string(ctx, key);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_File_getNumKeys(duk_context* ctx)
{
	kv_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getNumKeys(): File has been closed");
	duk_push_int(ctx, get_record_count(file));
	return 1;
}

static duk_ret_t
js_File_flush(duk_context* ctx)
{
	kv_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:flush(): File has been closed");
	save_file(file);
	return 0;
}

static duk_ret_t
js_File_close(duk_context* ctx)
{
	kv_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	close_file(file);
	duk_push_this(ctx);
	duk_push_pointer(ctx, NULL); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_File_read(duk_context* ctx)
{
	const char* key = duk_to_string(ctx, 0);
	
	bool        def_bool;
	double      def_num;
	const char* def_string;
	kv_file_t*  file;
	const char* value;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:read(): File has been closed");
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
js_File_write(duk_context* ctx)
{
	const char* key = duk_to_string(ctx, 0);
	
	kv_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:write(): File has been closed");
	write_string_rec(file, key, duk_to_string(ctx, 1));
	return 0;
}

static duk_ret_t
js_OpenRawFile(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	duk_require_string(ctx, 0);
	if (n_args >= 2) duk_require_boolean(ctx, 1);

	js_new_RawFile(ctx);
	return 1;
}

static duk_ret_t
js_new_RawFile(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* filename = duk_require_string(ctx, 0);
	bool writable = n_args >= 2 ? duk_require_boolean(ctx, 1) : false;

	sfs_file_t* file;

	file = sfs_fopen(g_fs, filename, "other", writable ? "w+b" : "rb");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenRawFile(): Failed to open file '%s' for %s",
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position - File has been closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position - File has been closed");
	if (!sfs_fseek(file, new_pos, SEEK_SET))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position - Failed to set read/write position");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:size - File has been closed");
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
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:close(): File has been closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): File has been closed");
	if (n_args < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes <= 0 || num_bytes > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RawFile:read(): Read size out of range (%u)", num_bytes);
	if (!(read_buffer = malloc(num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): Failed to allocate buffer for file read");
	num_bytes = (long)sfs_fread(read_buffer, 1, num_bytes, file);
	if (n_args < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	if (!(array = bytearray_from_buffer(read_buffer, (int)num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): Failed to create byte array");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): File has been closed");
	if (n_args < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes <= 0 || num_bytes > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RawFile:read(): Read size out of range (%i)", num_bytes);
	if (!(read_buffer = malloc(num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): Failed to allocate buffer for file read");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:write(): File has been closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:write(): Write error. The file may be read-only.");
	return 0;
}
