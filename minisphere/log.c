#include "minisphere.h"
#include "api.h"

static duk_ret_t _js_OpenLog        (duk_context* ctx);
static duk_ret_t _js_Log_finalize   (duk_context* ctx);
static duk_ret_t _js_Log_beginBlock (duk_context* ctx);
static duk_ret_t _js_Log_endBlock   (duk_context* ctx);
static duk_ret_t _js_Log_write      (duk_context* ctx);

static void _duk_push_sphere_Log (duk_context* ctx, ALLEGRO_FILE* file);

void
init_log_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "OpenLog", &_js_OpenLog);
}

static void
_duk_push_sphere_Log(duk_context* ctx, ALLEGRO_FILE* file)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, file); duk_put_prop_string(ctx, -2, "\xFF" "file_ptr");
	duk_push_c_function(ctx, &_js_Log_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &_js_Log_beginBlock, DUK_VARARGS); duk_put_prop_string(ctx, -2, "beginBlock");
	duk_push_c_function(ctx, &_js_Log_endBlock, DUK_VARARGS); duk_put_prop_string(ctx, -2, "endBlock");
	duk_push_c_function(ctx, &_js_Log_write, DUK_VARARGS); duk_put_prop_string(ctx, -2, "write");
}

static duk_ret_t
_js_OpenLog(duk_context* ctx)
{
	const char* filename = duk_get_string(ctx, 0);
	char* path = get_asset_path(filename, "logs", true);
	ALLEGRO_FILE* file = al_fopen(path, "a");
	free(path);
	if (file != NULL) {
		_duk_push_sphere_Log(ctx, file);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "OpenLog(): Unable to open log file '%s'", filename);
	}
}

static duk_ret_t
_js_Log_finalize(duk_context* ctx)
{
	ALLEGRO_FILE* file;
	duk_get_prop_string(ctx, 0, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_fclose(file);
	return 0;
}

static duk_ret_t
_js_Log_beginBlock(duk_context* ctx)
{
	const char* name = duk_to_string(ctx, 0);
	ALLEGRO_FILE* file = NULL;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	// TODO: implement beginBlock for logs
	return 0;
}

static duk_ret_t
_js_Log_endBlock(duk_context* ctx)
{
	ALLEGRO_FILE* file = NULL;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	// TODO: implement endBlock for logs
	return 0;
}

static duk_ret_t
_js_Log_write(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	ALLEGRO_FILE* file = NULL;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_fputs(file, text); al_fputc(file, '\n');
	return 0;
}
