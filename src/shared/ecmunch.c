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

#include "ecmunch.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <ChakraCore.h>

struct js_value
{
	unsigned int refcount;
	JsValueRef   ref;
};

struct api_info
{
	js_c_function_t callback;
	int             min_args;
};

static JsValueRef CHAKRA_CALLBACK do_native_call (JsValueRef callee, bool using_new, JsValueRef argv[], unsigned short argc, void* udata);
static js_value_t*                value_from_ref (JsValueRef ref);

JsContextRef    s_context;
js_value_t*     s_global;
JsRuntimeHandle s_runtime = NULL;

bool
js_init(void)
{
	JsValueRef global_ref;
	
	if (JsCreateRuntime(JsRuntimeAttributeDispatchSetExceptionsToDebugger, NULL, &s_runtime) != JsNoError)
		goto on_error;
	if (JsCreateContext(s_runtime, &s_context) != JsNoError)
		goto on_error;
	JsSetCurrentContext(s_context);
	JsGetGlobalObject(&global_ref);
	s_global = value_from_ref(global_ref);
	return true;

on_error:
	if (s_runtime != NULL)
		JsDisposeRuntime(s_runtime);
	s_context = NULL;
	s_runtime = NULL;
	return false;
}

void
js_uninit(void)
{
	js_value_unref(s_global);
	JsSetCurrentContext(JS_INVALID_REFERENCE);
	JsDisposeRuntime(s_runtime);
}

js_value_t*
js_global_object(void)
{
	return s_global;
}

js_value_t*
js_get_exception(void)
{
	JsValueRef ref;
	
	if (JsGetAndClearException(&ref) != JsNoError)
		return NULL;
	return value_from_ref(ref);
}

void
js_set_exception(js_value_t* value)
{
	JsSetException(value->ref);
}

js_value_t*
js_value_new_eval(const lstring_t* source)
{
	const wchar_t* codestring;
	size_t         codestring_len;
	JsValueRef     ref;
	JsValueRef     source_ref;

	JsCreateString(lstr_cstr(source), lstr_len(source), &source_ref);
	JsStringToPointer(source_ref, &codestring, &codestring_len);
	if (JsRunScript(codestring, JS_SOURCE_CONTEXT_NONE, L"inside the pig", &ref) != JsNoError)
		return NULL;
	return value_from_ref(ref);
}

js_value_t*
js_value_new_function(const char* name, js_c_function_t callback, int min_args)
{
	struct api_info* api_info;
	JsValueRef       name_ref;
	JsValueRef       ref;

	if (JsCreateString(name, strlen(name), &name_ref) != JsNoError)
		return NULL;
	api_info = calloc(1, sizeof(struct api_info));
	api_info->callback = callback;
	api_info->min_args = min_args;
	if (JsCreateNamedFunction(name_ref, do_native_call, api_info, &ref))
		return NULL;
	return value_from_ref(ref);
}

js_value_t*
js_value_new_int(int value)
{
	JsValueRef ref;

	if (JsIntToNumber(value, &ref) != JsNoError)
		return NULL;
	return value_from_ref(ref);
}

js_value_t*
js_value_new_number(double value)
{
	JsValueRef ref;
	
	if (JsDoubleToNumber(value, &ref) != JsNoError)
		return NULL;
	return value_from_ref(ref);
}

js_value_t*
js_value_new_error(const char* message)
{
	JsValueRef message_ref;
	JsValueRef ref;

	if (JsCreateString(message, strlen(message), &message_ref) != JsNoError)
		return NULL;
	if (JsCreateError(message_ref, &ref))
		return NULL;
	return value_from_ref(ref);
}

js_value_t*
js_value_new_string(const char* value)
{
	JsValueRef ref;

	if (JsCreateString(value, strlen(value), &ref) != JsNoError)
		return NULL;
	return value_from_ref(ref);
}

js_value_t*
js_value_ref(js_value_t* it)
{
	++it->refcount;
	return it;
}

void
js_value_unref(js_value_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	JsRelease(it->ref, NULL);
	free(it);
}

