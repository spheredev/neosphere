/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#ifndef SPHERE__API_H__INCLUDED
#define SPHERE__API_H__INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "jsal.h"

void   api_init                (bool node_compatible);
void   api_define_async_func   (const char* namespace_name, const char* name, js_function_t callback, intptr_t magic);
void   api_define_async_method (const char* class_name, const char* name, js_function_t callback, intptr_t magic);
void   api_define_const        (const char* enum_name, const char* name, double value);
void   api_define_class        (const char* name, int class_id, js_function_t constructor, js_finalizer_t finalizer, intptr_t magic);
void   api_define_func         (const char* namespace_name, const char* name, js_function_t callback, intptr_t magic);
void   api_define_method       (const char* class_name, const char* name, js_function_t callback, intptr_t magic);
void   api_define_object       (const char* namespace_name, const char* name, int class_id, void* udata);
void   api_define_prop         (const char* class_name, const char* name, bool enumerable, js_function_t getter, js_function_t setter);
void   api_define_static_prop  (const char* namespace_name, const char* name, js_function_t getter, js_function_t setter, intptr_t magic);
void   api_define_subclass     (const char* name, int class_id, int super_id, js_function_t constructor, js_finalizer_t finalizer, intptr_t magic);

void* jsal_get_class_obj        (int index, int class_id);
bool  jsal_is_class_obj         (int index, int class_id);
int   jsal_push_class_name      (int class_id);
int   jsal_push_class_obj       (int class_id, void* udata, bool in_ctor);
int   jsal_push_class_fatobj    (int class_id, bool in_ctor, size_t data_size, void* *out_data_ptr);
int   jsal_push_class_prototype (int class_id);
void* jsal_require_class_obj    (int index, int class_id);
void  jsal_set_class_ptr        (int index, void* ptr);

#endif // SPHERE__API_H__INCLUDED
