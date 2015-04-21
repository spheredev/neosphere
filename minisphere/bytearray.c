#include "minisphere.h"
#include "api.h"

#include "bytearray.h"

static duk_ret_t js_CreateByteArray           (duk_context* ctx);
static duk_ret_t js_CreateByteArrayFromString (duk_context* ctx);
static duk_ret_t js_CreateStringFromByteArray (duk_context* ctx);
static duk_ret_t js_HashByteArray             (duk_context* ctx);
static duk_ret_t js_ByteArray_finalize        (duk_context* ctx);
static duk_ret_t js_ByteArray_toString        (duk_context* ctx);
static duk_ret_t js_ByteArray_getProp         (duk_context* ctx);
static duk_ret_t js_ByteArray_setProp         (duk_context* ctx);
static duk_ret_t js_ByteArray_concat          (duk_context* ctx);
static duk_ret_t js_ByteArray_slice           (duk_context* ctx);

struct bytearray
{
	int      refcount;
	uint8_t* buffer;
	int      size;
};

bytearray_t*
new_bytearray(int size)
{
	bytearray_t* array;
	
	if (!(array = calloc(1, sizeof(bytearray_t))))
		goto on_error;
	if (!(array->buffer = calloc(size, 1))) goto on_error;
	array->size = size;
	return ref_bytearray(array);
	
on_error:
	free(array);
	return NULL;
}

bytearray_t*
bytearray_from_buffer(const void* buffer, int size)
{
	bytearray_t* array;

	if (!(array = new_bytearray(size)))
		return NULL;
	memcpy(array->buffer, buffer, size);
	return array;
}

bytearray_t*
bytearray_from_lstring(const lstring_t* string)
{
	bytearray_t* array;

	if (string->length > INT_MAX)
		return NULL;
	if (!(array = new_bytearray((int)string->length)))
		return NULL;
	memcpy(array->buffer, string->cstr, string->length);
	return array;
}

bytearray_t*
ref_bytearray(bytearray_t* array)
{
	++array->refcount;
	return array;
}

void
free_bytearray(bytearray_t* array)
{
	if (array == NULL || --array->refcount > 0)
		return;
	free(array->buffer);
	free(array);
}

uint8_t
get_byte(bytearray_t* array, int index)
{
	return array->buffer[index];
}

const uint8_t*
get_bytearray_buffer(bytearray_t* array)
{
	return array->buffer;
}


int
get_bytearray_size(bytearray_t* array)
{
	return array->size;
}

void
set_byte(bytearray_t* array, int index, uint8_t value)
{
	array->buffer[index] = value;
}

bytearray_t*
concat_bytearrays(bytearray_t* array1, bytearray_t* array2)
{
	bytearray_t* new_array;
	int          new_size;

	new_size = array1->size + array2->size;
	if (!(new_array = new_bytearray(new_size)))
		return NULL;
	memcpy(new_array->buffer, array1->buffer, array1->size);
	memcpy(new_array->buffer + array1->size, array2->buffer, array2->size);
	return new_array;
}

bytearray_t*
slice_bytearray(bytearray_t* array, int start, int length)
{
	bytearray_t* new_array;

	if (!(new_array = new_bytearray(length)))
		return NULL;
	memcpy(new_array->buffer, array->buffer + start, length);
	return new_array;
}

void
init_bytearray_api(void)
{
	register_api_func(g_duktape, NULL, "CreateByteArray", js_CreateByteArray);
	register_api_func(g_duktape, NULL, "CreateByteArrayFromString", js_CreateByteArrayFromString);
	register_api_func(g_duktape, NULL, "CreateStringFromByteArray", js_CreateStringFromByteArray);
	register_api_func(g_duktape, NULL, "HashByteArray", js_HashByteArray);
}

void
duk_push_sphere_bytearray(duk_context* ctx, bytearray_t* array)
{
	duk_push_object(ctx);
	duk_push_string(ctx, "bytearray"); duk_put_prop_string(ctx, -2, "\xFF" "sphere_type");
	duk_push_pointer(ctx, array); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_c_function(ctx, js_ByteArray_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, js_ByteArray_toString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "toString");
	duk_push_c_function(ctx, js_ByteArray_concat, DUK_VARARGS); duk_put_prop_string(ctx, -2, "concat");
	duk_push_c_function(ctx, js_ByteArray_slice, DUK_VARARGS); duk_put_prop_string(ctx, -2, "slice");
	duk_push_string(ctx, "length"); duk_push_int(ctx, array->size);
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0
		| DUK_DEFPROP_HAVE_WRITABLE | 0
		| DUK_DEFPROP_HAVE_VALUE);

	// return proxy object so we can catch array accesses
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Proxy");
	duk_dup(ctx, -3);
	duk_push_object(ctx);
	duk_push_c_function(ctx, js_ByteArray_getProp, DUK_VARARGS); duk_put_prop_string(ctx, -2, "get");
	duk_push_c_function(ctx, js_ByteArray_setProp, DUK_VARARGS); duk_put_prop_string(ctx, -2, "set");
	duk_new(ctx, 2);
	duk_remove(ctx, -2);
	duk_remove(ctx, -2);
}

