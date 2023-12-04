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

#include "compress.h"

#include <stdlib.h>
#include <string.h>
#include <zlib.h>

void*
z_deflate(const void* data, size_t size, int level, size_t *out_output_size)
{
	static const uInt CHUNK_SIZE = 65536;

	Bytef*       buffer = NULL;
	int          flush_flag = Z_NO_FLUSH;
	Bytef*       new_buffer;
	int          num_chunks = 0;
	size_t       out_size;
	int          result;
	z_stream     stream;

	memset(&stream, 0, sizeof(z_stream));
	stream.next_in = (Bytef*)data;
	stream.avail_in = (uInt)size;
	if (deflateInit(&stream, level) != Z_OK)
		goto on_error;
	do {
		if (stream.avail_out == 0) {
			if (!(new_buffer = realloc(buffer, ++num_chunks * CHUNK_SIZE)))
				goto on_error;
			stream.next_out = new_buffer + (num_chunks - 1) * CHUNK_SIZE;
			stream.avail_out = CHUNK_SIZE;
			buffer = new_buffer;
		}
		result = deflate(&stream, flush_flag);
		if (stream.avail_out > 0)
			flush_flag = Z_FINISH;
	} while (result != Z_STREAM_END);
	out_size = num_chunks * CHUNK_SIZE - stream.avail_out;
	deflateEnd(&stream);

	*out_output_size = out_size;
	return buffer;

on_error:
	deflateEnd(&stream);
	free(buffer);
	return NULL;
}

void*
z_inflate(const void* data, size_t size, size_t max_inflate, size_t *out_output_size)
{
	Bytef*   buffer = NULL;
	uInt     chunk_size;
	int      flush_flag = Z_NO_FLUSH;
	size_t   inflated_size;
	Bytef*   new_buffer;
	size_t   num_chunks;
	int      result;
	z_stream stream;

	memset(&stream, 0, sizeof(z_stream));
	stream.next_in = (Bytef*)data;
	stream.avail_in = (uInt)size;
	if (inflateInit(&stream) != Z_OK)
		goto on_error;
	chunk_size = max_inflate != 0 ? (uInt)max_inflate : 65536;
	num_chunks = 1;
	if (!(buffer = malloc(chunk_size + 1)))
		goto on_error;
	stream.next_out = buffer;
	stream.avail_out = chunk_size;
	do {
		if (stream.avail_out == 0) {
			if (max_inflate > 0)
				goto on_error;  // inflated data exceeds maximum size
			if (!(new_buffer = realloc(buffer, ++num_chunks * chunk_size + 1)))
				goto on_error;
			stream.next_out = new_buffer + (num_chunks - 1) * chunk_size;
			stream.avail_out = chunk_size;
			buffer = new_buffer;
		}
		if ((result = inflate(&stream, flush_flag)) == Z_DATA_ERROR)
			goto on_error;
		if (stream.avail_out > 0)
			flush_flag = Z_FINISH;
	} while (result != Z_STREAM_END);
	inflated_size = num_chunks * chunk_size - stream.avail_out;
	buffer[inflated_size] = '\0';  // handy NUL terminator
	inflateEnd(&stream);

	*out_output_size = inflated_size;
	return buffer;

on_error:
	inflateEnd(&stream);
	free(buffer);
	return NULL;
}
