#include "minisphere.h"
#include "api.h"

struct script
{
	bool          is_in_use;
	duk_uarridx_t id;
};

static script_t* script_from_js_function (void* heapptr);

static int s_next_id = 0;

script_t*
compile_script(const lstring_t* codestring, const char* name)
{
	script_t* script;
	
	if (!(script = calloc(1, sizeof(script_t))))
		return NULL;
	
	// this wouldn't be necessary if Duktape duk_get_heapptr() would give us a strong reference, but alas,
	// we're stuck with this ugliness where we store the compiled function in the global stash so it doesn't
	// get garbage collected.
	duk_push_global_stash(g_duk);
	if (!duk_get_prop_string(g_duk, -1, "scripts")) {
		duk_pop(g_duk);
		duk_push_array(g_duk); duk_put_prop_string(g_duk, -2, "scripts");
		duk_get_prop_string(g_duk, -1, "scripts");
	}
	script->id = s_next_id++;
	duk_push_string(g_duk, name);
	duk_compile_lstring_filename(g_duk, 0x0, codestring->cstr, codestring->length);
	duk_put_prop_index(g_duk, -2, script->id);
	duk_pop_2(g_duk);

	return script;
}

void
free_script(script_t* script)
{
	if (script == NULL)
		return;
	
	// unstash the compiled function, it's now safe to GC
	duk_push_global_stash(g_duk);
	if (!duk_get_prop_string(g_duk, -1, "scripts")) {
		duk_pop(g_duk);
		duk_push_array(g_duk);
	}
	duk_del_prop_index(g_duk, -1, script->id);
	duk_pop_2(g_duk);

	free(script);
}

void
run_script(script_t* script, bool allow_reentry)
{
	bool was_in_use;

	if (script == NULL)  // NULL is allowed, it's a no-op
		return;
	
	// is the script currently in use?
	if (script->is_in_use && !allow_reentry)
		return;  // do nothing if an instance is already running
	was_in_use = script->is_in_use;

	// retrieve the compiled script from the stash (so ugly...)
	duk_push_global_stash(g_duk);
	if (!duk_get_prop_string(g_duk, -1, "scripts")) {
		duk_pop(g_duk);
		duk_push_array(g_duk);
	}
	duk_get_prop_index(g_duk, -1, script->id);
	
	// execute the script
	duk_call(g_duk, 0);
	duk_pop(g_duk);
	
	script->is_in_use = was_in_use;
	duk_pop_2(g_duk);
}

script_t*
duk_require_sphere_script(duk_context* ctx, duk_idx_t index, const char* name)
{
	lstring_t* codestring;
	script_t*  script;

	index = duk_require_normalize_index(ctx, index);

	if (duk_is_callable(ctx, index)) {
		// caller passed function directly
		script = script_from_js_function(duk_get_heapptr(ctx, index));
	}
	else if (duk_is_string(ctx, index)) {
		// caller passed code string, compile it
		codestring = duk_require_lstring_t(ctx, index);
		script = compile_script(codestring, name);
		free_lstring(codestring);
	}
	else if (duk_is_null(ctx, index))
		return 0;
	else {
		duk_pop_2(ctx);
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "script must be string, function, or null");
	}
	return script;
}

static script_t*
script_from_js_function(void* heapptr)
{
	script_t* script;

	if (!(script = calloc(1, sizeof(script_t))))
		return NULL;

	duk_push_global_stash(g_duk);
	if (!duk_get_prop_string(g_duk, -1, "scripts")) {
		duk_pop(g_duk);
		duk_push_array(g_duk); duk_put_prop_string(g_duk, -2, "scripts");
		duk_get_prop_string(g_duk, -1, "scripts");
	}
	script->id = s_next_id++;
	duk_push_heapptr(g_duk, heapptr);
	duk_put_prop_index(g_duk, -2, script->id);
	duk_pop_2(g_duk);

	return script;
}
