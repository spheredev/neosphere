#include "minisphere.h"
#include "api.h"

int
compile_script(const lstring_t* codestring, const char* name)
{
	int script_id;
	
	duk_push_global_stash(g_duktape);
	if (!duk_get_prop_string(g_duktape, -1, "scripts")) {
		duk_pop(g_duktape);
		duk_push_array(g_duktape); duk_put_prop_string(g_duktape, -2, "scripts");
		duk_get_prop_string(g_duktape, -1, "scripts");
	}
	duk_get_prop_string(g_duktape, -1, "length"); script_id = duk_get_int(g_duktape, -1) + 1; duk_pop(g_duktape);
	duk_push_string(g_duktape, name);
	duk_compile_lstring_filename(g_duktape, 0x0, codestring->cstr, codestring->length);
	duk_put_prop_index(g_duktape, -2, script_id - 1);
	duk_pop_2(g_duktape);
	return script_id;
}

void
free_script(int script_id)
{
	if (script_id == 0)
		return;
	duk_push_global_stash(g_duktape);
	if (!duk_get_prop_string(g_duktape, -1, "scripts")) {
		duk_pop(g_duktape);
		duk_push_array(g_duktape);
	}
	duk_push_null(g_duktape);
	duk_put_prop_index(g_duktape, -2, script_id - 1);
	duk_pop_2(g_duktape);
}

void
run_script(int script_id, bool allow_reentry)
{
	bool is_in_use;

	if (script_id == 0)  // script 0 is guaranteed to be a no-op
		return;
	duk_push_global_stash(g_duktape);
	if (!duk_get_prop_string(g_duktape, -1, "scripts")) {
		duk_pop(g_duktape);
		duk_push_array(g_duktape);
	}
	duk_get_prop_index(g_duktape, -1, script_id - 1);
	if (duk_is_callable(g_duktape, -1)) {
		duk_get_prop_string(g_duktape, -1, "isInUse");
		is_in_use = duk_to_boolean(g_duktape, -1);
		duk_pop(g_duktape);
		if (!is_in_use || allow_reentry) {
			duk_push_true(g_duktape);
			duk_put_prop_string(g_duktape, -2, "isInUse");
			duk_call(g_duktape, 0);
			duk_get_prop_index(g_duktape, -2, script_id - 1);
			if (!duk_is_null(g_duktape, -1)) {
				duk_push_boolean(g_duktape, is_in_use);
				duk_put_prop_string(g_duktape, -2, "isInUse");
			}
			duk_pop(g_duktape);
		}
	}
	duk_pop_3(g_duktape);
}

int
duk_require_sphere_script(duk_context* ctx, duk_idx_t index, const char* name)
{
	lstring_t* codestring;
	int        script_id;

	index = duk_require_normalize_index(ctx, index);

	if (duk_is_callable(ctx, index)) {
		// caller passed function directly
		duk_push_global_stash(g_duktape);
		if (!duk_get_prop_string(g_duktape, -1, "scripts")) {
			duk_pop(g_duktape);
			duk_push_array(g_duktape); duk_put_prop_string(g_duktape, -2, "scripts");
			duk_get_prop_string(g_duktape, -1, "scripts");
		}
		duk_get_prop_string(g_duktape, -1, "length"); script_id = duk_get_int(g_duktape, -1) + 1; duk_pop(g_duktape);
		duk_dup(ctx, index);
		duk_put_prop_index(g_duktape, -2, script_id - 1);
		duk_pop_2(g_duktape);
	}
	else if (duk_is_string(ctx, index)) {
		// caller passed code string, compile it
		codestring = duk_require_lstring_t(ctx, index);
		script_id = compile_script(codestring, name);
		free_lstring(codestring);
	}
	else if (duk_is_null(ctx, index))
		return 0;
	else {
		duk_pop_2(ctx);
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "Script argument must be a string or function, or null");
	}
	return script_id;
}
