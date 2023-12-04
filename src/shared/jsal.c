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
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#if !defined(_WIN32)
#include <alloca.h>
#else
#include <malloc.h>
#endif

#include <ChakraCore.h>
#include "vector.h"

#if !defined(WIN32)
#define jsal_setjmp(env)          sigsetjmp(env, false)
#define jsal_longjmp(env, value)  siglongjmp(env, value)
#define jsal_jmpbuf               sigjmp_buf
#else
#define jsal_setjmp(env)          setjmp(env)
#define jsal_longjmp(env, value)  longjmp(env, value)
#define jsal_jmpbuf               jmp_buf
#endif

struct js_ref
{
	bool  weak_ref;
	JsRef value;
};

struct breakpoint
{
	int          column;
	char*        filename;
	unsigned int id;
	int          line;
};

struct function
{
	bool          async_flag;
	js_function_t callback;
	bool          ctor_only;
	intptr_t      magic;
	int           min_args;
};

struct module
{
	char*          filename;
	JsModuleRecord record;
};

struct module_job
{
	JsModuleRecord  module_record;
	char*           source;
	JsSourceContext source_context;
	size_t          source_size;
};

struct object
{
	void*          data;
	js_finalizer_t finalizer;
	JsValueRef     object;
};

struct rejection
{
	bool       handled;
	JsValueRef promise;
	JsValueRef value;
};

static void CHAKRA_CALLBACK        on_debugger_event           (JsDiagDebugEvent event_type, JsValueRef data, void* userdata);
static JsErrorCode CHAKRA_CALLBACK on_fetch_dynamic_import     (JsSourceContext importer, JsValueRef specifier, JsModuleRecord *out_module);
static JsErrorCode CHAKRA_CALLBACK on_fetch_imported_module    (JsModuleRecord importer, JsValueRef specifier, JsModuleRecord *out_module);
static void CHAKRA_CALLBACK        on_finalize_host_object     (void* userdata);
static JsValueRef CHAKRA_CALLBACK  on_js_to_native_call        (JsValueRef callee, JsValueRef argv[], unsigned short argc, JsNativeFunctionInfo* env, void* userdata);
static JsErrorCode CHAKRA_CALLBACK on_notify_module_ready      (JsModuleRecord module, JsValueRef exception);
static void CHAKRA_CALLBACK        on_reject_promise_unhandled (JsValueRef promise, JsValueRef reason, bool handled, void* userdata);
static void CHAKRA_CALLBACK        on_report_module_completion (JsModuleRecord module, JsValueRef exception);
static void CHAKRA_CALLBACK        on_resolve_reject_promise   (JsValueRef function, void* userdata);
static void                        decode_debugger_value       (void);
static const char*                 filename_from_script_id     (unsigned int script_id);
static JsModuleRecord              get_module_record           (const char* specifier, JsModuleRecord parent, const char* url, bool *out_is_new);
static js_ref_t*                   get_ref                     (int stack_index);
static JsValueRef                  get_value                   (int stack_index);
static JsPropertyIdRef             make_property_id            (JsValueRef key_value);
static js_ref_t*                   make_ref                    (JsRef value, bool weak_ref);
static JsValueRef                  pop_value                   (void);
static void                        push_debug_callback_args    (JsValueRef event_data);
static unsigned int                script_id_from_filename     (const char* filename);
static int                         push_value                  (JsValueRef value, bool weak_ref);
static void                        resize_stack                (int new_size);
static void                        throw_on_error              (void);
static except_t                    throw_value                 (JsValueRef value);

#if !defined(__APPLE__)
static int asprintf  (char* *out, const char* format, ...);
static int vasprintf (char* *out, const char* format, va_list ap);
#endif

static bool                 s_async_flag = false;
static js_break_callback_t  s_break_callback = NULL;
static vector_t*            s_breakpoints;
static JsValueRef           s_callee_value = JS_INVALID_REFERENCE;
static jsal_jmpbuf*         s_catch_label = NULL;
static js_import_callback_t s_import_callback = NULL;
static js_job_callback_t    s_job_callback = NULL;
static JsContextRef         s_js_context;
static JsValueRef           s_js_false;
static JsValueRef           s_js_null;
static JsRuntimeHandle      s_js_runtime = NULL;
static JsValueRef           s_js_true;
static JsValueRef           s_js_undefined;
static js_ref_t*            s_key_configurable;
static js_ref_t*            s_key_done;
static js_ref_t*            s_key_enumerable;
static js_ref_t*            s_key_get;
static js_ref_t*            s_key_length;
static js_ref_t*            s_key_next;
static js_ref_t*            s_key_set;
static js_ref_t*            s_key_value;
static js_ref_t*            s_key_writable;
static vector_t*            s_module_cache;
static js_module_callback_t s_module_callback = NULL;
static vector_t*            s_module_jobs;
static JsValueRef           s_newtarget_value = JS_INVALID_REFERENCE;
static JsSourceContext      s_next_source_context = 1;
static js_reject_callback_t s_reject_callback = NULL;
static vector_t*            s_rejections;
static int                  s_stack_base;
static JsValueRef           s_stash;
static JsValueRef           s_this_value = JS_INVALID_REFERENCE;
static vector_t*            s_value_stack;
static js_throw_callback_t  s_throw_callback = NULL;

bool
jsal_init(void)
{
	JsErrorCode result;

	result = JsCreateRuntime(
		JsRuntimeAttributeAllowScriptInterrupt
			| JsRuntimeAttributeDispatchSetExceptionsToDebugger
			| JsRuntimeAttributeEnableExperimentalFeatures
			| JsRuntimeAttributeEnableIdleProcessing,
		NULL, &s_js_runtime);
	if (result != JsNoError)
		goto on_error;
	if (JsCreateContext(s_js_runtime, &s_js_context) != JsNoError)
		goto on_error;
	JsSetCurrentContext(s_js_context);
	JsGetUndefinedValue(&s_js_undefined);
	JsGetNullValue(&s_js_null);
	JsGetTrueValue(&s_js_true);
	JsGetFalseValue(&s_js_false);

	// set up the callbacks
	JsSetPromiseContinuationCallback(on_resolve_reject_promise, NULL);
	JsSetHostPromiseRejectionTracker(on_reject_promise_unhandled, NULL);
	JsSetModuleHostInfo(NULL, JsModuleHostInfo_FetchImportedModuleCallback, on_fetch_imported_module);
	JsSetModuleHostInfo(NULL, JsModuleHostInfo_FetchImportedModuleFromScriptCallback, on_fetch_dynamic_import);
	JsSetModuleHostInfo(NULL, JsModuleHostInfo_NotifyModuleReadyCallback, on_notify_module_ready);
	JsSetModuleHostInfo(NULL, JsModuleHostInfo_ReportModuleCompletionCallback, on_report_module_completion);

	// set up the stash, used to store JS values behind the scenes.
	JsCreateObject(&s_stash);
	JsAddRef(s_stash, NULL);

	s_value_stack = vector_new(sizeof(js_ref_t));
	s_stack_base = 0;
	s_breakpoints = vector_new(sizeof(struct breakpoint));
	s_module_cache = vector_new(sizeof(struct module));
	s_module_jobs = vector_new(sizeof(struct module_job));
	s_rejections = vector_new(sizeof(struct rejection));

	vector_reserve(s_value_stack, 128);

	s_key_configurable = jsal_new_key("configurable");
	s_key_done = jsal_new_key("done");
	s_key_enumerable = jsal_new_key("enumerable");
	s_key_get = jsal_new_key("get");
	s_key_length = jsal_new_key("length");
	s_key_next = jsal_new_key("next");
	s_key_set = jsal_new_key("set");
	s_key_value = jsal_new_key("value");
	s_key_writable = jsal_new_key("writable");

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
	struct breakpoint* breakpoint;
	struct module*     module;

	iter_t iter;

	jsal_unref(s_key_configurable);
	jsal_unref(s_key_done);
	jsal_unref(s_key_enumerable);
	jsal_unref(s_key_get);
	jsal_unref(s_key_length);
	jsal_unref(s_key_next);
	jsal_unref(s_key_set);
	jsal_unref(s_key_value);
	jsal_unref(s_key_writable);

	iter = vector_enum(s_breakpoints);
	while (iter_next(&iter)) {
		breakpoint = iter.ptr;
		free(breakpoint->filename);
	}

	iter = vector_enum(s_module_cache);
	while ((module = iter_next(&iter))) {
		JsRelease(module->record, NULL);
		free(module->filename);
	}

	// clear value stack, releasing all references
	resize_stack(0);

	vector_free(s_breakpoints);
	vector_free(s_module_cache);
	vector_free(s_module_jobs);
	vector_free(s_value_stack);
	vector_free(s_rejections);
	JsRelease(s_stash, NULL);
	JsSetCurrentContext(JS_INVALID_REFERENCE);
	JsDisposeRuntime(s_js_runtime);
}

