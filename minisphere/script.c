#include "minisphere.h"

int
compile_script(const lstring_t* script, const char* name)
{
	int index;

	duk_push_global_stash(g_duktape);
	if (!duk_get_prop_string(g_duktape, -1, "scripts")) {
		duk_pop(g_duktape);
		duk_push_array(g_duktape); duk_put_prop_string(g_duktape, -2, "scripts");
		duk_get_prop_string(g_duktape, -1, "scripts");
	}
	duk_get_prop_string(g_duktape, -1, "length"); index = duk_get_int(g_duktape, -1); duk_pop(g_duktape);
	duk_push_string(g_duktape, name);
	duk_compile_lstring_filename(g_duktape, 0x0, script->cstr, script->length);
	duk_put_prop_index(g_duktape, -2, index);
	duk_pop_2(g_duktape);
	return index + 1;
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
