#include "minisphere.h"
#include "api.h"

#include "bytearray.h"

static duk_ret_t js_CreateStringFromByteArray (duk_context* ctx);
static duk_ret_t js_HashByteArray             (duk_context* ctx);
static duk_ret_t js_CreateByteArray           (duk_context* ctx);
static duk_ret_t js_CreateByteArrayFromString (duk_context* ctx);
static duk_ret_t js_DeflateByteArray          (duk_context* ctx);
static duk_ret_t js_InflateByteArray          (duk_context* ctx);
static duk_ret_t js_new_ByteArray             (duk_context* ctx);
static duk_ret_t js_ByteArray_finalize        (duk_context* ctx);
static duk_ret_t js_ByteArray_get_length      (duk_context* ctx);
static duk_ret_t js_ByteArray_toString        (duk_context* ctx);
static duk_ret_t js_ByteArray_getProp         (duk_context* ctx);
static duk_ret_t js_ByteArray_setProp         (duk_context* ctx);
static duk_ret_t js_ByteArray_concat          (duk_context* ctx);
static duk_ret_t js_ByteArray_deflate         (duk_context* ctx);
static duk_ret_t js_ByteArray_inflate         (duk_context* ctx);
static duk_ret_t js_ByteArray_resize          (duk_context* ctx);
static duk_ret_t js_ByteArray_slice           (duk_context* ctx);

struct bytearray
{
	int          refcount;
	unsigned int id;
	uint8_t*     buffer;
	int          size;
};

static unsigned int s_next_array_id = 0;

bytearray_t*
new_bytearray(int size)
{
	bytearray_t* array;
	
	console_log(3, "Creating new ByteArray %u size %i bytes",
		size, s_next_array_id);
	
	array = calloc(1, sizeof(bytearray_t));
	if (!(array->buffer = calloc(size, 1)))
		goto on_error;
	array->size = size;
	array->id = s_next_array_id++;
	
	return ref_bytearray(array);
	
on_error:
	free(array);
	return NULL;
}

bytearray_t*
bytearray_from_buffer(const void* buffer, int size)
{
	bytearray_t* array;

	console_log(3, "Creating bytearray from %i-byte buffer", size);
	
	if (!(array = new_bytearray(size)))
		return NULL;
	memcpy(array->buffer, buffer, size);
	
	return array;
}

bytearray_t*
bytearray_from_lstring(const lstring_t* string)
{
	bytearray_t* array;

	console_log(3, "Creating bytearray from %u-byte string", lstr_len(string));
	
	if (lstr_len(string) <= 65)  // log short strings only
		console_log(4, "  String: \"%s\"", lstr_cstr(string));
	if (lstr_len(string) > INT_MAX) return NULL;
	if (!(array = new_bytearray((int)lstr_len(string))))
		return NULL;
	memcpy(array->buffer, lstr_cstr(string), lstr_len(string));
	
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
	
	console_log(3, "Disposing Bytearray %u no longer in use", array->id);
	free(array->buffer);
	free(array);
}

uint8_t
get_byte(bytearray_t* array, int index)
{
	return array->buffer[index];
}

uint8_t*
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

	console_log(3, "Concatenating ByteArrays %u and %u",
		s_next_array_id, array1->id, array2->id);

	new_size = array1->size + array2->size;
	if (!(new_array = new_bytearray(new_size)))
		return NULL;
	memcpy(new_array->buffer, array1->buffer, array1->size);
	memcpy(new_array->buffer + array1->size, array2->buffer, array2->size);
	return new_array;
}