void
jsal_update(bool in_event_loop)
{
	JsValueRef         exception;
	bool               have_error;
	struct module_job* job;
	jsal_jmpbuf        label;
	jsal_jmpbuf*       last_catch_label;
	int                last_stack_base;
	JsModuleRecord     module_record;
	struct rejection*  rejection;
	JsValueRef         result;
	char*              source;

	iter_t iter;

	// parse ES modules.  the whole dependency graph is loaded in one fell swoop; doing so is
	// safe so long as we avoid calling JsParseModuleSource() recursively.
	while (vector_len(s_module_jobs) > 0) {
		job = vector_get(s_module_jobs, 0);
		if (job->source != NULL) {
			// module parse job: parse and compile an imported module
			source = job->source;  // ...because 'job' may be invalidated
			JsParseModuleSource(job->module_record, job->source_context,
				(BYTE*)job->source, (unsigned int)job->source_size,
				JsParseModuleSourceFlags_DataIsUTF8, &exception);
			free(source);
			vector_remove(s_module_jobs, 0);
		}
		else {
			// module evaluation job: execute a top-level or dynamic module.  this is tricky
			// because a module can call FlipScreen() or DoEvents() at load time.  to avoid
			// corrupting the queue or evaluating a module more than once, dequeue the job first.
			module_record = job->module_record;
			vector_remove(s_module_jobs, 0);
			JsModuleEvaluation(module_record, &result);
			if (!in_event_loop)
				throw_on_error();
			JsHasException(&have_error);
			if (have_error) {
				JsGetAndClearException(&exception);
				JsSetModuleHostInfo(module_record, JsModuleHostInfo_Exception, exception);
			}
		}
	}

	if (in_event_loop) {
		JsIdle(NULL);
		
		// check for broken promises.  if there are any uncaught rejections, throw the rejection value
		// as an exception out of the event loop.  this avoids errors in asynchronous code getting eaten
		// (most likely by the pig).
		exception = JS_INVALID_REFERENCE;
		iter = vector_enum(s_rejections);
		while ((rejection = iter_next(&iter))) {
			if (rejection->handled) {  // does it have a .then()?
				JsRelease(rejection->promise, NULL);
				continue;
			}
			last_catch_label = s_catch_label;
			last_stack_base = s_stack_base;
			s_stack_base = vector_len(s_value_stack);
			push_value(rejection->promise, true);
			push_value(rejection->value, true);
			if (jsal_setjmp(label) == 0) {
				s_catch_label = &label;
				if (s_reject_callback != NULL) {
					if (!s_reject_callback() && exception == JS_INVALID_REFERENCE)
						exception = rejection->value;
				}
			}
			else {
				resize_stack(s_stack_base);
				s_stack_base = last_stack_base;
				jsal_throw();  // rethrow
			}
			resize_stack(s_stack_base);
			s_catch_label = last_catch_label;
			s_stack_base = last_stack_base;
			JsRelease(rejection->promise, NULL);
		}
		vector_clear(s_rejections);
		if (exception != JS_INVALID_REFERENCE) {
			push_value(exception, false);
			jsal_throw();
		}
	}
}

bool
jsal_busy(void)
{
	return vector_len(s_module_jobs) > 0
		|| vector_len(s_rejections) > 0;
}

bool
jsal_vm_enabled(void)
{
	bool disabled;

	JsIsRuntimeExecutionDisabled(s_js_runtime, &disabled);
	return !disabled;
}

void
jsal_on_enqueue_job(js_job_callback_t callback)
{
	s_job_callback = callback;
}

void
jsal_on_import_module(js_import_callback_t callback)
{
	s_import_callback = callback;
}

void
jsal_on_module_complete(js_module_callback_t callback)
{
	/* callback stack: [ namespace ] */

	s_module_callback = callback;
}

void
jsal_on_reject_promise(js_reject_callback_t callback)
{
	s_reject_callback = callback;
}

void
jsal_call(int num_args)
{
	/* [ ... function arg1..argN ] -> [ ... retval ] */

	jsal_push_undefined();
	if (num_args > 0)
		jsal_insert(-num_args - 1);
	jsal_call_method(num_args);
}

void
jsal_call_method(int num_args)
{
	/* [ ... function this arg1..argN ] -> [ ... retval ] */

	JsValueRef* arguments;
	JsValueRef  function_ref;
	int         offset;
	JsValueRef  retval_ref;

	int i;

	num_args += 1;  // treat 'this' as first argument
	function_ref = get_value(-num_args - 1);
	offset = -num_args;
	arguments = alloca(num_args * sizeof(JsValueRef));
	for (i = 0; i < num_args; ++i)
		arguments[i] = get_value(i + offset);
	JsCallFunction(function_ref, arguments, (unsigned short)num_args, &retval_ref);
	jsal_pop(num_args + 1);
	throw_on_error();
	push_value(retval_ref, false);
}

unsigned int
jsal_compile(const char* filename)
{
	/* [ ... source ] -> [ ... function ] */

	JsValueRef function;
	JsValueRef name_string;
	JsValueRef source_string;

	source_string = pop_value();
	JsCreateString(filename, strlen(filename), &name_string);
	JsParse(source_string, s_next_source_context, name_string, JsParseScriptAttributeNone, &function);
	throw_on_error();
	push_value(function, false);
	return (unsigned int)s_next_source_context++;
}

void
jsal_construct(int num_args)
{
	/* [ ... constructor arg1..argN ] -> [ ... retval ] */

	JsValueRef* arguments;
	JsValueRef  function_ref;
	int         offset;
	JsValueRef  retval_ref;

	int i;

	function_ref = get_value(-num_args - 1);
	offset = -num_args;
	arguments = alloca((num_args + 1) * sizeof(JsValueRef));
	arguments[0] = s_js_undefined;
	for (i = 0; i < num_args; ++i)
		arguments[i + 1] = get_value(i + offset);
	JsConstructObject(function_ref, arguments, (unsigned short)(num_args + 1), &retval_ref);
	jsal_pop(num_args + 1);
	throw_on_error();
	push_value(retval_ref, false);
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
	throw_on_error();
}

void
jsal_def_prop_index(int object_index, int name)
{
	/* [ ... descriptor ] -> [ ... ] */

	JsValueRef descriptor;
	JsValueRef key;
	JsValueRef object;
	bool       result;

	object = get_value(object_index);
	descriptor = pop_value();
	JsIntToNumber(name, &key);
	JsConvertValueToString(key, &key);
	JsObjectDefineProperty(object, key, descriptor, &result);
	throw_on_error();
}

void
jsal_def_prop_string(int object_index, const char* name)
{
	/* [ ... descriptor ] -> [ ... ] */

	JsValueRef descriptor;
	JsValueRef key;
	JsValueRef object;
	bool       result;

	object = get_value(object_index);
	descriptor = pop_value();
	JsCreateString(name, strlen(name), &key);
	JsObjectDefineProperty(object, key, descriptor, &result);
	throw_on_error();
}

bool
jsal_del_global(void)
{
	/* [ ... key ] -> [ ... ] */

	JsPropertyIdRef key_ref;
	JsValueRef      object_ref;
	JsValueRef      result;
	bool            retval;

	JsGetGlobalObject(&object_ref);
	key_ref = make_property_id(pop_value());
	JsDeleteProperty(object_ref, key_ref, true, &result);
	throw_on_error();
	JsBooleanToBool(result, &retval);
	return retval;
}

bool
jsal_del_global_string(const char* name)
{
	jsal_push_string(name);
	return jsal_del_global();
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
	throw_on_error();
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
	js_ref_t* ref;

	ref = get_ref(from_index);
	return push_value(ref->value, ref->weak_ref);
}

void
jsal_enable_vm(bool enabled)
{
	if (enabled)
		JsEnableRuntimeExecution(s_js_runtime);
	else
		JsDisableRuntimeExecution(s_js_runtime);
}

except_t
jsal_error(js_error_type_t type, const char* format, ...)
{
	va_list ap;

	va_start(ap, format);
	jsal_error_va(type, format, ap);
	va_end(ap);
}

except_t
jsal_error_va(js_error_type_t type, const char* format, va_list ap)
{
	jsal_push_new_error_va(type, format, ap);
	jsal_throw();
}

void
jsal_eval_module(const char* specifier, const char* url)
{
	/* [ ... source ] -> [ ... exports ] */

	JsValueRef     exception;
	bool           is_new_module;
	JsModuleRecord module;
	JsValueRef     namespace;
	const char*    source;
	size_t         source_len;

	if (url == NULL)
		url = specifier;

	source = jsal_require_lstring(-1, &source_len);
	module = get_module_record(specifier, NULL, url, &is_new_module);
	if (is_new_module) {
		JsParseModuleSource(module,
			s_next_source_context++, (BYTE*)source, (unsigned int)source_len,
			JsParseModuleSourceFlags_DataIsUTF8, &exception);
	}
	jsal_pop(1);

	// note: a single call to jsal_update() here is enough, as it will process
	//       the entire dependency graph before returning.
	jsal_update(false);

	JsGetModuleHostInfo(module, JsModuleHostInfo_Exception, &exception);
	if (exception != JS_INVALID_REFERENCE)
		throw_value(exception);

	JsGetModuleNamespace(module, &namespace);
	push_value(namespace, true);
}

void
jsal_exec(void)
{
	/* [ ... source ] -> [ ... result ] */

	jsal_compile("%/jsal_exec.js");
	jsal_call(0);
}

void
jsal_freeze(int at_index)
{
	at_index = jsal_normalize_index(at_index);
	jsal_get_global_string("Object");
	jsal_get_prop_string(-1, "freeze");
	jsal_pull(-2);
	jsal_dup(at_index);
	jsal_call_method(1);
	jsal_pop(1);
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
jsal_get_buffer_ptr(int at_index, size_t *out_size)
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
	else if (type == JsDataView)
		JsGetDataViewStorage(value_ref, &value, &size);
	else
		return NULL;
	if (out_size != NULL)
		*out_size = size;
	return value;
}

