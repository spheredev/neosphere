#include "minisphere.h"
#include "api.h"

#include "bytearray.h"

static void duk_push_sphere_bytearray (duk_context* ctx, uint8_t* buffer, int size);

static duk_ret_t _js_CreateByteArray(duk_context* ctx);
static duk_ret_t _js_CreateByteArrayFromString(duk_context* ctx);
static duk_ret_t _js_CreateStringFromByteArray(duk_context* ctx);
static duk_ret_t _js_HashByteArray(duk_context* ctx);
static duk_ret_t _js_ByteArray_finalize(duk_context* ctx);
static duk_ret_t _js_ByteArray_getProp(duk_context* ctx);
static duk_ret_t _js_ByteArray_setProp(duk_context* ctx);
static duk_ret_t _js_ByteArray_concat(duk_context* ctx);
static duk_ret_t _js_ByteArray_slice(duk_context* ctx);

void
init_bytearray_api(void)
{
	register_api_func(g_duktape, NULL, "CreateByteArray", _js_CreateByteArray);
	register_api_func(g_duktape, NULL, "CreateByteArrayFromString", _js_CreateByteArrayFromString);
	register_api_func(g_duktape, NULL, "CreateStringFromByteArray", _js_CreateStringFromByteArray);
	register_api_func(g_duktape, NULL, "HashByteArray", _js_HashByteArray);
}

static void
duk_push_sphere_bytearray(duk_context* ctx, uint8_t* buffer, int size)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, buffer); duk_put_prop_string(ctx, -2, "\xFF" "buffer");
	duk_push_int(ctx, size); duk_put_prop_string(ctx, -2, "\xFF" "size");
	duk_push_c_function(ctx, _js_ByteArray_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, _js_ByteArray_concat, DUK_VARARGS); duk_put_prop_string(ctx, -2, "concat");
	duk_push_c_function(ctx, _js_ByteArray_slice, DUK_VARARGS); duk_put_prop_string(ctx, -2, "slice");
	duk_push_string(ctx, "length"); duk_push_int(ctx, size);
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0
		| DUK_DEFPROP_HAVE_WRITABLE | 0
		| DUK_DEFPROP_HAVE_VALUE);

	// return proxy object so we can catch array accesses
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Proxy");
	duk_dup(ctx, -3);
	duk_push_object(ctx);
	duk_push_c_function(ctx, _js_ByteArray_getProp, DUK_VARARGS); duk_put_prop_string(ctx, -2, "get");
	duk_push_c_function(ctx, _js_ByteArray_setProp, DUK_VARARGS); duk_put_prop_string(ctx, -2, "set");
	duk_new(ctx, 2);
	duk_remove(ctx, -2);
	duk_remove(ctx, -2);
}

static duk_ret_t
_js_CreateByteArray(duk_context* ctx)
{
	uint8_t* buffer;
	int      size;

	size = duk_to_int(ctx, 0);
	if (size > 0) {
		buffer = calloc(size, 1);
		duk_push_sphere_bytearray(ctx, buffer, size);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "CreateByteArray(): Size must not be zero or negative (caller passed %i)", size);
	}
}

static duk_ret_t
_js_CreateByteArrayFromString(duk_context* ctx)
{
	uint8_t*    buffer;
	const char* string;
	size_t      str_len;

	string = duk_require_lstring(ctx, 0, &str_len);
	if (str_len <= INT_MAX) {
		buffer = malloc(str_len);
		memcpy(buffer, string, str_len);
		duk_push_sphere_bytearray(ctx, buffer, (int)str_len);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ALLOC_ERROR, "CreateByteArrayFromString(): Input string too large, size of ByteArray cannot exceed 2 GB");
	}
}

static duk_ret_t
_js_CreateStringFromByteArray(duk_context* ctx)
{
	uint8_t* buffer;
	int      size;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "size"); size = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_lstring(ctx, (char*)buffer, size);
	return 1;
}

