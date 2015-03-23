#include "minisphere.h"
#include "api.h"

#include "file.h"

static duk_ret_t js_OpenFile        (duk_context* ctx);
static duk_ret_t js_RemoveFile      (duk_context* ctx);
static duk_ret_t js_File_finalize   (duk_context* ctx);
static duk_ret_t js_File_toString   (duk_context* ctx);
static duk_ret_t js_File_getKey     (duk_context* ctx);
static duk_ret_t js_File_getNumKeys (duk_context* ctx);
static duk_ret_t js_File_close      (duk_context* ctx);
static duk_ret_t js_File_flush      (duk_context* ctx);
static duk_ret_t js_File_read       (duk_context* ctx);
static duk_ret_t js_File_write      (duk_context* ctx);

static void duk_push_sphere_file (duk_context* ctx, ALLEGRO_CONFIG* conf, const char* path);

void
init_file_api(void)
{
	register_api_func(g_duktape, NULL, "OpenFile", js_OpenFile);
	register_api_func(g_duktape, NULL, "RemoveFile", js_RemoveFile);
}

static void
duk_push_sphere_file(duk_context* ctx, ALLEGRO_CONFIG* conf, const char* path)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, conf); duk_put_prop_string(ctx, -2, "\xFF" "conf_ptr");
	duk_push_pointer(ctx, (void*)path); duk_put_prop_string(ctx, -2, "\xFF" "path");
	duk_push_c_function(ctx, js_File_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, js_File_toString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "toString");
	duk_push_c_function(ctx, js_File_getKey, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getKey");
	duk_push_c_function(ctx, js_File_getNumKeys, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getNumKeys");
	duk_push_c_function(ctx, js_File_close, DUK_VARARGS); duk_put_prop_string(ctx, -2, "close");
	duk_push_c_function(ctx, js_File_flush, DUK_VARARGS); duk_put_prop_string(ctx, -2, "flush");
	duk_push_c_function(ctx, js_File_read, DUK_VARARGS); duk_put_prop_string(ctx, -2, "read");
	duk_push_c_function(ctx, js_File_write, DUK_VARARGS); duk_put_prop_string(ctx, -2, "write");
}

static duk_ret_t
js_OpenFile(duk_context* ctx)
{
	ALLEGRO_CONFIG* conf;
	const char*     filename;
	char*           path;

	filename = duk_require_string(ctx, 0);
	path = get_asset_path(filename, "save", true);
	if (al_filename_exists(path)) {
		conf = al_load_config_file(path);
		if (conf == NULL) goto on_error;
	}
	else {
		if ((conf = al_create_config()) == NULL) goto on_error;
		if (!al_save_config_file(path, conf)) goto on_error;
	}
	duk_push_sphere_file(ctx, conf, path);
	return 1;

on_error:
	duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenFile(): Failed to open or create file '%s'", filename);
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
js_File_finalize(duk_context* ctx)
{
	ALLEGRO_CONFIG* conf;
	const char*     path;

	duk_get_prop_string(ctx, 0, "\xFF" "conf_ptr"); conf = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "path"); path = duk_get_pointer(ctx, -1); duk_pop(ctx);
	if (conf != NULL) al_save_config_file(path, conf);
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
	ALLEGRO_CONFIG*       conf;
	ALLEGRO_CONFIG_ENTRY* conf_iter;
	int                   index;
	const char*           key;
	int                   i;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "conf_ptr"); conf = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (conf == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getKey(): File has already been closed");
	index = duk_to_int(ctx, 0);
	i = 0;
	key = al_get_first_config_entry(conf, NULL, &conf_iter);
	while (key != NULL) {
		if (i == index) {
			duk_push_string(ctx, key);
			return 1;
		}
		++i;
		key = al_get_next_config_entry(&conf_iter);
	}
	duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_File_getNumKeys(duk_context* ctx)
{
	ALLEGRO_CONFIG*       conf;
	ALLEGRO_CONFIG_ENTRY* conf_iter;
	int                   count;
	const char*           key;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "conf_ptr"); conf = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (conf == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getNumKeys(): File has already been closed");
	count = 0;
	key = al_get_first_config_entry(conf, NULL, &conf_iter);
	while (key != NULL) {
		++count;
		key = al_get_next_config_entry(&conf_iter);
	}
	duk_push_int(ctx, count);
	return 1;
}

static duk_ret_t
js_File_flush (duk_context* ctx)
{
	ALLEGRO_CONFIG* conf;
	const char*     path;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "conf_ptr"); conf = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "path"); path = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (conf == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:flush(): File has already been closed");
	al_save_config_file(path, conf);
	return 0;
}

static duk_ret_t
js_File_close(duk_context* ctx)
{
	ALLEGRO_CONFIG* conf;
	const char*     path;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "conf_ptr"); conf = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "path"); path = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (conf == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:close(): File has already been closed");
	al_save_config_file(path, conf);
	duk_push_this(ctx);
	duk_push_pointer(ctx, NULL); duk_put_prop_string(ctx, -2, "\xFF" "conf_ptr");
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_File_read(duk_context* ctx)
{
	ALLEGRO_CONFIG* conf;
	bool            def_bool;
	double          def_num;
	const char*     def_string;
	const char*     key;
	const char*     value_raw;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "conf_ptr"); conf = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (conf == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:read(): File has already been closed");
	key = duk_to_string(ctx, 0);
	value_raw = al_get_config_value(conf, NULL, key);
	switch (duk_get_type(ctx, 1)) {
	case DUK_TYPE_BOOLEAN:
		def_bool = duk_get_boolean(ctx, 1);
		if (value_raw != NULL)
			duk_push_boolean(ctx, strcmp(value_raw, "True") == 0
				|| strcmp(value_raw, "true") == 0
				|| strcmp(value_raw, "TRUE") == 0);
		else
			duk_push_boolean(ctx, def_bool);
		break;
	case DUK_TYPE_NUMBER:
		def_num = duk_get_number(ctx, 1);
		duk_push_number(ctx, value_raw != NULL ? atof(value_raw) : def_num);
		break;
	case DUK_TYPE_STRING:
		def_string = duk_get_string(ctx, 1);
		duk_push_string(ctx, value_raw != NULL ? value_raw : def_string);
		break;
	}
	return 1;
}

static duk_ret_t
js_File_write(duk_context* ctx)
{
	ALLEGRO_CONFIG* conf;
	const char*     key;
	const char*     value_str;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "conf_ptr"); conf = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (conf == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:write(): File has already been closed");
	key = duk_to_string(ctx, 0);
	value_str = duk_to_string(ctx, 1);
	al_set_config_value(conf, NULL, key, value_str);
	return 0;
}
