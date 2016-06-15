#include "minisphere.h"
#include "bytearray.h"

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
	
	console_log(3, "creating new bytearray #%u size %i bytes",
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

	console_log(3, "creating bytearray from %i-byte buffer", size);
	
	if (!(array = new_bytearray(size)))
		return NULL;
	memcpy(array->buffer, buffer, size);
	
	return array;
}

bytearray_t*
bytearray_from_lstring(const lstring_t* string)
{
	bytearray_t* array;

	console_log(3, "creating bytearray from %u-byte string", lstr_len(string));
	
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
	
	console_log(3, "disposing bytearray #%u no longer in use", array->id);
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

	console_log(3, "concatenating ByteArrays %u and %u",
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

	console_log(3, "deflating bytearray #%u from source bytearray #%u",
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

	console_log(3, "inflating bytearray #%u from source bytearray #%u",
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

	console_log(3, "copying %i-byte slice from bytearray #%u", length, array->id);
	
	if (!(new_array = new_bytearray(length)))
		return NULL;
	memcpy(new_array->buffer, array->buffer + start, length);
	return new_array;
}
