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

#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "jsal.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <setjmp.h>

#include <ChakraCore.h>
#include "lstring.h"
#include "vector.h"

struct jsal_ref
{
	JsValueRef value;
};

struct function
{
	jsal_callback_t callback;
	bool            ctor_only;
	int             min_args;
};

struct object
{
	void*           data;
	jsal_callback_t finalizer;
	JsValueRef      object;
};

#if defined(_WIN32)
int asprintf  (char* *out, const char* format, ...);
int vasprintf (char* *out, const char* format, va_list ap);
#endif

static JsValueRef CHAKRA_CALLBACK do_native_call   (JsValueRef callee, bool is_ctor, JsValueRef argv[], unsigned short argc, void* userdata);
static void CHAKRA_CALLBACK       finalize_object  (void* userdata);
static void                       free_ref         (jsal_ref_t* ref);
static JsValueRef                 get_value        (int stack_index);
static JsPropertyIdRef            make_property_id (JsValueRef key_value);
static jsal_ref_t*                make_ref         (JsValueRef value);
static JsValueRef                 pop_value        (void);
static int                        push_value       (JsValueRef value);
static void                       throw_if_error   (void);
static void                       throw_value      (JsValueRef value);

static vector_t*       s_catch_stack;
static JsContextRef    s_js_context;
static JsRuntimeHandle s_js_runtime = NULL;
static JsValueRef      s_stash;
static vector_t*       s_stack;
static int             s_stack_base;

bool
jsal_init(void)
{
	JsValueRef null_value;
	
	if (JsCreateRuntime(JsRuntimeAttributeDispatchSetExceptionsToDebugger, NULL, &s_js_runtime) != JsNoError)
		goto on_error;
	if (JsCreateContext(s_js_runtime, &s_js_context) != JsNoError)
		goto on_error;
	JsSetCurrentContext(s_js_context);

	// set up the stash, used to store JS values behind the scenes.
	JsCreateObject(&s_stash);
	JsGetNullValue(&null_value);
	JsSetPrototype(s_stash, null_value);
	JsAddRef(s_stash, NULL);

	s_stack = vector_new(sizeof(JsValueRef));
	s_catch_stack = vector_new(sizeof(jmp_buf));
	s_stack_base = 0;
	return true;

on_error:
	if (s_js_runtime != NULL)
		JsDisposeRuntime(s_js_runtime);
	s_js_runtime = NULL;
	return false;
}

void
jsal_uninit(void)
{
	JsValueRef value;
	
	iter_t iter;

	iter = vector_enum(s_stack);
	while (iter_next(&iter)) {
		value = *(JsValueRef*)iter.ptr;
		JsRelease(value, NULL);
	}
	vector_free(s_stack);
	vector_free(s_catch_stack);
	JsRelease(s_stash, NULL);
	JsSetCurrentContext(JS_INVALID_REFERENCE);
	JsDisposeRuntime(s_js_runtime);
}

void
jsal_call(int num_args)
{
	/* [ ... function arg1..argN ] -> [ .. retval ] */

	jsal_push_undefined();
	if (num_args > 0)
		jsal_insert(-num_args - 1);
	jsal_call_method(num_args);
}

void
jsal_call_method(int num_args)
{
	/* [ ... function this arg1..argN ] -> [ .. retval ] */

	JsValueRef* arguments;
	JsValueRef  function_ref;
	int         offset;
	JsValueRef  retval_ref;

	int i;

	num_args += 1;  // treat `this` as first argument
	arguments = malloc(num_args * sizeof(JsValueRef));
	function_ref = get_value(-num_args - 1);
	offset = -num_args;
	for (i = 0; i < num_args; ++i)
		arguments[i] = get_value(i + offset);
	JsCallFunction(function_ref, arguments, (unsigned short)num_args, &retval_ref);
	jsal_pop(num_args + 1);
	throw_if_error();
	push_value(retval_ref);
}

void
jsal_construct(int num_args)
{
	/* [ ... constructor arg1..argN ] -> [ .. retval ] */

	JsValueRef* arguments;
	JsValueRef  function_ref;
	int         offset;
	JsValueRef  retval_ref;

	int i;

	arguments = malloc(num_args * sizeof(JsValueRef));
	function_ref = get_value(-num_args - 1);
	offset = -num_args;
	for (i = 0; i < num_args; ++i)
		arguments[i] = get_value(i + offset);
	JsConstructObject(function_ref, arguments, (unsigned short)num_args, &retval_ref);
	jsal_pop(num_args + 1);
	throw_if_error();
	push_value(retval_ref);
}

