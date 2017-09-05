#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "api.h"

#include <stdlib.h>
#include <string.h>
#include "jsal.h"
#include "vector.h"

struct class_info
{
	js_finalizer_t finalizer;
	char*          name;
};

static vector_t* s_classes;

void
api_init(void)
{
	s_classes = vector_new(sizeof(struct class_info));
	
	// JavaScript 'global' binding (like Node.js)
	// also map global 'exports' to the global object, as TypeScript likes to add
	// exports even when compiling scripts as program code.
	jsal_push_global_object();
	jsal_push_eval("({ writable: false, enumerable: false, configurable: false })");
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
api_uninit(void)
{
	struct class_info* class_info;
	
	iter_t iter;

	iter = vector_enum(s_classes);
	while (iter_next(&iter)) {
		class_info = iter.ptr;
		free(class_info->name);
	}
	vector_free(s_classes);
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
api_define_class(const char* name, js_callback_t constructor, js_finalizer_t finalizer)
{
	// note: if no constructor function is given, a constructor binding will not be created.
	//       this is useful for types which can only be created via factory methods.

	struct class_info class_info;

	class_info.name = strdup(name);
	class_info.finalizer = finalizer;
	vector_push(s_classes, &class_info);

	// construct a prototype for the new class, leaving it on the
	// value stack afterwards.
	jsal_push_new_object();
	jsal_push_eval("({ writable: false, enumerable: false, configurable: false })");
	jsal_push_string(name);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "___internalClass");

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
api_define_function(const char* namespace_name, const char* name, js_callback_t callback)
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
api_define_method(const char* class_name, const char* name, js_callback_t callback)
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
api_define_property(const char* class_name, const char* name, js_callback_t getter, js_callback_t setter)
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
api_define_static_prop(const char* namespace_name, const char* name, js_callback_t getter, js_callback_t setter)
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

bool
jsal_is_class_obj(int index, const char* class_name)
{
	const char* obj_class_name;
	bool        result;

	if (!jsal_is_object_coercible(index))
		return false;

	jsal_get_prop_string(index, "___internalClass");
	obj_class_name = jsal_to_string(-1);
	result = strcmp(obj_class_name, class_name) == 0;
	jsal_pop(1);
	return result;
}

int
jsal_push_class_obj(const char* class_name, void* udata)
{
	struct class_info* class_info;
	js_finalizer_t     finalizer = NULL;
	int                index;
	
	iter_t iter;

	iter = vector_enum(s_classes);
	while (iter_next(&iter)) {
		class_info = iter.ptr;
		if (strcmp(class_name, class_info->name) == 0)
			finalizer = class_info->finalizer;
	}
	
	index = jsal_push_new_host_object(udata, NULL);
	if (finalizer != NULL)
		jsal_set_finalizer(-1, finalizer);

	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "prototypes");
	jsal_get_prop_string(-1, class_name);
	jsal_set_prototype(index);
	jsal_pop(2);

	return index;
}

int
jsal_push_class_prototype(const char* class_name)
{
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "prototypes");
	jsal_get_prop_string(-1, class_name);
	jsal_remove(-2);
	jsal_remove(-2);
	return jsal_get_top() - 1;
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