bytearray_t*
deflate_bytearray(bytearray_t* array, int level)
{
	static const int CHUNK_SIZE = 65536;
	
	uint8_t*     buffer = NULL;
	int          flush_flag;
	int          result;
	bytearray_t* new_array;
	uint8_t*     new_buffer;
	int          n_chunks = 0;
	size_t       out_size;
	z_stream     z;

	console_log(3, "Deflating ByteArray %u from source ByteArray %u",
		s_next_array_id, array->id);
	
	memset(&z, 0, sizeof(z_stream));
	z.next_in = (Bytef*)array->buffer;
	z.avail_in = array->size;
	if (deflateInit(&z, level) != Z_OK)
		goto on_error;
	flush_flag = Z_NO_FLUSH;
	do {
		if (z.avail_out == 0) {
			if (!(new_buffer = realloc(buffer, ++n_chunks * CHUNK_SIZE)))  // resize buffer
				goto on_error;
			z.next_out = new_buffer + (n_chunks - 1) * CHUNK_SIZE;
			z.avail_out = CHUNK_SIZE;
			buffer = new_buffer;
		}
		result = deflate(&z, flush_flag);
		if (z.avail_out > 0)
			flush_flag = Z_FINISH;
	} while (result != Z_STREAM_END);
	if ((out_size = CHUNK_SIZE * n_chunks - z.avail_out) > INT_MAX)
		goto on_error;
	deflateEnd(&z);

	// create a byte array from the deflated data
	new_array = calloc(1, sizeof(bytearray_t));
	new_array->id = s_next_array_id++;
	new_array->buffer = buffer;
	new_array->size = (int)out_size;
	return ref_bytearray(new_array);

on_error:
	deflateEnd(&z);
	free(buffer);
	return NULL;
}

bytearray_t*
inflate_bytearray(bytearray_t* array, int max_size)
{
	uint8_t*     buffer = NULL;
	size_t       chunk_size;
	int          flush_flag;
	int          result;
	bytearray_t* new_array;
	uint8_t*     new_buffer;
	int          n_chunks = 0;
	size_t       out_size;
	z_stream     z;

	console_log(3, "Inflating ByteArray %u from source ByteArray %u",
		s_next_array_id, array->id);
	
	memset(&z, 0, sizeof(z_stream));
	z.next_in = (Bytef*)array->buffer;
	z.avail_in = array->size;
	if (inflateInit(&z) != Z_OK)
		goto on_error;
	flush_flag = Z_NO_FLUSH;
	chunk_size = max_size != 0 ? max_size : 65536;
	do {
		if (z.avail_out == 0) {
			if (buffer != NULL && max_size != 0)
				goto on_error;
			if (!(new_buffer = realloc(buffer, ++n_chunks * chunk_size)))  // resize buffer
				goto on_error;
			z.next_out = new_buffer + (n_chunks - 1) * chunk_size;
			z.avail_out = (uInt)chunk_size;
			buffer = new_buffer;
		}
		if ((result = inflate(&z, flush_flag)) == Z_DATA_ERROR)
			goto on_error;
		if (z.avail_out > 0)
			flush_flag = Z_FINISH;
	} while (result != Z_STREAM_END);
	if ((out_size = chunk_size * n_chunks - z.avail_out) > INT_MAX)
		goto on_error;
	inflateEnd(&z);

	// create a byte array from the deflated data
	new_array = calloc(1, sizeof(bytearray_t));
	new_array->id = s_next_array_id++;
	new_array->buffer = buffer;
	new_array->size = (int)out_size;
	return ref_bytearray(new_array);

on_error:
	inflateEnd(&z);
	free(buffer);
	return NULL;
}

bool
resize_bytearray(bytearray_t* array, int new_size)
{
	uint8_t* new_buffer;

	// resize buffer, filling any new slots with zero bytes
	if (!(new_buffer = realloc(array->buffer, new_size)))
		return false;
	if (new_size > array->size)
		memset(new_buffer + array->size, 0, new_size - array->size);
	
	array->buffer = new_buffer;
	array->size = new_size;
	return true;
}

bytearray_t*
slice_bytearray(bytearray_t* array, int start, int length)
{
	bytearray_t* new_array;

	console_log(3, "Copying %i-byte slice from ByteArray %u", length, array->id);
	
	if (!(new_array = new_bytearray(length)))
		return NULL;
	memcpy(new_array->buffer, array->buffer + start, length);
	return new_array;
}

