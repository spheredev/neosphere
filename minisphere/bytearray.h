extern void init_bytearray_api (void);

extern void     duk_push_sphere_bytearray    (duk_context* ctx, uint8_t* buffer, int size);
extern uint8_t* duk_require_sphere_bytearray (duk_context* ctx, duk_idx_t index, int* out_size);
