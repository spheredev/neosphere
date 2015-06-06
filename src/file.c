#include "minisphere.h"
#include "api.h"

#include "file.h"

static duk_ret_t js_DoesFileExist   (duk_context* ctx);
static duk_ret_t js_CreateDirectory (duk_context* ctx);
static duk_ret_t js_RemoveDirectory (duk_context* ctx);
static duk_ret_t js_RemoveFile      (duk_context* ctx);
static duk_ret_t js_Rename          (duk_context* ctx);
static duk_ret_t js_OpenFile        (duk_context* ctx);
static duk_ret_t js_new_File        (duk_context* ctx);
static duk_ret_t js_File_finalize   (duk_context* ctx);
static duk_ret_t js_File_toString   (duk_context* ctx);
static duk_ret_t js_File_getKey     (duk_context* ctx);
static duk_ret_t js_File_getNumKeys (duk_context* ctx);
static duk_ret_t js_File_close      (duk_context* ctx);
static duk_ret_t js_File_flush      (duk_context* ctx);
static duk_ret_t js_File_read       (duk_context* ctx);
static duk_ret_t js_File_write      (duk_context* ctx);

struct file
{
	unsigned int    id;
	ALLEGRO_CONFIG* conf;
	bool            is_dirty;
	char*           path;
};

static unsigned int s_next_file_id = 0;

file_t*
open_file(const char* path)
{
	file_t* file;
	
	if (!(file = calloc(1, sizeof(file_t))))
		goto on_error;
	if (al_filename_exists(path)) {
		if (!(file->conf = al_load_config_file(path)))
			goto on_error;
	}
	else {
		if ((file->conf = al_create_config()) == NULL)
			goto on_error;
	}
	file->path = strdup(path);
	file->id = s_next_file_id++;
	if (g_game_path != NULL) {
		console_log(2, "engine: Opened KVP file from file system [file %u]", file->id);
		console_log(2, "  Path: %s", relativepath(path, NULL));
	}
	return file;

on_error:
	if (file != NULL) {
		if (file->conf != NULL)
			al_destroy_config(file->conf);
		free(file);
	}
	return NULL;
}

void
close_file(file_t* file)
{
	if (file == NULL)
		return;
	if (file->is_dirty)
		save_file(file);
	al_destroy_config(file->conf);
	if (g_game_path != NULL)
		console_log(2, "file %u: Closed file", file->id);
	free(file);
}

int
get_record_count(file_t* file)
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
get_record_name(file_t* file, int index)
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
read_bool_rec(file_t* file, const char* key, bool def_value)
{
	char* string;
	bool  value;

	string = read_string_rec(file, key, def_value ? "true" : "false");
	value = strcasecmp(string, "true") == 0;
	free(string);
	return value;
}

double
read_number_rec(file_t* file, const char* key, double def_value)
{
	char   def_string[500];
	char*  string;
	double value;

	sprintf(def_string, "%f", def_value);
	string = read_string_rec(file, key, def_string);
	value = atof(string);
	free(string);
	return value;
}

char*
read_string_rec(file_t* file, const char* key, const char* def_value)
{
	const char* read_value;
	char*       value;
	
	if (!(read_value = al_get_config_value(file->conf, NULL, key)))
		read_value = def_value;
	value = strdup(read_value);
	if (g_game_path != NULL)
		console_log(3, "file %u: Read value \"%f\" from key '%s'", file->id, value, key);
	return value;
}

bool
save_file(file_t* file)
{
	if (!al_save_config_file(file->path, file->conf))
		return false;
	if (g_game_path != NULL)
		console_log(3, "file %u: Saved file to file system", file->id);
	return true;
}

void
write_bool_rec(file_t* file, const char* key, bool value)
{
	al_set_config_value(file->conf, NULL, key, value ? "true" : "false");
	file->is_dirty = true;
	if (g_game_path != NULL) {
		console_log(3, "file %u: Wrote boolean value '%s' under key '%s'", 
			file->id, value ? "true" : "false", key);
	}
}

void
write_number_rec(file_t* file, const char* key, double value)
{
	char string[500];

	sprintf(string, "%f", value);
	al_set_config_value(file->conf, NULL, key, string);
	file->is_dirty = true;
	if (g_game_path != NULL)
		console_log(3, "file %u: Wrote number value %f under key '%s'", file->id, value, key);
}

void
write_string_rec(file_t* file, const char* key, const char* value)
{
	al_set_config_value(file->conf, NULL, key, value);
	file->is_dirty = true;
	if (g_game_path != NULL)
		console_log(3, "file %u: Wrote string value \"%s\" under key '%s'", file->id, value, key);
}

void
init_file_api(void)
{
	// File API function
	register_api_function(g_duk, NULL, "DoesFileExist", js_DoesFileExist);
	register_api_function(g_duk, NULL, "CreateDirectory", js_CreateDirectory);
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
}

static duk_ret_t
js_DoesFileExist(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	char* path;

	path = get_asset_path(filename, "save", true);
	duk_push_boolean(ctx, al_filename_exists(path));
	free(path);
	return 1;
}

static duk_ret_t
js_CreateDirectory(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	char* path;

	path = get_asset_path(name, "save", true);
	if (!al_make_directory(path))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateDirectory(): Failed to create directory '%s'", name);
	free(path);
	return 0;
}

static duk_ret_t
js_RemoveDirectory(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	char* path;

	path = get_asset_path(name, "save", true);
	if (rmdir(path) == -1)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RemoveDirectory(): Failed to remove directory '%s'", name);
	free(path);
	return 0;
}

static duk_ret_t
js_RemoveFile(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	
	char* path;

	path = get_asset_path(filename, "save", true);
	if (!al_filename_exists(path))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RemoveFile(): File '%s' doesn't exist", filename);
	if (!al_remove_filename(path))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RemoveFile(): Failed to delete file '%s'; may be read-only", filename);
	free(path);
	return 0;
}

static duk_ret_t
js_Rename(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	const char* new_filename = duk_require_string(ctx, 1);

	char* path;
	char* new_path;

	path = get_asset_path(filename, "save", true);
	new_path = get_asset_path(new_filename, "save", true);
	if (!al_filename_exists(path))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Rename(): File '%s' doesn't exist", filename);
	if (rename(path, new_path) == -1)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Rename(): Failed to rename file '%s' to '%s'", filename, new_filename);
	free(new_path);
	free(path);
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
	
	file_t* file;
	char*   path;

	path = get_asset_path(filename, "save", true);
	if (!(file = open_file(path)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenFile(): Failed to create or open file '%s'", filename);
	duk_push_sphere_obj(ctx, "File", file);
	return 1;
}

static duk_ret_t
js_File_finalize(duk_context* ctx)
{
	file_t* file;

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
	
	file_t*     file;
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
	file_t* file;
	int index = duk_require_int(ctx, 0);

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getKey(): File has been closed");
	duk_push_int(ctx, get_record_count(file));
	return 1;
}

static duk_ret_t
js_File_flush(duk_context* ctx)
{
	file_t* file;

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
	file_t* file;

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
	file_t* file;
	const char* key = duk_to_string(ctx, 0);
	
	bool        def_bool;
	double      def_num;
	const char* def_string;
	char*       value;

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
		free(value);
		break;
	}
	return 1;
}

static duk_ret_t
js_File_write(duk_context* ctx)
{
	file_t* file;
	const char* key = duk_to_string(ctx, 0);

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "File");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:write(): File has been closed");
	write_string_rec(file, key, duk_to_string(ctx, 1));
	return 0;
}