static duk_ret_t
_js_HashByteArray(duk_context* ctx)
{
	duk_push_string(ctx, "");
	return 1;
}

static duk_ret_t
_js_ByteArray_finalize(duk_context* ctx)
{
	uint8_t* buffer;
	
	duk_get_prop_string(ctx, 0, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
	free(buffer);
	return 0;
}

static duk_ret_t
_js_ByteArray_getProp(duk_context* ctx)
{
	uint8_t* buffer;
	int      offset;
	int      size;

	duk_get_prop_string(ctx, 0, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "size"); size = duk_get_int(ctx, -1); duk_pop(ctx);
	if (duk_is_number(ctx, 1)) {
		if ((offset = duk_to_int(ctx, 1)) < size) {
			duk_push_uint(ctx, buffer[offset]);
			return 1;
		}
		else {
			duk_error(ctx, DUK_ERR_RANGE_ERROR, "ByteArray[]: Offset [%i] is out of bounds (size: %i)", offset, size);
		}
	}
	else {
		duk_dup(ctx, 1);
		duk_get_prop(ctx, 0);
		return 1;
	}
}

static duk_ret_t
_js_ByteArray_setProp(duk_context* ctx)
{
	uint8_t* buffer;
	int      offset;
	int      size;

	duk_get_prop_string(ctx, 0, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "size"); size = duk_get_int(ctx, -1); duk_pop(ctx);
	if (duk_is_number(ctx, 1)) {
		offset = duk_to_int(ctx, 1);
		if (offset >= 0 && offset < size) {
			buffer[offset] = duk_get_uint(ctx, 2);
			return 0;
		}
		else {
			duk_error(ctx, DUK_ERR_RANGE_ERROR, "ByteArray[]: Offset [%i] is out of bounds (size: %i)", offset, size);
		}
	}
	else {
		duk_dup(ctx, 1);
		duk_dup(ctx, 2);
		duk_put_prop(ctx, 0);
		return 0;
	}
}

static duk_ret_t
_js_ByteArray_concat(duk_context* ctx)
{
	uint8_t  *buffer1, *buffer2;
	uint8_t* new_buffer;
	int      new_size;
	int      size1, size2;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "buffer"); buffer1 = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "size"); size1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "buffer"); buffer2 = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "size"); size2 = duk_get_int(ctx, -1); duk_pop(ctx);
	if ((unsigned int)(size1 + size2) <= INT_MAX) {
		new_size = size1 + size2;
		new_buffer = calloc(new_size, 1);
		memcpy(new_buffer, buffer1, size1);
		memcpy(new_buffer + size1, buffer2, size2);
		duk_push_sphere_bytearray(ctx, new_buffer, new_size);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ALLOC_ERROR, "ByteArray:concat(): Concatenation not possible, final size would exceed 2 GB (size1: %i, size2: %i)", size1, size2);
	}
}

static duk_ret_t
_js_ByteArray_slice(duk_context* ctx)
{
	uint8_t* buffer;
	int      end, end_norm;
	int      n_args;
	uint8_t* new_buffer;
	int      new_size;
	int      size;
	int      start;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "size"); size = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	n_args = duk_get_top(ctx);
	start = duk_to_int(ctx, 0);
	end = (n_args >= 2) ? duk_to_int(ctx, 1) : size;
	end_norm = end >= 0 ? end : size + end;
	if (end_norm > start && end_norm < size) {
		new_size = end_norm - start;
		if ((new_buffer = calloc(new_size, 1)) == NULL) goto on_alloc_error;
		memcpy(new_buffer, buffer + start, new_size);
		duk_push_sphere_bytearray(ctx, new_buffer, new_size);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "ByteArray:slice(): Start and/or end values out of bounds (start: %i, end: %i - size: %i)", start, end_norm, size);
	}

on_alloc_error:
	duk_error(ctx, DUK_ERR_ALLOC_ERROR, "ByteArray:slice(): Unable to allocate memory for new ByteArray");
}