void
jsal_compile(const char* filename)
{
	/* [ ... source ] -> [ ... function ] */

	JsValueRef function_ref;
	JsValueRef name_ref;
	JsValueRef source_ref;
	
	source_ref = pop_value();
	JsCreateString(filename, strlen(filename), &name_ref);
	JsParse(source_ref, JS_SOURCE_CONTEXT_NONE, name_ref, JsParseScriptAttributeNone, &function_ref);
	throw_if_error();
	push_value(function_ref);
}

void
jsal_def_prop(int object_index)
{
	/* [ ... key descriptor ] -> [ ... ] */

	JsValueRef      descriptor;
	JsPropertyIdRef key;
	JsValueRef      object;
	bool            result;

	object = get_value(object_index);
	descriptor = pop_value();
	key = make_property_id(pop_value());
	JsDefineProperty(object, key, descriptor, &result);
	throw_if_error();
}

void
jsal_def_prop_index(int object_index, int name)
{
	/* [ ... descriptor ] -> [ ... ] */

	object_index = jsal_normalize_index(object_index);
	jsal_push_sprintf("%d", name);
	jsal_insert(-2);
	jsal_def_prop(object_index);
}

void
jsal_def_prop_string(int object_index, const char* name)
{
	/* [ ... descriptor ] -> [ ... ] */

	object_index = jsal_normalize_index(object_index);
	jsal_push_string(name);
	jsal_insert(-2);
	jsal_def_prop(object_index);
}

bool
jsal_del_prop(int object_index)
{
	/* [ ... key ] -> [ ... ] */

	JsPropertyIdRef key_ref;
	JsValueRef      object_ref;
	JsValueRef      result;
	bool            retval;

	object_ref = get_value(object_index);
	key_ref = make_property_id(pop_value());
	JsDeleteProperty(object_ref, key_ref, true, &result);
	throw_if_error();
	JsBooleanToBool(result, &retval);
	return retval;
}

bool
jsal_del_prop_index(int object_index, int name)
{
	object_index = jsal_normalize_index(object_index);
	jsal_push_sprintf("%d", name);
	return jsal_del_prop(object_index);
}

bool
jsal_del_prop_string(int object_index, const char* name)
{
	object_index = jsal_normalize_index(object_index);
	jsal_push_string(name);
	return jsal_del_prop(object_index);
}

int
jsal_dup(int from_index)
{
	JsValueRef value;

	value = get_value(from_index);
	return push_value(value);
}

void
jsal_error(jsal_error_t type, const char* format, ...)
{
	va_list ap;

	va_start(ap, format);
	jsal_error_va(type, format, ap);
	va_end(ap);
}

void
jsal_error_va(jsal_error_t type, const char* format, va_list ap)
{
	jsal_push_new_error_va(type, format, ap);
	jsal_throw();
}

void
jsal_gc(void)
{
	JsCollectGarbage(s_js_runtime);
}

bool
jsal_get_boolean(int at_index)
{
	bool       value;
	JsValueRef value_ref;

	value_ref = get_value(at_index);
	if (JsBooleanToBool(value_ref, &value) != JsNoError)
		return false;
	return value;
}

void*
jsal_get_buffer(int at_index, size_t *out_size)
{
	unsigned int  size;
	JsValueType   type;
	ChakraBytePtr value;
	JsValueRef    value_ref;

	value_ref = get_value(at_index);
	JsGetValueType(value_ref, &type);
	if (type == JsTypedArray)
		JsGetTypedArrayStorage(value_ref, &value, &size, NULL, NULL);
	else if (type == JsArrayBuffer)
		JsGetArrayBufferStorage(value_ref, &value, &size);
	else
		return NULL;
	*out_size = size;
	return value;
}

bool
jsal_get_global(void)
{
	/* [ ... key ] -> [ ... value ] */

	JsPropertyIdRef key;
	JsValueRef      object;
	JsValueRef      value;

	key = make_property_id(pop_value());
	JsGetGlobalObject(&object);
	JsGetProperty(object, key, &value);
	throw_if_error();
	push_value(value);
	return !jsal_is_undefined(-1);
}

