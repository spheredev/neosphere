#ifndef MINISPHERE__API_H__INCLUDED
#define MINISPHERE__API_H__INCLUDED

#include <stdbool.h>

#include "jsal.h"

void   api_init               (void);
void   api_define_const       (const char* enum_name, const char* name, double value);
void   api_define_class       (const char* name, js_callback_t constructor, js_callback_t finalizer);
void   api_define_function    (const char* namespace_name, const char* name, js_callback_t fn);
void   api_define_method      (const char* class_name, const char* name, js_callback_t fn);
void   api_define_object      (const char* namespace_name, const char* name, const char* class_name, void* udata);
void   api_define_property    (const char* class_name, const char* name, js_callback_t getter, js_callback_t setter);
void   api_define_static_prop (const char* namespace_name, const char* name, js_callback_t getter, js_callback_t setter);

void  jsal_error_blame          (int blame_offset, js_error_type_t type, const char* format, ...);
bool  jsal_is_class_obj         (int index, const char* class_name);
int   jsal_push_class_obj       (const char* class_name, void* ptr);
int   jsal_push_class_prototype (const char* class_name);
void* jsal_require_class_obj    (int index, const char* class_name);
void  jsal_set_class_ptr        (int index, void* ptr);

#endif // MINISPHERE__API_H__INCLUDED
