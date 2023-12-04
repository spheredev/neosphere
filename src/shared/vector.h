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

#ifndef SPHERE__VECTOR_H__INCLUDED
#define SPHERE__VECTOR_H__INCLUDED

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

#endif // SPHERE__VECTOR_H__INCLUDED
