#ifndef MINISPHERE__VECTOR_H__INCLUDED
#define MINISPHERE__VECTOR_H__INCLUDED

#include <stddef.h>
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

vector_t* vector_new    (size_t pitch);
vector_t* vector_dup    (const vector_t* vector);
void      vector_free   (vector_t* vector);
void      vector_clear  (vector_t* vector);
size_t    vector_len    (const vector_t* vector);
bool      vector_push   (vector_t* vector, const void* in_object);
void      vector_remove (vector_t* vector, size_t index);
vector_t* vector_sort   (vector_t* vector, int(*comparer)(const void* in_a, const void* in_b));

void*  vector_get  (const vector_t* vector, size_t index);
void   vector_set  (vector_t* vector, size_t index, const void* in_object);
iter_t vector_enum (const vector_t* vector);
void*  vector_next (iter_t* inout_iter);

#endif // MINISPHERE__VECTOR_H__INCLUDED
