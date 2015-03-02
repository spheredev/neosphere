#include "minisphere.h"
#include "api.h"
#include "bytearray.h"

#include "rawfile.h"

static duk_ret_t js_HashRawFile(duk_context* ctx);
static duk_ret_t js_OpenRawFile(duk_context* ctx);
static duk_ret_t js_RawFile_finalize(duk_context* ctx);
static duk_ret_t js_RawFile_getPosition(duk_context* ctx);
static duk_ret_t js_RawFile_getSize(duk_context* ctx);
static duk_ret_t js_RawFile_setPosition(duk_context* ctx);
static duk_ret_t js_RawFile_close(duk_context* ctx);
static duk_ret_t js_RawFile_read(duk_context* ctx);
static duk_ret_t js_RawFile_write(duk_context* ctx);

void
init_rawfile_api(void)
{
	duk_push_global_object(g_duktape);
	duk_push_c_function(g_duktape, js_HashRawFile, DUK_VARARGS); duk_put_prop_string(g_duktape, -2, "HashRawFile");
	duk_push_c_function(g_duktape, js_OpenRawFile, DUK_VARARGS); duk_put_prop_string(g_duktape, -2, "OpenRawFile");
	duk_pop(g_duktape);
}

static duk_ret_t
js_HashRawFile(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	ALLEGRO_FILE* file;
	char*         path;

	path = get_asset_path(filename, "other", false);
	file = al_fopen(path, "rb");
	free(path);
	if (file == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "HashRawFile(): Unable to open file '%s' for reading");
	al_fclose(file);
	duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_OpenRawFile(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* filename = duk_require_string(ctx, 0);
	bool writable = n_args >= 2 ? duk_require_boolean(ctx, 1) : false;

	ALLEGRO_FILE* file;
	char*         path;

	path = get_asset_path(filename, "other", writable);
	file = al_fopen(path, writable ? "w+b" : "rb");
	free(path);
	if (file != NULL) {
		duk_push_object(ctx);
		duk_push_pointer(ctx, file); duk_put_prop_string(ctx, -2, "\xFF" "file_ptr");
		duk_push_c_function(ctx, js_RawFile_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
		duk_push_c_function(ctx, js_RawFile_getPosition, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getPosition");
		duk_push_c_function(ctx, js_RawFile_getSize, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getSize");
		duk_push_c_function(ctx, js_RawFile_setPosition, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setPosition");
		duk_push_c_function(ctx, js_RawFile_close, DUK_VARARGS); duk_put_prop_string(ctx, -2, "close");
		duk_push_c_function(ctx, js_RawFile_read, DUK_VARARGS); duk_put_prop_string(ctx, -2, "read");
		duk_push_c_function(ctx, js_RawFile_write, DUK_VARARGS); duk_put_prop_string(ctx, -2, "write");
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "OpenRawFile(): Unable to open file '%s' for %s",
			filename,
			writable ? "writing" : "reading");
	}
}

static duk_ret_t
js_RawFile_finalize(duk_context* ctx)
{
	ALLEGRO_FILE* file;

	duk_get_prop_string(ctx, 0, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	if (file != NULL) al_fclose(file);
	return 0;
}

static duk_ret_t
js_RawFile_getPosition(duk_context* ctx)
{
	ALLEGRO_FILE* file;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file != NULL) {
		duk_push_int(ctx, al_ftell(file));
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "RawFile:getPosition(): Attempt to use RawFile object after file has been closed");
	}
}

static duk_ret_t
js_RawFile_getSize(duk_context* ctx)
{
	ALLEGRO_FILE* file;
	int64_t       cur_pos;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file != NULL) {
		cur_pos = al_ftell(file);
		al_fseek(file, 0, ALLEGRO_SEEK_END);
		duk_push_int(ctx, al_ftell(file));
		al_fseek(file, cur_pos, ALLEGRO_SEEK_SET);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "RawFile:getPosition(): Attempt to use RawFile object after file has been closed");
	}
}

static duk_ret_t
js_RawFile_setPosition(duk_context* ctx)
{
	int new_pos = duk_require_int(ctx, 0);
	
	ALLEGRO_FILE* file;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file != NULL) {
		if (!al_fseek(file, new_pos, ALLEGRO_SEEK_SET))
			duk_error(ctx, DUK_ERR_ERROR, "RawFile:setPosition(): Failed to set RawFile read/write position");
		return 0;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "RawFile:setPosition(): Attempt to use RawFile object after file has been closed");
	}
}

static duk_ret_t
js_RawFile_close(duk_context* ctx)
{
	ALLEGRO_FILE* file;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file != NULL) {
		al_fclose(file);
		duk_push_this(ctx);
		duk_push_pointer(ctx, NULL); duk_put_prop_string(ctx, -2, "\xFF" "file_ptr");
		duk_pop(ctx);
		return 0;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "RawFile:close(): Attempt to use RawFile object after file has been closed");
	}
}

static duk_ret_t
js_RawFile_read(duk_context* ctx)
{
	int num_bytes = duk_require_int(ctx, 0);

	uint8_t*      buffer;
	ALLEGRO_FILE* file;

	if (num_bytes <= 0) {
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "RawFile:read(): Must read at least 1 byte and less than 2GB; caller requested %i bytes", num_bytes);
	}
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file != NULL) {
		if ((buffer = malloc(num_bytes)) == NULL) {
			duk_error(ctx, DUK_ERR_ALLOC_ERROR, "RawFile:read(): Failed to allocate buffer for RawFile read");
		}
		num_bytes = al_fread(file, buffer, num_bytes);
		duk_push_sphere_bytearray(ctx, buffer, num_bytes);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "RawFile:read(): Attempt to use RawFile object after file has been closed");
	}
}

static duk_ret_t
js_RawFile_write(duk_context* ctx)
{
	uint8_t*      buffer;
	int           buf_size;
	ALLEGRO_FILE* file;

	duk_require_object_coercible(ctx, 0);

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file != NULL) {
		duk_get_prop_string(ctx, 0, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, 0, "\xFF" "size"); buf_size = duk_get_int(ctx, -1); duk_pop(ctx);
		if (buffer == NULL || buf_size <= 0)
			duk_error(ctx, DUK_ERR_TYPE_ERROR, "RawFile:write(): Expected a ByteArray as argument, caller passed something else");
		if (al_fwrite(file, buffer, buf_size) != buf_size)
			duk_error(ctx, DUK_ERR_ERROR, "RawFile:write(): Write error. The RawFile may be read-only.");
		return 0;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "RawFile:write(): Attempt to use RawFile object after file has been closed");
	}
}
