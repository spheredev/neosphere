#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "api.h"

#include "duktape.h"
#include "duk_rubber.h"

void
api_init(duk_context* ctx)
{
	// JavaScript 'global' binding (like Node.js)
	duk_push_global_object(ctx);
	duk_push_string(ctx, "global");
	duk_push_global_object(ctx);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_SET_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	// set up a prototype stash.  this ensures the prototypes for built-in classes
	// remain accessible internally even if the constructors are overwritten.
	duk_push_global_stash(ctx);
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, "prototypes");
	duk_pop(ctx);
}

void
api_define_const(duk_context* ctx, const char* enum_name, const char* name, double value)
{
	duk_push_global_object(ctx);

	// ensure the namespace object exists
	if (enum_name != NULL) {
		if (!duk_get_prop_string(ctx, -1, enum_name)) {
			duk_pop(ctx);
			duk_push_string(ctx, enum_name);
			duk_push_object(ctx);
			duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
				| DUK_DEFPROP_CLEAR_ENUMERABLE
				| DUK_DEFPROP_SET_WRITABLE
				| DUK_DEFPROP_SET_CONFIGURABLE);
			duk_get_prop_string(ctx, -1, enum_name);
		}
	}

	duk_push_string(ctx, name);
	duk_push_number(ctx, value);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	if (enum_name != NULL) {
		// generate a TypeScript-style bidirectional enumeration:
		//     enum[key] = value
		//     enum[value] = key
		duk_push_number(ctx, value);
		duk_to_string(ctx, -1);
		duk_push_string(ctx, name);
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
			| DUK_DEFPROP_CLEAR_ENUMERABLE
			| DUK_DEFPROP_CLEAR_WRITABLE
			| DUK_DEFPROP_SET_CONFIGURABLE);
	}

	if (enum_name != NULL)
		duk_pop(ctx);
	duk_pop(ctx);
}

void
api_define_class(duk_context* ctx, const char* name, duk_c_function constructor, duk_c_function finalizer)
{
	// note: if no constructor function is given, a constructor binding will not be created.
	//       this is useful for types which can only be created via factory methods.
	
	// construct a prototype for the new class, leaving it on the
	// value stack afterwards.
	duk_push_object(ctx);
	duk_push_string(ctx, name);
	duk_put_prop_string(ctx, -2, "\xFF" "ctor");
	if (finalizer != NULL) {
		duk_push_c_function(ctx, finalizer, DUK_VARARGS);
		duk_put_prop_string(ctx, -2, "\xFF" "dtor");
	}

	// save the prototype to the Duktape global stash.  this ensures it remains
	// accessible internally even if the constructor is overwritten.
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "prototypes");
	duk_dup(ctx, -3);
	duk_put_prop_string(ctx, -2, name);
	duk_pop_2(ctx);

	if (constructor != NULL) {
		duk_push_c_function(ctx, constructor, DUK_VARARGS);
		duk_push_string(ctx, "name");
		duk_push_string(ctx, name);
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
		duk_push_string(ctx, "prototype");
		duk_dup(ctx, -3);
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE);
		
		duk_push_global_object(ctx);
		duk_push_string(ctx, name);
		duk_dup(ctx, -3);
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
			| DUK_DEFPROP_SET_WRITABLE
			| DUK_DEFPROP_SET_CONFIGURABLE);
		duk_pop_2(ctx);
	}
	
	duk_pop(ctx);
}

void
api_define_function(duk_context* ctx, const char* namespace_name, const char* name, duk_c_function fn)
{
	duk_push_global_object(ctx);

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!duk_get_prop_string(ctx, -1, namespace_name)) {
			duk_pop(ctx);
			duk_push_string(ctx, namespace_name);
			duk_push_object(ctx);
			duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
				| DUK_DEFPROP_SET_WRITABLE
				| DUK_DEFPROP_SET_CONFIGURABLE);
			duk_get_prop_string(ctx, -1, namespace_name);
		}
	}

	duk_push_string(ctx, name);
	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_push_string(ctx, "name");
	duk_push_string(ctx, name);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_SET_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	if (namespace_name != NULL)
		duk_pop(ctx);
	duk_pop(ctx);
}

