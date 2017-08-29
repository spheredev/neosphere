#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "api.h"

#include <stdlib.h>
#include <string.h>
#include "jsal.h"

void
api_init()
{
	// JavaScript 'global' binding (like Node.js)
	// also map global `exports` to the global object, as TypeScript likes to add
	// exports even when compiling scripts as program code.
	jsal_push_global_object();
	jsal_push_eval("({ writable: true, enumerable: true, configurable: true })");
	jsal_push_global_object();
	jsal_put_prop_string(-2, "value");
	jsal_dup(-1);
	jsal_def_prop_string(-3, "global");
	jsal_def_prop_string(-2, "exports");
	jsal_pop(1);

	// set up a prototype stash.  this ensures the prototypes for built-in classes
	// remain accessible internally even if their constructors are overwritten.
	jsal_push_hidden_stash();
	jsal_push_new_object();
	jsal_put_prop_string(-2, "prototypes");
	jsal_pop(1);
}

void
api_define_const(const char* enum_name, const char* name, double value)
{
	jsal_push_global_object();

	// ensure the namespace object exists
	if (enum_name != NULL) {
		if (!jsal_get_prop_string(-1, enum_name)) {
			jsal_pop(1);
			jsal_push_eval("({ enumerable: false, writable: true, configurable: true })");
			jsal_push_new_object();
			jsal_put_prop_string(-2, "value");
			jsal_def_prop_string(-2, enum_name);
			jsal_get_prop_string(-1, enum_name);
		}
	}

	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_push_number(value);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, name);
	if (enum_name != NULL) {
		// generate a TypeScript-style bidirectional enumeration:
		//     enum[key] = value
		//     enum[value] = key
		jsal_push_number(value);
		jsal_to_string(-1);
		jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
		jsal_push_string(name);
		jsal_put_prop_string(-2, "value");
		jsal_def_prop(-2);
	}

	if (enum_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_class(const char* name, jsal_callback_t constructor, jsal_callback_t finalizer)
{
	// note: if no constructor function is given, a constructor binding will not be created.
	//       this is useful for types which can only be created via factory methods.

	// construct a prototype for the new class, leaving it on the
	// value stack afterwards.
	jsal_push_new_object();
	jsal_push_string(name);
	jsal_put_prop_string(-2, "___ctor");
	if (finalizer != NULL) {
		jsal_push_function(finalizer, "finalize", 0, 0);
		jsal_put_prop_string(-2, "___dtor");
	}

	// save the prototype to the hidden stash.  this ensures it remains accessible
	// internally even if the constructor is overwritten.
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "prototypes");
	jsal_dup(-3);
	jsal_put_prop_string(-2, name);
	jsal_pop(2);

	if (constructor != NULL) {
		jsal_push_constructor(constructor, name, 0, 0);
		
		jsal_push_new_object();
		jsal_dup(-3);
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-2, "prototype");

		jsal_push_global_object();
		jsal_push_eval("({ writable: true, configurable: true })");
		jsal_pull(-3);
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-2, name);
		jsal_pop(1);
	}

	jsal_pop(1);
}

void
api_define_function(const char* namespace_name, const char* name, jsal_callback_t callback)
{
	jsal_push_global_object();

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!jsal_get_prop_string(-1, namespace_name)) {
			jsal_pop(1);
			jsal_push_eval("({ writable: true, configurable: true })");
			jsal_push_new_object();
			jsal_put_prop_string(-2, "value");
			jsal_def_prop_string(-2, namespace_name);
			jsal_get_prop_string(-1, namespace_name);
		}
	}

	jsal_push_eval("({ writable: true, configurable: true })");
	jsal_push_function(callback, name, 0, 0);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, name);
	
	if (namespace_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_method(const char* class_name, const char* name, jsal_callback_t callback)
{
	jsal_push_global_object();
	if (class_name != NULL) {
		// load the prototype from the prototype stash
		jsal_push_hidden_stash();
		jsal_get_prop_string(-1, "prototypes");
		jsal_get_prop_string(-1, class_name);
	}

	jsal_push_eval("({ writable: true, configurable: true })");
	jsal_push_function(callback, name, 0, 0);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, name);

	if (class_name != NULL)
		jsal_pop(3);
	jsal_pop(1);
}