bool
jsal_get_global_string(const char* name)
{
	/* [ ... ] -> [ ... value ] */

	jsal_push_string(name);
	return jsal_get_global();
}

void*
jsal_get_host_data(int at_index)
{
	JsValueRef     object;
	struct object* object_info;
	void*          ptr;

	object = get_value(at_index);
	if (JsGetExternalData(object, &ptr) != JsNoError)
		return NULL;
	object_info = ptr;
	return object_info->data;
}

int
jsal_get_int(int index)
{
	int        value;
	JsValueRef value_ref;

	value_ref = get_value(index);
	if (JsNumberToInt(value_ref, &value) != JsNoError)
		return 0;
	return value;
}

int
jsal_get_length(int at_index)
{
	int value;

	if (!jsal_get_prop_string(at_index, "length"))
		return 0;
	value = jsal_get_int(-1);
	jsal_pop(1);
	return value;
}

const char*
jsal_get_lstring(int index, size_t *out_length)
{
	static int        counter = 0;
	static lstring_t* retval[10];

	lstring_t*     string_ptr;
	const wchar_t* value;
	size_t         value_length;
	JsValueRef     value_ref;

	value_ref = get_value(index);
	if (JsStringToPointer(value_ref, &value, &value_length) != JsNoError)
		return NULL;
	lstr_free(retval[counter]);
	retval[counter] = lstr_from_wide(value, value_length);
	string_ptr = retval[counter];
	counter = (counter + 1) % 10;
	*out_length = lstr_len(string_ptr);
	return lstr_cstr(string_ptr);
}

double
jsal_get_number(int index)
{
	double     value;
	JsValueRef value_ref;

	value_ref = get_value(index);
	if (JsNumberToDouble(value_ref, &value) != JsNoError)
		return NAN;
	return value;
}

bool
jsal_get_prop(int object_index)
{
	/* [ ... key ] -> [ ... value ] */

	JsPropertyIdRef key_ref;
	JsValueRef      object_ref;
	JsValueRef      value_ref;

	object_ref = get_value(object_index);
	key_ref = make_property_id(pop_value());
	JsGetProperty(object_ref, key_ref, &value_ref);
	throw_if_error();
	push_value(value_ref);
	return !jsal_is_undefined(-1);
}

bool
jsal_get_prop_index(int object_index, int name)
{
	/* [ ... ] -> [ ... value ] */

	object_index = jsal_normalize_index(object_index);
	jsal_push_sprintf("%d", name);
	return jsal_get_prop(object_index);
}

bool
jsal_get_prop_string(int object_index, const char* name)
{
	/* [ ... ] -> [ ... value ] */

	object_index = jsal_normalize_index(object_index);
	jsal_push_string(name);
	return jsal_get_prop(object_index);
}

void
jsal_get_prototype(int object_index)
{
	/* [ ... ] -> [ ... prototype ] */

	JsValueRef object;
	JsValueRef prototype;

	object = get_value(object_index);
	JsGetPrototype(object, &prototype);
	throw_if_error();
	push_value(prototype);
}

const char*
jsal_get_string(int index)
{
	size_t length;
	
	return jsal_get_lstring(index, &length);
}

int
jsal_get_top(void)
{
	return vector_len(s_stack) - s_stack_base;
}


bool
jsal_has_prop(int object_index)
{
	/* [ ... key ] -> [ ... ] */
	
	bool            has_property;
	JsPropertyIdRef key_ref;
	JsValueRef      object_ref;

	object_ref = get_value(object_index);
	key_ref = make_property_id(pop_value());
	JsHasProperty(object_ref, key_ref, &has_property);
	return has_property;
}

bool
jsal_has_prop_index(int object_index, int name)
{
	object_index = jsal_normalize_index(object_index);
	jsal_push_sprintf("%d", name);
	return jsal_has_prop(object_index);
}

bool
jsal_has_prop_string(int object_index, const char* name)
{
	object_index = jsal_normalize_index(object_index);
	jsal_push_string(name);
	return jsal_has_prop(object_index);
}

void
jsal_insert(int at_index)
{
	/* [ ... value ] -> [ ... value ... ] */
	
	JsValueRef value;
	
	at_index = jsal_normalize_index(at_index);

	if (at_index == jsal_get_top() - 1)
		return;  // nop
	value = pop_value();
	vector_insert(s_stack, at_index + s_stack_base, &value);
}

bool
jsal_is_array(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsArray;
}

