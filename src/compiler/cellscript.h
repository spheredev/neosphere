#ifndef CELL__CELLSCRIPT_H__INCLUDED
#define CELL__CELLSCRIPT_H__INCLUDED

void api_init               (duk_context* ctx);
void api_define_function    (duk_context* ctx, const char* namespace_name, const char* name, duk_c_function fn);
void api_define_static_prop (duk_context* ctx, const char* namespace_name, const char* name, duk_c_function getter, duk_c_function setter);

#endif // CELL__CELLSCRIPT_H__INCLUDED
