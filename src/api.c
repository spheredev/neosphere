#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "vector.h"

static double const SPHERE_API_VERSION = 1.5;

static const char* const SPHERE_EXTENSIONS[] =
{
	"minisphere",
	"sphere-legacy-api",
	"sphere-obj-constructors",
	"sphere-obj-props",
	"sphere-map-engine",
	"sphere-galileo",
	"minisphere-new-sockets",
	"minisphere-rng-object",
	"frameskip-api",
	"set-script-function",
};

static duk_ret_t duk_on_create_error(duk_context* ctx);

static duk_ret_t js_GetVersion           (duk_context* ctx);
static duk_ret_t js_GetVersionString     (duk_context* ctx);
static duk_ret_t js_GetExtensions        (duk_context* ctx);
static duk_ret_t js_RequireSystemScript  (duk_context* ctx);
static duk_ret_t js_RequireScript        (duk_context* ctx);
static duk_ret_t js_EvaluateSystemScript (duk_context* ctx);
static duk_ret_t js_EvaluateScript       (duk_context* ctx);
static duk_ret_t js_IsSkippedFrame       (duk_context* ctx);
static duk_ret_t js_GetDirectoryList     (duk_context* ctx);
static duk_ret_t js_GetFileList          (duk_context* ctx);
static duk_ret_t js_GetFrameRate         (duk_context* ctx);
static duk_ret_t js_GetGameList          (duk_context* ctx);
static duk_ret_t js_GetMaxFrameSkips     (duk_context* ctx);
static duk_ret_t js_GetScreenHeight      (duk_context* ctx);
static duk_ret_t js_GetScreenWidth       (duk_context* ctx);
static duk_ret_t js_GetSeconds           (duk_context* ctx);
static duk_ret_t js_GetTime              (duk_context* ctx);
static duk_ret_t js_SetFrameRate         (duk_context* ctx);
static duk_ret_t js_SetMaxFrameSkips     (duk_context* ctx);
static duk_ret_t js_Abort                (duk_context* ctx);
static duk_ret_t js_Alert                (duk_context* ctx);
static duk_ret_t js_CreateStringFromCode (duk_context* ctx);
static duk_ret_t js_Delay                (duk_context* ctx);
static duk_ret_t js_ExecuteGame          (duk_context* ctx);
static duk_ret_t js_Exit                 (duk_context* ctx);
static duk_ret_t js_FlipScreen           (duk_context* ctx);
static duk_ret_t js_GarbageCollect       (duk_context* ctx);
static duk_ret_t js_Print                (duk_context* ctx);
static duk_ret_t js_RestartGame          (duk_context* ctx);
static duk_ret_t js_UnskipFrame          (duk_context* ctx);

static vector_t*  s_extensions;
static int        s_framerate = 0;
static lstring_t* s_user_agent;

