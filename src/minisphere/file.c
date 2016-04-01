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
static duk_ret_t js_FileStream_readDouble   (duk_context* ctx);
static duk_ret_t js_FileStream_readFloat    (duk_context* ctx);
static duk_ret_t js_FileStream_readInt      (duk_context* ctx);
static duk_ret_t js_FileStream_readPString  (duk_context* ctx);
static duk_ret_t js_FileStream_readString   (duk_context* ctx);
static duk_ret_t js_FileStream_readUInt     (duk_context* ctx);
static duk_ret_t js_FileStream_write        (duk_context* ctx);
static duk_ret_t js_FileStream_writeDouble  (duk_context* ctx);
static duk_ret_t js_FileStream_writeFloat   (duk_context* ctx);
static duk_ret_t js_FileStream_writeInt     (duk_context* ctx);
static duk_ret_t js_FileStream_writePString (duk_context* ctx);
static duk_ret_t js_FileStream_writeString  (duk_context* ctx);
static duk_ret_t js_FileStream_writeUInt    (duk_context* ctx);

struct kev_file
{
	unsigned int    id;
	sandbox_t*      fs;
	ALLEGRO_CONFIG* conf;
	char*           filename;
	bool            is_dirty;
};

static bool read_vsize_int   (sfs_file_t* file, intmax_t* p_value, int size, bool little_endian);
static bool read_vsize_uint  (sfs_file_t* file, intmax_t* p_value, int size, bool little_endian);
static bool write_vsize_int  (sfs_file_t* file, intmax_t value, int size, bool little_endian);
static bool write_vsize_uint (sfs_file_t* file, intmax_t value, int size, bool little_endian);

static unsigned int s_next_file_id = 0;

