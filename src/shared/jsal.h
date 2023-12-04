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

#ifndef SPHERE__JSAL_H__INCLUDED
#define SPHERE__JSAL_H__INCLUDED

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define except_t __attribute__((noreturn)) void
#elif defined(_MSC_VER)
#define except_t __declspec(noreturn) void
#else
#define except_t void
#endif

typedef struct js_ref js_ref_t;

typedef
enum js_buffer_type
{
	JS_ARRAYBUFFER,
	JS_INT8ARRAY,
	JS_INT16ARRAY,
	JS_INT32ARRAY,
	JS_UINT8ARRAY,
	JS_UINT8ARRAY_CLAMPED,
	JS_UINT16ARRAY,
	JS_UINT32ARRAY,
	JS_FLOAT32ARRAY,
	JS_FLOAT64ARRAY,
} js_buffer_type_t;

typedef
enum js_debug_event
{
	JS_DEBUG_BREAKPOINT,
	JS_DEBUG_THROW,
} js_debug_event_t;

typedef
enum js_step
{
	JS_STEP_CONTINUE,
	JS_STEP_IN,
	JS_STEP_OUT,
	JS_STEP_OVER,
} js_step_t;

typedef
enum js_error_type
{
	JS_ERROR,
	JS_RANGE_ERROR,
	JS_REF_ERROR,
	JS_SYNTAX_ERROR,
	JS_TYPE_ERROR,
	JS_URI_ERROR,
} js_error_type_t;

typedef bool      (* js_function_t)        (int num_args, bool is_ctor, intptr_t magic);
typedef js_step_t (* js_break_callback_t)  (void);
typedef void      (* js_finalizer_t)       (void* host_ptr);
typedef void      (* js_job_callback_t)    (void);
typedef bool      (* js_reject_callback_t) (void);
typedef void      (* js_throw_callback_t)  (void);
typedef void      (* js_import_callback_t) (void);
typedef void      (* js_module_callback_t) (const char* specifier, bool has_error);

