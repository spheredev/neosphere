#include "cell.h"
#include "api.h"

#include "assets.h"
#include "encoding.h"

static void define_accessor    (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function getter, duk_c_function setter);
static void define_function    (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);
static void define_static_prop (duk_context* ctx, const char* namespace_name, const char* name, duk_c_function getter, duk_c_function setter);

static duk_ret_t js_system_get_name    (duk_context* ctx);
static duk_ret_t js_system_get_version (duk_context* ctx);

void
api_init(duk_context* ctx)
{
	// JavaScript 'global' binding (like Node.js)
	duk_push_global_object(ctx);
	duk_push_string(ctx, "global");
	duk_push_global_object(ctx);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_CLEAR_CONFIGURABLE);
	duk_pop(ctx);

	// register the Cellscript API
	define_static_prop(ctx, "system", "name", js_system_get_name, NULL);
	define_static_prop(ctx, "system", "version", js_system_get_version, NULL);
}

static void
define_accessor(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function getter, duk_c_function setter)
{
	duk_uint_t flags;
	int        obj_index;

	duk_push_global_object(ctx);
	if (ctor_name != NULL) {
		duk_push_global_stash(ctx);
		duk_get_prop_string(ctx, -1, "prototypes");
		duk_get_prop_string(ctx, -1, ctor_name);
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
	if (ctor_name != NULL)
		duk_pop_3(ctx);
	duk_pop(ctx);
}

static void
define_function(duk_context* ctx, const char* namespace_name, const char* name, duk_c_function fn)
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

static void
define_static_prop(duk_context* ctx, const char* namespace_name, const char* name, duk_c_function getter, duk_c_function setter)
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

static no_return
duk_error_blamed(duk_context* ctx, int blame_offset, duk_errcode_t err_code, const char* fmt, ...)
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

static duk_ret_t
js_system_get_name(duk_context * ctx)
{
	duk_push_string(ctx, COMPILER_NAME);
	return 1;
}

static duk_ret_t
js_system_get_version(duk_context * ctx)
{
	duk_push_string(ctx, VERSION_NAME);
	return 1;
}