void
init_bytearray_api(void)
{
	// core ByteArray API
	register_api_function(g_duk, NULL, "CreateStringFromByteArray", js_CreateStringFromByteArray);
	register_api_function(g_duk, NULL, "HashByteArray", js_HashByteArray);

	// ByteArray object
	register_api_function(g_duk, NULL, "CreateByteArray", js_CreateByteArray);
	register_api_function(g_duk, NULL, "CreateByteArrayFromString", js_CreateByteArrayFromString);
	register_api_ctor(g_duk, "ByteArray", js_new_ByteArray, js_ByteArray_finalize);
	register_api_prop(g_duk, "ByteArray", "length", js_ByteArray_get_length, NULL);
	register_api_prop(g_duk, "ByteArray", "size", js_ByteArray_get_length, NULL);
	register_api_function(g_duk, "ByteArray", "toString", js_ByteArray_toString);
	register_api_function(g_duk, "ByteArray", "concat", js_ByteArray_concat);
	register_api_function(g_duk, "ByteArray", "deflate", js_ByteArray_deflate);
	register_api_function(g_duk, "ByteArray", "inflate", js_ByteArray_inflate);
	register_api_function(g_duk, "ByteArray", "slice", js_ByteArray_slice);
}

void
duk_push_sphere_bytearray(duk_context* ctx, bytearray_t* array)
{
	duk_idx_t obj_index;
	
	duk_push_sphere_obj(ctx, "ByteArray", ref_bytearray(array));
	obj_index = duk_normalize_index(ctx, -1);

	// return proxy object so we can catch array accesses
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Proxy");
	duk_dup(ctx, obj_index);
	duk_push_object(ctx);
	duk_push_c_function(ctx, js_ByteArray_getProp, DUK_VARARGS); duk_put_prop_string(ctx, -2, "get");
	duk_push_c_function(ctx, js_ByteArray_setProp, DUK_VARARGS); duk_put_prop_string(ctx, -2, "set");
	duk_new(ctx, 2);
	duk_get_prototype(ctx, obj_index);
	duk_set_prototype(ctx, -2);
	duk_remove(ctx, -2);
	duk_remove(ctx, -2);
}

bytearray_t*
duk_require_sphere_bytearray(duk_context* ctx, duk_idx_t index)
{
	return duk_require_sphere_obj(ctx, index, "ByteArray");
}

static duk_ret_t
js_CreateStringFromByteArray(duk_context* ctx)
{
	bytearray_t* array = duk_require_sphere_bytearray(ctx, 0);

	duk_push_lstring(ctx, (char*)array->buffer, array->size);
	return 1;
}