kev_file_t*
open_kev_file(sandbox_t* fs, const char* filename)
{
	kev_file_t*   file;
	ALLEGRO_FILE* memfile = NULL;
	void*         slurp;
	size_t        slurp_size;
	
	console_log(2, "opening kevfile #%u as `%s`", s_next_file_id, filename);
	file = calloc(1, sizeof(kev_file_t));
	if (slurp = sfs_fslurp(fs, filename, NULL, &slurp_size)) {
		memfile = al_open_memfile(slurp, slurp_size, "rb");
		if (!(file->conf = al_load_config_file_f(memfile)))
			goto on_error;
		al_fclose(memfile);
		free(slurp);
	}
	else {
		console_log(3, "    `%s` doesn't exist", filename);
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
	
	console_log(2, "reading key `%s` from kevfile #%u", key, file->id);
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

	console_log(3, "saving kevfile #%u as `%s`", file->id, file->filename);
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
	console_log(3, "writing boolean to kevfile #%u, key `%s`", file->id, key);
	al_set_config_value(file->conf, NULL, key, value ? "true" : "false");
	file->is_dirty = true;
}

void
write_number_rec(kev_file_t* file, const char* key, double value)
{
	char string[500];

	console_log(3, "writing number to kevfile #%u, key `%s`", file->id, key);
	sprintf(string, "%f", value);
	al_set_config_value(file->conf, NULL, key, string);
	file->is_dirty = true;
}

void
write_string_rec(kev_file_t* file, const char* key, const char* value)
{
	console_log(3, "writing string to kevfile #%u, key `%s`", file->id, key);
	al_set_config_value(file->conf, NULL, key, value);
	file->is_dirty = true;
}

static bool
read_vsize_int(sfs_file_t* file, intmax_t* p_value, int size, bool little_endian)
{
	// NOTE: supports decoding values up to 48-bit (6 bytes).  don't specify
	//       size > 6 unless you want a segfault!

	uint8_t data[6];
	int     mul = 1;
	
	int i;
	
	if (sfs_fread(data, 1, size, file) != size)
		return false;
	
	// variable-sized int decoding adapted from Node.js
	if (little_endian) {
		*p_value = data[i = 0];
		while (++i < size && (mul *= 0x100))
			*p_value += data[i] * mul;
	}
	else {
		*p_value = data[i = size - 1];
		while (i > 0 && (mul *= 0x100))
			*p_value += data[--i] * mul;
	}
	if (*p_value >= mul * 0x80)
		*p_value -= pow(2, 8 * size);

	return true;
}

static bool
read_vsize_uint(sfs_file_t* file, intmax_t* p_value, int size, bool little_endian)
{
	// NOTE: supports decoding values up to 48-bit (6 bytes).  don't specify
	//       size > 6 unless you want a segfault!

	uint8_t data[6];
	int     mul = 1;

	int i;

	if (sfs_fread(data, 1, size, file) != size)
		return false;

	// variable-sized uint decoding adapted from Node.js
	if (little_endian) {
		*p_value = data[i = 0];
		while (++i < size && (mul *= 0x100))
			*p_value += data[i] * mul;
	}
	else {
		*p_value = data[--size];
		while (size > 0 && (mul *= 0x100))
			*p_value += data[--size] * mul;
	}

	return true;
}

static bool
write_vsize_int(sfs_file_t* file, intmax_t value, int size, bool little_endian)
{
	// NOTE: supports encoding values up to 48-bit (6 bytes).  don't specify
	//       size > 6 unless you want a segfault!

	uint8_t data[6];
	int     mul = 1;
	int     sub = 0;

	int i;

	// variable-sized int encoding adapted from Node.js
	if (little_endian) {
		data[i = 0] = value & 0xFF;
		while (++i < size && (mul *= 0x100)) {
			if (value < 0 && sub == 0 && data[i - 1] != 0)
				sub = 1;
			data[i] = (value / mul - sub) & 0xFF;
		}
	}
	else {
		data[i = size - 1] = value & 0xFF;
		while (--i >= 0 && (mul *= 0x100)) {
			if (value < 0 && sub == 0 && data[i + 1] != 0)
				sub = 1;
			data[i] = (value / mul - sub) & 0xFF;
		}
	}
	
	return sfs_fwrite(data, 1, size, file) == size;
}

static bool
write_vsize_uint(sfs_file_t* file, intmax_t value, int size, bool little_endian)
{
	// NOTE: supports encoding values up to 48-bit (6 bytes).  don't specify
	//       size > 6 unless you want a segfault!
	
	uint8_t data[6];
	int     mul = 1;

	int i;

	// variable-sized uint encoding adapted from Node.js
	if (little_endian) {
		data[i = 0] = value & 0xFF;
		while (++i < size && (mul *= 0x100))
			data[i] = (value / mul) & 0xFF;
	}
	else {
		data[i = size - 1] = value & 0xFF;
		while (--i >= 0 && (mul *= 0x100))
			data[i] = (value / mul) & 0xFF;
	}

	return sfs_fwrite(data, 1, size, file) == size;
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
	register_api_prop(g_duk, "FileStream", "size", js_FileStream_get_length, NULL);
	register_api_method(g_duk, "FileStream", "close", js_FileStream_close);
	register_api_method(g_duk, "FileStream", "read", js_FileStream_read);
	register_api_method(g_duk, "FileStream", "readDouble", js_FileStream_readDouble);
	register_api_method(g_duk, "FileStream", "readFloat", js_FileStream_readFloat);
	register_api_method(g_duk, "FileStream", "readInt", js_FileStream_readInt);
	register_api_method(g_duk, "FileStream", "readPString", js_FileStream_readPString);
	register_api_method(g_duk, "FileStream", "readString", js_FileStream_readString);
	register_api_method(g_duk, "FileStream", "readUInt", js_FileStream_readUInt);
	register_api_method(g_duk, "FileStream", "write", js_FileStream_write);
	register_api_method(g_duk, "FileStream", "writeDouble", js_FileStream_writeDouble);
	register_api_method(g_duk, "FileStream", "writeFloat", js_FileStream_writeFloat);
	register_api_method(g_duk, "FileStream", "writeInt", js_FileStream_writeInt);
	register_api_method(g_duk, "FileStream", "writePString", js_FileStream_writePString);
	register_api_method(g_duk, "FileStream", "writeString", js_FileStream_writeString);
	register_api_method(g_duk, "FileStream", "writeUInt", js_FileStream_writeUInt);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateDirectory(): unable to create directory `%s`", name);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "HashRawFile(): unable to open file `%s` for reading");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateDirectory(): unable to remove directory `%s`", name);
	return 0;
}

