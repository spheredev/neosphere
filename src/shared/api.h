#ifndef SPHERE__API_H__INCLUDED
#define SPHERE__API_H__INCLUDED

#include <stdbool.h>

#include "jsal.h"

void   api_init               (void);
void   api_define_const       (const char* enum_name, const char* name, double value);
void   api_define_class       (const char* name, int class_id, js_function_t constructor, js_finalizer_t finalizer);
void   api_define_function    (const char* namespace_name, const char* name, js_function_t fn);
void   api_define_method      (const char* class_name, const char* name, js_function_t fn);
void   api_define_object      (const char* namespace_name, const char* name, int class_id, void* udata);
void   api_define_property    (const char* class_name, const char* name, bool enumerable, js_function_t getter, js_function_t setter);
void   api_define_static_prop (const char* namespace_name, const char* name, js_function_t getter, js_function_t setter);

bool  jsal_is_class_obj         (int index, int class_id);
int   jsal_push_class_name      (int class_id);
int   jsal_push_class_obj       (int class_id, void* ptr, bool in_ctor);
int   jsal_push_class_prototype (int class_id);
void* jsal_require_class_obj    (int index, int class_id);
void  jsal_set_class_ptr        (int index, void* ptr);

#endif // SPHERE__API_H__INCLUDED
