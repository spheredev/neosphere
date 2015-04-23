#include "minisphere.h"
#include "api.h"
#include "bytearray.h"

#include "rawfile.h"

static duk_ret_t js_HashRawFile          (duk_context* ctx);
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

void
init_rawfile_api(void)
{
	// RawFile API functions
	register_api_func(g_duk, NULL, "HashRawFile", js_HashRawFile);
	
	// RawFile object
	register_api_func(g_duk, NULL, "OpenRawFile", js_OpenRawFile);
	register_api_ctor(g_duk, "RawFile", js_new_RawFile, js_RawFile_finalize);
	register_api_func(g_duk, "RawFile", "toString", js_RawFile_toString);
	register_api_prop(g_duk, "RawFile", "length", js_RawFile_get_size, NULL);
	register_api_prop(g_duk, "RawFile", "position", js_RawFile_get_position, js_RawFile_set_position);
	register_api_prop(g_duk, "RawFile", "size", js_RawFile_get_size, NULL);
	register_api_func(g_duk, "RawFile", "getPosition", js_RawFile_get_position);
	register_api_func(g_duk, "RawFile", "setPosition", js_RawFile_set_position);
	register_api_func(g_duk, "RawFile", "getSize", js_RawFile_get_size);
	register_api_func(g_duk, "RawFile", "close", js_RawFile_close);
	register_api_func(g_duk, "RawFile", "read", js_RawFile_read);
	register_api_func(g_duk, "RawFile", "readString", js_RawFile_readString);
	register_api_func(g_duk, "RawFile", "write", js_RawFile_write);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "HashRawFile(): Failed to open file '%s' for reading");
	fclose(file);
	// TODO: implement raw file hashing
	duk_error_ni(ctx, -1, DUK_ERR_ERROR, "HashRawFile(): Function is not yet implemented");
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

	FILE* file;
	char* path;

	path = get_asset_path(filename, "other", writable);
	file = fopen(path, writable ? "w+b" : "rb");
	free(path);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenRawFile(): Failed to open file '%s' for %s",
			filename, writable ? "writing" : "reading");
	duk_push_sphere_obj(ctx, "RawFile", file);
	return 1;
}

static duk_ret_t
js_RawFile_finalize(duk_context* ctx)
{
	FILE* file;

	file = duk_require_sphere_obj(ctx, 0, "RawFile");
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
js_RawFile_get_position(duk_context* ctx)
{
	FILE* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position - File has been closed");
	duk_push_int(ctx, ftell(file));
	return 1;
}

static duk_ret_t
js_RawFile_set_position(duk_context* ctx)
{
	int new_pos = duk_require_int(ctx, 0);

	FILE* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position - File has been closed");
	if (!fseek(file, new_pos, SEEK_SET))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position - Failed to set read/write position");
	return 0;
}

static duk_ret_t
js_RawFile_get_size(duk_context* ctx)
{
	FILE* file;
	long  file_pos;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:size - File has been closed");
	file_pos = ftell(file);
	fseek(file, 0, SEEK_END);
	duk_push_int(ctx, ftell(file));
	fseek(file, file_pos, SEEK_SET);
	return 1;
}

static duk_ret_t
js_RawFile_close(duk_context* ctx)
{
	FILE* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_push_pointer(ctx, NULL); duk_put_prop_string(ctx, -2, "\xFF" "file_ptr");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:close(): File has been closed");
	fclose(file);
	return 0;
}

static duk_ret_t
js_RawFile_read(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	long num_bytes = n_args >= 1 ? duk_require_int(ctx, 0) : 0;
	
	bytearray_t* array;
	FILE*        file;
	long         pos;
	void*        read_buffer;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): File has been closed");
	if (n_args < 1) {  // if no arguments, read entire file back to front
		pos = ftell(file);
		num_bytes = (fseek(file, 0, SEEK_END), ftell(file));
		fseek(file, 0, SEEK_SET);
	}
	if (num_bytes <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RawFile:read(): Read size out of range (%u)", num_bytes);
	if (!(read_buffer = malloc(num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): Failed to allocate buffer for file read");
	num_bytes = (long)fread(read_buffer, 1, num_bytes, file);
	if (n_args < 1)  // reset file position after whole-file read
		fseek(file, pos, SEEK_SET);
	if (!(array = bytearray_from_buffer(read_buffer, (int)num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): Failed to create byte array");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_RawFile_readString(duk_context* ctx)
{
	size_t num_bytes = duk_require_uint(ctx, 0);

	FILE* file;
	void* read_buffer;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (num_bytes <= 0 || num_bytes > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RawFile:read(): Read size out of range (%i)", num_bytes / 1048576);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): File has been closed");
	if (!(read_buffer = malloc(num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): Failed to allocate buffer for file read");
	num_bytes = fread(read_buffer, 1, num_bytes, file);
	duk_push_lstring(ctx, read_buffer, num_bytes);
	return 1;
}

static duk_ret_t
js_RawFile_write(duk_context* ctx)
{
	bytearray_t* array;
	const void*  data;
	FILE*        file;
	size_t       write_size;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "RawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:write(): File has been closed");
	if (duk_is_string(ctx, 0))
		data = duk_get_lstring(ctx, 0, &write_size);
	else {
		array = duk_require_sphere_obj(ctx, 0, "ByteArray");
		data = get_bytearray_buffer(array);
		write_size = get_bytearray_size(array);
	}
	if (fwrite(data, 1, write_size, file) != write_size)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:write(): Write error. The file may be read-only.");
	return 0;
}
