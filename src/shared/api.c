#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "api.h"

#include <stdlib.h>
#include <string.h>
#include "jsal.h"
#include "vector.h"

struct class_data
{
	js_finalizer_t finalizer;
	int            id;
	char*          name;
};

struct object_data
{
	int            class_id;
	js_finalizer_t finalizer;
	void*          ptr;
};

static void        on_finalize_sphere_object (void* host_ptr);
static int         class_id_from_name        (const char* name);
static const char* class_name_from_id        (int class_id);

static vector_t* s_classes;
static js_ref_t* s_prototypes;

void
api_init(void)
{
	s_classes = vector_new(sizeof(struct class_data));
	
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

	// set up an object to keep prototypes.  this way prototypes for built-in classes
	// remain accessible internally even if their constructors are overwritten.
	jsal_push_new_object();
	s_prototypes = jsal_ref(-1);
	jsal_pop(1);
}

void
api_uninit(void)
{
	struct class_data* class_data;
	
	iter_t iter;

	jsal_unref(s_prototypes);
	iter = vector_enum(s_classes);
	while (iter_next(&iter)) {
		class_data = iter.ptr;
		free(class_data->name);
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
api_define_class(const char* name, int class_id, js_function_t constructor, js_finalizer_t finalizer)
{
	// note: if no constructor function is given, a constructor binding will not be created.
	//       this is useful for types which can only be created via factory methods.

	struct class_data class_data;

	class_data.id = class_id;
	class_data.name = strdup(name);
	class_data.finalizer = finalizer;
	vector_push(s_classes, &class_data);

	// construct a JS prototype for the new class
	jsal_push_new_object();
	jsal_push_ref(s_prototypes);
	jsal_dup(-2);
	jsal_put_prop_index(-2, class_id);
	jsal_pop(1);

	// register a global constructor, if applicable
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
api_define_function(const char* namespace_name, const char* name, js_function_t callback)
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
api_define_method(const char* class_name, const char* name, js_function_t callback)
{
	jsal_push_global_object();
	if (class_name != NULL) {
		// load the prototype from the prototype stash
		jsal_push_ref(s_prototypes);
		jsal_get_prop_index(-1, class_id_from_name(class_name));
	}

	jsal_push_eval("({ writable: true, configurable: true })");
	jsal_push_function(callback, name, 0, 0);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, name);

	if (class_name != NULL)
		jsal_pop(2);
	jsal_pop(1);
}

void
api_define_object(const char* namespace_name, const char* name, int class_id, void* udata)
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
	jsal_push_class_obj(class_id, udata);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, name);
	if (namespace_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_property(const char* class_name, const char* name, js_function_t getter, js_function_t setter)
{
	jsal_push_global_object();
	if (class_name != NULL) {
		jsal_push_ref(s_prototypes);
		jsal_get_prop_index(-1, class_id_from_name(class_name));
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
		jsal_pop(2);
	jsal_pop(1);
}

void
api_define_static_prop(const char* namespace_name, const char* name, js_function_t getter, js_function_t setter)
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
jsal_is_class_obj(int index, int class_id)
{
	struct object_data* data;

	if (!jsal_is_object(index))
		return false;

	data = jsal_get_host_data(index);
	return class_id == data->class_id;
}

int
jsal_push_class_obj(int class_id, void* udata)
{
	struct class_data*  class_data;
	js_finalizer_t      finalizer = NULL;
	int                 index;
	struct object_data* object_data;
	
	iter_t iter;

	iter = vector_enum(s_classes);
	while (iter_next(&iter)) {
		class_data = iter.ptr;
		if (class_id == class_data->id)
			finalizer = class_data->finalizer;
	}
	
	object_data = malloc(sizeof(struct object_data));
	object_data->class_id = class_id;
	object_data->finalizer = finalizer;
	object_data->ptr = udata;
	index = jsal_push_new_host_object(object_data, on_finalize_sphere_object);

	jsal_push_ref(s_prototypes);
	jsal_get_prop_index(-1, class_id);
	jsal_set_prototype(index);
	jsal_pop(1);

	return index;
}

int
jsal_push_class_prototype(int class_id)
{
	jsal_push_ref(s_prototypes);
	jsal_get_prop_index(-1, class_id);
	jsal_remove(-2);
	return jsal_get_top() - 1;
}

void*
jsal_require_class_obj(int index, int class_id)
{
	struct object_data* data;
	
	data = jsal_get_host_data(index);
	if (data->class_id != class_id) {
		jsal_dup(index);
		jsal_push_new_error(JS_TYPE_ERROR, "'%s' is not a %s object", jsal_to_string(-1), class_name_from_id(class_id));
		jsal_remove(-2);
		jsal_throw();
	}
	return data->ptr;
}

void
jsal_set_class_ptr(int index, void* udata)
{
	struct object_data* data;

	data = jsal_get_host_data(index);
	data->ptr = udata;
}

static void
on_finalize_sphere_object(void* host_ptr)
{
	struct object_data* object;

	object = host_ptr;
	if (object->finalizer != NULL) 
		object->finalizer(object->ptr);
	free(object);
}

static int
class_id_from_name(const char* name)
{
	struct class_data* class;

	iter_t iter;

	iter = vector_enum(s_classes);
	while (iter_next(&iter)) {
		class = iter.ptr;
		if (strcmp(name, class->name) == 0)
			return class->id;
	}
	return 0;
}

static const char*
class_name_from_id(int class_id)
{
	struct class_data* class;
	
	iter_t iter;

	iter = vector_enum(s_classes);
	while (iter_next(&iter)) {
		class = iter.ptr;
		if (class_id == class->id)
			return class->name;
	}
	return NULL;
}
