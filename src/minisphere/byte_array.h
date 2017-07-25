#ifndef MINISPHERE__BYTE_ARRAY_H__INCLUDED
#define MINISPHERE__BYTE_ARRAY_H__INCLUDED

typedef struct bytearray bytearray_t;

bytearray_t*   bytearray_new          (int size);
bytearray_t*   bytearray_from_buffer  (const void* buffer, int size);
bytearray_t*   bytearray_from_lstring (const lstring_t* string);
bytearray_t*   bytearray_ref          (bytearray_t* array);
void           bytearray_free         (bytearray_t* array);
uint8_t*       bytearray_buffer       (bytearray_t* array);
int            bytearray_len          (bytearray_t* array);
bytearray_t*   bytearray_concat       (bytearray_t* array1, bytearray_t* array2);
bytearray_t*   bytearray_deflate      (bytearray_t* array, int level);
uint8_t        bytearray_get          (bytearray_t* array, int index);
bytearray_t*   bytearray_inflate      (bytearray_t* array, int max_size);
void           bytearray_set          (bytearray_t* array, int index, uint8_t value);
bytearray_t*   bytearray_slice        (bytearray_t* array, int start, int length);

#endif // MINISPHERE__BYTE_ARRAY_H__INCLUDED