void
initialize_api(duk_context* ctx)
{
	int num_extensions;

	int i;

	printf("Initializing Sphere API\n");

	s_user_agent = new_lstring("v%.1f (compatible; %s)", SPHERE_API_VERSION, ENGINE_NAME);
	printf("  Sphere %s\n", lstring_cstr(s_user_agent));

	// register API extensions
	s_extensions = new_vector(sizeof(char*));
	num_extensions = sizeof(SPHERE_EXTENSIONS) / sizeof(SPHERE_EXTENSIONS[0]);
	for (i = 0; i < num_extensions; ++i) {
		printf("  %s\n", SPHERE_EXTENSIONS[i]);
		register_api_extension(SPHERE_EXTENSIONS[i]);
	}

	// inject __defineGetter__/__defineSetter__ polyfills
	duk_eval_string(ctx, "Object.defineProperty(Object.prototype, '__defineGetter__', { value: function(name, func) {"
		"Object.defineProperty(this, name, { get: func, configurable: true }); } });");
	duk_eval_string(ctx, "Object.defineProperty(Object.prototype, '__defineSetter__', { value: function(name, func) {"
		"Object.defineProperty(this, name, { set: func, configurable: true }); } });");
	
	// register core API functions
	register_api_function(ctx, NULL, "GetVersion", js_GetVersion);
	register_api_function(ctx, NULL, "GetVersionString", js_GetVersionString);
	register_api_function(ctx, NULL, "GetExtensions", js_GetExtensions);
	register_api_function(ctx, NULL, "EvaluateScript", js_EvaluateScript);
	register_api_function(ctx, NULL, "EvaluateSystemScript", js_EvaluateSystemScript);
	register_api_function(ctx, NULL, "RequireScript", js_RequireScript);
	register_api_function(ctx, NULL, "RequireSystemScript", js_RequireSystemScript);
	register_api_function(ctx, NULL, "IsSkippedFrame", js_IsSkippedFrame);
	register_api_function(ctx, NULL, "GetDirectoryList", js_GetDirectoryList);
	register_api_function(ctx, NULL, "GetFileList", js_GetFileList);
	register_api_function(ctx, NULL, "GetFrameRate", js_GetFrameRate);
	register_api_function(ctx, NULL, "GetGameList", js_GetGameList);
	register_api_function(ctx, NULL, "GetMaxFrameSkips", js_GetMaxFrameSkips);
	register_api_function(ctx, NULL, "GetScreenHeight", js_GetScreenHeight);
	register_api_function(ctx, NULL, "GetScreenWidth", js_GetScreenWidth);
	register_api_function(ctx, NULL, "GetSeconds", js_GetSeconds);
	register_api_function(ctx, NULL, "GetTime", js_GetTime);
	register_api_function(ctx, NULL, "SetFrameRate", js_SetFrameRate);
	register_api_function(ctx, NULL, "SetMaxFrameSkips", js_SetMaxFrameSkips);
	register_api_function(ctx, NULL, "Abort", js_Abort);
	register_api_function(ctx, NULL, "Alert", js_Alert);
	register_api_function(ctx, NULL, "CreateStringFromCode", js_CreateStringFromCode);
	register_api_function(ctx, NULL, "Delay", js_Delay);
	register_api_function(ctx, NULL, "Exit", js_Exit);
	register_api_function(ctx, NULL, "ExecuteGame", js_ExecuteGame);
	register_api_function(ctx, NULL, "FlipScreen", js_FlipScreen);
	register_api_function(ctx, NULL, "GarbageCollect", js_GarbageCollect);
	register_api_function(ctx, NULL, "Print", js_Print);
	register_api_function(ctx, NULL, "RestartGame", js_RestartGame);
	register_api_function(ctx, NULL, "UnskipFrame", js_UnskipFrame);
	
	// set up RequireScript() inclusion tracking table
	duk_push_global_stash(ctx);
	duk_push_object(ctx); duk_put_prop_string(ctx, -2, "RequireScript");
	duk_pop(ctx);

	// register error callback (adds filename and line number to duk_require_xxx() errors)
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Duktape");
	duk_push_c_function(ctx, duk_on_create_error, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "errCreate");
	duk_pop_2(ctx);
}

void
shutdown_api(void)
{
	printf("Shutting down Sphere API\n");

	free_lstring(s_user_agent);
}

void
register_api_const(duk_context* ctx, const char* name, double value)
{
	duk_push_global_object(ctx);
	duk_push_string(ctx, name); duk_push_number(ctx, value);
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0
		| DUK_DEFPROP_HAVE_ENUMERABLE | 0
		| DUK_DEFPROP_HAVE_WRITABLE | 0
		| DUK_DEFPROP_HAVE_VALUE);
	duk_pop(ctx);
}

void
register_api_ctor(duk_context* ctx, const char* name, duk_c_function fn, duk_c_function finalizer)
{
	duk_push_global_object(ctx);
	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_push_object(ctx);
	duk_push_string(ctx, name); duk_put_prop_string(ctx, -2, "\xFF" "ctor");
	if (finalizer != NULL) {
		duk_push_c_function(ctx, finalizer, DUK_VARARGS);
		duk_put_prop_string(ctx, -2, "\xFF" "dtor");
	}
	duk_put_prop_string(ctx, -2, "prototype");
	duk_put_prop_string(ctx, -2, name);
	duk_pop(ctx);
}

bool
register_api_extension(const char* designation)
{
	char* string;

	if (!(string = strdup(designation))) return false;
	if (!push_back_vector(s_extensions, &string))
		return false;
	return true;
}

void
register_api_function(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn)
{
	duk_push_global_object(ctx);
	if (ctor_name != NULL) {
		duk_get_prop_string(ctx, -1, ctor_name);
		duk_get_prop_string(ctx, -1, "prototype");
	}
	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, name);
	if (ctor_name != NULL)
		duk_pop_2(ctx);
	duk_pop(ctx);
}

