// WARNING: be careful when using this!
// to cut down on the number of casts needed when using this generic vector implementation,
// void pointers are thrown around with abandon. as a result, it is not type safe at all
// and WILL blow up in your face if you're careless with it. you have been warned!

#include "vector.h"

static bool maybe_resize_vector(vector_t* vector, size_t min_items);

struct vector
{
	size_t        pitch;
	size_t        max_items;
	size_t        num_items;
	unsigned char *buffer;
};

vector_t*
new_vector(size_t pitch)
{
	vector_t* vector;

	if (!(vector = calloc(1, sizeof(vector_t))))
		return NULL;
	vector->pitch = pitch;
	return vector;
}

void
free_vector(vector_t* vector)
{
	if (vector == NULL)
		return;
	free(vector->buffer);
	free(vector);
}

void*
get_vector_item(const vector_t* vector, size_t index)
{
	return vector->buffer + index * vector->pitch;
}

void
set_vector_item(vector_t* vector, size_t index, const void* in_object)
{
	unsigned char* p;

	p = vector->buffer + index * vector->pitch;
	memcpy(p, in_object, vector->pitch);
}

size_t
get_vector_size(const vector_t* vector)
{
	return vector->num_items;
}

void
clear_vector(vector_t* vector)
{
	vector->num_items = 0;
	maybe_resize_vector(vector, 0);
}

bool
push_back_vector(vector_t* vector, const void* in_object)
{
	if (!maybe_resize_vector(vector, vector->num_items + 1))
		return false;
	memcpy(vector->buffer + vector->num_items * vector->pitch, in_object, vector->pitch);
	++vector->num_items;
	return true;
}

void
remove_vector_item(vector_t* vector, size_t index)
{
	size_t         move_size;
	unsigned char* p;

	--vector->num_items;
	move_size = (vector->num_items - index) * vector->pitch;
	p = vector->buffer + index * vector->pitch;
	memmove(p, p + vector->pitch, move_size);
	maybe_resize_vector(vector, vector->num_items);
}

iter_t
iterate_vector(const vector_t* vector)
{
	iter_t iter;

	iter.vector = vector;
	iter.ptr = vector->buffer;
	iter.index = 0;
	return iter;
}

void*
next_vector_item(iter_t* iter)
{
	void*           pcurrent;
	void*           ptail;
	const vector_t* vector;

	vector = iter->vector;
	ptail = vector->buffer + vector->num_items * vector->pitch;
	pcurrent = iter->ptr < ptail ? iter->ptr : NULL;
	if (pcurrent != NULL)
		iter->ptr = (char*)pcurrent + vector->pitch;
	else
		iter->ptr = NULL;
	if (pcurrent > (void*)vector->buffer)
		++iter->index;
	return pcurrent;
}

static bool
maybe_resize_vector(vector_t* vector, size_t min_items)
{
	unsigned char* new_buffer;
	size_t         new_max;
	
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