bytearray_t*
duk_require_sphere_bytearray(duk_context* ctx, duk_idx_t index)
{
	bytearray_t* array;
	const char*  type;

	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	if (!duk_get_prop_string(ctx, index, "\xFF" "sphere_type"))
		goto on_error;
	type = duk_get_string(ctx, -1); duk_pop(ctx);
	if (strcmp(type, "bytearray") != 0) goto on_error;
	duk_get_prop_string(ctx, index, "\xFF" "ptr"); array = duk_get_pointer(ctx, -1); duk_pop(ctx);
	return array;

on_error:
	duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "Not a Sphere byte array");
}

static duk_ret_t
js_CreateByteArray(duk_context* ctx)
{
	int size = duk_require_int(ctx, 0);
	
	bytearray_t* array;

	if (size < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "CreateByteArray(): Size cannot be negative (%i)", size);
	if (!(array = new_bytearray(size)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateByteArray(): Failed to create new byte array");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_CreateByteArrayFromString(duk_context* ctx)
{
	lstring_t* string = duk_require_lstring_t(ctx, 0);

	bytearray_t* array;
	
	if (string->length > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "CreateByteArrayFromString(): Input string too large, size of byte array cannot exceed 2 GB");
	if (!(array = bytearray_from_lstring(string)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateByteArrayFromString(): Failed to create byte array from string");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_CreateStringFromByteArray(duk_context* ctx)
{
	bytearray_t* array = duk_require_sphere_bytearray(ctx, 0);

	duk_push_lstring(ctx, (char*)array->buffer, array->size);
	return 1;
}

static duk_ret_t
js_HashByteArray(duk_context* ctx)
{
	// TODO: implement byte array hashing
	duk_error_ni(ctx, -1, DUK_ERR_ERROR, "HashByteArray(): Function is not yet implemented");
}

static duk_ret_t
js_ByteArray_finalize(duk_context* ctx)
{
	bytearray_t* array;
	
	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); array = duk_get_pointer(ctx, -1); duk_pop(ctx);
	free_bytearray(array);
	return 0;
}

static duk_ret_t
js_ByteArray_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object byte_array]");
	return 1;
}

static duk_ret_t
js_ByteArray_getProp(duk_context* ctx)
{
	bytearray_t* array;
	int          index;
	int          size;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); array = duk_get_pointer(ctx, -1); duk_pop(ctx);
	if (duk_is_number(ctx, 1)) {
		index = duk_to_int(ctx, 1);
		size = get_bytearray_size(array);
		if (index < 0 || index >= size)
			duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray[]: Index is out of bounds (%i - size: %i)", index, size);
		duk_push_uint(ctx, get_byte(array, index));
		return 1;
	}
	else {
		duk_dup(ctx, 1);
		duk_get_prop(ctx, 0);
		return 1;
	}
}

static duk_ret_t
js_ByteArray_setProp(duk_context* ctx)
{
	bytearray_t* array;
	int          index;
	int          size;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); array = duk_get_pointer(ctx, -1); duk_pop(ctx);
	if (duk_is_number(ctx, 1)) {
		index = duk_to_int(ctx, 1);
		size = get_bytearray_size(array);
		if (index < 0 || index >= size)
			duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray[]: Index is out of bounds (%i - size: %i)", index, size);
		set_byte(array, index, duk_require_uint(ctx, 2));
		return 0;
	}
	else {
		duk_dup(ctx, 1);
		duk_dup(ctx, 2);
		duk_put_prop(ctx, 0);
		return 0;
	}
}

static duk_ret_t
js_ByteArray_concat(duk_context* ctx)
{
	bytearray_t* array2 = duk_require_sphere_bytearray(ctx, 0);
	
	bytearray_t* array;
	bytearray_t* new_array;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); array = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (array->size + array2->size > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray:concat(): Unable to concatenate, final size would exceed 2 GB (size1: %u, size2: %u)", array->size, array2->size);
	if (!(new_array = concat_bytearrays(array, array2)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ByteArray:concat(): Failed to create concatenated byte array");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_ByteArray_slice(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int start = duk_require_int(ctx, 0);
	int end = (n_args >= 2) ? duk_require_int(ctx, 1) : INT_MAX;
	
	bytearray_t* array;
	int          end_norm;
	bytearray_t* new_array;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); array = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	end_norm = fmin(end >= 0 ? end : array->size + end, array->size);
	if (end_norm < start || end_norm > array->size)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray:slice(): Start and/or end values out of bounds (start: %i, end: %i - size: %i)", start, end_norm, array->size);
	if (!(new_array = slice_bytearray(array, start, end_norm - start)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ByteArray:slice(): Failed to create sliced byte array");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}