void
register_api_prop(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function getter, duk_c_function setter)
{
	duk_uint_t flags;
	int        obj_index;
	
	duk_push_global_object(ctx);
	if (ctor_name != NULL) {
		duk_get_prop_string(ctx, -1, ctor_name);
		duk_get_prop_string(ctx, -1, "prototype");
	}
	obj_index = duk_normalize_index(ctx, -1);
	duk_push_string(ctx, name);
	flags = DUK_DEFPROP_HAVE_ENUMERABLE | 0
		| DUK_DEFPROP_CONFIGURABLE | 0;
	if (getter != NULL) {
		duk_push_c_function(ctx, getter, DUK_VARARGS);
		flags |= DUK_DEFPROP_HAVE_GETTER;
	}
	if (setter != NULL) {
		duk_push_c_function(ctx, setter, DUK_VARARGS);
		flags |= DUK_DEFPROP_HAVE_SETTER;
	}
	duk_def_prop(g_duk, obj_index, flags);
	if (ctor_name != NULL)
		duk_pop_2(ctx);
	duk_pop(ctx);
}

duk_bool_t
duk_is_sphere_obj(duk_context* ctx, duk_idx_t index, const char* ctor_name)
{
	const char* obj_ctor_name;
	duk_bool_t  result;

	index = duk_require_normalize_index(ctx, index);
	if (!duk_is_object_coercible(ctx, index))
		return 0;

	duk_get_prop_string(ctx, index, "\xFF" "ctor");
	obj_ctor_name = duk_safe_to_string(ctx, -1);
	result = strcmp(obj_ctor_name, ctor_name) == 0;
	duk_pop(ctx);
	return result;
}

noreturn
duk_error_ni(duk_context* ctx, int blame_offset, duk_errcode_t err_code, const char* fmt, ...)
{
	va_list ap;
	
	const char*   filename = NULL;
	int           line_number;
	const char*   full_path;

	// get filename and line number from Duktape call stack
	duk_push_global_object(g_duk);
	duk_get_prop_string(g_duk, -1, "Duktape");
	duk_get_prop_string(g_duk, -1, "act"); duk_push_int(g_duk, -2 + blame_offset); duk_call(g_duk, 1);
	if (!duk_is_object(g_duk, -1)) {
		duk_pop(g_duk);
		duk_get_prop_string(g_duk, -1, "act"); duk_push_int(g_duk, -2); duk_call(g_duk, 1);
	}
	duk_remove(g_duk, -2);
	duk_get_prop_string(g_duk, -1, "lineNumber"); line_number = duk_to_int(g_duk, -1); duk_pop(g_duk);
	duk_get_prop_string(g_duk, -1, "function");
	duk_get_prop_string(g_duk, -1, "fileName"); full_path = duk_safe_to_string(g_duk, -1); duk_pop(g_duk);
	duk_pop_2(g_duk);

	// strip directory path from filename
	filename = strrchr(full_path, ALLEGRO_NATIVE_PATH_SEP);
	filename = filename != NULL ? filename + 1 : full_path;

	// throw the exception
	va_start(ap, fmt);
	duk_error_va_raw(ctx, err_code, filename, line_number, fmt, ap);
}

void
duk_push_sphere_obj(duk_context* ctx, const char* ctor_name, void* udata)
{
	duk_idx_t index;
	
	duk_push_object(ctx); index = duk_normalize_index(ctx, -1);
	duk_push_pointer(ctx, udata); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, ctor_name);
	duk_get_prop_string(ctx, -1, "prototype");
	if (duk_get_prop_string(ctx, -1, "\xFF" "dtor")) {
		duk_set_finalizer(ctx, index);
	}
	else
		duk_pop(ctx);
	duk_set_prototype(ctx, index);
	duk_pop_2(ctx);
}

void*
duk_require_sphere_obj(duk_context* ctx, duk_idx_t index, const char* ctor_name)
{
	void* udata;

	index = duk_require_normalize_index(ctx, index);
	if (!duk_is_sphere_obj(ctx, index, ctor_name))
		duk_error(ctx, DUK_ERR_TYPE_ERROR, "not a Sphere %s", ctor_name);
	duk_get_prop_string(ctx, index, "\xFF" "udata"); udata = duk_get_pointer(ctx, -1); duk_pop(ctx);
	return udata;
}

