#ifndef FATCERBERUS__VECTOR_H__INCLUDED
#define FATCERBERUS__VECTOR_H__INCLUDED

#include <stddef.h>
#include <stdbool.h>
#include <memory.h>

typedef struct vector vector_t;

typedef
struct iter
{
	vector_t* vector;
	void*     ptr;
	int       index;
} iter_t;

vector_t* vector_new     (size_t pitch);
vector_t* vector_dup     (const vector_t* vector);
void      vector_free    (vector_t* vector);
int       vector_len     (const vector_t* vector);
void      vector_clear   (vector_t* vector);
iter_t    vector_enum    (vector_t* vector);
void*     vector_get     (const vector_t* vector, int index);
bool      vector_insert  (vector_t* vector, int index, const void* in_object);
bool      vector_pop     (vector_t* it, int num_items);
bool      vector_push    (vector_t* vector, const void* in_object);
void      vector_put     (vector_t* vector, int index, const void* in_object);
void      vector_remove  (vector_t* vector, int index);
bool      vector_reserve (vector_t* it, int min_items);
bool      vector_resize  (vector_t* vector, int new_size);
vector_t* vector_sort    (vector_t* vector, int(*comparer)(const void* in_a, const void* in_b));

void*     iter_next     (iter_t* inout_iter);
void      iter_remove   (iter_t* iter);

#endif // FATCERBERUS__VECTOR_H__INCLUDED
