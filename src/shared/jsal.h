/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2017, Fat Cerberus
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
 *  * Neither the name of miniSphere nor the names of its contributors may be
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

#ifndef FATCERBERUS__JSAL_H__INCLUDED
#define FATCERBERUS__JSAL_H__INCLUDED

#include <stdbool.h>
#include <stdint.h>

typedef struct js_ref js_ref_t;

typedef
enum js_error
{
	JS_ERROR,
	JS_RANGE_ERROR,
	JS_REF_ERROR,
	JS_SYNTAX_ERROR,
	JS_TYPE_ERROR,
	JS_URI_ERROR,
} js_error_t;

typedef int (* jsal_callback_t) (int num_args, bool is_ctor);

bool        jsal_init                     (void);
void        jsal_uninit                   (void);
bool        jsal_call                     (int num_args);
void        jsal_compile                  (const char* filename);
int         jsal_dup                      (int from_index);
void        jsal_error                    (js_error_t type, const char* message, ...);
void        jsal_error_va                 (js_error_t type, const char* message, va_list ap);
void*       jsal_get_buffer               (int at_index, size_t *out_size);
int         jsal_get_int                  (int at_index);
double      jsal_get_number               (int at_index);
bool        jsal_get_property             (int object_index);
bool        jsal_get_index_property       (int object_index, int name);
bool        jsal_get_named_property       (int object_index, const char* name);
const char* jsal_get_string               (int at_index);
bool        jsal_has_property             (int object_index);
bool        jsal_has_index_property       (int object_index, int name);
bool        jsal_has_named_property       (int object_index, const char* name);
bool        jsal_insert                   (int at_index);
bool        jsal_is_array                 (int stack_index);
bool        jsal_is_boolean               (int stack_index);
bool        jsal_is_buffer                (int stack_index);
bool        jsal_is_error                 (int stack_index);
bool        jsal_is_function              (int stack_index);
bool        jsal_is_null                  (int stack_index);
bool        jsal_is_number                (int stack_index);
bool        jsal_is_object                (int stack_index);
bool        jsal_is_object_coercible      (int stack_index);
bool        jsal_is_string                (int stack_index);
bool        jsal_is_symbol                (int stack_index);
bool        jsal_is_undefined             (int stack_index);
int         jsal_normalize_index          (int index);
void        jsal_pop                      (int num_values);
int         jsal_push_boolean             (bool value);
int         jsal_push_eval                (const char* source);
int         jsal_push_function            (jsal_callback_t callback, const char* name, int min_args);
int         jsal_push_global_object       (void);
int         jsal_push_int                 (int value);
int         jsal_push_known_symbol        (const char* name);
int         jsal_push_new_array           (void);
int         jsal_push_new_error           (js_error_t type, const char* format, ...);
int         jsal_push_new_error_va        (js_error_t type, const char* format, va_list ap);
int         jsal_push_new_object          (void);
int         jsal_push_new_symbol          (const char* description);
int         jsal_push_null                (void);
int         jsal_push_number              (double value);
int         jsal_push_ref                 (js_ref_t* ref);
int         jsal_push_sprintf             (const char* format, ...);
int         jsal_push_string              (const char* value);
int         jsal_push_undefined           (void);
js_ref_t*   jsal_ref                      (int at_index);
void        jsal_remove                   (int at_index);
bool        jsal_replace                  (int at_index);
bool        jsal_require_boolean          (int at_index);
void*       jsal_require_buffer           (int at_index, size_t *out_size);
void        jsal_require_function         (int at_index);
int         jsal_require_int              (int at_index);
void        jsal_require_null             (int at_index);
double      jsal_require_number           (int at_index);
void        jsal_require_object_coercible (int at_index);
const char* jsal_require_string           (int at_index);
void        jsal_require_undefined        (int at_index);
bool        jsal_set_property             (int object_index);
bool        jsal_set_index_property       (int object_index, int name);
bool        jsal_set_named_property       (int object_index, const char* name);
void        jsal_throw                    (void);
const char* jsal_to_string                (int at_index);
void        jsal_unref                    (js_ref_t* ref);

#endif // FATCERBERUS__JSAL_H__INCLUDED