bool
jsal_get_global(void)
{
	/* [ ... key ] -> [ ... value ] */

	JsValueRef key;
	JsValueRef object;
	JsValueRef value;

	key = pop_value();
	JsGetGlobalObject(&object);
	JsObjectGetProperty(object, key, &value);
	throw_on_error();
	push_value(value, true);
	return value != s_js_undefined;
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

	jsal_get_prop_key(at_index, s_key_length);
	value = jsal_get_int(-1);
	jsal_pop(1);
	return value;
}

const char*
jsal_get_lstring(int index, size_t *out_length)
{
	static int   counter = 0;
	static char* retval[25];
	static char  retval_cache[25][256];

	char*      buffer;
	size_t     length;
	JsValueRef value;

	value = get_value(index);
	if (JsCopyString(value, NULL, 0, &length) != JsNoError)
		return NULL;
	if (out_length != NULL)
		*out_length = length;
	if (length < sizeof retval_cache[0]) {
		buffer = retval_cache[counter];
		JsCopyString(value, buffer, sizeof retval_cache[counter], NULL);
		counter = (counter + 1) % 25;
	}
	else {
		if (!(buffer = malloc(length + 1)))
			return NULL;
		JsCopyString(value, buffer, length + 1, NULL);
		free(retval[counter]);
		retval[counter] = buffer;
		counter = (counter + 1) % 25;
	}
	buffer[length] = '\0';  // NUL terminator
	return buffer;
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

	JsPropertyIdRef key;
	JsValueRef      key_value;
	js_ref_t*       object_ref;
	JsValueRef      value;

	object_ref = get_ref(object_index);
	if (jsal_is_number(-1)) {
		key_value = pop_value();
		JsGetIndexedProperty(object_ref->value, key_value, &value);
	}
	else {
		key = make_property_id(pop_value());
		JsGetProperty(object_ref->value, key, &value);
	}
	throw_on_error();
	push_value(value, object_ref->weak_ref);
	return value != s_js_undefined;
}

bool
jsal_get_prop_index(int object_index, int name)
{
	/* [ ... ] -> [ ... value ] */

	JsValueRef index;
	js_ref_t*  object_ref;
	JsValueRef value;

	object_ref = get_ref(object_index);
	JsIntToNumber(name, &index);
	JsGetIndexedProperty(object_ref->value, index, &value);
	throw_on_error();
	push_value(value, object_ref->weak_ref);
	return value != s_js_undefined;
}

bool
jsal_get_prop_key(int object_index, js_ref_t* key)
{
	/* [ ... ] -> [ ... value ] */

	js_ref_t*  object_ref;
	JsValueRef value;

	object_ref = get_ref(object_index);
	JsGetProperty(object_ref->value, key->value, &value);
	throw_on_error();
	push_value(value, object_ref->weak_ref);
	return value != s_js_undefined;
}

bool
jsal_get_prop_string(int object_index, const char* name)
{
	/* [ ... ] -> [ ... value ] */

	JsPropertyIdRef key;
	js_ref_t*       object_ref;
	JsValueRef      value;

	object_ref = get_ref(object_index);
	JsCreatePropertyId(name, strlen(name), &key);
	JsGetProperty(object_ref->value, key, &value);
	throw_on_error();
	push_value(value, object_ref->weak_ref);
	return value != s_js_undefined;
}

void
jsal_get_prototype(int object_index)
{
	/* [ ... ] -> [ ... prototype ] */

	JsValueRef object;
	JsValueRef prototype;

	object = get_value(object_index);
	JsGetPrototype(object, &prototype);
	throw_on_error();
	push_value(prototype, false);
}

const char*
jsal_get_string(int index)
{
	return jsal_get_lstring(index, NULL);
}

int
jsal_get_top(void)
{
	return vector_len(s_value_stack) - s_stack_base;
}

unsigned int
jsal_get_uint(int index)
{
	int        value;
	JsValueRef value_ref;

	value_ref = get_value(index);
	if (JsNumberToInt(value_ref, &value) != JsNoError)
		return 0;
	return (unsigned int)value;
}

bool
jsal_has_own_prop(int object_index)
{
	/* [ ... key ] -> [ ... ] */

	bool            has_property;
	JsPropertyIdRef key_ref;
	JsValueRef      object_ref;

	object_ref = get_value(object_index);
	key_ref = make_property_id(pop_value());
	JsHasOwnProperty(object_ref, key_ref, &has_property);
	return has_property;
}

bool
jsal_has_own_prop_index(int object_index, int name)
{
	object_index = jsal_normalize_index(object_index);
	jsal_push_sprintf("%d", name);
	return jsal_has_own_prop(object_index);
}

