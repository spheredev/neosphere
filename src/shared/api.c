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
	int            super_id;
	js_ref_t*      prototype;
};

struct object_data
{
	int            class_id;
	js_finalizer_t finalizer;
	void*          ptr;
};

static void        on_finalize_sphere_object (void* host_ptr);
static bool        get_obj_data_checked      (int stack_index, int class_id, struct object_data* *out_data_ptr);
static int         class_id_from_name        (const char* name);
static const char* class_name_from_id        (int class_id);

static int       s_class_index[1000];
static vector_t* s_classes;
static js_ref_t* s_key_prototype;

void
api_init(bool node_compatible)
{
	s_classes = vector_new(sizeof(struct class_data));

	s_key_prototype = jsal_new_key("prototype");

	if (node_compatible) {
		// Node.js-compatible 'global' binding
		jsal_push_global_object();
		jsal_push_global_object();
		jsal_to_propdesc_value(false, false, false);
		jsal_def_prop_string(-2, "global");
		jsal_pop(1);
	}
}

void
api_uninit(void)
{
	struct class_data* class_data;

	iter_t iter;

	jsal_unref(s_key_prototype);
	iter = vector_enum(s_classes);
	while (iter_next(&iter)) {
		class_data = iter.ptr;
		jsal_unref(class_data->prototype);
		free(class_data->name);
	}
	vector_free(s_classes);
}

