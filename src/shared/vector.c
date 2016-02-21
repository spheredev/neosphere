// WARNING: be careful when using this!
// to cut down on the number of casts needed when using this generic vector implementation,
// void pointers are thrown around with abandon. as a result, it is not type safe at all
// and WILL blow up in your face if you're careless with it. you have been warned!

#include "vector.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool vector_resize (vector_t* vector, size_t min_items);

struct vector
{
	size_t   pitch;
	uint8_t* buffer;
	size_t   num_items;
	size_t   max_items;
};

vector_t*
vector_new(size_t pitch)
{
	vector_t* vector;

	if (!(vector = calloc(1, sizeof(vector_t))))
		return NULL;
	vector->pitch = pitch;
	vector_resize(vector, 8);
	return vector;
}

vector_t*
vector_dup(const vector_t* vector)
{
	vector_t* copy;

	copy = vector_new(vector->pitch);
	vector_resize(copy, vector->num_items);
	memcpy(copy->buffer, vector->buffer, vector->pitch * vector->num_items);
	return copy;
}

void
vector_free(vector_t* vector)
{
	if (vector == NULL)
		return;
	free(vector->buffer);
	free(vector);
}

void*
vector_get(const vector_t* vector, size_t index)
{
	return vector->buffer + index * vector->pitch;
}

void
vector_set(vector_t* vector, size_t index, const void* in_object)
{
	uint8_t* p_item;

	p_item = vector->buffer + index * vector->pitch;
	memcpy(p_item, in_object, vector->pitch);
}

size_t
vector_len(const vector_t* vector)
{
	return vector->num_items;
}

void
vector_clear(vector_t* vector)
{
	vector->num_items = 0;
	vector_resize(vector, 8);
}

bool
vector_push(vector_t* vector, const void* in_object)
{
	if (!vector_resize(vector, vector->num_items + 1))
		return false;
	memcpy(vector->buffer + vector->num_items * vector->pitch, in_object, vector->pitch);
	++vector->num_items;
	return true;
}

void
vector_remove(vector_t* vector, size_t index)
{
	size_t   move_size;
	uint8_t* p_item;

	--vector->num_items;
	move_size = (vector->num_items - index) * vector->pitch;
	p_item = vector->buffer + index * vector->pitch;
	memmove(p_item, p_item + vector->pitch, move_size);
	vector_resize(vector, vector->num_items);
}

vector_t*
vector_sort(vector_t* vector, int (*comparer)(const void* in_a, const void* in_b))
{
	qsort(vector->buffer, vector->num_items, vector->pitch, comparer);
	return vector;
}

iter_t
vector_enum(const vector_t* vector)
{
	iter_t iter;

	iter.vector = vector;
	iter.ptr = NULL;
	iter.index = -1;
	return iter;
}

void*
vector_next(iter_t* iter)
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

static bool
vector_resize(vector_t* vector, size_t min_items)
{
	uint8_t* new_buffer;
	size_t   new_max;
	
	new_max = vector->max_items;
	if (min_items > vector->max_items)  // is the buffer too small?
		new_max = min_items * 2;
	else if (min_items < vector->max_items / 4)  // if item count drops below 1/4 of peak size, shrink the buffer
		new_max = min_items * 2;
	if (new_max != vector->max_items) {
		if (!(new_buffer = realloc(vector->buffer, new_max * vector->pitch)) && new_max > 0)
			return false;
		vector->buffer = new_buffer;
		vector->max_items = new_max;
	}
	return true;
}