static duk_ret_t
js_RemoveFile(duk_context* ctx)
{
	const char* filename;
	
	filename = duk_require_path(ctx, 0, "save", false);
	if (!sfs_unlink(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RemoveFile(): unable to delete file `%s`", filename);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Rename(): unable to rename file `%s` to `%s`", name1, name2);
	return 0;
}

static duk_ret_t
js_OpenFile(duk_context* ctx)
{
	kev_file_t* file;
	const char* filename;

	filename = duk_require_path(ctx, 0, "save", false);
	if (!(file = open_kev_file(g_fs, filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenFile(): unable to create or open file `%s`", filename);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File(): unable to create or open file `%s`", filename);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getNumKeys(): file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getKey(): file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:flush(): file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:read(): file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:write(): file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenRawFile(): unable to open file `%s` for %s",
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenRawFile(): unable to open file `%s` for %s",
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position: file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position: file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:size: file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:close(): file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:write(): file was closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to open file `%s` in mode `%s`",
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
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
	int          num_bytes;
	long         pos;

	argc = duk_get_top(ctx);
	num_bytes = argc >= 1 ? duk_require_int(ctx, 0) : 0;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (argc < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "string length must be zero or greater");
	buffer = duk_push_fixed_buffer(ctx, num_bytes);
	num_bytes = (int)sfs_fread(buffer, 1, num_bytes, file);
	if (argc < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	duk_push_buffer_object(ctx, -1, 0, num_bytes, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_FileStream_readDouble(duk_context* ctx)
{
	int         argc;
	uint8_t     data[8];
	sfs_file_t* file;
	bool        little_endian;
	double      value;

	int i;

	argc = duk_get_top(ctx);
	little_endian = argc >= 1 ? duk_require_boolean(ctx, 0) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (sfs_fread(data, 1, 8, file) != 8)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read double from file");
	if (little_endian == is_cpu_little_endian())
		memcpy(&value, data, 8);
	else {
		for (i = 0; i < 8; ++i)
			((uint8_t*)&value)[i] = data[7 - i];
	}
	duk_push_number(ctx, value);
	return 1;
}

static duk_ret_t
js_FileStream_readFloat(duk_context* ctx)
{
	int         argc;
	uint8_t     data[8];
	sfs_file_t* file;
	bool        little_endian;
	float       value;

	int i;

	argc = duk_get_top(ctx);
	little_endian = argc >= 1 ? duk_require_boolean(ctx, 0) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (sfs_fread(data, 1, 4, file) != 4)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read float from file");
	if (little_endian == is_cpu_little_endian())
		memcpy(&value, data, 4);
	else {
		for (i = 0; i < 4; ++i)
			((uint8_t*)&value)[i] = data[3 - i];
	}
	duk_push_number(ctx, value);
	return 1;
}

static duk_ret_t
js_FileStream_readInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	int         num_bytes;
	intmax_t    value;

	argc = duk_get_top(ctx);
	num_bytes = duk_require_int(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "int byte size must be in [1-6] range");
	if (!read_vsize_int(file, &value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read int from file");
	duk_push_number(ctx, (double)value);
	return 1;
}

static duk_ret_t
js_FileStream_readPString(duk_context* ctx)
{
	int          argc;
	void*        buffer;
	sfs_file_t*  file;
	intmax_t     length;
	bool         little_endian;
	int          uint_size;

	argc = duk_get_top(ctx);
	uint_size = duk_require_int(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (uint_size < 1 || uint_size > 4)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "length bytes must be in [1-4] range (got: %d)", uint_size);
	if (!read_vsize_uint(file, &length, uint_size, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read pstring from file");
	buffer = malloc((size_t)length);
	if (sfs_fread(buffer, 1, (size_t)length, file) != (size_t)length)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read pstring from file");
	duk_push_lstring(ctx, buffer, (size_t)length);
	free(buffer);
	return 1;
}

static duk_ret_t
js_FileStream_readString(duk_context* ctx)
{
	// FileStream:readString([numBytes]);
	// Reads data from the stream, returning it in an ArrayBuffer.
	// Arguments:
	//     numBytes: Optional. The number of bytes to read. If not provided, the
	//               entire file is read.

	int          argc;
	void*        buffer;
	sfs_file_t*  file;
	int          num_bytes;
	long         pos;

	argc = duk_get_top(ctx);
	num_bytes = argc >= 1 ? duk_require_int(ctx, 0) : 0;
	
	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (argc < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "string length must be zero or greater");
	buffer = malloc(num_bytes);
	num_bytes = (int)sfs_fread(buffer, 1, num_bytes, file);
	if (argc < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	duk_push_lstring(ctx, buffer, num_bytes);
	free(buffer);
	return 1;
}

static duk_ret_t
js_FileStream_readUInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	int         num_bytes;
	intmax_t     value;

	argc = duk_get_top(ctx);
	num_bytes = duk_require_int(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "uint byte size must be in [1-6] range");
	if (!read_vsize_uint(file, &value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read uint from file");
	duk_push_number(ctx, (double)value);
	return 1;
}

static duk_ret_t
js_FileStream_write(duk_context* ctx)
{
	const void* data;
	sfs_file_t* file;
	duk_size_t  num_bytes;

	duk_require_stack_top(ctx, 1);
	data = duk_require_buffer_data(ctx, 0, &num_bytes);

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (sfs_fwrite(data, 1, num_bytes, file) != num_bytes)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write data to file");
	return 0;
}

static duk_ret_t
js_FileStream_writeDouble(duk_context* ctx)
{
	int         argc;
	uint8_t     data[8];
	sfs_file_t* file;
	bool        little_endian;
	double      value;

	int i;

	argc = duk_get_top(ctx);
	value = duk_require_number(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (little_endian == is_cpu_little_endian())
		memcpy(data, &value, 8);
	else {
		for (i = 0; i < 8; ++i)
			data[i] = ((uint8_t*)&value)[7 - i];
	}
	if (sfs_fwrite(data, 1, 8, file) != 8)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write double to file");
	return 0;
}

static duk_ret_t
js_FileStream_writeFloat(duk_context* ctx)
{
	int         argc;
	uint8_t     data[4];
	sfs_file_t* file;
	bool        little_endian;
	float       value;

	int i;

	argc = duk_get_top(ctx);
	value = duk_require_number(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (little_endian == is_cpu_little_endian())
		memcpy(data, &value, 4);
	else {
		for (i = 0; i < 4; ++i)
			data[i] = ((uint8_t*)&value)[3 - i];
	}
	if (sfs_fwrite(data, 1, 4, file) != 4)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write float to file");
	return 0;
}

static duk_ret_t
js_FileStream_writeInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	intmax_t    min_value;
	intmax_t    max_value;
	int         num_bytes;
	intmax_t    value;

	argc = duk_get_top(ctx);
	value = (intmax_t)duk_require_number(ctx, 0);
	num_bytes = duk_require_int(ctx, 1);
	little_endian = argc >= 3 ? duk_require_boolean(ctx, 2) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "int byte size must be in [1-6] range");
	min_value = pow(-2, num_bytes * 8 - 1);
	max_value = pow(2, num_bytes * 8 - 1) - 1;
	if (value < min_value || value > max_value)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "value is unrepresentable in `%d` bytes", num_bytes);
	if (!write_vsize_int(file, value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write int to file");
	return 0;
}

static duk_ret_t
js_FileStream_writePString(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	intmax_t    max_len;
	intmax_t    num_bytes;
	const char* string;
	duk_size_t  string_len;
	int         uint_size;

	argc = duk_get_top(ctx);
	string = duk_require_lstring(ctx, 0, &string_len);
	uint_size = duk_require_int(ctx, 1);
	little_endian = argc >= 3 ? duk_require_boolean(ctx, 2) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (uint_size < 1 || uint_size > 4)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "length bytes must be in [1-4] range", uint_size);
	max_len = pow(2, uint_size * 8) - 1;
	num_bytes = (intmax_t)string_len;
	if (num_bytes > max_len)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "string is too long for `%d`-byte length", uint_size);
	if (!write_vsize_uint(file, num_bytes, uint_size, little_endian)
		|| sfs_fwrite(string, 1, string_len, file) != string_len)
	{
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write pstring to file");
	}
	return 0;
}

static duk_ret_t
js_FileStream_writeString(duk_context* ctx)
{
	const void* data;
	sfs_file_t* file;
	duk_size_t  num_bytes;

	duk_require_stack_top(ctx, 1);
	data = duk_get_lstring(ctx, 0, &num_bytes);

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (sfs_fwrite(data, 1, num_bytes, file) != num_bytes)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write string to file");
	return 0;
}

static duk_ret_t
js_FileStream_writeUInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	intmax_t    max_value;
	int         num_bytes;
	intmax_t    value;

	argc = duk_get_top(ctx);
	value = (intmax_t)duk_require_number(ctx, 0);
	num_bytes = duk_require_int(ctx, 1);
	little_endian = argc >= 3 ? duk_require_boolean(ctx, 2) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "uint byte size must be in [1-6] range");
	max_value = pow(2, num_bytes * 8) - 1;
	if (value < 0 || value > max_value)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "value is unrepresentable in `%d` bytes", num_bytes);
	if (!write_vsize_uint(file, value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write int to file");
	return 0;
}