bool         jsal_init                     (void);
void         jsal_uninit                   (void);
void         jsal_update                   (bool in_event_loop);
bool         jsal_busy                     (void);
bool         jsal_vm_enabled               (void);
void         jsal_on_enqueue_job           (js_job_callback_t callback);
void         jsal_on_import_module         (js_import_callback_t callback);
void         jsal_on_module_complete       (js_module_callback_t callback); 
void         jsal_on_reject_promise        (js_reject_callback_t callback);
void         jsal_call                     (int num_args);
void         jsal_call_method              (int num_args);
unsigned int jsal_compile                  (const char* filename);
void         jsal_construct                (int num_args);
void         jsal_def_prop                 (int object_index);
void         jsal_def_prop_index           (int object_index, int name);
void         jsal_def_prop_string          (int object_index, const char* name);
bool         jsal_del_global               (void);
bool         jsal_del_global_string        (const char* name);
bool         jsal_del_prop                 (int object_index);
bool         jsal_del_prop_index           (int object_index, int name);
bool         jsal_del_prop_string          (int object_index, const char* name);
int          jsal_dup                      (int from_index);
void         jsal_enable_vm                (bool enabled);
except_t     jsal_error                    (js_error_type_t type, const char* message, ...);
except_t     jsal_error_va                 (js_error_type_t type, const char* message, va_list ap);
void         jsal_eval_module              (const char* specifier, const char* url);
void         jsal_exec                     (void);
void         jsal_freeze                   (int at_index);
void         jsal_gc                       (void);
bool         jsal_get_boolean              (int at_index);
void*        jsal_get_buffer_ptr           (int at_index, size_t *out_size);
bool         jsal_get_global               (void);
bool         jsal_get_global_string        (const char* name);
void*        jsal_get_host_data            (int at_index);
int          jsal_get_int                  (int at_index);
int          jsal_get_length               (int at_index);
const char*  jsal_get_lstring              (int at_index, size_t *out_length);
double       jsal_get_number               (int at_index);
bool         jsal_get_prop                 (int object_index);
bool         jsal_get_prop_index           (int object_index, int name);
bool         jsal_get_prop_key             (int object_index, js_ref_t* key);
bool         jsal_get_prop_string          (int object_index, const char* name);
void         jsal_get_prototype            (int object_index);
const char*  jsal_get_string               (int at_index);
int          jsal_get_top                  (void);
unsigned int jsal_get_uint                 (int at_index);
bool         jsal_has_own_prop             (int object_index);
bool         jsal_has_own_prop_index       (int object_index, int name);
bool         jsal_has_own_prop_string      (int object_index, const char* name);
bool         jsal_has_prop                 (int object_index);
bool         jsal_has_prop_index           (int object_index, int name);
bool         jsal_has_prop_key             (int object_index, js_ref_t* key);
bool         jsal_has_prop_string          (int object_index, const char* name);
void         jsal_insert                   (int at_index);
bool         jsal_is_array                 (int stack_index);
bool         jsal_is_async_call            (void);
bool         jsal_is_async_function        (int stack_index);
bool         jsal_is_boolean               (int stack_index);
bool         jsal_is_buffer                (int stack_index);
bool         jsal_is_error                 (int stack_index);
bool         jsal_is_function              (int stack_index);
bool         jsal_is_null                  (int stack_index);
bool         jsal_is_number                (int stack_index);
bool         jsal_is_object                (int stack_index);
bool         jsal_is_object_coercible      (int stack_index);
bool         jsal_is_string                (int stack_index);
bool         jsal_is_subclass_ctor         (void);
bool         jsal_is_symbol                (int stack_index);
bool         jsal_is_undefined             (int stack_index);
void         jsal_make_buffer              (int object_index, js_buffer_type_t buffer_type, void* buffer, size_t num_items);
js_ref_t*    jsal_new_key                  (const char* name);
bool         jsal_next                     (int iter_index);
int          jsal_normalize_index          (int index);
void         jsal_parse                    (int at_index);
void         jsal_pop                      (int num_values);
js_ref_t*    jsal_pop_ref                  (void);
int          jsal_push_boolean             (bool value);
int          jsal_push_boolean_false       (void);
int          jsal_push_boolean_true        (void);
int          jsal_push_callee              (void);
int          jsal_push_global_object       (void);
int          jsal_push_hidden_stash        (void);
int          jsal_push_int                 (int value);
int          jsal_push_known_symbol        (const char* name);
int          jsal_push_lstring             (const char* value, size_t length);
int          jsal_push_new_array           (void);
int          jsal_push_new_bare_object     (void);
int          jsal_push_new_buffer          (js_buffer_type_t type, size_t length, void* *out_data_ptr);
int          jsal_push_new_constructor     (js_function_t callback, const char* name, int min_args, intptr_t magic);
int          jsal_push_new_error           (js_error_type_t type, const char* format, ...);
int          jsal_push_new_error_va        (js_error_type_t type, const char* format, va_list ap);
int          jsal_push_new_function        (js_function_t callback, const char* name, int min_args, bool async, intptr_t magic);
int          jsal_push_new_host_object     (js_finalizer_t finalizer, size_t data_size, void* *out_data_ptr);
int          jsal_push_new_iterator        (int for_index);
int          jsal_push_new_object          (void);
int          jsal_push_new_promise         (js_ref_t* *out_resolver, js_ref_t* *out_rejector);
int          jsal_push_new_symbol          (const char* description);
int          jsal_push_newtarget           (void);
int          jsal_push_null                (void);
int          jsal_push_number              (double value);
int          jsal_push_ref                 (const js_ref_t* ref);
int          jsal_push_ref_weak            (const js_ref_t* ref);
int          jsal_push_sprintf             (const char* format, ...);
int          jsal_push_string              (const char* value);
int          jsal_push_this                (void);
int          jsal_push_uint                (unsigned int value);
int          jsal_push_undefined           (void);
void         jsal_pull                     (int from_index);
void         jsal_put_prop                 (int object_index);
void         jsal_put_prop_index           (int object_index, int name);
void         jsal_put_prop_key             (int object_index, js_ref_t* key);
void         jsal_put_prop_string          (int object_index, const char* name);
js_ref_t*    jsal_ref                      (int at_index);
void         jsal_remove                   (int at_index);
bool         jsal_replace                  (int at_index);
void         jsal_require_array            (int at_index);
bool         jsal_require_boolean          (int at_index);
void*        jsal_require_buffer_ptr       (int at_index, size_t *out_size);
void         jsal_require_function         (int at_index);
int          jsal_require_int              (int at_index);
const char*  jsal_require_lstring          (int at_index, size_t *out_length);
void         jsal_require_null             (int at_index);
double       jsal_require_number           (int at_index);
void         jsal_require_object           (int at_index);
void         jsal_require_object_coercible (int at_index);
const char*  jsal_require_string           (int at_index);
void         jsal_require_symbol           (int at_index);
unsigned int jsal_require_uint             (int at_index);
void         jsal_require_undefined        (int at_index);
void         jsal_set_async_call_flag      (bool is_async);
void         jsal_set_finalizer            (int at_index, js_finalizer_t callback);
void         jsal_set_host_data            (int at_index, void* ptr);
void         jsal_set_prototype            (int object_index);
void         jsal_set_top                  (int new_top);
void         jsal_stringify                (int at_index);
except_t     jsal_throw                    (void);
bool         jsal_to_boolean               (int at_index);
int          jsal_to_int                   (int at_index);
double       jsal_to_number                (int at_index);
void         jsal_to_object                (int at_index);
int          jsal_to_propdesc_get          (bool enumerable, bool configurable);
int          jsal_to_propdesc_get_set      (bool enumerable, bool configurable);
int          jsal_to_propdesc_set          (bool enumerable, bool configurable);
int          jsal_to_propdesc_value        (bool writable, bool enumerable, bool configurable);
const char*  jsal_to_string                (int at_index);
unsigned int jsal_to_uint                  (int at_index);
bool         jsal_try                      (js_function_t callback, int num_args);
bool         jsal_try_call                 (int num_args);
bool         jsal_try_call_method          (int num_args);
bool         jsal_try_compile              (const char* filename);
bool         jsal_try_construct            (int num_args);
bool         jsal_try_eval_module          (const char* specifier, const char* url);
bool         jsal_try_parse                (int at_index);
void         jsal_unref                    (js_ref_t* ref);

bool jsal_debug_init               (js_break_callback_t callback);
void jsal_debug_uninit             (void);
void jsal_debug_on_throw           (js_throw_callback_t callback);
int  jsal_debug_breakpoint_add     (const char* filename, unsigned int line, unsigned int column);
void jsal_debug_breakpoint_inject  (void);
void jsal_debug_breakpoint_remove  (int index);
bool jsal_debug_inspect_breakpoint (int index);
bool jsal_debug_inspect_call       (int call_index);
bool jsal_debug_inspect_eval       (int call_index, const char* source, bool *out_errored);
bool jsal_debug_inspect_object     (unsigned int object_id, int property_index);
bool jsal_debug_inspect_var        (int call_index, int var_index);

#endif // SPHERE__JSAL_H__INCLUDED