bool
jsal_is_boolean(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsBoolean;
}

bool
jsal_is_buffer(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsArrayBuffer
		|| type == JsTypedArray;
}

bool
jsal_is_error(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsError;
}

bool
jsal_is_function(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsFunction;
}

bool
jsal_is_null(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsNull;
}

bool
jsal_is_number(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsNumber;
}

bool
jsal_is_object(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsObject
		|| type == JsArray
		|| type == JsArrayBuffer
		|| type == JsDataView
		|| type == JsError
		|| type == JsFunction
		|| type == JsTypedArray;
}

bool
jsal_is_object_coercible(int at_index)
{
	return !jsal_is_undefined(at_index)
		&& !jsal_is_null(at_index);
}

bool
jsal_is_string(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsString;
}

bool
jsal_is_symbol(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsSymbol;
}

bool
jsal_is_undefined(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsUndefined;
}

int
jsal_normalize_index(int index)
{
	int real_index;
	int top;
	
	real_index = index;
	top = jsal_get_top();
	if (real_index < 0)
		real_index += top;
	if (real_index < 0 || real_index >= top)
		jsal_error(JS_REF_ERROR, "invalid stack index '%d'", index);
	return real_index;
}

void
jsal_parse(int at_index)
{
	at_index = jsal_normalize_index(at_index);
	jsal_push_eval("JSON.parse");
	jsal_push_eval("JSON");
	jsal_dup(at_index);
	jsal_call_method(1);
	jsal_replace(at_index);
}

void
jsal_pop(int num_values)
{
	int top;
	
	top = jsal_get_top();
	if (num_values > top)
		jsal_error(JS_RANGE_ERROR, "cannot pop %d values", num_values);
	jsal_set_top(top - num_values);
}

int
jsal_push_boolean(bool value)
{
	JsValueRef ref;

	JsBoolToBoolean(value, &ref);
	return push_value(ref);
}

int
jsal_push_constructor(jsal_callback_t callback, const char* name, int min_args)
{
	JsValueRef       function;
	struct function* function_data;
	JsValueRef       name_string;

	function_data = calloc(1, sizeof(struct function));
	function_data->callback = callback;
	function_data->ctor_only = true;
	function_data->min_args = min_args;
	JsCreateString(name, strlen(name), &name_string);
	JsCreateNamedFunction(name_string, do_native_call, function_data, &function);
	return push_value(function);
}

int
jsal_push_eval(const char* source)
{
	JsValueRef name_ref;
	JsValueRef source_ref;
	JsValueRef ref;

	JsCreateString(source, strlen(source), &source_ref);
	JsCreateString("eval()", 6, &name_ref);
	JsRun(source_ref, JS_SOURCE_CONTEXT_NONE, name_ref, JsParseScriptAttributeNone, &ref);
	throw_if_error();
	return push_value(ref);
}

int
jsal_push_function(jsal_callback_t callback, const char* name, int min_args)
{
	JsValueRef       function;
	struct function* function_data;
	JsValueRef       name_string;
	
	function_data = calloc(1, sizeof(struct function));
	function_data->callback = callback;
	function_data->min_args = min_args;
	JsCreateString(name, strlen(name), &name_string);
	JsCreateNamedFunction(name_string, do_native_call, function_data, &function);
	return push_value(function);
}

int
jsal_push_global_object(void)
{
	JsValueRef object;

	JsGetGlobalObject(&object);
	return push_value(object);
}

int
jsal_push_hidden_stash(void)
{
	return push_value(s_stash);
}

int
jsal_push_int(int value)
{
	JsValueRef ref;

	JsIntToNumber(value, &ref);
	return push_value(ref);
}

int
jsal_push_known_symbol(const char* name)
{
	jsal_push_global_object();
	jsal_get_prop_string(-1, "Symbol");
	jsal_get_prop_string(-1, name);
	jsal_remove(-2);
	jsal_remove(-2);
	return jsal_get_top() - 1;
}

int
jsal_push_lstring(const char* value, size_t length)
{
	JsValueRef ref;

	JsCreateString(value, length, &ref);
	return push_value(ref);
}

int
jsal_push_new_array(void)
{
	JsValueRef ref;

	JsCreateArray(0, &ref);
	return push_value(ref);
}

int
jsal_push_new_bare_object(void)
{
	int index;

	index = jsal_push_new_object();
	jsal_push_null();
	jsal_set_prototype(-2);
	return index;
}

