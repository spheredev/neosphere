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
#include <stdio.h>
#include <math.h>
#include <setjmp.h>

#include <ChakraCore.h>
#include "lstring.h"
#include "vector.h"

struct function
{
	jsal_callback_t callback;
	int             min_args;
};

#if defined(_WIN32)
int asprintf  (char* *out, const char* format, ...);
int vasprintf (char* *out, const char* format, va_list ap);
#endif

static JsValueRef CHAKRA_CALLBACK do_native_call (JsValueRef callee, bool is_ctor, JsValueRef argv[], unsigned short argc, void* userdata);
static JsValueRef                 get_ref        (int stack_index);
static JsPropertyIdRef            key_to_prop_id (JsValueRef key_ref);
static JsValueRef                 pop_ref        (void);
static int                        push_ref       (JsValueRef ref);
static void                       throw_if_error (void);
static void                       throw_ref      (JsValueRef ref);

static vector_t*       s_catch_stack;
static JsContextRef    s_js_context;
static JsRuntimeHandle s_js_runtime = NULL;
static vector_t*       s_stack;
static int             s_stack_base;

bool
jsal_init(void)
{
	if (JsCreateRuntime(JsRuntimeAttributeDispatchSetExceptionsToDebugger, NULL, &s_js_runtime) != JsNoError)
		goto on_error;
	if (JsCreateContext(s_js_runtime, &s_js_context) != JsNoError)
		goto on_error;
	JsSetCurrentContext(s_js_context);

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
	JsValueRef value_ref;
	
	iter_t iter;

	iter = vector_enum(s_stack);
	while (iter_next(&iter)) {
		value_ref = *(JsValueRef*)iter.ptr;
		JsRelease(value_ref, NULL);
	}
	vector_free(s_stack);
	vector_free(s_catch_stack);
	JsSetCurrentContext(JS_INVALID_REFERENCE);
	JsDisposeRuntime(s_js_runtime);
}

bool
jsal_call(int num_args)
{
	/* [ ... function this arg1..argN ] -> [ .. retval ] */

	JsValueRef* arguments;
	JsValueRef  error_ref;
	JsValueRef  function_ref;
	int         index;
	JsErrorCode result;
	JsValueRef  retval_ref;

	int i;

	num_args += 1;  // treat `this` as first argument
	arguments = malloc(num_args * sizeof(JsValueRef));
	function_ref = get_ref(-num_args - 1);
	index = vector_len(s_stack) - num_args;
	for (i = 0; i < num_args; ++i)
		arguments[i] = get_ref(i + index);
	result = JsCallFunction(function_ref, arguments, (unsigned short)num_args, &retval_ref);
	jsal_pop(num_args + 1);
	if (result == JsErrorScriptException) {
		JsGetAndClearException(&error_ref);
		throw_ref(error_ref);
	}
	push_ref(retval_ref);
	return true;
}

