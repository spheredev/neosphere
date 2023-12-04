/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "neosphere.h"
#include "byte_array.h"

#include "compress.h"

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

	if (!(array = calloc(1, sizeof(bytearray_t))))
		goto on_error;
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
	if (lstr_len(string) > INT_MAX)
		return NULL;
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
	uint8_t*     deflation;
	bytearray_t* new_array;
	size_t       output_size;

	console_log(3, "deflating byte array #%u from source byte array #%u",
		s_next_array_id, array->id);
	deflation = z_deflate(array->buffer, array->size, level, &output_size);
	if (deflation == NULL || output_size > INT_MAX)
		goto on_error;

	if (!(new_array = calloc(1, sizeof(bytearray_t))))
		goto on_error;
	new_array->id = s_next_array_id++;
	new_array->buffer = deflation;
	new_array->size = (int)output_size;
	return bytearray_ref(new_array);

on_error:
	free(deflation);
	return NULL;
}

bytearray_t*
bytearray_inflate(bytearray_t* array, int max_size)
{
	uint8_t*     inflation;
	bytearray_t* new_array;
	size_t       output_size;

	console_log(3, "inflating byte array #%u from source byte array #%u",
		s_next_array_id, array->id);
	inflation = z_inflate(array->buffer, array->size, max_size, &output_size);
	if (inflation == NULL || output_size > INT_MAX)
		goto on_error;

	if (!(new_array = calloc(1, sizeof(bytearray_t))))
		goto on_error;
	new_array->id = s_next_array_id++;
	new_array->buffer = inflation;
	new_array->size = (int)output_size;
	return bytearray_ref(new_array);

on_error:
	free(inflation);
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
