#ifndef CELL__UTILITY_H__INCLUDED
#define CELL__UTILITY_H__INCLUDED

#include <stdbool.h>
#include <stddef.h>

void  duk_ref_heapptr   (duk_context* ctx, void* heapptr);
void  duk_unref_heapptr (duk_context* ctx, void* heapptr);
void* fslurp            (const char* filename, size_t *out_size);
bool  fspew             (const void* buffer, size_t size, const char* filename);
bool  wildcmp           (const char* filename, const char* pattern);

#endif