static duk_ret_t
duk_on_create_error(duk_context* ctx)
{
	const char* filename;
	int         line;
	const char* message;
	
	if (!duk_is_error(ctx, 0)) return 1;
	duk_get_prop_string(ctx, 0, "message"); message = duk_get_string(ctx, -1); duk_pop(ctx);
	if (strstr(message, "not ") != message || strcmp(message, "not callable") == 0)
		return 1;
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Duktape");
	duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -4); duk_call(ctx, 1);
	duk_get_prop_string(ctx, -1, "lineNumber"); line = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "function");
	duk_get_prop_string(ctx, -1, "fileName"); filename = duk_get_string(ctx, -1); duk_pop(ctx);
	duk_pop_n(ctx, 4);
	duk_push_string(ctx, "fileName"); duk_push_string(ctx, filename);
	duk_def_prop(ctx, 0, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_HAVE_WRITABLE | DUK_DEFPROP_WRITABLE
		| DUK_DEFPROP_HAVE_CONFIGURABLE | DUK_DEFPROP_CONFIGURABLE
		| DUK_DEFPROP_HAVE_ENUMERABLE | 0);
	duk_push_string(ctx, "lineNumber"); duk_push_int(ctx, line);
	duk_def_prop(ctx, 0, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_HAVE_WRITABLE | DUK_DEFPROP_WRITABLE
		| DUK_DEFPROP_HAVE_CONFIGURABLE | DUK_DEFPROP_CONFIGURABLE
		| DUK_DEFPROP_HAVE_ENUMERABLE | 0);
	return 1;
}

static duk_ret_t
js_GetVersion(duk_context* ctx)
{
	duk_push_number(ctx, SPHERE_API_VERSION);
	return 1;
}

static duk_ret_t
js_GetVersionString(duk_context* ctx)
{
	duk_push_string(ctx, lstring_cstr(s_user_agent));
	return 1;
}

static duk_ret_t
js_GetExtensions(duk_context* ctx)
{
	char**  i_string;

	iter_t iter;
	int    i;

	duk_push_array(ctx);
	iter = iterate_vector(s_extensions); i = 0;
	while (i_string = next_vector_item(&iter)) {
		duk_push_string(ctx, *i_string);
		duk_put_prop_index(ctx, -2, i++);
	}
	return 1;
}

static duk_ret_t
js_EvaluateScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_asset_path(script_file, "scripts", false);
	if (!al_filename_exists(script_path))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "EvaluateScript(): Script file not found '%s'", script_file);
	duk_eval_file_noresult(ctx, script_path);
	free(script_path);
	return 0;
}

static duk_ret_t
js_EvaluateSystemScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_sys_asset_path(script_file, "system/scripts");
	if (!al_filename_exists(script_path))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "EvaluateSystemScript(): System script not found '%s'", script_file);
	duk_eval_file_noresult(ctx, script_path);
	free(script_path);
	return 0;
}

static duk_ret_t
js_RequireScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_asset_path(script_file, "scripts", false);
	if (!al_filename_exists(script_path))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RequireScript(): Script file not found '%s'", script_file);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, script_path);
	bool is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_push_true(ctx); duk_put_prop_string(ctx, -2, script_path);
		duk_eval_file_noresult(ctx, script_path);
	}
	duk_pop_2(ctx);
	free(script_path);
	return 0;
}

static duk_ret_t
js_RequireSystemScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_sys_asset_path(script_file, "system/scripts");
	if (!al_filename_exists(script_path))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RequireSystemScript(): System script not found '%s'", script_file);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, script_path);
	bool is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_push_true(ctx); duk_put_prop_string(ctx, -2, script_path);
		duk_eval_file_noresult(ctx, script_path);
	}
	duk_pop_2(ctx);
	free(script_path);
	return 0;
}

static duk_ret_t
js_IsSkippedFrame(duk_context* ctx)
{
	duk_push_boolean(ctx, is_skipped_frame());
	return 1;
}

static duk_ret_t
js_GetDirectoryList(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* directory_name = n_args >= 1 ? duk_require_string(ctx, 0) : "";

	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_PATH*     file_path;
	ALLEGRO_FS_ENTRY* fs;
	char*             path;

	int i;

	path = get_asset_path(directory_name, NULL, false);
	fs = al_create_fs_entry(path);
	free(path);
	duk_push_array(ctx);
	i = 0;
	if (al_get_fs_entry_mode(fs) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fs)) {
		while (file_info = al_read_directory(fs)) {
			if (al_get_fs_entry_mode(file_info) & ALLEGRO_FILEMODE_ISDIR) {
				file_path = al_create_path(al_get_fs_entry_name(file_info));
				duk_push_string(ctx, al_get_path_filename(file_path)); duk_put_prop_index(ctx, -2, i);
				al_destroy_path(file_path);
				++i;
			}
		}
	}
	al_destroy_fs_entry(fs);
	return 1;
}