void
api_define_method(duk_context* ctx, const char* class_name, const char* name, duk_c_function fn)
{
	duk_push_global_object(ctx);
	if (class_name != NULL) {
		// load the prototype from the prototype stash
		duk_push_global_stash(ctx);
		duk_get_prop_string(ctx, -1, "prototypes");
		duk_get_prop_string(ctx, -1, class_name);
	}

	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_push_string(ctx, "name");
	duk_push_string(ctx, name);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);

	// for a defprop, Duktape expects the key to be pushed first, then the value; however, since
	// we have the value (the function being registered) on the stack already by this point, we
	// need to shuffle things around to make everything work.
	duk_push_string(ctx, name);
	duk_insert(ctx, -2);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_SET_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);

	if (class_name != NULL)
		duk_pop_3(ctx);
	duk_pop(ctx);
}

void
api_define_object(duk_context* ctx, const char* namespace_name, const char* name, const char* class_name, void* udata)
{
	duk_push_global_object(ctx);

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!duk_get_prop_string(ctx, -1, namespace_name)) {
			duk_pop(ctx);
			duk_push_string(ctx, namespace_name);
			duk_push_object(ctx);
			duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
				| DUK_DEFPROP_SET_WRITABLE
				| DUK_DEFPROP_SET_CONFIGURABLE);
			duk_get_prop_string(ctx, -1, namespace_name);
		}
	}

	duk_push_string(ctx, name);
	duk_push_class_obj(ctx, class_name, udata);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	if (namespace_name != NULL)
		duk_pop(ctx);
	duk_pop(ctx);
}

void
api_define_property(duk_context* ctx, const char* class_name, const char* name, duk_c_function getter, duk_c_function setter)
{
	duk_uint_t flags;
	int        obj_index;

	duk_push_global_object(ctx);
	if (class_name != NULL) {
		duk_push_global_stash(ctx);
		duk_get_prop_string(ctx, -1, "prototypes");
		duk_get_prop_string(ctx, -1, class_name);
	}
	obj_index = duk_normalize_index(ctx, -1);
	duk_push_string(ctx, name);
	flags = DUK_DEFPROP_SET_CONFIGURABLE;
	if (getter != NULL) {
		duk_push_c_function(ctx, getter, DUK_VARARGS);
		flags |= DUK_DEFPROP_HAVE_GETTER;
	}
	if (setter != NULL) {
		duk_push_c_function(ctx, setter, DUK_VARARGS);
		flags |= DUK_DEFPROP_HAVE_SETTER;
	}
	duk_def_prop(ctx, obj_index, flags);
	if (class_name != NULL)
		duk_pop_3(ctx);
	duk_pop(ctx);
}

void
api_define_static_prop(duk_context* ctx, const char* namespace_name, const char* name, duk_c_function getter, duk_c_function setter)
{
	int       flags;
	duk_idx_t obj_index;

	duk_push_global_object(ctx);

	// ensure the namespace object exists
	if (namespace_name != NULL) {
		if (!duk_get_prop_string(ctx, -1, namespace_name)) {
			duk_pop(ctx);
			duk_push_string(ctx, namespace_name);
			duk_push_object(ctx);
			duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
				| DUK_DEFPROP_SET_WRITABLE
				| DUK_DEFPROP_SET_CONFIGURABLE);
			duk_get_prop_string(ctx, -1, namespace_name);
		}
	}

	obj_index = duk_normalize_index(ctx, -1);
	duk_push_string(ctx, name);
	flags = DUK_DEFPROP_SET_CONFIGURABLE;
	if (getter != NULL) {
		duk_push_c_function(ctx, getter, DUK_VARARGS);
		flags |= DUK_DEFPROP_HAVE_GETTER;
	}
	if (setter != NULL) {
		duk_push_c_function(ctx, setter, DUK_VARARGS);
		flags |= DUK_DEFPROP_HAVE_SETTER;
	}
	duk_def_prop(ctx, obj_index, flags);
	if (namespace_name != NULL)
		duk_pop(ctx);
	duk_pop(ctx);
}