bool
jsal_has_own_prop_string(int object_index, const char* name)
{
	object_index = jsal_normalize_index(object_index);
	jsal_push_string(name);
	return jsal_has_own_prop(object_index);
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
jsal_has_prop_key(int object_index, js_ref_t* key)
{
	bool       has_property;
	JsValueRef object_ref;

	object_ref = get_value(object_index);
	JsHasProperty(object_ref, key->value, &has_property);
	return has_property;
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

	js_ref_t ref;

	at_index = jsal_normalize_index(at_index);

	if (at_index == jsal_get_top() - 1)
		return;  // nop
	ref = *(js_ref_t*)vector_get(s_value_stack, vector_len(s_value_stack) - 1);
	vector_insert(s_value_stack, at_index + s_stack_base, &ref);
	vector_pop(s_value_stack, 1);
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
jsal_is_async_call(void)
{
	return s_async_flag;
}

bool
jsal_is_async_function(int stack_index)
{
	bool        is_async = false;
	const char* name;

	if (!jsal_is_function(stack_index))
		return false;
	jsal_get_prop_string(stack_index, "constructor");
	if (!jsal_is_object(-1)) {
		jsal_pop(1);
		return false;
	}
	jsal_get_prop_string(-1, "name");
	if ((name = jsal_get_string(-1)))
		is_async = strcmp(name, "AsyncFunction") == 0;
	jsal_pop(2);
	return is_async;
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
		|| type == JsTypedArray
		|| type == JsDataView;
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
	JsValueRef value;

	value = get_value(stack_index);
	return value == s_js_null;
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
	JsValueRef value;

	value = get_value(at_index);
	return value != s_js_undefined
		&& value != s_js_null;
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
jsal_is_subclass_ctor(void)
{
	return s_newtarget_value != s_callee_value;
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
	JsValueRef  value;

	value = get_value(stack_index);
	return value == s_js_undefined;
}

void
jsal_make_buffer(int object_index, js_buffer_type_t buffer_type, void* buffer, size_t num_items)
{
	JsValueRef       object;
	JsTypedArrayType type;

	object = get_value(object_index);
	type = buffer_type == JS_UINT8ARRAY ? JsArrayTypeUint8
		: buffer_type == JS_UINT8ARRAY_CLAMPED ? JsArrayTypeUint8Clamped
		: buffer_type == JS_UINT16ARRAY ? JsArrayTypeUint16
		: buffer_type == JS_UINT32ARRAY ? JsArrayTypeUint32
		: buffer_type == JS_INT8ARRAY ? JsArrayTypeInt8
		: buffer_type == JS_INT16ARRAY ? JsArrayTypeInt16
		: buffer_type == JS_INT32ARRAY ? JsArrayTypeInt32
		: buffer_type == JS_FLOAT32ARRAY ? JsArrayTypeFloat32
		: buffer_type == JS_FLOAT64ARRAY ? JsArrayTypeFloat64
		: JsArrayTypeUint8;
	JsSetIndexedPropertiesToExternalData(object, buffer, type, (unsigned int)num_items);
}

js_ref_t*
jsal_new_key(const char* name)
{
	JsPropertyIdRef key;

	JsCreatePropertyId(name, strlen(name), &key);
	return make_ref(key, false);
}

bool
jsal_next(int iter_index)
{
	bool finished;

	iter_index = jsal_normalize_index(iter_index);
	jsal_get_prop_key(iter_index, s_key_next);
	jsal_dup(iter_index);
	jsal_call_method(0);
	jsal_get_prop_key(-1, s_key_done);
	finished = jsal_to_boolean(-1);
	jsal_pop(1);
	if (!finished) {
		jsal_get_prop_key(-1, s_key_value);
		jsal_remove(-2);
		return true;
	}
	else {
		jsal_pop(1);
		return false;
	}
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
		jsal_error(JS_REF_ERROR, "Invalid stack index '%d'", index);
	return real_index;
}

void
jsal_parse(int at_index)
{
	at_index = jsal_normalize_index(at_index);
	jsal_get_global_string("JSON");
	jsal_get_prop_string(-1, "parse");
	jsal_pull(-2);
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
		jsal_error(JS_RANGE_ERROR, "Too many values popped ('%d')", num_values);
	jsal_set_top(top - num_values);
}

js_ref_t*
jsal_pop_ref(void)
{
	js_ref_t* ref;

	ref = jsal_ref(-1);
	jsal_pop(1);
	return ref;
}

int
jsal_push_boolean(bool value)
{
	return push_value(value ? s_js_true : s_js_false, true);
}

int
jsal_push_boolean_false(void)
{
	return push_value(s_js_false, true);
}

int
jsal_push_boolean_true(void)
{
	return push_value(s_js_true, true);
}

int
jsal_push_callee(void)
{
	return push_value(s_callee_value, true);
}


int
jsal_push_global_object(void)
{
	JsValueRef object;

	JsGetGlobalObject(&object);
	return push_value(object, true);
}

int
jsal_push_hidden_stash(void)
{
	return push_value(s_stash, true);
}

int
jsal_push_int(int value)
{
	JsValueRef ref;

	JsDoubleToNumber((double)value, &ref);
	return push_value(ref, false);
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
	return push_value(ref, false);
}

int
jsal_push_new_array(void)
{
	JsValueRef ref;

	JsCreateArray(0, &ref);
	return push_value(ref, false);
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
jsal_push_new_buffer(js_buffer_type_t type, size_t size, void* *out_data_ptr)
{
	JsValueRef  buffer_obj;
	JsErrorCode result;
	int         stack_index;

	result = type == JS_INT8ARRAY ? JsCreateTypedArray(JsArrayTypeInt8, JS_INVALID_REFERENCE, 0, (unsigned int)size, &buffer_obj)
		: type == JS_INT16ARRAY ? JsCreateTypedArray(JsArrayTypeInt16, JS_INVALID_REFERENCE, 0, (unsigned int)size, &buffer_obj)
		: type == JS_INT32ARRAY ? JsCreateTypedArray(JsArrayTypeInt32, JS_INVALID_REFERENCE, 0, (unsigned int)size, &buffer_obj)
		: type == JS_UINT8ARRAY ? JsCreateTypedArray(JsArrayTypeUint8, JS_INVALID_REFERENCE, 0, (unsigned int)size, &buffer_obj)
		: type == JS_UINT8ARRAY_CLAMPED ? JsCreateTypedArray(JsArrayTypeUint8Clamped, JS_INVALID_REFERENCE, 0, (unsigned int)size, &buffer_obj)
		: type == JS_UINT16ARRAY ? JsCreateTypedArray(JsArrayTypeUint16, JS_INVALID_REFERENCE, 0, (unsigned int)size, &buffer_obj)
		: type == JS_UINT32ARRAY ? JsCreateTypedArray(JsArrayTypeUint32, JS_INVALID_REFERENCE, 0, (unsigned int)size, &buffer_obj)
		: type == JS_FLOAT32ARRAY ? JsCreateTypedArray(JsArrayTypeFloat32, JS_INVALID_REFERENCE, 0, (unsigned int)size, &buffer_obj)
		: type == JS_FLOAT64ARRAY ? JsCreateTypedArray(JsArrayTypeFloat64, JS_INVALID_REFERENCE, 0, (unsigned int)size, &buffer_obj)
		: JsCreateArrayBuffer((unsigned int)size, &buffer_obj);
	stack_index = push_value(buffer_obj, false);
	if (out_data_ptr != NULL)
		*out_data_ptr = jsal_get_buffer_ptr(stack_index, NULL);
	return stack_index;
}

int
jsal_push_new_constructor(js_function_t callback, const char* name, int min_args, intptr_t magic)
{
	JsValueRef       function;
	struct function* function_data;
	JsValueRef       name_string;

	function_data = calloc(1, sizeof(struct function));
	function_data->callback = callback;
	function_data->ctor_only = true;
	function_data->magic = magic;
	function_data->min_args = min_args;
	JsCreateString(name, strlen(name), &name_string);
	JsCreateEnhancedFunction(on_js_to_native_call, name_string, function_data, &function);
	return push_value(function, false);
}

int
jsal_push_new_error(js_error_type_t type, const char* format, ...)
{
	va_list ap;
	int     index;

	va_start(ap, format);
	index = jsal_push_new_error_va(type, format, ap);
	va_end(ap);
	return index;
}

int
jsal_push_new_error_va(js_error_type_t type, const char* format, va_list ap)
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
	return push_value(ref, false);
}

int
jsal_push_new_function(js_function_t callback, const char* name, int min_args, bool async, intptr_t magic)
{
	JsValueRef       function;
	struct function* function_data;
	JsValueRef       name_string;

	function_data = calloc(1, sizeof(struct function));
	function_data->async_flag = async;
	function_data->callback = callback;
	function_data->magic = magic;
	function_data->min_args = min_args;
	JsCreateString(name, strlen(name), &name_string);
	JsCreateEnhancedFunction(on_js_to_native_call, name_string, function_data, &function);
	return push_value(function, false);
}

int
jsal_push_new_host_object(js_finalizer_t finalizer, size_t data_size, void* *out_data_ptr)
{
	/* [ ... prototype ] -> [ ... ] */

	void*          data_ptr;
	JsValueRef     object;
	struct object* object_info;
	JsValueRef     prototype;

	prototype = pop_value();

	object_info = calloc(1, sizeof(struct object) + data_size);
	data_ptr = &object_info[1];
	if (out_data_ptr != NULL)
		*out_data_ptr = data_ptr;

	JsCreateExternalObjectWithPrototype(object_info, on_finalize_host_object, prototype, &object);
	object_info->data = data_ptr;
	object_info->finalizer = finalizer;
	object_info->object = object;
	return push_value(object, false);
}

int
jsal_push_new_iterator(int for_index)
{
	/* [ ... list_obj ... ] -> [ ... list_obj ... iter ] */
	
	JsValueRef key_list;
	JsValueRef object;

	for_index = jsal_normalize_index(for_index);
	jsal_push_known_symbol("iterator");
	if (jsal_get_prop(for_index)) {
		jsal_dup(for_index);
		jsal_call_method(0);
	}
	else {
		jsal_pop(1);
		object = get_value(for_index);
		JsGetOwnPropertyNames(object, &key_list);
		push_value(key_list, true);
		jsal_push_new_iterator(-1);
		jsal_remove(-2);
	}
	return jsal_get_top() - 1;
}

int
jsal_push_new_object(void)
{
	JsValueRef ref;

	JsCreateObject(&ref);
	return push_value(ref, false);
}

int
jsal_push_new_promise(js_ref_t* *out_resolver, js_ref_t* *out_rejector)
{
	JsValueRef promise;
	JsValueRef rejector;
	JsValueRef resolver;

	JsCreatePromise(&promise, &resolver, &rejector);
	if (out_resolver != NULL)
		*out_resolver = make_ref(resolver, false);
	if (out_rejector != NULL)
		*out_rejector = make_ref(rejector, false);
	return push_value(promise, false);
}

int
jsal_push_new_symbol(const char* description)
{
	JsValueRef name_ref;
	JsValueRef ref;

	JsCreateString(description, strlen(description), &name_ref);
	JsCreateSymbol(name_ref, &ref);
	return push_value(ref, false);
}

int
jsal_push_newtarget(void)
{
	if (s_newtarget_value == JS_INVALID_REFERENCE)
		jsal_error(JS_REF_ERROR, "No known 'new.target' binding");

	// it's safe for this to be a weak reference: `new.target` can't be garbage collected
	// while the function using it runs and anything pushed onto the value stack
	// is unwound on return, so the stack entry can't persist beyond that point by
	// definition.
	return push_value(s_newtarget_value, true);
}

int
jsal_push_null(void)
{
	return push_value(s_js_null, true);
}

int
jsal_push_number(double value)
{
	JsValueRef ref;

	JsDoubleToNumber(value, &ref);
	return push_value(ref, false);
}

int
jsal_push_ref(const js_ref_t* ref)
{
	return push_value(ref->value, false);
}

int
jsal_push_ref_weak(const js_ref_t* ref)
{
	// IMPORTANT: value is stored on the value stack as a weak reference.  to avoid a
	//            nasty segfault, avoid calling jsal_unref() until you know for sure the
	//            value is off the stack.

	return push_value(ref->value, true);
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
	JsValueRef string_value;

	JsCreateString(value, strlen(value), &string_value);
	return push_value(string_value, false);
}

int
jsal_push_this(void)
{
	if (s_this_value == JS_INVALID_REFERENCE)
		jsal_error(JS_REF_ERROR, "No known 'this' binding");

	// it's safe for this to be a weak reference: `this` can't be garbage collected
	// while the function using it runs and anything pushed onto the value stack
	// is unwound on return, so the stack entry can't persist beyond that point by
	// definition.
	return push_value(s_this_value, true);
}

int
jsal_push_uint(unsigned int value)
{
	JsValueRef ref;

	JsDoubleToNumber((double)value, &ref);
	return push_value(ref, false);
}

int
jsal_push_undefined(void)
{
	return push_value(s_js_undefined, true);
}

void
jsal_pull(int from_index)
{
	js_ref_t ref;

	from_index = jsal_normalize_index(from_index);
	ref = *(js_ref_t*)vector_get(s_value_stack, from_index + s_stack_base);
	vector_push(s_value_stack, &ref);
	vector_remove(s_value_stack, from_index + s_stack_base);
}

void
jsal_put_prop(int object_index)
{
	/* [ ... key value ] -> [ ... ] */

	JsPropertyIdRef key;
	JsValueRef      key_value;
	JsValueRef      object;
	JsValueRef      value;

	object = get_value(object_index);
	value = pop_value();
	if (jsal_is_number(-1)) {
		key_value = pop_value();
		JsSetIndexedProperty(object, key_value, value);
	}
	else {
		key = make_property_id(pop_value());
		JsSetProperty(object, key, value, true);
	}
	throw_on_error();
}

void
jsal_put_prop_index(int object_index, int name)
{
	/* [ ... value ] -> [ ... ] */

	JsValueRef index;
	JsValueRef object;
	JsValueRef value;

	object = get_value(object_index);
	value = pop_value();
	JsIntToNumber(name, &index);
	JsSetIndexedProperty(object, index, value);
	throw_on_error();
}

void
jsal_put_prop_key(int object_index, js_ref_t* key)
{
	/* [ ... value ] -> [ ... ] */

	JsValueRef      object;
	JsValueRef      value;

	object = get_value(object_index);
	value = pop_value();
	JsSetProperty(object, key->value, value, true);
	throw_on_error();
}

void
jsal_put_prop_string(int object_index, const char* name)
{
	/* [ ... value ] -> [ ... ] */

	JsPropertyIdRef key;
	JsValueRef      object;
	JsValueRef      value;

	object = get_value(object_index);
	value = pop_value();
	JsCreatePropertyId(name, strlen(name), &key);
	JsSetProperty(object, key, value, true);
	throw_on_error();
}

js_ref_t*
jsal_ref(int at_index)
{
	js_ref_t*  ref;
	js_ref_t*  stack_ref;

	if (!(ref = calloc(1, sizeof(js_ref_t))))
		return NULL;

	// IMPORTANT: stack entry becomes a weak reference after this; to avoid a segfault, make
	//            sure the value is off the stack before calling jsal_unref().
	stack_ref = get_ref(at_index);
	if (stack_ref->weak_ref)
		JsAddRef(stack_ref->value, NULL);  // stack ref is weak, pin it
	stack_ref->weak_ref = true;

	ref->value = stack_ref->value;
	return ref;
}

void
jsal_remove(int at_index)
{
	/* [ ... value ... ] -> [ ... ] */

	js_ref_t* ref;

	at_index = jsal_normalize_index(at_index);
	ref = vector_get(s_value_stack, at_index + s_stack_base);
	if (!ref->weak_ref)
		JsRelease(ref->value, NULL);
	vector_remove(s_value_stack, at_index + s_stack_base);
}

bool
jsal_replace(int at_index)
{
	/* [ ... old_value ... new_value ] -> [ ... new_value ... ] */

	js_ref_t* old_ref;
	js_ref_t* ref;

	at_index = jsal_normalize_index(at_index);

	if (at_index == jsal_get_top() - 1)
		return true;  // nop
	old_ref = vector_get(s_value_stack, at_index + s_stack_base);
	ref = vector_get(s_value_stack, vector_len(s_value_stack) - 1);
	if (!old_ref->weak_ref)
		JsRelease(old_ref->value, NULL);
	vector_put(s_value_stack, at_index + s_stack_base, ref);
	vector_pop(s_value_stack, 1);
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
jsal_require_buffer_ptr(int at_index, size_t *out_size)
{
	void* retval;

	if (!jsal_is_buffer(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a buffer", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
	retval = jsal_get_buffer_ptr(at_index, out_size);
	return retval;
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
	const char* retval;

	if (!(retval = jsal_get_lstring(at_index, out_length))) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a string", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
	return retval;
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
	return jsal_require_lstring(at_index, NULL);
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

unsigned int
jsal_require_uint(int at_index)
{
	if (!jsal_is_number(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a number", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
	return jsal_get_uint(at_index);
}

void
jsal_require_undefined(int at_index)
{
	if (!jsal_is_undefined(at_index)) {
		jsal_dup(at_index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not 'undefined'", jsal_to_string(-1));
		jsal_remove(-2);
		jsal_throw();
	}
}

void
jsal_set_async_call_flag(bool is_async)
{
	s_async_flag = is_async;
}

void
jsal_set_finalizer(int at_index, js_finalizer_t callback)
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
	throw_on_error();
}

void
jsal_set_top(int new_top)
{
	int new_size;

	new_size = new_top + s_stack_base;
	resize_stack(new_size);
}

void
jsal_stringify(int at_index)
{
	at_index = jsal_normalize_index(at_index);
	jsal_get_global_string("JSON");
	jsal_get_prop_string(-1, "stringify");
	jsal_pull(-2);
	jsal_dup(at_index);
	jsal_call_method(1);
	jsal_replace(at_index);
}

except_t
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
	JsValueRef value;

	at_index = jsal_normalize_index(at_index);
	value = get_value(at_index);
	JsConvertValueToBoolean(value, &value);
	throw_on_error();
	push_value(value, false);
	jsal_replace(at_index);
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
	JsValueRef value;

	at_index = jsal_normalize_index(at_index);
	value = get_value(at_index);
	JsConvertValueToNumber(value, &value);
	throw_on_error();
	push_value(value, false);
	jsal_replace(at_index);
	return jsal_get_number(at_index);
}

void
jsal_to_object(int at_index)
{
	JsValueRef value;

	at_index = jsal_normalize_index(at_index);
	value = get_value(at_index);
	JsConvertValueToObject(value, &value);
	throw_on_error();
	push_value(value, false);
	jsal_replace(at_index);
}

int
jsal_to_propdesc_get(bool enumerable, bool configurable)
{
	/* [ ... getter ] -> [ ... prop_desc ] */

	int index;

	index = jsal_push_new_object();
	jsal_push_boolean(enumerable);
	jsal_put_prop_key(-2, s_key_enumerable);
	jsal_push_boolean(configurable);
	jsal_put_prop_key(-2, s_key_configurable);
	jsal_pull(-2);
	jsal_put_prop_key(-2, s_key_get);
	return index;
}

int
jsal_to_propdesc_get_set(bool enumerable, bool configurable)
{
	/* [ ... getter setter ] -> [ ... prop_desc ] */

	int index;

	index = jsal_push_new_object();
	jsal_push_boolean(enumerable);
	jsal_put_prop_key(-2, s_key_enumerable);
	jsal_push_boolean(configurable);
	jsal_put_prop_key(-2, s_key_configurable);
	jsal_pull(-3);
	jsal_put_prop_key(-2, s_key_get);
	jsal_pull(-2);
	jsal_put_prop_key(-2, s_key_set);
	return index;
}

int
jsal_to_propdesc_set(bool enumerable, bool configurable)
{
	/* [ ... setter ] -> [ ... prop_desc ] */

	int index;

	index = jsal_push_new_object();
	jsal_push_boolean(enumerable);
	jsal_put_prop_key(-2, s_key_enumerable);
	jsal_push_boolean(configurable);
	jsal_put_prop_key(-2, s_key_configurable);
	jsal_pull(-2);
	jsal_put_prop_key(-2, s_key_set);
	return index;
}

int
jsal_to_propdesc_value(bool writable, bool enumerable, bool configurable)
{
	/* [ ... value ] -> [ ... prop_desc ] */

	int index;

	index = jsal_push_new_object();
	jsal_push_boolean(writable);
	jsal_put_prop_key(-2, s_key_writable);
	jsal_push_boolean(enumerable);
	jsal_put_prop_key(-2, s_key_enumerable);
	jsal_push_boolean(configurable);
	jsal_put_prop_key(-2, s_key_configurable);
	jsal_pull(-2);
	jsal_put_prop_key(-2, s_key_value);
	return index;
}

const char*
jsal_to_string(int at_index)
{
	JsValueRef value;

	at_index = jsal_normalize_index(at_index);
	value = get_value(at_index);
	JsConvertValueToString(value, &value);
	throw_on_error();
	push_value(value, false);
	jsal_replace(at_index);
	return jsal_get_string(at_index);
}

unsigned int
jsal_to_uint(int at_index)
{
	jsal_to_number(at_index);
	return jsal_get_uint(at_index);
}

bool
jsal_try(js_function_t callback, int num_args)
{
	/* [ ... arg1..argN ] -> [ ... retval ] */

	jsal_jmpbuf  label;
	jsal_jmpbuf* last_catch_label;
	int          last_stack_base;
	JsValueRef   result;
	bool         retval = true;

	last_catch_label = s_catch_label;
	last_stack_base = s_stack_base;
	s_stack_base = vector_len(s_value_stack) - num_args;
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		if (!callback(num_args, false, 0))
			jsal_push_undefined();
		result = pop_value();
	}
	else {
		result = pop_value();
		retval = false;
	}
	resize_stack(s_stack_base);
	s_catch_label = last_catch_label;
	s_stack_base = last_stack_base;
	push_value(result, false);
	return retval;
}

bool
jsal_try_call(int num_args)
{
	/* [ ... function arg1..argN ] -> [ ... retval ] */

	jsal_jmpbuf  label;
	jsal_jmpbuf* last_catch_label;

	last_catch_label = s_catch_label;
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		jsal_call(num_args);
		s_catch_label = last_catch_label;
		return true;
	}
	else {
		s_catch_label = last_catch_label;
		return false;
	}
}

bool
jsal_try_call_method(int num_args)
{
	/* [ ... function this arg1..argN ] -> [ ... retval ] */

	jsal_jmpbuf  label;
	jsal_jmpbuf* last_catch_label;

	last_catch_label = s_catch_label;
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		jsal_call_method(num_args);
		s_catch_label = last_catch_label;
		return true;
	}
	else {
		s_catch_label = last_catch_label;
		return false;
	}
}

bool
jsal_try_compile(const char* filename)
{
	/* [ ... source ] -> [ ... function ] */

	jsal_jmpbuf  label;
	jsal_jmpbuf* last_catch_label;

	last_catch_label = s_catch_label;
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		jsal_compile(filename);
		s_catch_label = last_catch_label;
		return true;
	}
	else {
		s_catch_label = last_catch_label;
		return false;
	}
}

bool
jsal_try_construct(int num_args)
{
	/* [ ... constructor arg1..argN ] -> [ ... retval ] */

	jsal_jmpbuf  label;
	jsal_jmpbuf* last_catch_label;

	last_catch_label = s_catch_label;
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		jsal_construct(num_args);
		s_catch_label = last_catch_label;
		return true;
	}
	else {
		s_catch_label = last_catch_label;
		return false;
	}
}

bool
jsal_try_eval_module(const char* specifier, const char* url)
{
	/* [ ... source ] -> [ ... exports ] */

	jsal_jmpbuf  label;
	jsal_jmpbuf* last_catch_label;

	last_catch_label = s_catch_label;
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		jsal_eval_module(specifier, url);
		s_catch_label = last_catch_label;
		return true;
	}
	else {
		s_catch_label = last_catch_label;
		return false;
	}
}

bool
jsal_try_parse(int at_index)
{
	jsal_jmpbuf  label;
	jsal_jmpbuf* last_catch_label;

	last_catch_label = s_catch_label;
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		jsal_parse(at_index);
		s_catch_label = last_catch_label;
		return true;
	}
	else {
		s_catch_label = last_catch_label;
		return false;
	}
}

void
jsal_unref(js_ref_t* ref)
{
	if (ref == NULL)
		return;
	if (!ref->weak_ref)
		JsRelease(ref->value, NULL);
	free(ref);
}

bool
jsal_debug_init(js_break_callback_t on_breakpoint)
{
	s_break_callback = on_breakpoint;

	if (JsDiagStartDebugging(s_js_runtime, on_debugger_event, NULL) != JsNoError)
		return false;
	JsDiagSetBreakOnException(s_js_runtime, JsDiagBreakOnExceptionAttributeUncaught);
	return true;
}

void
jsal_debug_uninit(void)
{
	// this causes a crash on shutdown on recent ChakraCore builds, so it's commented out for now.
	// as neoSphere currently runs the debugger nonstop, skipping this shouldn't have any ill effects.
	// the OS will clean up any leaks when the process terminates.
	// https://github.com/microsoft/ChakraCore/issues/6160
	//JsDiagStopDebugging(s_js_runtime, NULL);
}

void
jsal_debug_on_throw(js_throw_callback_t callback)
{
	s_throw_callback = callback;
}

int
jsal_debug_breakpoint_add(const char* filename, unsigned int line, unsigned int column)
{
	struct breakpoint breakpoint;
	JsValueRef        result;
	unsigned int      script_id;

	breakpoint.filename = strdup(filename);
	breakpoint.line = line;
	breakpoint.column = column;
	breakpoint.id = 0;
	script_id = script_id_from_filename(filename);
	if (script_id < UINT_MAX) {
		if (JsDiagSetBreakpoint(script_id, line, column, &result) != JsNoError)
			goto finished;
		push_value(result, true);
		jsal_get_prop_string(-1, "breakpointId");
		breakpoint.id = jsal_get_uint(-1);
		jsal_pop(2);
	}

finished:
	vector_push(s_breakpoints, &breakpoint);
	return vector_len(s_breakpoints) - 1;
}

void
jsal_debug_breakpoint_inject(void)
{
	JsDiagRequestAsyncBreak(s_js_runtime);
}

void
jsal_debug_breakpoint_remove(int index)
{
	struct breakpoint* breakpoint;
	unsigned int       id;
	JsValueRef         list;

	breakpoint = vector_get(s_breakpoints, index);

	JsDiagGetBreakpoints(&list);
	push_value(list, true);
	jsal_push_new_iterator(-1);
	while (jsal_next(-1)) {
		jsal_get_prop_string(-1, "breakpointId");
		id = jsal_get_uint(-1);
		if (breakpoint->id == id)
			JsDiagRemoveBreakpoint(id);
		jsal_pop(2);
	}
	jsal_pop(2);
	vector_remove(s_breakpoints, index);
}

bool
jsal_debug_inspect_call(int call_index)
{
	/* [ ... ] -> [ ... filename function_name line column ] */

	JsValueRef   backtrace;
	JsValueRef   function_data;
	unsigned int handle;

	if (JsDiagGetStackTrace(&backtrace) != JsNoError)
		return false;
	push_value(backtrace, true);
	if (jsal_get_prop_index(-1, call_index)) {
		jsal_get_prop_string(-1, "scriptId");
		jsal_push_string(filename_from_script_id(jsal_get_uint(-1)));
		jsal_replace(-2);

		jsal_get_prop_string(-2, "functionHandle");
		handle = jsal_get_uint(-1);
		JsDiagGetObjectFromHandle(handle, &function_data);
		push_value(function_data, true);
		if (!jsal_get_prop_string(-1, "name")) {
			jsal_pop(1);
			jsal_push_string("");
		}
		jsal_remove(-2);
		jsal_remove(-2);

		jsal_get_prop_string(-3, "line");
		jsal_get_prop_string(-4, "column");
		jsal_remove(-5);
		jsal_remove(-5);
		return true;
	}
	else {
		jsal_pop(2);
		return false;
	}
}

bool
jsal_debug_inspect_breakpoint(int index)
{
	/* [ ... ] -> [ ... filename line column ] */

	struct breakpoint* breakpoint;

	if (index >= vector_len(s_breakpoints))
		return false;
	breakpoint = vector_get(s_breakpoints, index);
	jsal_push_string(breakpoint->filename);
	jsal_push_int(breakpoint->line);
	jsal_push_int(breakpoint->column);
	return true;
}

bool
jsal_debug_inspect_eval(int call_index, const char* source, bool *out_errored)
{
	/* [ ... ] -> [ type value_summary handle ] */

	JsErrorCode error_code;
	JsValueRef  result;
	JsValueRef  source_string;

	JsCreateString(source, strlen(source), &source_string);
	error_code = JsDiagEvaluate(source_string, call_index, JsParseScriptAttributeNone, false, &result);
	if (error_code != JsNoError && error_code != JsErrorScriptException)
		return false;
	*out_errored = error_code != JsNoError;
	push_value(result, true);
	decode_debugger_value();
	jsal_remove(-4);  // remove 'name' from result (not relevant)
	return true;
}

bool
jsal_debug_inspect_object(unsigned int handle, int property_index)
{
	/* [ ... ] -> [ ... key value handle ] */

	JsValueRef results;

	if (JsDiagGetProperties(handle, property_index, 1, &results) != JsNoError)
		return false;
	push_value(results, true);
	jsal_get_prop_string(-1, "properties");
	if (!jsal_get_prop_index(-1, 0)) {
		jsal_pop(3);
		return false;
	}
	decode_debugger_value();
	jsal_remove(-3);  // remove 'type' from result (not relevant...?)
	jsal_remove(-4);
	jsal_remove(-4);
	return true;
}

bool
jsal_debug_inspect_var(int call_index, int var_index)
{
	/* [ ... ] -> [ ... name type value handle ] */

	JsValueRef frame_info;

	if (JsDiagGetStackProperties(call_index, &frame_info) != JsNoError)
		return false;
	push_value(frame_info, true);
	jsal_get_prop_string(-1, "locals");
	if (jsal_get_prop_index(-1, var_index)) {
		decode_debugger_value();
		jsal_remove(-5);
		jsal_remove(-5);
		return true;
	}
	else {
		jsal_pop(3);
		return false;
	}
}

static void
decode_debugger_value(void)
{
	/* [ ... descriptor ] -> [ ... name type value handle ] */

	bool        is_object = false;
	const char* summary;
	const char* type;

	jsal_get_prop_string(-1, "name");
	jsal_get_prop_string(-2, "type");
	type = jsal_get_string(-1);
	if (strcmp(type, "undefined") == 0) {
		jsal_push_undefined();
	}
	else if (jsal_has_prop_string(-3, "value")) {
		// if the 'value' key is present then it always encodes the exact value.
		jsal_get_prop_string(-3, "value");
	}
	else {
		jsal_get_prop_string(-3, "display");
		summary = jsal_get_string(-1);
		is_object = strcmp(type, "object") == 0
			|| strcmp(type, "function") == 0;
	}
	
	// if the value is an object, 'display' will just be the string "{...}" and the caller
	// will need to use its handle to actually inspect the object.  for non-objects, push
	// `null` in lieu of a handle.
	if (is_object)
		jsal_get_prop_string(-4, "handle");
	else
		jsal_push_null();

	// remove the descriptor from the stack
	jsal_remove(-5);
}

static js_ref_t*
get_ref(int stack_index)
{
	js_ref_t* ref;

	stack_index = jsal_normalize_index(stack_index);
	ref = vector_get(s_value_stack, stack_index + s_stack_base);
	return ref;
}

static JsValueRef
get_value(int stack_index)
{
	js_ref_t* ref;

	stack_index = jsal_normalize_index(stack_index);
	ref = vector_get(s_value_stack, stack_index + s_stack_base);
	return ref->value;
}

static JsPropertyIdRef
make_property_id(JsValueRef key)
{
	char*           key_name;
	size_t          key_length;
	JsValueType     key_type;
	JsPropertyIdRef property_id;

	JsGetValueType(key, &key_type);
	if (key_type == JsSymbol) {
		JsGetPropertyIdFromSymbol(key, &property_id);
	}
	else {
		JsCopyString(key, NULL, 0, &key_length);
		key_name = alloca(key_length);
		JsCopyString(key, key_name, key_length, NULL);
		JsCreatePropertyId(key_name, key_length, &property_id);
	}
	return property_id;
}

js_ref_t*
make_ref(JsRef value, bool weak_ref)
{
	js_ref_t* ref;

	if (!weak_ref)
		JsAddRef(value, NULL);

	if (!(ref = calloc(1, sizeof(js_ref_t))))
		return NULL;
	ref->value = value;
	ref->weak_ref = weak_ref;
	return ref;
}

static JsValueRef
pop_value(void)
{
	int        index;
	js_ref_t*  ref;
	JsValueRef value;

	index = vector_len(s_value_stack) - 1;
	ref = vector_get(s_value_stack, index);
	value = ref->value;
	if (!ref->weak_ref)
		JsRelease(ref->value, NULL);
	vector_pop(s_value_stack, 1);
	return value;
}

static void
push_debug_callback_args(JsValueRef event_data)
{
	push_value(event_data, true);
	jsal_get_prop_string(-1, "scriptId");
	jsal_push_string(filename_from_script_id(jsal_get_uint(-1)));
	jsal_replace(-2);
	jsal_get_prop_string(-2, "line");
	jsal_get_prop_string(-3, "column");
	jsal_remove(-4);
}

static unsigned int
script_id_from_filename(const char* filename)
{
	bool         have_name;
	unsigned int script_id = UINT_MAX;
	JsValueRef   script_list;

	JsDiagGetScripts(&script_list);
	push_value(script_list, true);
	jsal_push_new_iterator(-1);
	while (jsal_next(-1)) {
		have_name = jsal_get_prop_string(-1, "fileName");
		if (have_name && strcmp(filename, jsal_get_string(-1)) == 0) {
			jsal_get_prop_string(-2, "scriptId");
			script_id = jsal_get_uint(-1);
			jsal_pop(3);
			break;
		}
		else {
			jsal_pop(2);
		}
	}
	jsal_pop(2);
	return script_id;
}

static const char*
filename_from_script_id(unsigned int script_id)
{
	const char* filename = NULL;
	JsValueRef  script_list;

	JsDiagGetScripts(&script_list);
	push_value(script_list, true);
	jsal_push_new_iterator(-1);
	while (jsal_next(-1)) {
		jsal_get_prop_string(-1, "scriptId");
		if (script_id == jsal_get_uint(-1)) {
			jsal_get_prop_string(-2, "fileName");
			filename = jsal_get_string(-1);
			jsal_pop(3);
			break;
		}
		else {
			jsal_pop(2);
		}
	}
	jsal_pop(2);
	return filename;
}

static JsModuleRecord
get_module_record(const char* specifier, JsModuleRecord parent_record, const char* url, bool *out_is_new)
{
	struct module* cached;
	struct module  module;
	JsModuleRecord module_record;
	JsValueRef     specifier_ref;
	JsValueRef     url_ref;

	iter_t iter;

	*out_is_new = false;
	iter = vector_enum(s_module_cache);
	while ((cached = iter_next(&iter))) {
		if (strcmp(specifier, cached->filename) == 0)
			return cached->record;
	}

	*out_is_new = true;
	JsCreateString(specifier, strlen(specifier), &specifier_ref);
	JsCreateString(url, strlen(url), &url_ref);
	JsInitializeModuleRecord(parent_record, specifier_ref, &module_record);
	JsSetModuleHostInfo(module_record, JsModuleHostInfo_HostDefined, specifier_ref);
	JsSetModuleHostInfo(module_record, JsModuleHostInfo_FetchImportedModuleCallback, on_fetch_imported_module);
	JsSetModuleHostInfo(module_record, JsModuleHostInfo_FetchImportedModuleFromScriptCallback, on_fetch_dynamic_import);
	JsSetModuleHostInfo(module_record, JsModuleHostInfo_NotifyModuleReadyCallback, on_notify_module_ready);
	JsSetModuleHostInfo(module_record, JsModuleHostInfo_Url, url_ref);
	JsAddRef(module_record, NULL);
	module.filename = strdup(specifier);
	module.record = module_record;
	vector_push(s_module_cache, &module);
	return module_record;
}

static int
push_value(JsValueRef value, bool weak_ref)
{
	js_ref_t ref;

	if (!weak_ref)
		JsAddRef(value, NULL);
	ref.value = value;
	ref.weak_ref = weak_ref;
	vector_push(s_value_stack, &ref);
	return vector_len(s_value_stack) - s_stack_base - 1;
}

static void
resize_stack(int new_size)
{
	js_ref_t  ref;
	js_ref_t* ref_ptr;
	int       old_size;

	int i;

	old_size = vector_len(s_value_stack);
	if (new_size < old_size) {
		for (i = new_size; i < old_size; ++i) {
			ref_ptr = vector_get(s_value_stack, i);
			if (!ref_ptr->weak_ref)
				JsRelease(ref_ptr->value, NULL);
		}
	}
	vector_resize(s_value_stack, new_size);
	if (new_size > old_size) {
		ref.value = s_js_undefined;
		ref.weak_ref = true;
		for (i = old_size; i < new_size; ++i)
			vector_put(s_value_stack, i, &ref);
	}
}

static void
throw_on_error(void)
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

static except_t
throw_value(JsValueRef value)
{
	push_value(value, false);
	if (s_catch_label != NULL) {
		jsal_longjmp(*s_catch_label, 1);
	}
	else {
		printf("JS exception thrown from unguarded C code!\n");
		printf("-> %s\n", jsal_to_string(-1));
		abort();
	}
}

static void CHAKRA_CALLBACK
on_debugger_event(JsDiagDebugEvent event_type, JsValueRef data, void* userdata)
{
	struct breakpoint* breakpoint;
	JsValueRef         breakpoint_info;
	const char*        filename;
	unsigned int       handle;
	jsal_jmpbuf        label;
	jsal_jmpbuf*       last_catch_label;
	int                last_stack_base;
	const char*        name;
	JsValueRef         properties;
	unsigned int       script_id;
	js_step_t          step = JS_STEP_CONTINUE;
	JsDiagStepType     step_type;
	char*              traceback = NULL;

	iter_t iter;

	switch (event_type) {
		case JsDiagDebugEventSourceCompile:
			push_value(data, true);
			jsal_get_prop_string(-1, "scriptId");
			jsal_get_prop_string(-2, "fileName");
			script_id = jsal_get_uint(-2);
			filename = jsal_get_string(-1);
			jsal_pop(3);
			iter = vector_enum(s_breakpoints);
			while (iter_next(&iter)) {
				breakpoint = iter.ptr;
				if (strcmp(filename, breakpoint->filename) == 0) {
					if (JsDiagSetBreakpoint(script_id, breakpoint->line, breakpoint->column, &breakpoint_info) != JsNoError)
						continue;
					push_value(breakpoint_info, true);
					jsal_get_prop_string(-1, "breakpointId");
					breakpoint->id = jsal_get_uint(-1);
					jsal_pop(2);
				}
			}
			break;
		case JsDiagDebugEventRuntimeException:
			last_catch_label = s_catch_label;
			last_stack_base = s_stack_base;
			s_stack_base = vector_len(s_value_stack);
			push_value(data, true);
			jsal_get_prop_string(-1, "exception");
			jsal_get_prop_string(-1, "handle");
			handle = jsal_get_uint(-1);
			jsal_pop(3);
			JsDiagGetProperties(handle, 0, UINT_MAX, &properties);
			push_value(properties, true);
			jsal_get_prop_string(-1, "properties");
			jsal_push_new_iterator(-1);
			while (jsal_next(-1)) {
				jsal_get_prop_string(-1, "name");
				name = jsal_get_string(-1);
				if (name != NULL && strcmp(name, "stack") == 0) {
					jsal_get_prop_string(-2, "value");
					traceback = strdup(jsal_get_string(-1));
					jsal_pop(3);
					break;
				}
				jsal_pop(2);
			}
			jsal_pop(3);
			jsal_push_string(traceback != NULL ? traceback : "");
			push_debug_callback_args(data);
			free(traceback);
			if (jsal_setjmp(label) == 0) {
				s_catch_label = &label;
				if (s_throw_callback != NULL)
					s_throw_callback();
			}
			else {
				jsal_pop(1);
			}
			resize_stack(s_stack_base);
			s_catch_label = last_catch_label;
			s_stack_base = last_stack_base;
			// fallthrough;
		case JsDiagDebugEventAsyncBreak:
		case JsDiagDebugEventBreakpoint:
		case JsDiagDebugEventDebuggerStatement:
		case JsDiagDebugEventStepComplete:
			last_catch_label = s_catch_label;
			last_stack_base = s_stack_base;
			s_stack_base = vector_len(s_value_stack);
			push_debug_callback_args(data);
			if (jsal_setjmp(label) == 0) {
				s_catch_label = &label;
				step = s_break_callback();
			}
			else {
				jsal_pop(1);
			}
			resize_stack(s_stack_base);
			s_catch_label = last_catch_label;
			s_stack_base = last_stack_base;
			step_type = step == JS_STEP_IN ? JsDiagStepTypeStepIn
				: step == JS_STEP_OUT ? JsDiagStepTypeStepOut
				: step == JS_STEP_OVER ? JsDiagStepTypeStepOver
				: JsDiagStepTypeContinue;
			JsDiagSetStepType(step_type);
			break;
	}
}

static JsErrorCode CHAKRA_CALLBACK
on_fetch_dynamic_import(JsSourceContext importer, JsValueRef specifier, JsModuleRecord *out_module)
{
	return on_fetch_imported_module(NULL, specifier, out_module);
}

static JsErrorCode CHAKRA_CALLBACK
on_fetch_imported_module(JsModuleRecord importer, JsValueRef module_name, JsModuleRecord *out_module)
{
	// note: be careful, `importer` will be NULL if we were chained from
	//       on_fetch_dynamic_import().

	JsValueRef        caller_id;
	JsValueRef        exception;
	const char*       specifier;
	bool              is_new_module;
	jsal_jmpbuf       label;
	jsal_jmpbuf*      last_catch_label;
	int               last_stack_base;
	struct module_job job;
	JsModuleRecord    module;
	const char*       source;
	size_t            source_len;
	const char*       url;
	JsValueRef        url_ref;

	if (s_import_callback == NULL)
		return JsErrorInvalidArgument;

	last_catch_label = s_catch_label;
	last_stack_base = s_stack_base;
	s_stack_base = vector_len(s_value_stack);
	push_value(module_name, true);
	if (importer != NULL) {
		JsGetModuleHostInfo(importer, JsModuleHostInfo_HostDefined, &caller_id);
		push_value(caller_id, true);
	}
	else {
		jsal_push_null();
	}
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		s_import_callback();
		if (jsal_get_top() < 2)
			jsal_error(JS_TYPE_ERROR, "Internal error in module callback");
		specifier = jsal_require_string(-3);
		url = jsal_require_string(-2);
		source = jsal_require_lstring(-1, &source_len);
		module = get_module_record(specifier, importer, url, &is_new_module);
		if (is_new_module) {
			job.source_context = s_next_source_context++;
			job.module_record = module;
			job.source = strdup(source);
			job.source_size = strlen(source);
			vector_push(s_module_jobs, &job);
		}
		resize_stack(s_stack_base);
		s_catch_label = last_catch_label;
		s_stack_base = last_stack_base;
		*out_module = module;
		return JsNoError;
	}
	else {
		exception = pop_value();
		if (importer != NULL) {
			JsGetModuleHostInfo(importer, JsModuleHostInfo_Url, &url_ref);
			push_value(exception, true);
			push_value(url_ref, true);
			jsal_put_prop_string(-2, "url");
			jsal_pop(1);
		}
		push_value(module_name, true);
		specifier = jsal_get_string(-1);
		module = get_module_record(specifier, importer, specifier, &is_new_module);
		JsSetModuleHostInfo(module, JsModuleHostInfo_Exception, exception);
		resize_stack(s_stack_base);
		s_catch_label = last_catch_label;
		s_stack_base = last_stack_base;
		*out_module = module;
		return JsNoError;
	}
}

