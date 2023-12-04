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

// WARNING: be careful when using this!
// to cut down on the number of casts needed when using this generic vector implementation,
// void pointers are thrown around with abandon.  as a result, it is not type safe at all
// and WILL blow up in your face if you're careless with it.  you have been warned!

#include "vector.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool ensure_space (vector_t* vector, int min_items, bool compacting);

struct vector
{
	uint8_t* buffer;
	int      max_items;
	int      num_items;
	size_t   pitch;
	int      reserve;
};

vector_t*
vector_new(size_t pitch)
{
	vector_t* vector;

	if (!(vector = calloc(1, sizeof(vector_t))))
		return NULL;
	vector->pitch = pitch;
	vector_reserve(vector, 8);
	return vector;
}

vector_t*
vector_dup(const vector_t* it)
{
	vector_t* copy;

	copy = vector_new(it->pitch);
	ensure_space(copy, it->num_items, false);
	memcpy(copy->buffer, it->buffer, it->pitch * it->num_items);
	copy->num_items = it->num_items;
	return copy;
}

void
vector_free(vector_t* it)
{
	if (it == NULL)
		return;
	free(it->buffer);
	free(it);
}

int
vector_len(const vector_t* it)
{
	return it->num_items;
}

void
vector_clear(vector_t* it)
{
	it->num_items = 0;
	ensure_space(it, 8, true);
}

iter_t
vector_enum(vector_t* it)
{
	iter_t iter;

	iter.vector = it;
	iter.ptr = NULL;
	iter.index = -1;
	return iter;
}

void*
vector_get(const vector_t* it, int index)
{
	return it->buffer + index * it->pitch;
}

bool
vector_insert(vector_t* it, int index, const void* in_object)
{
	if (!ensure_space(it, it->num_items + 1, false))
		return false;
	memmove(it->buffer + (index + 1) * it->pitch,
		it->buffer + index * it->pitch,
		(it->num_items - index) * it->pitch);
	memcpy(it->buffer + index * it->pitch, in_object, it->pitch);
	++it->num_items;
	return true;
}

bool
vector_pop(vector_t* it, int num_items)
{
	return vector_resize(it, it->num_items - num_items);
}

bool
vector_push(vector_t* it, const void* in_object)
{
	if (!ensure_space(it, it->num_items + 1, false))
		return false;
	memcpy(it->buffer + it->num_items * it->pitch, in_object, it->pitch);
	++it->num_items;
	return true;
}

void
vector_put(vector_t* it, int index, const void* in_object)
{
	uint8_t* p_item;

	p_item = it->buffer + index * it->pitch;
	memcpy(p_item, in_object, it->pitch);
}

void
vector_remove(vector_t* it, int index)
{
	size_t   move_size;
	uint8_t* p_item;

	--it->num_items;
	move_size = (it->num_items - index) * it->pitch;
	p_item = it->buffer + index * it->pitch;
	memmove(p_item, p_item + it->pitch, move_size);
	ensure_space(it, it->num_items, true);
}

bool
vector_reserve(vector_t* it, int min_items)
{
	int new_max;

	new_max = min_items;
	while (it->max_items > new_max)
		new_max *= 2;
	it->reserve = min_items;
	if (!ensure_space(it, new_max, true))
		return false;
	return true;
}

bool
vector_resize(vector_t* it, int new_size)
{
	if (!ensure_space(it, new_size, true))
		return false;
	it->num_items = new_size;
	return true;
}

vector_t*
vector_sort(vector_t* it, int (*comparer)(const void* in_a, const void* in_b))
{
	qsort(it->buffer, it->num_items, it->pitch, comparer);
	return it;
}

void*
iter_next(iter_t* iter)
{
	const vector_t* vector;
	void*           p_tail;

	vector = iter->vector;
	iter->ptr = iter->ptr != NULL ? (uint8_t*)iter->ptr + vector->pitch
		: vector->buffer;
	++iter->index;
	p_tail = vector->buffer + vector->num_items * vector->pitch;
	return (iter->ptr < p_tail) ? iter->ptr : NULL;
}

void
iter_remove(iter_t* iter)
{
	vector_t* vector;

	vector = iter->vector;
	vector_remove(vector, iter->index);
	--iter->index;
	iter->ptr = iter->index >= 0
		? vector->buffer + iter->index * vector->pitch
		: NULL;
}

static bool
ensure_space(vector_t* vector, int min_items, bool compacting)
{
	uint8_t* new_buffer;
	int      new_max;

	new_max = vector->max_items;
	if (new_max < vector->reserve)
		new_max = vector->reserve;
	while (new_max < min_items)  // is the buffer too small?
		new_max *= 2;

	// if the vector drops below 1/4 load, shrink the buffer so it doesn't
	// grow unbounded
	if (compacting && min_items < vector->max_items / 4)
		new_max = min_items * 2;
	if (new_max < vector->reserve)
		new_max = vector->reserve;

	if (new_max != vector->max_items) {
		if (!(new_buffer = realloc(vector->buffer, new_max * vector->pitch)) && new_max > 0)
			return false;
		vector->buffer = new_buffer;
		vector->max_items = new_max;
	}
	return true;
}
