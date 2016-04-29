#ifndef MINISPHERE__BYTEARRAY_H__INCLUDED
#define MINISPHERE__BYTEARRAY_H__INCLUDED

typedef struct bytearray bytearray_t;

bytearray_t*   new_bytearray          (int size);
bytearray_t*   bytearray_from_buffer  (const void* buffer, int size);
bytearray_t*   bytearray_from_lstring (const lstring_t* string);
bytearray_t*   ref_bytearray          (bytearray_t* array);
void           free_bytearray         (bytearray_t* array);
uint8_t        get_byte               (bytearray_t* array, int index);
uint8_t*       get_bytearray_buffer   (bytearray_t* array);
int            get_bytearray_size     (bytearray_t* array);
void           set_byte               (bytearray_t* array, int index, uint8_t value);
bytearray_t*   concat_bytearrays      (bytearray_t* array1, bytearray_t* array2);
bytearray_t*   deflate_bytearray      (bytearray_t* array, int level);
bytearray_t*   inflate_bytearray      (bytearray_t* array, int max_size);
bool           resize_bytearray       (bytearray_t* array, int new_size);
bytearray_t*   slice_bytearray        (bytearray_t* array, int start, int length);

void         init_bytearray_api           (void);
void         duk_push_sphere_bytearray    (duk_context* ctx, bytearray_t* array);
bytearray_t* duk_require_sphere_bytearray (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__BYTEARRAY_H__INCLUDED
