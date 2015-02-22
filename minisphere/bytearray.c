#include "minisphere.h"
#include "api.h"

#include "bytearray.h"

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
duk_push_sphere_bytearray(duk_context* ctx, uint8_t* buffer, size_t size)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, buffer); duk_put_prop_string(ctx, -2, "\xFF" "buffer");
	duk_push_uint(ctx, size); duk_put_prop_string(ctx, -2, "\xFF" "size");
	duk_push_c_function(ctx, _js_ByteArray_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, _js_ByteArray_concat, DUK_VARARGS); duk_put_prop_string(ctx, -2, "concat");
	duk_push_c_function(ctx, _js_ByteArray_slice, DUK_VARARGS); duk_put_prop_string(ctx, -2, "slice");
	
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
	size_t   size;

	size = duk_to_uint(ctx, 0);
	buffer = calloc(size, 1);
	duk_push_sphere_bytearray(ctx, buffer, size);
	return 1;
}

static duk_ret_t
_js_CreateByteArrayFromString(duk_context* ctx)
{
	uint8_t*    buffer;
	const char* string;
	size_t      str_len;

	string = duk_require_lstring(ctx, 0, &str_len);
	buffer = malloc(str_len);
	memcpy(buffer, string, str_len);
	duk_push_sphere_bytearray(ctx, buffer, str_len);
	return 1;
}

static duk_ret_t
_js_CreateStringFromByteArray(duk_context* ctx)
{
	uint8_t* buffer;
	size_t   size;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "size"); size = duk_get_uint(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_lstring(ctx, (char*)buffer, size);
	return 1;
}

static duk_ret_t
_js_HashByteArray(duk_context* ctx)
{
	duk_push_null(ctx);
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
	uint8_t*     buffer;
	unsigned int offset;
	size_t       size;

	duk_get_prop_string(ctx, 0, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "size"); size = duk_get_uint(ctx, -1); duk_pop(ctx);
	if ((offset = duk_require_uint(ctx, 1)) < size) {
		duk_push_uint(ctx, buffer[offset]);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "ByteArray[]: Byte offset (%u) is outside of the buffer", offset);
	}
}

static duk_ret_t
_js_ByteArray_setProp(duk_context* ctx)
{
	uint8_t*     buffer;
	unsigned int offset;
	size_t       size;

	duk_get_prop_string(ctx, 0, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "size"); size = duk_get_uint(ctx, -1); duk_pop(ctx);
	if ((offset = duk_require_uint(ctx, 1)) < size) {
		buffer[offset] = duk_get_uint(ctx, 2);
		return 0;
	}
	else {
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "ByteArray[]: Byte offset (%u) is outside of the buffer", offset);
	}
}

static duk_ret_t
_js_ByteArray_concat(duk_context* ctx)
{
	uint8_t* buffer;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_null(ctx);
	return 1;
}

static duk_ret_t
_js_ByteArray_slice(duk_context* ctx)
{
	uint8_t* buffer;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "buffer"); buffer = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_null(ctx);
	return 1;
}