int
jsal_push_new_buffer(size_t size)
{
	JsValueRef ref;

	JsCreateArrayBuffer((unsigned int)size, &ref);
	return push_value(ref);
}

int
jsal_push_new_error(jsal_error_t type, const char* format, ...)
{
	va_list ap;
	int     index;

	va_start(ap, format);
	index = jsal_push_new_error_va(type, format, ap);
	va_end(ap);
	return index;
}

int
jsal_push_new_error_va(jsal_error_t type, const char* format, va_list ap)
{
	char*       message;
	JsValueRef  message_ref;
	JsValueRef  ref;
	JsErrorCode result;

	vasprintf(&message, format, ap);
	JsCreateString(message, strlen(message), &message_ref);
	result = type == JS_RANGE_ERROR ? JsCreateRangeError(message_ref, &ref)
		: type == JS_REF_ERROR ? JsCreateReferenceError(message_ref, &ref)
		: type == JS_SYNTAX_ERROR ? JsCreateSyntaxError(message_ref, &ref)
		: type == JS_TYPE_ERROR ? JsCreateTypeError(message_ref, &ref)
		: type == JS_URI_ERROR ? JsCreateURIError(message_ref, &ref)
		: JsCreateError(message_ref, &ref);
	return push_value(ref);
}

int
jsal_push_new_host_object(void* data, jsal_callback_t finalizer)
{
	JsValueRef     object;
	struct object* object_info;

	object_info = calloc(1, sizeof(struct object));
	JsCreateExternalObject(object_info, NULL, &object);

	object_info->data = data;
	object_info->finalizer = finalizer;
	object_info->object = object;
	
	return push_value(object);
}

int
jsal_push_new_object(void)
{
	JsValueRef ref;

	JsCreateObject(&ref);
	return push_value(ref);
}

int
jsal_push_new_symbol(const char* description)
{
	JsValueRef name_ref;
	JsValueRef ref;

	JsCreateString(description, strlen(description), &name_ref);
	JsCreateSymbol(name_ref, &ref);
	return push_value(ref);
}

int
jsal_push_null(void)
{
	JsValueRef ref;

	JsGetNullValue(&ref);
	return push_value(ref);
}

int
jsal_push_number(double value)
{
	JsValueRef ref;
	
	JsDoubleToNumber(value, &ref);
	return push_value(ref);
}

int
jsal_push_ref(jsal_ref_t* ref)
{
	return push_value(ref->value);
}

int
jsal_push_sprintf(const char* format, ...)
{
	va_list ap;
	int     index;
	char*   string;

	va_start(ap, format);
	vasprintf(&string, format, ap);
	va_end(ap);
	index = jsal_push_string(string);
	free(string);
	return index;
}

int
jsal_push_string(const char* value)
{
	JsValueRef ref;

	JsCreateString(value, strlen(value), &ref);
	return push_value(ref);
}

int
jsal_push_undefined(void)
{
	JsValueRef ref;

	JsGetUndefinedValue(&ref);
	return push_value(ref);
}

int
jsal_pull(int from_index)
{
	JsValueRef value;

	value = get_value(from_index);
	jsal_remove(from_index);
	return push_value(value);
}

void
jsal_put_prop(int object_index)
{
	/* [ ... key value ] -> [ ... ] */

	JsPropertyIdRef key;
	JsValueRef      object;
	JsValueRef      value;

	object = get_value(object_index);
	value = pop_value();
	key = make_property_id(pop_value());
	JsSetProperty(object, key, value, true);
	throw_if_error();
}

void
jsal_put_prop_index(int object_index, int name)
{
	/* [ ... value ] -> [ ... ] */

	object_index = jsal_normalize_index(object_index);
	jsal_push_sprintf("%d", name);
	jsal_insert(-2);
	jsal_put_prop(object_index);
}

void
jsal_put_prop_string(int object_index, const char* name)
{
	/* [ ... value ] -> [ ... ] */

	object_index = jsal_normalize_index(object_index);
	jsal_push_string(name);
	jsal_insert(-2);
	jsal_put_prop(object_index);
}

jsal_ref_t*
jsal_ref(int at_index)
{
	jsal_ref_t*  ref;
	JsValueRef value;

	value = get_value(at_index);
	JsAddRef(value, NULL);

	ref = calloc(1, sizeof(jsal_ref_t));
	ref->value = value;
	return ref;
}

