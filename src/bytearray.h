typedef struct bytearray bytearray_t;

extern bytearray_t*   new_bytearray          (int size);
extern bytearray_t*   bytearray_from_buffer  (const void* buffer, int size);
extern bytearray_t*   bytearray_from_lstring (const lstring_t* string);
extern bytearray_t*   ref_bytearray          (bytearray_t* array);
extern void           free_bytearray         (bytearray_t* array);
extern uint8_t        get_byte               (bytearray_t* array, int index);
extern const uint8_t* get_bytearray_buffer   (bytearray_t* array);
extern int            get_bytearray_size     (bytearray_t* array);
extern void           set_byte               (bytearray_t* array, int index, uint8_t value);
extern bytearray_t*   concat_bytearrays      (bytearray_t* array1, bytearray_t* array2);
extern bytearray_t*   slice_bytearray        (bytearray_t* array, int start, int length);

extern void         init_bytearray_api           (void);
extern void         duk_push_sphere_bytearray    (duk_context* ctx, bytearray_t* array);
extern bytearray_t* duk_require_sphere_bytearray (duk_context* ctx, duk_idx_t index);