int
js_value_as_int(const js_value_t* it)
{
	int value;

	if (JsNumberToInt(it->ref, &value) != JsNoError)
		return 0;
	return value;
}

double
js_value_as_number(const js_value_t* it)
{
	double value;
	
	if (JsNumberToDouble(it->ref, &value) != JsNoError)
		return NAN;
	return value;
}

const char*
js_value_as_string(const js_value_t* it)
{
	static lstring_t* retval = NULL;

	const wchar_t* widestr;
	size_t         widestr_len;

	if (JsStringToPointer(it->ref, &widestr, &widestr_len) != JsNoError)
		return NULL;
	if (retval != NULL)
		lstr_free(retval);
	retval = lstr_from_wide(widestr, widestr_len);
	return lstr_cstr(retval);
}

bool
js_value_is_function(const js_value_t* it)
{
	JsValueType type;

	JsGetValueType(it->ref, &type);
	return type == JsFunction;
}

bool
js_value_is_number(const js_value_t* it)
{
	JsValueType type;
	
	JsGetValueType(it->ref, &type);
	return type == JsNumber;
}

bool
js_value_is_object(const js_value_t* it)
{
	JsValueType type;

	JsGetValueType(it->ref, &type);
	return type == JsObject
		|| type == JsArray
		|| type == JsArrayBuffer
		|| type == JsDataView
		|| type == JsError
		|| type == JsFunction
		|| type == JsTypedArray;
}

bool
js_value_is_string(const js_value_t* it)
{
	JsValueType type;

	JsGetValueType(it->ref, &type);
	return type == JsString;
}

js_value_t*
js_value_get(js_value_t* it, const char* name)
{
	size_t          name_len;
	JsValueRef      name_ref;
	const wchar_t*  name_wstr;
	JsPropertyIdRef prop_ref;
	JsValueRef      value_ref;

	JsCreateString(name, strlen(name), &name_ref);
	JsStringToPointer(name_ref, &name_wstr, &name_len);
	JsGetPropertyIdFromName(name_wstr, &prop_ref);
	if (JsGetProperty(it->ref, prop_ref, &value_ref) != JsNoError)
		return NULL;
	return value_from_ref(value_ref);
}

bool
js_value_set(js_value_t* it, const char* name, js_value_t* value)
{
	size_t          name_len;
	JsValueRef      name_ref;
	const wchar_t*  name_wstr;
	JsPropertyIdRef prop_ref;

	JsCreateString(name, strlen(name), &name_ref);
	JsStringToPointer(name_ref, &name_wstr, &name_len);
	JsGetPropertyIdFromName(name_wstr, &prop_ref);
	if (JsSetProperty(it->ref, prop_ref, value->ref, true) != JsNoError)
		return false;
	return true;
}

static JsValueRef CHAKRA_CALLBACK
do_native_call(JsValueRef callee, bool using_new, JsValueRef argv[], unsigned short argc, void* udata)
{
	struct api_info* api_info;
	js_value_t**     arguments;
	js_value_t*      retval;
	js_value_t*      this_value;
	JsValueRef       undefined;

	int i;

	api_info = udata;
	
	if (argc - 1 < api_info->min_args) {
		js_set_exception(js_value_new_error("not enough arguments"));
		return NULL;
	}
	
	this_value = value_from_ref(argv[0]);
	arguments = malloc((argc - 1) * sizeof(js_value_t*));
	for (i = 0; i < argc - 1; ++i)
		arguments[i] = value_from_ref(argv[i + 1]);
	retval = api_info->callback(this_value, argc - 1, arguments, using_new);
	js_value_unref(this_value);
	for (i = 0; i < argc - 1; ++i)
		js_value_unref(arguments[i]);
	free(arguments);
	if (retval != NULL) {
		js_value_unref(retval);
		return retval->ref;
	}
	else {
		JsGetUndefinedValue(&undefined);
		return undefined;
	}
}

static js_value_t*
value_from_ref(JsValueRef ref)
{
	js_value_t* value;

	JsAddRef(ref, NULL);

	value = calloc(1, sizeof(js_value_t));
	value->ref = ref;
	return js_value_ref(value);
}