void
api_define_async_func(const char* namespace_name, const char* name, js_function_t callback, intptr_t magic)
{
	jsal_push_global_object();

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!jsal_get_prop_string(-1, namespace_name)) {
			jsal_push_new_object();
			jsal_replace(-2);

			// <namespace>[Symbol.toStringTag] = <name>;
			jsal_push_known_symbol("toStringTag");
			jsal_push_string(namespace_name);
			jsal_to_propdesc_value(false, false, false);
			jsal_def_prop(-3);

			// global.<name> = <namespace>
			jsal_dup(-1);
			jsal_to_propdesc_value(true, false, true);
			jsal_def_prop_string(-3, namespace_name);
		}
	}

	jsal_push_new_function(callback, name, 0, true, magic);
	jsal_to_propdesc_value(true, false, true);
	jsal_def_prop_string(-2, name);

	if (namespace_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_async_method(const char* class_name, const char* name, js_function_t callback, intptr_t magic)
{
	jsal_push_global_object();
	if (class_name != NULL) {
		// load the prototype from the prototype stash
		jsal_push_class_prototype(class_id_from_name(class_name));
	}

	jsal_push_new_function(callback, name, 0, true, magic);
	jsal_to_propdesc_value(true, false, true);
	if (strncmp(name, "@@", 2) == 0) {
		jsal_push_known_symbol(&name[2]);
		jsal_pull(-2);
		jsal_def_prop(-3);
	}
	else {
		jsal_def_prop_string(-2, name);
	}

	if (class_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_const(const char* enum_name, const char* name, double value)
{
	jsal_push_global_object();

	// ensure the namespace object exists
	if (enum_name != NULL) {
		if (!jsal_get_prop_string(-1, enum_name)) {
			jsal_push_new_object();
			jsal_replace(-2);

			// <namespace>[Symbol.toStringTag] = <name>;
			jsal_push_known_symbol("toStringTag");
			jsal_push_string(enum_name);
			jsal_to_propdesc_value(false, false, false);
			jsal_def_prop(-3);

			// global.<name> = <namespace>
			jsal_dup(-1);
			jsal_to_propdesc_value(true, false, true);
			jsal_def_prop_string(-3, enum_name);
		}
	}

	jsal_push_number(value);
	jsal_to_propdesc_value(false, false, true);
	jsal_def_prop_string(-2, name);
	if (enum_name != NULL) {
		// generate a TypeScript-style bidirectional enumeration:
		//     enum[key] = value
		//     enum[value] = key
		jsal_push_number(value);
		jsal_to_string(-1);
		jsal_push_string(name);
		jsal_to_propdesc_value(false, false, true);
		jsal_def_prop(-3);
	}

	if (enum_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_class(const char* name, int class_id, js_function_t constructor, js_finalizer_t finalizer, intptr_t magic)
{
	// note: if no constructor function is given, a constructor binding will not be created.
	//       this is useful for types which can only be created via factory methods.

	api_define_subclass(name, class_id, -1, constructor, finalizer, magic);
}

void
api_define_func(const char* namespace_name, const char* name, js_function_t callback, intptr_t magic)
{
	jsal_push_global_object();

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!jsal_get_prop_string(-1, namespace_name)) {
			jsal_push_new_object();
			jsal_replace(-2);

			// <namespace>[Symbol.toStringTag] = <name>;
			jsal_push_known_symbol("toStringTag");
			jsal_push_string(namespace_name);
			jsal_to_propdesc_value(false, false, false);
			jsal_def_prop(-3);

			// global.<name> = <namespace>
			jsal_dup(-1);
			jsal_to_propdesc_value(true, false, true);
			jsal_def_prop_string(-3, namespace_name);
		}
	}

	jsal_push_new_function(callback, name, 0, false, magic);
	jsal_to_propdesc_value(true, false, true);
	jsal_def_prop_string(-2, name);

	if (namespace_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_method(const char* class_name, const char* name, js_function_t callback, intptr_t magic)
{
	jsal_push_global_object();
	if (class_name != NULL) {
		// load the prototype from the prototype stash
		jsal_push_class_prototype(class_id_from_name(class_name));
	}

	jsal_push_new_function(callback, name, 0, false, magic);
	jsal_to_propdesc_value(true, false, true);
	if (strncmp(name, "@@", 2) == 0) {
		jsal_push_known_symbol(&name[2]);
		jsal_pull(-2);
		jsal_def_prop(-3);
	}
	else {
		jsal_def_prop_string(-2, name);
	}

	if (class_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_object(const char* namespace_name, const char* name, int class_id, void* udata)
{
	jsal_push_global_object();

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!jsal_get_prop_string(-1, namespace_name)) {
			jsal_push_new_object();
			jsal_replace(-2);

			// <namespace>[Symbol.toStringTag] = <name>;
			jsal_push_known_symbol("toStringTag");
			jsal_push_string(namespace_name);
			jsal_to_propdesc_value(false, false, false);
			jsal_def_prop(-3);

			// global.<name> = <namespace>
			jsal_dup(-1);
			jsal_to_propdesc_value(true, false, true);
			jsal_def_prop_string(-3, namespace_name);
		}
	}

	jsal_push_class_obj(class_id, udata, false);
	jsal_to_propdesc_value(false, false, true);
	jsal_def_prop_string(-2, name);
	if (namespace_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_prop(const char* class_name, const char* name, bool enumerable, js_function_t getter, js_function_t setter)
{
	jsal_push_global_object();
	if (class_name != NULL) {
		// load the prototype from the prototype stash
		jsal_push_class_prototype(class_id_from_name(class_name));
	}

	// populate the property descriptor
	jsal_push_new_object();
	jsal_push_boolean_true();
	jsal_put_prop_string(-2, "configurable");
	jsal_push_boolean(enumerable);
	jsal_put_prop_string(-2, "enumerable");
	if (getter != NULL) {
		jsal_push_new_function(getter, "get", 0, false, 0);
		jsal_put_prop_string(-2, "get");
	}
	if (setter != NULL) {
		jsal_push_new_function(setter, "set", 0, false, 0);
		jsal_put_prop_string(-2, "set");
	}

	jsal_def_prop_string(-2, name);
	if (class_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_static_prop(const char* namespace_name, const char* name, js_function_t getter, js_function_t setter, intptr_t magic)
{
	jsal_push_global_object();

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!jsal_get_prop_string(-1, namespace_name)) {
			jsal_push_new_object();
			jsal_replace(-2);

			// <namespace>[Symbol.toStringTag] = <name>;
			jsal_push_known_symbol("toStringTag");
			jsal_push_string(namespace_name);
			jsal_to_propdesc_value(false, false, false);
			jsal_def_prop(-3);

			// global.<name> = <namespace>
			jsal_dup(-1);
			jsal_to_propdesc_value(true, false, true);
			jsal_def_prop_string(-3, namespace_name);
		}
	}

	// populate the property descriptor
	jsal_push_new_object();
	jsal_push_boolean_true();
	jsal_put_prop_string(-2, "configurable");
	if (getter != NULL) {
		jsal_push_new_function(getter, "get", 0, false, magic);
		jsal_put_prop_string(-2, "get");
	}
	if (setter != NULL) {
		jsal_push_new_function(setter, "set", 0, false, magic);
		jsal_put_prop_string(-2, "set");
	}
	jsal_def_prop_string(-2, name);
	if (namespace_name != NULL)
		jsal_pop(1);
	jsal_pop(1);
}

void
api_define_subclass(const char* name, int class_id, int super_id, js_function_t constructor, js_finalizer_t finalizer, intptr_t magic)
{
	// note: if no constructor function is given, a constructor binding will not be created.
	//       this is useful for types which can only be created via factory methods.

	struct class_data class_data;
	js_ref_t*         prototype;

	// construct a prototype and leave it on the stack
	jsal_push_new_object();
	if (super_id >= 0) {
		jsal_push_class_prototype(super_id);
		jsal_set_prototype(-2);
	}
	prototype = jsal_ref(-1);

	// <prototype>[Symbol.toStringTag] = <name>;
	jsal_push_known_symbol("toStringTag");
	jsal_push_new_object();
	jsal_push_string(name);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop(-3);

	// IMPORTANT: `class_id` should never exceed 999.
	s_class_index[class_id] = vector_len(s_classes);

	class_data.id = class_id;
	class_data.super_id = super_id;
	class_data.finalizer = finalizer;
	class_data.name = strdup(name);
	class_data.prototype = prototype;
	vector_push(s_classes, &class_data);

	// register a global constructor, if applicable
	if (constructor != NULL) {
		jsal_push_new_constructor(constructor, name, 0, magic);

		// <prototype>.constructor = <ctor>;
		jsal_push_new_object();
		jsal_dup(-2);
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-3, "constructor");

		// <ctor>.prototype = <prototype>
		jsal_push_new_object();
		jsal_dup(-3);
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-2, "prototype");

		// global.<name> = <ctor>;
		jsal_push_global_object();
		jsal_pull(-2);
		jsal_to_propdesc_value(true, false, true);
		jsal_def_prop_string(-2, name);
		jsal_pop(1);
	}

	jsal_pop(1);
}

void*
jsal_get_class_obj(int index, int class_id)
{
	struct object_data* data;

	if (get_obj_data_checked(index, class_id, &data))
		return data->ptr;
	else
		return NULL;
}

bool
jsal_is_class_obj(int index, int class_id)
{
	return get_obj_data_checked(index, class_id, NULL);
}

int
jsal_push_class_name(int class_id)
{
	struct class_data* class_data;
	const char*        name;

	iter_t iter;

	iter = vector_enum(s_classes);
	while (iter_next(&iter)) {
		class_data = iter.ptr;
		if (class_id == class_data->id)
			name = class_data->name;
	}
	return jsal_push_string(name);
}

int
jsal_push_class_obj(int class_id, void* udata, bool in_ctor)
{
	int index;

	index = jsal_push_class_fatobj(class_id, in_ctor, 0, NULL);
	jsal_set_class_ptr(index, udata);
	return index;
}

int
jsal_push_class_fatobj(int class_id, bool in_ctor, size_t data_size, void* *out_data_ptr)
{
	struct class_data*  class_data;
	js_finalizer_t      finalizer = NULL;
	int                 index;
	struct object_data* object_data;
	js_ref_t*           prototype;

	class_data = vector_get(s_classes, s_class_index[class_id]);
	finalizer = class_data->finalizer;
	prototype = class_data->prototype;

	// note: accessing JS properties from native code is reasonably expensive; if
	//       `new.target` is the same as the constructor actually called (not a subclass),
	//       we can just use the known prototype.  this speeds up object creation.
	if (in_ctor && jsal_is_subclass_ctor()) {
		jsal_push_newtarget();
		jsal_get_prop_key(-1, s_key_prototype);
		jsal_replace(-2);
	}
	else {
		jsal_push_ref_weak(prototype);
	}

	index = jsal_push_new_host_object(on_finalize_sphere_object, sizeof(struct object_data) + data_size, (void**)&object_data);
	object_data->class_id = class_id;
	object_data->finalizer = finalizer;
	object_data->ptr = &object_data[1];
	if (out_data_ptr != NULL)
		*out_data_ptr = object_data->ptr;

	return index;
}

int
jsal_push_class_prototype(int class_id)
{
	struct class_data* class_data;

	iter_t iter;

	iter = vector_enum(s_classes);
	while (iter_next(&iter)) {
		class_data = iter.ptr;
		if (class_id == class_data->id)
			break;
	}

	return jsal_push_ref_weak(class_data->prototype);
}

void*
jsal_require_class_obj(int index, int class_id)
{
	struct object_data* data;

	if (!get_obj_data_checked(index, class_id, &data)) {
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

static bool
get_obj_data_checked(int stack_index, int class_id, struct object_data* *out_data_ptr)
{
	struct class_data*  class_data;
	struct object_data* data;
	int                 super_id;

	if (!(data = jsal_get_host_data(stack_index)))
		return false;
	if (data->class_id == class_id)
		goto found_it;  // fast path
	super_id = data->class_id;
	while (super_id >= 0) {
		class_data = vector_get(s_classes, s_class_index[super_id]);
		if (class_id == class_data->id)
			goto found_it;
		super_id = class_data->super_id;
	}
	return false;

found_it:
	if (out_data_ptr != NULL)
		*out_data_ptr = data;
	return true;
}