static void CHAKRA_CALLBACK
on_finalize_host_object(void* userdata)
{
	struct object* object_info;

	object_info = userdata;
	if (object_info->finalizer != NULL)
		object_info->finalizer(object_info->data);
	free(object_info);
}

static JsValueRef CHAKRA_CALLBACK
on_js_to_native_call(JsValueRef callee, JsValueRef argv[], unsigned short argc, JsNativeFunctionInfo* env, void* userdata)
{
	JsValueRef       call_args[2];
	struct function* function_data;
	bool             has_exception = false;
	bool             has_return;
	bool             is_ctor_call;
	bool             last_async_flag;
	JsValueRef       last_callee_value;
	jsal_jmpbuf*     last_catch_label;
	JsValueRef       last_newtarget_value;
	int              last_stack_base;
	JsValueRef       last_this_value;
	jsal_jmpbuf      label;
	JsValueRef       promise;
	JsValueRef       reject_func;
	JsValueRef       resolve_func;
	JsValueRef       retval = JS_INVALID_REFERENCE;

	int i;

	function_data = userdata;

	last_stack_base = s_stack_base;
	last_async_flag = s_async_flag;
	last_callee_value = s_callee_value;
	last_catch_label = s_catch_label;
	last_newtarget_value = s_newtarget_value;
	last_this_value = s_this_value;

	// set up a stack frame and call the native function
	s_stack_base = vector_len(s_value_stack);
	s_async_flag = function_data->async_flag;
	s_callee_value = callee;
	s_newtarget_value = env->newTargetArg;
	s_this_value = env->thisArg;
	for (i = 1; i < argc; ++i)
		push_value(argv[i], true);
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		is_ctor_call = env->isConstructCall;
		if (is_ctor_call && function_data->async_flag) {
			push_value(callee, true);  // note: gets popped during unwind
			jsal_get_prop_string(-1, "name");
			jsal_error(JS_TYPE_ERROR, "Async '%s()' cannot be called with 'new'", jsal_to_string(-1));
		}
		if (!is_ctor_call && function_data->ctor_only) {
			push_value(callee, true);  // note: gets popped during unwind
			jsal_get_prop_string(-1, "name");
			jsal_error(JS_TYPE_ERROR, "Constructor '%s()' requires 'new'", jsal_to_string(-1));
		}
		if (argc - 1 < function_data->min_args) {
			push_value(callee, true);  // note: gets popped during unwind
			jsal_get_prop_string(-1, "name");
			jsal_error(JS_TYPE_ERROR, "Not enough arguments for '%s()'", jsal_to_string(-1));
		}
		has_return = function_data->callback(argc - 1, is_ctor_call, function_data->magic);
		if (has_return)
			retval = get_value(-1);
		else
			retval = s_js_undefined;
	}
	else {
		// if an error gets thrown into C code, 'jsal_throw()' leaves it on top
		// of the value stack.
		has_exception = true;
		retval = pop_value();
		if (!s_async_flag)
			JsSetException(retval);
	}
	
	if (s_async_flag) {
		JsCreatePromise(&promise, &resolve_func, &reject_func);
		call_args[0] = s_js_undefined;
		call_args[1] = retval;
		if (!has_exception)
			JsCallFunction(resolve_func, call_args, 2, NULL);
		else
			JsCallFunction(reject_func, call_args, 2, NULL);
		retval = promise;
	}
	
	// tear down the stack frame for the call
	resize_stack(s_stack_base);
	s_async_flag = last_async_flag;
	s_callee_value = last_callee_value;
	s_catch_label = last_catch_label;
	s_newtarget_value = last_newtarget_value;
	s_this_value = last_this_value;
	s_stack_base = last_stack_base;
	
	return retval;
}