void
jsal_remove(int at_index)
{
	/* [ ... value ... ] -> [ ... ] */

	at_index = jsal_normalize_index(at_index);

	vector_remove(s_stack, at_index + s_stack_base);
}

bool
jsal_replace(int at_index)
{
	/* [ ... old_value ... new_value ] -> [ ... new_value ... ] */

	JsValueRef ref;

	at_index = jsal_normalize_index(at_index);

	if (at_index == jsal_get_top() - 1)
		return true;  // nop
	ref = pop_value();
	vector_put(s_stack, at_index + s_stack_base, &ref);
	return true;
}

void
jsal_require_array(int at_index)
{
	if (!jsal_is_array(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not an array", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
}

bool
jsal_require_boolean(int at_index)
{
	if (!jsal_is_boolean(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a boolean", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
	return jsal_get_boolean(at_index);
}

void*
jsal_require_buffer(int at_index, size_t *out_size)
{
	if (!jsal_is_buffer(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a buffer", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
	return jsal_get_buffer(at_index, out_size);
}

void
jsal_require_function(int at_index)
{
	if (!jsal_is_function(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a function", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
}

int
jsal_require_int(int at_index)
{
	if (!jsal_is_number(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a number", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
	return jsal_get_int(at_index);
}

const char*
jsal_require_lstring(int at_index, size_t *out_length)
{
	if (!jsal_is_string(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a string", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
	return jsal_get_lstring(at_index, out_length);
}

void
jsal_require_null(int at_index)
{
	if (!jsal_is_null(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not 'null'", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
}

double
jsal_require_number(int at_index)
{
	if (!jsal_is_number(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a number", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
	return jsal_get_number(at_index);
}

void
jsal_require_object(int at_index)
{
	if (!jsal_is_object(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not an object", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
}

void
jsal_require_object_coercible(int at_index)
{
	if (!jsal_is_object_coercible(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not object-coercible", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
}

const char*
jsal_require_string(int at_index)
{
	size_t length;
	
	return jsal_require_lstring(at_index, &length);
}

void
jsal_require_symbol(int at_index)
{
	if (!jsal_is_symbol(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a symbol", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
}

void
jsal_require_undefined(int at_index)
{
	if (!jsal_is_null(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not 'undefined'", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
}

void
jsal_set_finalizer(int at_index, jsal_callback_t callback)
{
	JsValueRef     object;
	struct object* object_info;
	void*          ptr;

	object = get_value(at_index);
	if (JsGetExternalData(object, &ptr) != JsNoError)
		return;
	object_info = ptr;
	object_info->finalizer = callback;
}

void
jsal_set_host_data(int at_index, void* ptr)
{
	void*          data;
	JsValueRef     object;
	struct object* object_info;

	object = get_value(at_index);
	if (JsGetExternalData(object, &data) != JsNoError)
		return;
	object_info = data;
	object_info->data = ptr;
}

void
jsal_set_prototype(int object_index)
{
	/* [ ... prototype ] -> [ ... ] */

	JsValueRef object;
	JsValueRef prototype;

	object = get_value(object_index);
	prototype = pop_value();
	JsSetPrototype(object, prototype);
	throw_if_error();
}

void
jsal_set_top(int new_top)
{
	int        old_size;
	int        new_size;
	JsValueRef undef_value;

	int i;

	old_size = vector_len(s_stack);
	new_size = new_top + s_stack_base;
	vector_resize(s_stack, new_size);
	if (new_size > old_size) {
		JsGetUndefinedValue(&undef_value);
		for (i = old_size; i < new_size; ++i) {
			JsAddRef(undef_value, NULL);
			vector_put(s_stack, i, &undef_value);
		}
	}
}

void
jsal_stringify(int at_index)
{
	at_index = jsal_normalize_index(at_index);
	jsal_push_eval("JSON.stringify");
	jsal_push_eval("JSON");
	jsal_dup(at_index);
	jsal_call_method(1);
	jsal_replace(at_index);
}

void
jsal_throw(void)
{
	/* [ ... exception ] -> [ ... ] */

	JsValueRef ref;

	ref = pop_value();
	throw_value(ref);
}

bool
jsal_to_boolean(int at_index)
{
	JsValueRef ref;

	at_index = jsal_normalize_index(at_index);
	ref = get_value(at_index);
	JsConvertValueToBoolean(ref, &ref);
	throw_if_error();
	vector_put(s_stack, at_index + s_stack_base, &ref);
	return jsal_get_boolean(at_index);
}

int
jsal_to_int(int at_index)
{
	jsal_to_number(at_index);
	return jsal_get_int(at_index);
}

double
jsal_to_number(int at_index)
{
	JsValueRef ref;

	at_index = jsal_normalize_index(at_index);
	ref = get_value(at_index);
	JsConvertValueToNumber(ref, &ref);
	throw_if_error();
	vector_put(s_stack, at_index + s_stack_base, &ref);
	return jsal_get_number(at_index);
}

void
jsal_to_object(int at_index)
{
	JsValueRef ref;

	at_index = jsal_normalize_index(at_index);
	ref = get_value(at_index);
	JsConvertValueToObject(ref, &ref);
	throw_if_error();
	vector_put(s_stack, at_index + s_stack_base, &ref);
}

const char*
jsal_to_string(int at_index)
{
	JsValueRef ref;

	at_index = jsal_normalize_index(at_index);
	ref = get_value(at_index);
	JsConvertValueToString(ref, &ref);
	throw_if_error();
	vector_put(s_stack, at_index + s_stack_base, &ref);
	return jsal_get_string(at_index);
}

bool
jsal_try_call(int num_args)
{
	/* [ ... function arg1..argN ] -> [ .. retval ] */

	jmp_buf label;

	if (setjmp(label) == 0) {
		vector_push(s_catch_stack, label);
		jsal_call(num_args);
		vector_pop(s_catch_stack, 1);
		return true;
	}
	else {
		return false;
	}
}

bool
jsal_try_call_method(int num_args)
{
	/* [ ... function this arg1..argN ] -> [ .. retval ] */

	jmp_buf label;

	if (setjmp(label) == 0) {
		vector_push(s_catch_stack, label);
		jsal_call_method(num_args);
		vector_pop(s_catch_stack, 1);
		return true;
	}
	else {
		return false;
	}
}

bool
jsal_try_compile(const char* filename)
{
	/* [ ... source ] -> [ .. function ] */

	jmp_buf label;

	if (setjmp(label) == 0) {
		vector_push(s_catch_stack, label);
		jsal_compile(filename);
		vector_pop(s_catch_stack, 1);
		return true;
	}
	else {
		return false;
	}
}

bool
jsal_try_construct(int num_args)
{
	/* [ ... constructor arg1..argN ] -> [ .. retval ] */

	jmp_buf label;

	if (setjmp(label) == 0) {
		vector_push(s_catch_stack, label);
		jsal_construct(num_args);
		vector_pop(s_catch_stack, 1);
		return true;
	}
	else {
		return false;
	}
}

bool
jsal_try_parse(int at_index)
{
	jmp_buf label;

	if (setjmp(label) == 0) {
		vector_push(s_catch_stack, label);
		jsal_parse(at_index);
		vector_pop(s_catch_stack, 1);
		return true;
	}
	else {
		return false;
	}
}

void
jsal_unref(jsal_ref_t* ref)
{
	JsRelease(ref->value, NULL);
	free(ref);
}

static void
throw_if_error(void)
{
	JsValueRef  error_ref;
	bool        has_exception;
	JsErrorCode result;

	JsHasException(&has_exception);
	if (has_exception) {
		result = JsGetAndClearException(&error_ref);
		throw_value(error_ref);
	}
}

static void
free_ref(jsal_ref_t* ref)
{
	JsRelease(ref->value, NULL);
	free(ref);
}

static JsValueRef
get_value(int stack_index)
{
	JsValueRef value;

	stack_index = jsal_normalize_index(stack_index);
	value = *(JsValueRef*)vector_get(s_stack, stack_index + s_stack_base);
	return value;
}

static JsPropertyIdRef
make_property_id(JsValueRef key)
{
	JsPropertyIdRef ref;
	size_t          key_length;
	const wchar_t*  key_string;
	JsValueType     key_type;

	JsGetValueType(key, &key_type);
	if (key_type == JsSymbol) {
		JsGetPropertyIdFromSymbol(key, &ref);
	}
	else {
		JsStringToPointer(key, &key_string, &key_length);
		JsGetPropertyIdFromName(key_string, &ref);
	}
	return ref;
}

jsal_ref_t*
make_ref(JsValueRef value)
{
	jsal_ref_t* ref;
	
	JsAddRef(value, NULL);

	ref = calloc(1, sizeof(jsal_ref_t));
	ref->value = value;
	return ref;
}

static JsValueRef
pop_value(void)
{
	int        index;
	JsValueRef ref;

	index = vector_len(s_stack) - 1;
	ref = *(JsValueRef*)vector_get(s_stack, index);
	vector_pop(s_stack, 1);
	JsRelease(ref, NULL);
	return ref;
}

static int
push_value(JsValueRef value)
{
	JsAddRef(value, NULL);
	vector_push(s_stack, &value);
	return vector_len(s_stack) - s_stack_base - 1;
}

static void
throw_value(JsValueRef value)
{
	int     index;
	jmp_buf label;

	push_value(value);
	index = vector_len(s_catch_stack) - 1;
	if (index >= 0) {
		memcpy(label, vector_get(s_catch_stack, index), sizeof(jmp_buf));
		vector_pop(s_catch_stack, 1);
		longjmp(label, 1);
	}
	else {
		printf("JSAL exception thrown from unguarded C code!\n");
		printf("-> %s\n", jsal_to_string(-1));
		exit(EXIT_FAILURE);
	}
}

static JsValueRef CHAKRA_CALLBACK
do_native_call(JsValueRef callee, bool is_ctor, JsValueRef argv[], unsigned short argc, void* userdata)
{
	jsal_ref_t*      callee_ref;
	JsValueRef       exception;
	struct function* function_data;
	bool             has_return;
	int              last_stack_base;
	jmp_buf          label;
	JsValueRef       retval = JS_INVALID_REFERENCE;

	int i;

	function_data = userdata;

	last_stack_base = s_stack_base;
	s_stack_base = vector_len(s_stack);
	callee_ref = make_ref(callee);
	for (i = 0; i < argc; ++i)
		push_value(argv[i]);
	if (setjmp(label) == 0) {
		vector_push(s_catch_stack, label);
		if (!is_ctor && function_data->ctor_only) {
			push_value(callee);  // note: gets popped during unwind
			jsal_get_prop_string(-1, "name");
			jsal_error(JS_TYPE_ERROR, "'%s()' requires 'new'", jsal_to_string(-1));
		}
		if (argc - 1 < function_data->min_args) {
			push_value(callee);  // note: gets popped during unwind
			jsal_get_prop_string(-1, "name");
			jsal_error(JS_TYPE_ERROR, "too few arguments for '%s()'", jsal_to_string(-1));
		}
		has_return = function_data->callback(callee_ref, argc - 1, is_ctor);
		if (has_return)
			retval = pop_value();
		vector_pop(s_catch_stack, 1);
	}
	else {
		// if an error gets thrown into C code, `jsal_throw()` leaves it on top
		// of the value stack.
		exception = pop_value();
		JsSetException(exception);
		retval = exception;
	}
	free_ref(callee_ref);
	vector_resize(s_stack, s_stack_base);
	s_stack_base = last_stack_base;
	return retval;
}

static void CHAKRA_CALLBACK
finalize_object(void* userdata)
{
	JsValueRef	   exception;
	jmp_buf        label;
	struct object* object_info;
	int            old_stack_base;

	object_info = userdata;
	old_stack_base = s_stack_base;
	s_stack_base = vector_len(s_stack);
	push_value(object_info->object);
	if (setjmp(label) == 0) {
		vector_push(s_catch_stack, label);
		if (object_info->finalizer != NULL)
			object_info->finalizer(NULL, 0, false);
		vector_pop(s_catch_stack, 1);
	}
	else {
		exception = pop_value();
		JsSetException(exception);
	}
	vector_resize(s_stack, s_stack_base);
	s_stack_base = old_stack_base;
	free(object_info);
}

#if defined(_WIN32)
int
asprintf(char** out, const char* format, ...)
{
	va_list ap;
	int     buf_size;

	va_start(ap, format);
	buf_size = vasprintf(out, format, ap);
	va_end(ap);
	return buf_size;
}

int
vasprintf(char** out, const char* format, va_list ap)
{
	va_list apc;
	int     buf_size;

	va_copy(apc, ap);
	buf_size = vsnprintf(NULL, 0, format, apc) + 1;
	va_end(apc);
	*out = malloc(buf_size);
	vsnprintf(*out, buf_size, format, ap);
	return buf_size;
}
#endif