static duk_ret_t
js_GetFileList(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* directory_name = n_args >= 1 ? duk_require_string(ctx, 0) : "save";
	
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_PATH*     file_path;
	ALLEGRO_FS_ENTRY* fs;
	char*             path;

	int i;

	path = get_asset_path(directory_name, NULL, false);
	fs = al_create_fs_entry(path);
	free(path);
	duk_push_array(ctx);
	i = 0;
	if (al_get_fs_entry_mode(fs) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fs)) {
		while (file_info = al_read_directory(fs)) {
			if (al_get_fs_entry_mode(file_info) & ALLEGRO_FILEMODE_ISFILE) {
				file_path = al_create_path(al_get_fs_entry_name(file_info));
				duk_push_string(ctx, al_get_path_filename(file_path)); duk_put_prop_index(ctx, -2, i);
				al_destroy_path(file_path);
				++i;
			}
		}
	}
	al_destroy_fs_entry(fs);
	return 1;
}

static duk_ret_t
js_GetFrameRate(duk_context* ctx)
{
	duk_push_int(ctx, s_framerate);
	return 1;
}

static duk_ret_t
js_GetGameList(duk_context* ctx)
{
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_PATH*     file_path;
	ALLEGRO_FS_ENTRY* fs;
	char*             path;
	ALLEGRO_CONFIG*   sgm;

	int i;

	path = get_sys_asset_path("games", NULL);
	fs = al_create_fs_entry(path);
	free(path);
	duk_push_array(ctx);
	i = 0;
	if (al_get_fs_entry_mode(fs) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fs)) {
		while (file_info = al_read_directory(fs)) {
			if (al_get_fs_entry_mode(file_info) & ALLEGRO_FILEMODE_ISDIR) {
				file_path = al_create_path_for_directory(al_get_fs_entry_name(file_info));
				al_set_path_filename(file_path, "game.sgm");
				if ((sgm = al_load_config_file(al_path_cstr(file_path, ALLEGRO_NATIVE_PATH_SEP))) != NULL) {
					duk_push_object(ctx);
					duk_push_string(ctx, al_get_path_component(file_path, -1)); duk_put_prop_string(ctx, -2, "directory");
					duk_push_string(ctx, al_get_config_value(sgm, NULL, "name")); duk_put_prop_string(ctx, -2, "name");
					duk_push_string(ctx, al_get_config_value(sgm, NULL, "author")); duk_put_prop_string(ctx, -2, "author");
					duk_push_string(ctx, al_get_config_value(sgm, NULL, "description")); duk_put_prop_string(ctx, -2, "description");
					duk_put_prop_index(ctx, -2, i);
					al_destroy_config(sgm);
				}
				al_destroy_path(file_path);
				++i;
			}
		}
	}
	al_destroy_fs_entry(fs);
	return 1;
}

static duk_ret_t
js_GetMaxFrameSkips(duk_context* ctx)
{
	duk_push_int(ctx, get_max_frameskip());
	return 1;
}

static duk_ret_t
js_GetScreenHeight(duk_context* ctx)
{
	duk_push_int(ctx, g_res_y);
	return 1;
}

static duk_ret_t
js_GetScreenWidth(duk_context* ctx)
{
	duk_push_int(ctx, g_res_x);
	return 1;
}

static duk_ret_t
js_GetSeconds(duk_context* ctx)
{
	duk_push_number(ctx, al_get_time());
	return 1;
}

static duk_ret_t
js_GetTime(duk_context* ctx)
{
	duk_push_number(ctx, floor(al_get_time() * 1000));
	return 1;
}

