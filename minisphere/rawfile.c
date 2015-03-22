#include "minisphere.h"
#include "api.h"
#include "bytearray.h"

#include "rawfile.h"

static duk_ret_t js_HashRawFile         (duk_context* ctx);
static duk_ret_t js_OpenRawFile         (duk_context* ctx);
static duk_ret_t js_RawFile_finalize    (duk_context* ctx);
static duk_ret_t js_RawFile_toString    (duk_context* ctx);
static duk_ret_t js_RawFile_getPosition (duk_context* ctx);
static duk_ret_t js_RawFile_getSize     (duk_context* ctx);
static duk_ret_t js_RawFile_setPosition (duk_context* ctx);
static duk_ret_t js_RawFile_close       (duk_context* ctx);
static duk_ret_t js_RawFile_read        (duk_context* ctx);
static duk_ret_t js_RawFile_write       (duk_context* ctx);

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

	FILE* file;
	char* path;

	path = get_asset_path(filename, "other", false);
	file = fopen(path, "rb");
	free(path);
	if (file == NULL)
		js_error(JS_ERROR, -1, "HashRawFile(): Failed to open file '%s' for reading");
	fclose(file);
	duk_push_string(ctx, "");
	return 1;
}

static duk_ret_t
js_OpenRawFile(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* filename = duk_require_string(ctx, 0);
	bool writable = n_args >= 2 ? duk_require_boolean(ctx, 1) : false;

	FILE* file;
	char* path;

	path = get_asset_path(filename, "other", writable);
	file = fopen(path, writable ? "w+b" : "rb");
	free(path);
	if (file != NULL) {
		duk_push_object(ctx);
		duk_push_pointer(ctx, file); duk_put_prop_string(ctx, -2, "\xFF" "file_ptr");
		duk_push_c_function(ctx, js_RawFile_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
		duk_push_c_function(ctx, js_RawFile_toString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "toString");
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
	FILE* file;

	duk_get_prop_string(ctx, 0, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	if (file != NULL) fclose(file);
	return 0;
}

static duk_ret_t
js_RawFile_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object rawfile]");
	return 1;
}

static duk_ret_t
js_RawFile_getPosition(duk_context* ctx)
{
	FILE* file;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file != NULL) {
		duk_push_int(ctx, ftell(file));
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "RawFile:getPosition(): Attempt to use RawFile object after file has been closed");
	}
}

static duk_ret_t
js_RawFile_getSize(duk_context* ctx)
{
	FILE* file;
	long  file_pos;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file == NULL)
		js_error(JS_ERROR, -1, "RawFile:getPosition(): File has already been closed");
	file_pos = ftell(file);
	fseek(file, 0, SEEK_END);
	duk_push_int(ctx, ftell(file));
	fseek(file, file_pos, SEEK_SET);
	return 1;
}

static duk_ret_t
js_RawFile_setPosition(duk_context* ctx)
{
	int new_pos = duk_require_int(ctx, 0);
	
	FILE* file;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file != NULL)
		js_error(JS_ERROR, -1, "RawFile:setPosition(): File has already been closed");
	if (!fseek(file, new_pos, SEEK_SET))
		js_error(JS_ERROR, -1, "RawFile:setPosition(): Failed to set read/write position (internal error)");
	return 0;
}

static duk_ret_t
js_RawFile_close(duk_context* ctx)
{
	FILE* file;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file != NULL) {
		fclose(file);
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

	bytearray_t*  array;
	FILE*         file;
	void*         read_buffer;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (num_bytes <= 0)
		js_error(JS_RANGE_ERROR, -1, "RawFile:read(): Must read at least 1 byte and less than 2GB; user requested %i bytes", num_bytes);
	if (file == NULL)
		js_error(JS_ERROR, -1, "RawFile:read(): File has already been closed");
	if (!(read_buffer = malloc(num_bytes)))
		js_error(JS_ERROR, -1, "RawFile:read(): Failed to allocate buffer for file read (internal error)");
	num_bytes = fread(read_buffer, 1, num_bytes, file);
	if (!(array = bytearray_from_buffer(read_buffer, num_bytes)))
		js_error(JS_ERROR, -1, "RawFile:read(): Failed to create byte array (internal error)");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_RawFile_write(duk_context* ctx)
{
	bytearray_t* array = duk_require_sphere_bytearray(ctx, 0);
	
	const void* data;
	FILE*       file;
	size_t      write_size;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (file == NULL)
		js_error(JS_ERROR, -1, "RawFile:write(): File has already been closed");
	data = get_bytearray_buffer(array);
	write_size = get_bytearray_size(array);
	if (fwrite(data, 1, write_size, file) != write_size)
		js_error(JS_ERROR, -1, "RawFile:write(): Write error. The file may be read-only.");
	return 0;
}