void
duk_error_blame(duk_context* ctx, int blame_offset, duk_errcode_t err_code, const char* fmt, ...)
{
	va_list ap;
	char*   filename;
	int     line_number;

	// get filename and line number from Duktape call stack
	dukrub_inspect_callstack_entry(ctx, -1 + blame_offset);
	if (!duk_is_object(ctx, -1)) {
		// note: the topmost call is assumed to be a Duktape/C activation.  that's
		//       probably not what's responsible for the error, so blame the function
		//       just below it if there's nobody else to blame.
		dukrub_inspect_callstack_entry(ctx, -2);
		duk_replace(ctx, -2);
	}
	duk_get_prop_string(ctx, -1, "lineNumber");
	duk_get_prop_string(ctx, -2, "function");
	duk_get_prop_string(ctx, -1, "fileName");
	filename = strdup(duk_safe_to_string(ctx, -1));
	line_number = duk_to_int(ctx, -3);
	duk_pop_n(ctx, 4);

	// construct an Error object
	va_start(ap, fmt);
	duk_push_error_object_va(ctx, err_code, fmt, ap);
	va_end(ap);
	duk_push_string(ctx, filename);
	duk_put_prop_string(ctx, -2, "fileName");
	duk_push_int(ctx, line_number);
	duk_put_prop_string(ctx, -2, "lineNumber");
	free(filename);

	duk_throw(ctx);
}

duk_bool_t
duk_is_class_obj(duk_context* ctx, duk_idx_t index, const char* class_name)
{
	const char* obj_class_name;
	duk_bool_t  result;

	index = duk_require_normalize_index(ctx, index);
	if (!duk_is_object_coercible(ctx, index))
		return 0;

	duk_get_prop_string(ctx, index, "\xFF" "ctor");
	obj_class_name = duk_safe_to_string(ctx, -1);
	result = strcmp(obj_class_name, class_name) == 0;
	duk_pop(ctx);
	return result;
}

void
duk_push_class_obj(duk_context* ctx, const char* class_name, void* udata)
{
	duk_push_object(ctx);
	duk_to_class_obj(ctx, -1, class_name, udata);
	
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "prototypes");
	duk_get_prop_string(ctx, -1, class_name);
	duk_set_prototype(ctx, -4);
	duk_pop_2(ctx);
}

void*
duk_require_class_obj(duk_context* ctx, duk_idx_t index, const char* class_name)
{
	void* udata;

	index = duk_require_normalize_index(ctx, index);
	if (!duk_is_class_obj(ctx, index, class_name))
		duk_error(ctx, DUK_ERR_TYPE_ERROR, "%s object required", class_name);
	duk_get_prop_string(ctx, index, "\xFF" "udata");
	udata = duk_get_pointer(ctx, -1);
	duk_pop(ctx);
	return udata;
}

void
duk_to_class_obj(duk_context* ctx, duk_idx_t idx, const char* class_name, void* udata)
{
	idx = duk_require_normalize_index(ctx, idx);

	duk_push_pointer(ctx, udata);
	duk_put_prop_string(ctx, idx, "\xFF" "udata");

	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "prototypes");
	duk_get_prop_string(ctx, -1, class_name);
	if (duk_get_prop_string(ctx, -1, "\xFF" "dtor"))
		duk_set_finalizer(ctx, idx);
	else
		duk_pop(ctx);
	duk_pop_3(ctx);
}
