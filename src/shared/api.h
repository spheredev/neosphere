#ifndef MINISPHERE__API_H__INCLUDED
#define MINISPHERE__API_H__INCLUDED

#include "duktape.h"

void   api_init               (duk_context* ctx);
void   api_define_const       (duk_context* ctx, const char* enum_name, const char* name, double value);
void   api_define_class       (duk_context* ctx, const char* name, duk_c_function constructor, duk_c_function finalizer);
void   api_define_function    (duk_context* ctx, const char* namespace_name, const char* name, duk_c_function fn);
void   api_define_method      (duk_context* ctx, const char* class_name, const char* name, duk_c_function fn);
void   api_define_object      (duk_context* ctx, const char* namespace_name, const char* name, const char* class_name, void* udata);
void   api_define_property    (duk_context* ctx, const char* class_name, const char* name, duk_c_function getter, duk_c_function setter);
void   api_define_static_prop (duk_context* ctx, const char* namespace_name, const char* name, duk_c_function getter, duk_c_function setter);

void       duk_error_blamed      (duk_context* ctx, int blame_offset, duk_errcode_t err_code, const char* fmt, ...);
duk_bool_t duk_is_class_obj      (duk_context* ctx, duk_idx_t index, const char* class_name);
void       duk_push_class_obj    (duk_context* ctx, const char* class_name, void* udata);
void*      duk_require_class_obj (duk_context* ctx, duk_idx_t index, const char* class_name);
void       duk_to_class_obj      (duk_context* ctx, duk_idx_t idx, const char* class_name, void* udata);

#endif // MINISPHERE__API_H__INCLUDED
