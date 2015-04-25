typedef enum js_error js_error_t;

extern void initialize_api        (duk_context* ctx);
extern void register_api_const    (duk_context* ctx, const char* name, double value);
extern void register_api_ctor     (duk_context* ctx, const char* name, duk_c_function fn, duk_c_function finalizer);
extern void register_api_function (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);
extern void register_api_prop     (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function getter, duk_c_function setter);

extern duk_bool_t duk_is_sphere_obj      (duk_context* ctx, duk_idx_t index, const char* ctor_name);
extern noreturn   duk_error_ni           (duk_context* ctx, int blame_offset, duk_errcode_t err_code, const char* fmt, ...);
extern void       duk_push_sphere_obj    (duk_context* ctx, const char* ctor_name, void* udata);
extern void*      duk_require_sphere_obj (duk_context* ctx, duk_idx_t index, const char* ctor_name);
