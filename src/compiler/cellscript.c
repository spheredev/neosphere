#include "cell.h"
#include "cellscript.h"

#include "assets.h"

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
	api_define_static_prop(ctx, "system", "name", js_system_get_name, NULL);
	api_define_static_prop(ctx, "system", "version", js_system_get_version, NULL);
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
