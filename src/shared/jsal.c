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

#include <ChakraCore.h>
#include "vector.h"

struct function
{
	jsal_callback_t callback;
	int           min_args;
};

static JsValueRef CHAKRA_CALLBACK do_native_call (JsValueRef callee, bool is_ctor, JsValueRef argv[], unsigned short argc, void* userdata);
static JsValueRef                 get_js_value   (int stack_index);
static JsValueRef                 pop_js_value   (void);
static int                        push_js_value  (JsValueRef ref);

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
	while (vector_next(&iter)) {
		value_ref = *(JsValueRef*)iter.ptr;
		JsRelease(value_ref, NULL);
	}
	vector_free(s_stack);
	JsSetCurrentContext(JS_INVALID_REFERENCE);
	JsDisposeRuntime(s_js_runtime);
}

bool
jsal_call(int num_args)
{
	JsValueRef* arguments;
	JsValueRef  function_ref;
	int         index;
	JsValueRef  retval_ref;

	int i;

	num_args += 1;  // treat `this` as first argument
	arguments = malloc(num_args * sizeof(JsValueRef));
	function_ref = get_js_value(-num_args - 1);
	index = vector_len(s_stack) - num_args;
	for (i = 0; i < num_args; ++i)
		arguments[i] = get_js_value(i + index);
	JsCallFunction(function_ref, arguments, (unsigned short)num_args, &retval_ref);
	jsal_pop(num_args + 1);
	push_js_value(retval_ref);
	return true;
}

bool
jsal_is_array(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_js_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsArray;
}

bool
jsal_is_boolean(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_js_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsBoolean;
}

bool
jsal_is_buffer_object(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_js_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsArrayBuffer
		|| type == JsTypedArray;
}

bool
jsal_is_error(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_js_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsError;
}

bool
jsal_is_number(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_js_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsNumber;
}

bool
jsal_is_object(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_js_value(stack_index);
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
jsal_is_string(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_js_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsString;
}

bool
jsal_is_symbol(int stack_index)
{
	JsValueRef  ref;
	JsValueType type;

	ref = get_js_value(stack_index);
	JsGetValueType(ref, &type);
	return type == JsSymbol;
}

void
jsal_pop(int num_values)
{
	int i;

	for (i = 0; i < num_values; ++i)
		vector_remove(s_stack, vector_len(s_stack) - 1);
}

int
jsal_push_boolean(bool value)
{
	JsValueRef ref;

	JsBoolToBoolean(value, &ref);
	return push_js_value(ref);
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
	return push_js_value(ref);
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
	return push_js_value(ref);
}

int
jsal_push_global_object(void)
{
	JsValueRef ref;

	JsGetGlobalObject(&ref);
	return push_js_value(ref);
}

int
jsal_push_int(int value)
{
	JsValueRef ref;

	JsIntToNumber(value, &ref);
	return push_js_value(ref);
}

int
jsal_push_new_array(void)
{
	JsValueRef ref;

	JsCreateArray(0, &ref);
	return push_js_value(ref);
}

int
jsal_push_new_error(const char* message, jsal_error_type_t type)
{
	JsValueRef  message_ref;
	JsValueRef  ref;
	JsErrorCode result;

	JsCreateString(message, strlen(message), &message_ref);
	result = type == JS_ERROR_RANGE ? JsCreateRangeError(message_ref, &ref)
		: type == JS_ERROR_REF ? JsCreateReferenceError(message_ref, &ref)
		: type == JS_ERROR_SYNTAX ? JsCreateSyntaxError(message_ref, &ref)
		: type == JS_ERROR_TYPE ? JsCreateTypeError(message_ref, &ref)
		: type == JS_ERROR_URI ? JsCreateURIError(message_ref, &ref)
		: JsCreateError(message_ref, &ref);
	return push_js_value(ref);
}

int
jsal_push_new_object(void)
{
	JsValueRef ref;

	JsCreateObject(&ref);
	return push_js_value(ref);
}

int
jsal_push_new_symbol(const char* description)
{
	JsValueRef name_ref;
	JsValueRef ref;

	JsCreateString(description, strlen(description), &name_ref);
	JsCreateSymbol(name_ref, &ref);
	return push_js_value(ref);
}

int
jsal_push_null(void)
{
	JsValueRef ref;

	JsGetNullValue(&ref);
	return push_js_value(ref);
}

int
jsal_push_number(double value)
{
	JsValueRef ref;
	
	JsDoubleToNumber(value, &ref);
	return push_js_value(ref);
}

int
jsal_push_sprintf(const char* format, ...)
{
	static char buffer[1024];

	va_list ap;

	// FIXME: unwanted truncation
	va_start(ap, format);
	vsnprintf(buffer, 1024, format, ap);
	va_end(ap);
	return jsal_push_string(buffer);
}

int
jsal_push_string(const char* value)
{
	JsValueRef ref;

	JsCreateString(value, strlen(value), &ref);
	return push_js_value(ref);
}

int
jsal_push_undefined(void)
{
	JsValueRef ref;

	JsGetUndefinedValue(&ref);
	return push_js_value(ref);
}

void
jsal_throw(void)
{
	JsValueRef ref;

	ref = pop_js_value();
	JsSetException(ref);
}

static JsValueRef
get_js_value(int stack_index)
{
	JsValueRef ref;

	if (stack_index < 0)
		stack_index += vector_len(s_stack);
	ref = *(JsValueRef*)vector_get(s_stack, stack_index);
}

static JsValueRef
pop_js_value(void)
{
	int        index;
	JsValueRef ref;

	index = vector_len(s_stack) - 1;
	ref = get_js_value(index);
	vector_remove(s_stack, index);
	JsRelease(ref, NULL);
	return ref;
}

static int
push_js_value(JsValueRef ref)
{
	JsAddRef(ref, NULL);
	vector_push(s_stack, &ref);
	return vector_len(s_stack) - 1;
}

static JsValueRef CHAKRA_CALLBACK
do_native_call(JsValueRef callee, bool is_ctor, JsValueRef argv[], unsigned short argc, void* userdata)
{
	struct function* function;
	int              old_stack_base;
	JsValueRef       retval_ref;

	int i;

	function = userdata;
	old_stack_base = s_stack_base;
	s_stack_base = vector_len(s_stack);
	for (i = 0; i < argc; ++i)
		push_js_value(argv[i]);
	function->callback(is_ctor);
	s_stack_base = old_stack_base;
}
