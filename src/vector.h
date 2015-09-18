#ifndef MINISPHERE__VECTOR_H__INCLUDED
#define MINISPHERE__VECTOR_H__INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

typedef struct vector vector_t;

typedef
struct iter
{
	const vector_t* vector;
	void*           ptr;
	size_t          index;
} iter_t;

vector_t* new_vector(size_t pitch);
void      free_vector        (vector_t* vector);
void*     get_vector_item    (const vector_t* vector, size_t index);
void      set_vector_item    (vector_t* vector, size_t index, const void* in_object);
size_t    get_vector_size    (const vector_t* vector);
void      clear_vector       (vector_t* vector);
bool      push_back_vector   (vector_t* vector, const void* in_object);
void      remove_vector_item (vector_t* vector, size_t index);

iter_t iterate_vector   (const vector_t* vector);
void*  next_vector_item (iter_t* inout_iter);

#endif // MINISPHERE__VECTOR_H__INCLUDED