int
jsal_dup(int from_index)
{
	JsValueRef ref;

	ref = get_ref(from_index);
	return push_ref(ref);
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

bool
jsal_get_boolean(int index)
{
	bool       value;
	JsValueRef value_ref;

	value_ref = get_ref(index);
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

	value_ref = get_ref(at_index);
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

int
jsal_get_int(int index)
{
	int        value;
	JsValueRef value_ref;

	value_ref = get_ref(index);
	if (JsNumberToInt(value_ref, &value) != JsNoError)
		return 0;
	return value;
}

double
jsal_get_number(int index)
{
	double     value;
	JsValueRef value_ref;

	value_ref = get_ref(index);
	if (JsNumberToDouble(value_ref, &value) != JsNoError)
		return NAN;
	return value;
}

bool
jsal_get_property(int object_index)
{
	/* [ ... key ] -> [ ... value ] */

	JsPropertyIdRef key_ref;
	JsValueRef      object_ref;
	JsValueRef      value_ref;

	object_ref = get_ref(object_index);
	key_ref = key_to_prop_id(pop_ref());
	JsGetProperty(object_ref, key_ref, &value_ref);
	push_ref(value_ref);
	return !jsal_is_undefined(-1);
}

bool
jsal_get_index_property(int object_index, int name)
{
	/* [ ... ] -> [ ... value ] */

	object_index = jsal_normalize_index(object_index);
	jsal_push_sprintf("%d", name);
	return jsal_get_property(object_index);
}

bool
jsal_get_named_property(int object_index, const char* name)
{
	/* [ ... ] -> [ ... value ] */

	object_index = jsal_normalize_index(object_index);
	jsal_push_string(name);
	return jsal_get_property(object_index);
}

const char*
jsal_get_string(int index)
{
	static lstring_t* retval = NULL;

	const wchar_t* value;
	size_t         value_length;
	JsValueRef     value_ref;

	value_ref = get_ref(index);
	if (JsStringToPointer(value_ref, &value, &value_length) != JsNoError)
		return NULL;
	lstr_free(retval);
	retval = lstr_from_wide(value, value_length);
	return lstr_cstr(retval);
}

bool
jsal_has_property(int object_index)
{
	/* [ ... key ] -> [ ... ] */
	
	bool            has_property;
	JsPropertyIdRef key_ref;
	JsValueRef      object_ref;

	object_ref = get_ref(object_index);
	key_ref = key_to_prop_id(pop_ref());
	JsHasProperty(object_ref, key_ref, &has_property);
	return has_property;
}

bool
jsal_has_index_property(int object_index, int name)
{
	object_index = jsal_normalize_index(object_index);
	jsal_push_sprintf("%d", name);
	return jsal_has_property(object_index);
}

bool
jsal_has_named_property(int object_index, const char* name)
{
	object_index = jsal_normalize_index(object_index);
	jsal_push_string(name);
	return jsal_has_property(object_index);
}

bool
jsal_insert(int at_index)
{
	/* [ ... value ] -> [ ... value ... ] */
	
	JsValueRef ref;
	
	at_index = jsal_normalize_index(at_index);

	if (at_index == vector_len(s_stack) - 1)
		return true;  // nop
	ref = pop_ref();
	vector_insert(s_stack, at_index, &ref);
	return true;
}

bool
jsal_is_array(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_ref(stack_index);
	JsGetValueType(ref, &type);
	return type == JsArray;
}

bool
jsal_is_boolean(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_ref(stack_index);
	JsGetValueType(ref, &type);
	return type == JsBoolean;
}

bool
jsal_is_buffer(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_ref(stack_index);
	JsGetValueType(ref, &type);
	return type == JsArrayBuffer
		|| type == JsTypedArray;
}

bool
jsal_is_error(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_ref(stack_index);
	JsGetValueType(ref, &type);
	return type == JsError;
}

bool
jsal_is_function(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_ref(stack_index);
	JsGetValueType(ref, &type);
	return type == JsFunction;
}

bool
jsal_is_null(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_ref(stack_index);
	JsGetValueType(ref, &type);
	return type == JsNull;
}

bool
jsal_is_number(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_ref(stack_index);
	JsGetValueType(ref, &type);
	return type == JsNumber;
}

bool
jsal_is_object(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_ref(stack_index);
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

	ref = get_ref(stack_index);
	JsGetValueType(ref, &type);
	return type == JsString;
}

bool
jsal_is_symbol(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_ref(stack_index);
	JsGetValueType(ref, &type);
	return type == JsSymbol;
}

bool
jsal_is_undefined(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_ref(stack_index);
	JsGetValueType(ref, &type);
	return type == JsUndefined;
}

int
jsal_normalize_index(int index)
{
	int real_index;
	int top;
	
	real_index = index;
	top = vector_len(s_stack);
	if (real_index < 0)
		real_index += top - s_stack_base;
	if (real_index < 0 || real_index >= top)
		jsal_error(JS_REF_ERROR, "invalid stack index '%d'", index);
	return real_index;
}

void
jsal_pop(int num_values)
{
	vector_resize(s_stack, vector_len(s_stack) - num_values);
}

int
jsal_push_boolean(bool value)
{
	JsValueRef ref;

	JsBoolToBoolean(value, &ref);
	return push_ref(ref);
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
	return push_ref(ref);
}

int
jsal_push_function(jsal_callback_t callback, const char* name, int min_args)
{
	struct function* function;
	JsValueRef       name_ref;
	JsValueRef       ref;
	
	function = calloc(1, sizeof(struct function));
	function->callback = callback;
	function->min_args = min_args;
	JsCreateString(name, strlen(name), &name_ref);
	JsCreateNamedFunction(name_ref, do_native_call, function, &ref);
	return push_ref(ref);
}

int
jsal_push_global_object(void)
{
	JsValueRef ref;

	JsGetGlobalObject(&ref);
	return push_ref(ref);
}

int
jsal_push_int(int value)
{
	JsValueRef ref;

	JsIntToNumber(value, &ref);
	return push_ref(ref);
}

int
jsal_push_known_symbol(const char* name)
{
	jsal_push_global_object();
	jsal_get_named_property(-1, "Symbol");
	jsal_get_named_property(-1, name);
	jsal_remove(-2);
	jsal_remove(-2);
	return vector_len(s_stack) - 1;
}

int
jsal_push_new_array(void)
{
	JsValueRef ref;

	JsCreateArray(0, &ref);
	return push_ref(ref);
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
	return push_ref(ref);
}

int
jsal_push_new_object(void)
{
	JsValueRef ref;

	JsCreateObject(&ref);
	return push_ref(ref);
}

int
jsal_push_new_symbol(const char* description)
{
	JsValueRef name_ref;
	JsValueRef ref;

	JsCreateString(description, strlen(description), &name_ref);
	JsCreateSymbol(name_ref, &ref);
	return push_ref(ref);
}

int
jsal_push_null(void)
{
	JsValueRef ref;

	JsGetNullValue(&ref);
	return push_ref(ref);
}

int
jsal_push_number(double value)
{
	JsValueRef ref;
	
	JsDoubleToNumber(value, &ref);
	return push_ref(ref);
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
	return push_ref(ref);
}

int
jsal_push_undefined(void)
{
	JsValueRef ref;

	JsGetUndefinedValue(&ref);
	return push_ref(ref);
}

void
jsal_remove(int at_index)
{
	/* [ ... value ... ] -> [ ... ] */

	at_index = jsal_normalize_index(at_index);

	vector_remove(s_stack, at_index);
}

bool
jsal_replace(int at_index)
{
	/* [ ... old_value ... new_value ] -> [ ... new_value ... ] */

	JsValueRef ref;

	at_index = jsal_normalize_index(at_index);

	if (at_index == vector_len(s_stack) - 1)
		return true;  // nop
	ref = pop_ref();
	vector_put(s_stack, at_index, &ref);
	return true;
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
	if (!jsal_is_string(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a string", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
	return jsal_get_string(at_index);
}

bool
jsal_set_property(int object_index)
{
	/* [ ... key value ] -> [ ... ] */

	JsPropertyIdRef key_ref;
	JsValueRef      object_ref;
	JsValueRef      value_ref;

	object_ref = get_ref(object_index);
	value_ref = pop_ref();
	key_ref = key_to_prop_id(pop_ref());
	if (JsSetProperty(object_ref, key_ref, value_ref, false) != JsNoError)
		return false;
	return true;
}

bool
jsal_set_index_property(int object_index, int name)
{
	/* [ ... value ] -> [ ... ] */

	object_index = jsal_normalize_index(object_index);
	jsal_push_sprintf("%d", name);
	jsal_insert(-2);
	return jsal_set_property(object_index);
}

bool
jsal_set_named_property(int object_index, const char* name)
{
	/* [ ... value ] -> [ ... ] */

	object_index = jsal_normalize_index(object_index);
	jsal_push_string(name);
	jsal_insert(-2);
	return jsal_set_property(object_index);
}

void
jsal_throw(void)
{
	/* [ ... exception ] -> [ ... ] */

	JsValueRef ref;

	ref = pop_ref();
	throw_ref(ref);
}

const char*
jsal_to_string(int at_index)
{
	JsValueRef ref;

	at_index = jsal_normalize_index(at_index);
	ref = get_ref(at_index);
	JsConvertValueToString(ref, &ref);
	vector_put(s_stack, at_index, &ref);
	return jsal_get_string(at_index);
}

static void
throw_if_error(void)
{
	JsValueRef error_ref;
	bool       has_exception;

	JsHasException(&has_exception);
	if (has_exception) {
		JsGetAndClearException(&error_ref);
		throw_ref(error_ref);
	}
}

static JsValueRef
get_ref(int stack_index)
{
	JsValueRef ref;

	stack_index = jsal_normalize_index(stack_index);
	ref = *(JsValueRef*)vector_get(s_stack, stack_index + s_stack_base);
	return ref;
}

static JsPropertyIdRef
key_to_prop_id(JsValueRef key_ref)
{
	JsPropertyIdRef ref;
	size_t          key_length;
	const wchar_t*  key_string;
	JsValueType     key_type;

	JsGetValueType(key_ref, &key_type);
	if (key_type == JsSymbol) {
		JsGetPropertyIdFromSymbol(key_ref, &ref);
	}
	else {
		JsStringToPointer(key_ref, &key_string, &key_length);
		JsGetPropertyIdFromName(key_string, &ref);
	}
	return ref;
}

static JsValueRef
pop_ref(void)
{
	int        index;
	JsValueRef ref;

	index = vector_len(s_stack) - 1;
	ref = get_ref(index);
	vector_remove(s_stack, index);
	JsRelease(ref, NULL);
	return ref;
}

static int
push_ref(JsValueRef ref)
{
	JsAddRef(ref, NULL);
	vector_push(s_stack, &ref);
	return vector_len(s_stack) - 1;
}

static void
throw_ref(JsValueRef ref)
{
	int     index;
	jmp_buf label;

	push_ref(ref);
	index = vector_len(s_catch_stack) - 1;
	if (index >= 0) {
		JsSetException(ref);
		memcpy(label, vector_get(s_catch_stack, index), sizeof(jmp_buf));
		vector_remove(s_catch_stack, index);
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
	struct function* function;
	jmp_buf          label;
	int              old_stack_base;
	JsValueRef       retval_ref = JS_INVALID_REFERENCE;

	int i;

	function = userdata;
	old_stack_base = s_stack_base;
	s_stack_base = vector_len(s_stack);
	for (i = 0; i < argc; ++i)
		push_ref(argv[i]);
	if (setjmp(label) == 0) {
		vector_push(s_catch_stack, label);
		if (function->callback(argc - 1, is_ctor) > 0)
			retval_ref = pop_ref();
	}
	else {
		retval_ref = pop_ref();
	}
	s_stack_base = old_stack_base;
	return retval_ref;
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