static JsErrorCode CHAKRA_CALLBACK
on_notify_module_ready(JsModuleRecord module, JsValueRef exception)
{
	struct module_job job;

	memset(&job, 0, sizeof(struct module_job));
	job.module_record = module;
	job.source = NULL;
	vector_push(s_module_jobs, &job);
	return JsNoError;
}

static void CHAKRA_CALLBACK
on_reject_promise_unhandled(JsValueRef promise, JsValueRef reason, bool handled, void* userdata)
{
	struct rejection* entry;
	struct rejection  rejection;

	iter_t iter;

	if (!handled) {
		JsAddRef(promise, NULL);
		rejection.handled = false;
		rejection.promise = promise;
		rejection.value = reason;
		vector_push(s_rejections, &rejection);
	}
	else {
		iter = vector_enum(s_rejections);
		while ((entry = iter_next(&iter))) {
			if (entry->promise == promise) {
				entry->handled = true;
				break;  // safe to short-circuit
			}
		}
	}
}

static void CHAKRA_CALLBACK
on_report_module_completion(JsModuleRecord module, JsValueRef exception)
{
	jsal_jmpbuf  label;
	jsal_jmpbuf* last_catch_label;
	int          last_stack_base;
	JsValueRef   namespace;
	js_ref_t*    rejector;
	char*        specifier;
	JsValueRef   specifier_ref;

	JsGetModuleHostInfo(module, JsModuleHostInfo_HostDefined, &specifier_ref);
	push_value(specifier_ref, true);
	specifier = strdup(jsal_get_string(-1));
	jsal_pop(1);

	last_catch_label = s_catch_label;
	last_stack_base = s_stack_base;
	s_stack_base = vector_len(s_value_stack);
	if (exception == JS_INVALID_REFERENCE) {
		JsGetModuleNamespace(module, &namespace);
		push_value(namespace, true);
	}
	else {
		push_value(exception, true);
	}
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		if (s_module_callback != NULL)
			s_module_callback(specifier, exception != JS_INVALID_REFERENCE);
	}
	else {
		// if an error gets thrown into C code, 'jsal_throw()' leaves it on top
		// of the value stack.
		exception = pop_value();
		jsal_push_new_promise(NULL, &rejector);
		jsal_push_ref_weak(rejector);
		push_value(exception, false);
		jsal_call(1);
	}
	resize_stack(s_stack_base);
	s_catch_label = last_catch_label;
	s_stack_base = last_stack_base;

	free(specifier);
}

static void CHAKRA_CALLBACK
on_resolve_reject_promise(JsValueRef task, void* userdata)
{
	JsValueRef   exception;
	jsal_jmpbuf  label;
	jsal_jmpbuf* last_catch_label;
	int          last_stack_base;

	last_catch_label = s_catch_label;
	last_stack_base = s_stack_base;
	s_stack_base = vector_len(s_value_stack);
	push_value(task, true);
	if (jsal_setjmp(label) == 0) {
		s_catch_label = &label;
		if (s_job_callback == NULL)
			jsal_error(JS_ERROR, "No async/promise continuation support");
		s_job_callback();
	}
	else {
		// if an error gets thrown into C code, 'jsal_throw()' leaves it on top
		// of the value stack.
		exception = pop_value();
		JsSetException(exception);
	}
	resize_stack(s_stack_base);
	s_catch_label = last_catch_label;
	s_stack_base = last_stack_base;
}

#if !defined(__APPLE__)
static int
asprintf(char** out, const char* format, ...)
{
	va_list ap;
	int     buf_size;

	va_start(ap, format);
	buf_size = vasprintf(out, format, ap);
	va_end(ap);
	return buf_size;
}

static int
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
