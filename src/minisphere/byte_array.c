#include "minisphere.h"
#include "byte_array.h"

struct bytearray
{
	int          refcount;
	unsigned int id;
	uint8_t*     buffer;
	int          size;
};

static unsigned int s_next_array_id = 0;

bytearray_t*
bytearray_new(int size)
{
	bytearray_t* array;

	console_log(3, "creating new byte array #%u size %d bytes",
		size, s_next_array_id);

	array = calloc(1, sizeof(bytearray_t));
	if (!(array->buffer = calloc(size, 1)))
		goto on_error;
	array->size = size;
	array->id = s_next_array_id++;

	return bytearray_ref(array);

on_error:
	free(array);
	return NULL;
}

bytearray_t*
bytearray_from_buffer(const void* buffer, int size)
{
	bytearray_t* array;

	console_log(3, "creating byte array from %d-byte buffer", size);

	if (!(array = bytearray_new(size)))
		return NULL;
	memcpy(array->buffer, buffer, size);

	return array;
}

bytearray_t*
bytearray_from_lstring(const lstring_t* string)
{
	bytearray_t* array;

	console_log(3, "creating byte array from %u-byte string", lstr_len(string));

	if (lstr_len(string) <= 65)  // log short strings only
		console_log(4, "  String: \"%s\"", lstr_cstr(string));
	if (lstr_len(string) > INT_MAX) return NULL;
	if (!(array = bytearray_new((int)lstr_len(string))))
		return NULL;
	memcpy(array->buffer, lstr_cstr(string), lstr_len(string));

	return array;
}

bytearray_t*
bytearray_ref(bytearray_t* array)
{
	++array->refcount;
	return array;
}

void
bytearray_unref(bytearray_t* array)
{
	if (array == NULL || --array->refcount > 0)
		return;

	console_log(3, "disposing byte array #%u no longer in use", array->id);
	free(array->buffer);
	free(array);
}

uint8_t*
bytearray_buffer(bytearray_t* array)
{
	return array->buffer;
}


int
bytearray_len(bytearray_t* array)
{
	return array->size;
}

uint8_t
bytearray_get(bytearray_t* array, int index)
{
	return array->buffer[index];
}

void
bytearray_set(bytearray_t* array, int index, uint8_t value)
{
	array->buffer[index] = value;
}

bytearray_t*
bytearray_concat(bytearray_t* array1, bytearray_t* array2)
{
	bytearray_t* new_array;
	int          new_size;

	console_log(3, "concatenating ByteArrays %u and %u",
		s_next_array_id, array1->id, array2->id);

	new_size = array1->size + array2->size;
	if (!(new_array = bytearray_new(new_size)))
		return NULL;
	memcpy(new_array->buffer, array1->buffer, array1->size);
	memcpy(new_array->buffer + array1->size, array2->buffer, array2->size);
	return new_array;
}

bytearray_t*
bytearray_deflate(bytearray_t* array, int level)
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

	console_log(3, "deflating byte array #%u from source byte array #%u",
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
	return bytearray_ref(new_array);

on_error:
	deflateEnd(&z);
	free(buffer);
	return NULL;
}

bytearray_t*
bytearray_inflate(bytearray_t* array, int max_size)
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

	console_log(3, "inflating byte array #%u from source byte array #%u",
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
	return bytearray_ref(new_array);

on_error:
	inflateEnd(&z);
	free(buffer);
	return NULL;
}

bytearray_t*
bytearray_slice(bytearray_t* array, int start, int length)
{
	bytearray_t* new_array;

	console_log(3, "copying %d-byte slice from byte array #%u", length, array->id);

	if (!(new_array = bytearray_new(length)))
		return NULL;
	memcpy(new_array->buffer, array->buffer + start, length);
	return new_array;
}