static duk_ret_t
js_SetFrameRate(duk_context* ctx)
{
	int framerate = duk_require_int(ctx, 0);
	
	if (framerate < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetFrameRate(): Frame rate cannot be negative (%i)", framerate);
	s_framerate = framerate;
	return 0;
}

static duk_ret_t
js_SetMaxFrameSkips(duk_context* ctx)
{
	int max_skips = duk_require_int(ctx, 0);

	if (max_skips < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetMaxFrameSkips(): Value cannot be negative (%i)", max_skips);
	set_max_frameskip(max_skips);
	return 0;
}

static duk_ret_t
js_Alert(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* text = n_args >= 1 && !duk_is_null_or_undefined(ctx, 0)
		? duk_to_string(ctx, 0) : "It's 8:12... do you know where the pig is?\n\nIt's...\n\n\n\n\n\n\nBEHIND YOU! *MUNCH*";
	int stack_offset = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	const char* caller_info;
	const char* filename;
	int         line_number;
	const char* full_path;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Alert(): Stack offset cannot be positive");

	// get filename and line number of Alert() call
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Duktape");
	duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3 + stack_offset); duk_call(ctx, 1);
	if (!duk_is_object(ctx, -1)) {
		duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3); duk_call(ctx, 1);
	}
	duk_remove(ctx, -2);
	duk_get_prop_string(ctx, -1, "lineNumber"); line_number = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "function");
	duk_get_prop_string(ctx, -1, "fileName"); full_path = duk_get_string(ctx, -1); duk_pop(ctx);
	duk_pop_2(ctx);

	// show the message
	filename = strrchr(full_path, ALLEGRO_NATIVE_PATH_SEP);
	filename = filename != NULL ? filename + 1 : full_path;
	caller_info =
		duk_push_sprintf(ctx, "%s (line %i)", filename, line_number),
		duk_get_string(ctx, -1);
	al_show_native_message_box(g_display, "Alert from Sphere game", caller_info, text, NULL, 0x0);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Abort(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* message = n_args >= 1 ? duk_to_string(ctx, 0) : "Some type of weird pig just ate your game! ......................and you*munch*";
	int stack_offset = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Abort(): Stack offset cannot be positive");
	duk_error_ni(ctx, -1 + stack_offset, DUK_ERR_ERROR, "%s", message);
}

static duk_ret_t
js_CreateStringFromCode(duk_context* ctx)
{
	int code = duk_require_int(ctx, 0);

	char cstr[2];

	if (code < 0 || code > 255)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "CreateStringFromCode(): Character code is out of ASCII range (%i)", code);
	cstr[0] = (char)code; cstr[1] = '\0';
	duk_push_string(ctx, cstr);
	return 1;
}

static duk_ret_t
js_Delay(duk_context* ctx)
{
	double millisecs = floor(duk_require_number(ctx, 0));
	
	double end_time;
	double time_left;

	if (millisecs < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Delay(): Time cannot be negative (%.0f)", millisecs);
	end_time = al_get_time() + millisecs / 1000;
	do {
		time_left = al_get_time() - end_time;
		if (time_left > 0.001)  // engine may stall with < 1ms timeout
			al_wait_for_event_timed(g_events, NULL, time_left);
		do_events();
	} while (al_get_time() < end_time);
	return 0;
}

static duk_ret_t
js_ExecuteGame(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	char* path;
	
	if (!(path = get_sys_asset_path(filename, "games")))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ExecuteGame(): Unable to execute game '%s'", filename);
	if (!(g_last_game_path = strdup(al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP))))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ExecuteGame(): Failed to save last game path");
	al_destroy_path(g_game_path);
	g_game_path = al_create_path(path);
	if (strcasecmp(al_get_path_filename(g_game_path), "game.sgm") != 0) {
		al_destroy_path(g_game_path);
		g_game_path = al_create_path_for_directory(path);
	}
	al_set_path_filename(g_game_path, NULL);
	free(path);
	restart_engine();
}

static duk_ret_t
js_Exit(duk_context* ctx)
{
	exit_game(false);
}

static duk_ret_t
js_FlipScreen(duk_context* ctx)
{
	flip_screen(s_framerate);
	return 0;
}

static duk_ret_t
js_GarbageCollect(duk_context* ctx)
{
	duk_gc(ctx, 0x0);
	duk_gc(ctx, 0x0);
	return 0;
}

static duk_ret_t
js_Print(duk_context* ctx)
{
	const char* text = duk_safe_to_string(ctx, 0);
	
	printf("%s\n", text);
	return 0;
}

static duk_ret_t
js_RestartGame(duk_context* ctx)
{
	restart_engine();
}

static duk_ret_t
js_UnskipFrame(duk_context* ctx)
{
	unskip_frame();
	return 0;
}
