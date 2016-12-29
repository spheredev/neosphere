#ifndef CELL__UTILITY_H__INCLUDED
#define CELL__UTILITY_H__INCLUDED

#include <stdbool.h>
#include <stddef.h>

void* duk_ref_heapptr   (duk_context* ctx, duk_idx_t idx);
void  duk_unref_heapptr (duk_context* ctx, void* heapptr);
bool  fexist            (const char* filename);
void* fslurp            (const char* filename, size_t *out_size);
bool  fspew             (const void* buffer, size_t size, const char* filename);
bool  wildcmp           (const char* filename, const char* pattern);

#endif
