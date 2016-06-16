#ifndef MINISPHERE__API_H__INCLUDED
#define MINISPHERE__API_H__INCLUDED

void   initialize_api           (duk_context* ctx);
void   shutdown_api             (void);
void   api_register_const       (duk_context* ctx, const char* enum_name, const char* name, double value);
void   api_register_ctor        (duk_context* ctx, const char* name, duk_c_function fn, duk_c_function finalizer);
void   api_register_method      (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);
void   api_register_prop        (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function getter, duk_c_function setter);
void   api_register_static_func (duk_context* ctx, const char* namespace_name, const char* name, duk_c_function fn);
void   api_register_static_obj  (duk_context* ctx, const char* namespace_name, const char* name, const char* ctor_name, void* udata);
void   api_register_static_prop (duk_context* ctx, const char* namespace_name, const char* name, duk_c_function getter, duk_c_function setter);
void   api_register_type        (duk_context* ctx, const char* name, duk_c_function finalizer);

noreturn   duk_error_ni           (duk_context* ctx, int blame_offset, duk_errcode_t err_code, const char* fmt, ...);
duk_bool_t duk_is_sphere_obj      (duk_context* ctx, duk_idx_t index, const char* ctor_name);
void       duk_push_sphere_obj    (duk_context* ctx, const char* ctor_name, void* udata);
void*      duk_require_sphere_obj (duk_context* ctx, duk_idx_t index, const char* ctor_name);

#endif // MINISPHERE__API_H__INCLUDED
