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

#include "jsal.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <ChakraCore.h>
#include "path.h"
#include "vector.h"

struct api_info
{
	js_callback_t callback;
	int           min_args;
};

static JsValueRef CHAKRA_CALLBACK do_native_call (JsValueRef callee, bool using_new, JsValueRef argv[], unsigned short argc, void* udata);
static JsValueRef                 get_js_value   (int stack_index);
static JsValueRef                 pop_js_value   (void);
static int                        push_js_value  (JsValueRef ref);

static JsContextRef    s_js_context;
static JsRuntimeHandle s_js_runtime = NULL;
static vector_t*       s_value_stack;

bool
js_init(void)
{
	if (JsCreateRuntime(JsRuntimeAttributeDispatchSetExceptionsToDebugger, NULL, &s_js_runtime) != JsNoError)
		goto on_error;
	if (JsCreateContext(s_js_runtime, &s_js_context) != JsNoError)
		goto on_error;
	JsSetCurrentContext(s_js_context);

	s_value_stack = vector_new(sizeof(JsValueRef));
	return true;

on_error:
	if (s_js_runtime != NULL)
		JsDisposeRuntime(s_js_runtime);
	s_js_runtime = NULL;
	return false;
}

void
js_uninit(void)
{
	JsValueRef value_ref;
	
	iter_t iter;

	iter = vector_enum(s_value_stack);
	while (vector_next(&iter)) {
		value_ref = *(JsValueRef*)iter.ptr;
		JsRelease(value_ref, NULL);
	}
	vector_free(s_value_stack);
	JsSetCurrentContext(JS_INVALID_REFERENCE);
	JsDisposeRuntime(s_js_runtime);
}

int
js_push_eval(const char* source, size_t len)
{
	const char* const SCRIPT_NAME = "eval";
	
	JsValueRef name_ref;
	JsValueRef source_ref;
	JsValueRef ref;

	JsCreateString(source, len, &source_ref);
	JsCreateString(SCRIPT_NAME, strlen(SCRIPT_NAME), &name_ref);
	JsRun(source_ref, JS_SOURCE_CONTEXT_NONE, name_ref, JsParseScriptAttributeNone, &ref);
	return push_js_value(ref);
}

int
js_push_global_object(void)
{
	JsValueRef ref;

	JsGetGlobalObject(&ref);
	return push_js_value(ref);
}

int
js_push_int(int value)
{
	JsValueRef ref;

	JsIntToNumber(value, &ref);
	return push_js_value(ref);
}

int
js_push_new_array(void)
{
	JsValueRef ref;

	JsCreateArray(0, &ref);
	return push_js_value(ref);
}

int
js_push_new_error(const char* message, js_error_type_t type)
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
js_push_new_object(void)
{
	JsValueRef ref;

	JsCreateObject(&ref);
	return push_js_value(ref);
}

int
js_push_new_symbol(const char* description)
{
	JsValueRef name_ref;
	JsValueRef ref;

	JsCreateString(description, strlen(description), &name_ref);
	JsCreateSymbol(name_ref, &ref);
	return push_js_value(ref);
}

int
js_push_number(double value)
{
	JsValueRef ref;
	
	JsDoubleToNumber(value, &ref);
	return push_js_value(ref);
}

int
js_push_string(const char* value)
{
	JsValueRef ref;

	JsCreateString(value, strlen(value), &ref);
	return push_js_value(ref);
}

static JsValueRef
get_js_value(int stack_index)
{
	JsValueRef ref;

	if (stack_index < 0)
		stack_index += vector_len(s_value_stack);
	ref = *(JsValueRef*)vector_get(s_value_stack, stack_index);
}

static JsValueRef
pop_js_value(void)
{
	int        index;
	JsValueRef ref;

	index = vector_len(s_value_stack) - 1;
	ref = get_js_value(index);
	vector_remove(s_value_stack, index);
	JsRelease(ref, NULL);
	return ref;
}

static int
push_js_value(JsValueRef ref)
{
	JsAddRef(ref, NULL);
	vector_push(s_value_stack, &ref);
	return vector_len(s_value_stack) - 1;
}