void
api_define_object(const char* namespace_name, const char* name, const char* class_name, void* udata)
{
	jsal_push_global_object();

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!jsal_get_prop_string(-1, namespace_name)) {
			jsal_pop(1);
			jsal_push_eval("({ writable: true, configurable: true })");
			jsal_push_new_object();
			jsal_put_prop_string(-2, "value");
			jsal_def_prop_string(-2, namespace_name);
			jsal_get_prop_string(-1, namespace_name);
		}
	}

	jsal_push_eval("({ enumerable: false, writable: false, configurable: true })");
	jsal_push_class_obj(class_name, udata);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, name);
	if (namespace_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_property(const char* class_name, const char* name, jsal_callback_t getter, jsal_callback_t setter)
{
	jsal_push_global_object();
	if (class_name != NULL) {
		jsal_push_hidden_stash();
		jsal_get_prop_string(-1, "prototypes");
		jsal_get_prop_string(-1, class_name);
	}

	// populate the property descriptor
	jsal_push_eval("({ configurable: true })");
	if (getter != NULL) {
		jsal_push_function(getter, "get", 0, 0);
		jsal_put_prop_string(-2, "get");
	}
	if (setter != NULL) {
		jsal_push_function(setter, "set", 0, 0);
		jsal_put_prop_string(-2, "set");
	}
	
	jsal_def_prop_string(-2, name);
	if (class_name != NULL)
		jsal_pop(3);
	jsal_pop(1);
}

void
api_define_static_prop(const char* namespace_name, const char* name, jsal_callback_t getter, jsal_callback_t setter)
{
	jsal_push_global_object();

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!jsal_get_prop_string(-1, namespace_name)) {
			jsal_pop(1);
			jsal_push_eval("({ writable: true, configurable: true })");
			jsal_push_new_object();
			jsal_put_prop_string(-2, "value");
			jsal_def_prop_string(-2, namespace_name);
			jsal_get_prop_string(-1, namespace_name);
		}
	}

	// populate the property descriptor
	jsal_push_eval("({ configurable: true })");
	if (getter != NULL) {
		jsal_push_function(getter, "get", 0, 0);
		jsal_put_prop_string(-2, "get");
	}
	if (setter != NULL) {
		jsal_push_function(setter, "set", 0, 0);
		jsal_put_prop_string(-2, "set");
	}
	jsal_def_prop_string(-2, name);
	if (namespace_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
jsal_error_blame(int blame_offset, jsal_error_t type, const char* format, ...)
{
	va_list ap;
	char*   filename;
	int     line_number;

	// get filename and line number from Duktape call stack
	/*jsal_inspect_callstack_entry(-1 + blame_offset);
	if (!jsal_is_object(-1)) {
		// note: the topmost call is assumed to be a Duktape/C activation.  that's
		//       probably not what's responsible for the error, so blame the function
		//       just below it if there's nobody else to blame.
		jsal_inspect_callstack_entry(-2);
		jsal_replace(-2);
	}
	jsal_get_prop_string(-1, "lineNumber");
	jsal_get_prop_string(-2, "function");
	jsal_get_prop_string(-1, "fileName");
	filename = strdup(jsal_to_string(-1));
	line_number = jsal_to_int(-3);
	jsal_pop(4);*/

	filename = strdup("eatyPig.js");
	line_number = 812;

	// construct an Error object
	va_start(ap, format);
	jsal_push_new_error_va(type, format, ap);
	va_end(ap);
	jsal_push_string(filename);
	jsal_put_prop_string(-2, "fileName");
	jsal_push_int(line_number);
	jsal_put_prop_string(-2, "lineNumber");
	free(filename);

	jsal_throw();
}

bool
jsal_is_class_obj(int index, const char* class_name)
{
	const char* obj_class_name;
	bool        result;

	if (!jsal_is_object_coercible(index))
		return false;

	jsal_get_prop_string(index, "___ctor");
	obj_class_name = jsal_to_string(-1);
	result = strcmp(obj_class_name, class_name) == 0;
	jsal_pop(1);
	return result;
}

int
jsal_push_class_obj(const char* class_name, void* udata)
{
	int index;
	
	index = jsal_push_new_host_object(udata, NULL);

	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "prototypes");
	jsal_get_prop_string(-1, class_name);
	jsal_set_prototype(index);
	jsal_pop(2);

	return index;
}

void*
jsal_require_class_obj(int index, const char* class_name)
{
	if (!jsal_is_class_obj(index, class_name)) {
		jsal_dup(index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a %s object", jsal_to_string(-1), class_name);
		jsal_remove(-2);
		jsal_throw();
	}
	return jsal_get_host_data(index);
}

void
jsal_set_class_ptr(int index, void* udata)
{
	jsal_set_host_data(index, udata);
}