static duk_ret_t
js_DeflateByteArray(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	bytearray_t* array = duk_require_sphere_bytearray(ctx, 0);
	int level = n_args >= 2 ? duk_require_int(ctx, 1) : -1;

	bytearray_t* new_array;

	if ((level < 0 || level > 9) && n_args >= 2)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "DeflateByteArray(): Compression level is out of range (%i)", level);
	if (!(new_array = deflate_bytearray(array, level)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "DeflateByteArray(): Failed to deflate source ByteArray");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_HashByteArray(duk_context* ctx)
{
	duk_require_sphere_bytearray(ctx, 0);
	
	// TODO: implement byte array hashing
	duk_error_ni(ctx, -1, DUK_ERR_ERROR, "HashByteArray(): Function is not yet implemented");
}

static duk_ret_t
js_InflateByteArray(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	bytearray_t* array = duk_require_sphere_bytearray(ctx, 0);
	int max_size = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	bytearray_t* new_array;

	if (max_size < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "InflateByteArray(): Buffer size cannot be negative");
	if (!(new_array = inflate_bytearray(array, max_size)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "InflateByteArray(): Failed to inflate source ByteArray");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_CreateByteArray(duk_context* ctx)
{
	duk_require_number(ctx, 0);

	js_new_ByteArray(ctx);
	return 1;
}

static duk_ret_t
js_CreateByteArrayFromString(duk_context* ctx)
{
	duk_require_string(ctx, 0);

	js_new_ByteArray(ctx);
	return 1;
}

static duk_ret_t
js_new_ByteArray(duk_context* ctx)
{
	bytearray_t* array;
	lstring_t*   string;
	int          size;

	if (duk_is_string(ctx, 0)) {
		string = duk_require_lstring_t(ctx, 0);
		if (lstr_len(string) > INT_MAX)
			duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray(): Input string is too long");
		if (!(array = bytearray_from_lstring(string)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ByteArray(): Failed to create byte array from string");
		lstr_free(string);
	}
	else {
		size = duk_require_int(ctx, 0);
		if (size < 0)
			duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray(): Size cannot be negative", size);
		if (!(array = new_bytearray(size)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ByteArray(): Failed to create new byte array");
	}
	duk_push_sphere_bytearray(ctx, array);
	free_bytearray(array);
	return 1;
}

static duk_ret_t
js_ByteArray_finalize(duk_context* ctx)
{
	bytearray_t* array;
	
	array = duk_require_sphere_bytearray(ctx, 0);
	free_bytearray(array);
	return 0;
}

static duk_ret_t
js_ByteArray_get_length(duk_context* ctx)
{
	bytearray_t* array;

	duk_push_this(ctx);
	array = duk_require_sphere_bytearray(ctx, -1);
	duk_pop(ctx);
	duk_push_int(ctx, get_bytearray_size(array));
	return 1;
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

	array = duk_require_sphere_bytearray(ctx, 0);
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

	array = duk_require_sphere_bytearray(ctx, 0);
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
	array = duk_require_sphere_bytearray(ctx, -1);
	duk_pop(ctx);
	if (array->size + array2->size > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray:concat(): Unable to concatenate, final size would exceed 2 GB (size1: %u, size2: %u)", array->size, array2->size);
	if (!(new_array = concat_bytearrays(array, array2)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ByteArray:concat(): Failed to create concatenated byte array");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_ByteArray_deflate(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int level = n_args >= 1 ? duk_require_int(ctx, 0) : -1;
	
	bytearray_t* array;
	bytearray_t* new_array;

	duk_push_this(ctx);
	array = duk_require_sphere_bytearray(ctx, -1);
	duk_pop(ctx);
	if ((level < 0 || level > 9) && n_args >= 1)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray:deflate(): Compression level is out of range (%i)", level);
	if (!(new_array = deflate_bytearray(array, level)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ByteArray:deflate(): Failed to deflate source ByteArray");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_ByteArray_inflate(duk_context* ctx)
{
	bytearray_t* array;
	bytearray_t* new_array;

	int n_args = duk_get_top(ctx);
	int max_size = n_args >= 1 ? duk_require_int(ctx, 0) : 0;
	
	duk_push_this(ctx);
	array = duk_require_sphere_bytearray(ctx, -1);
	duk_pop(ctx);
	if (max_size < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray:inflate(): Buffer size cannot be negative");
	if (!(new_array = inflate_bytearray(array, max_size)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ByteArray:inflate(): Failed to inflate source ByteArray");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_ByteArray_resize(duk_context* ctx)
{
	int new_size = duk_require_int(ctx, 0);

	bytearray_t* array;

	duk_push_this(ctx);
	array = duk_require_sphere_bytearray(ctx, -1);
	duk_pop(ctx);
	if (new_size < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray:resize(): Size cannot be negative");
	resize_bytearray(array, new_size);
	return 0;
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
	array = duk_require_sphere_bytearray(ctx, -1);
	duk_pop(ctx);
	end_norm = fmin(end >= 0 ? end : array->size + end, array->size);
	if (end_norm < start || end_norm > array->size)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ByteArray:slice(): Start and/or end values out of bounds (start: %i, end: %i - size: %i)", start, end_norm, array->size);
	if (!(new_array = slice_bytearray(array, start, end_norm - start)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ByteArray:slice(): Failed to create sliced byte array");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}
