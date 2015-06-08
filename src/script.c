#include "minisphere.h"
#include "api.h"
#include "utility.h"

struct script
{
	unsigned int  refcount;
	bool          is_in_use;
	duk_uarridx_t id;
};

static script_t* script_from_js_function (void* heapptr);

static int s_next_id = 0;

bool
try_evaluate_file(const char* path)
{
	const char* err_msg;
	FILE*       file = NULL;
	char*       source = NULL;
	size_t      size;

	duk_peval_file(g_duk, path);
	if (duk_is_error(g_duk, -1)) {
		duk_dup(g_duk, -1);
		err_msg = duk_to_string(g_duk, -1);
		if (strstr(err_msg, "char decode failed") == NULL) {
			duk_pop(g_duk);
			return false;
		}
		else {
			duk_pop(g_duk);
			if (!(file = fopen(path, "rb"))) goto on_error;
			fseek(file, 0, SEEK_END); size = ftell(file);
			fseek(file, 0, SEEK_SET);
			if (!(source = malloc(size + 1))) goto on_error;
			fread(source, 1, size, file); source[size] = '\0';
			fclose(file);
			duk_pop(g_duk);
			duk_push_cstr_to_utf8(g_duk, source);
			duk_push_string(g_duk, path);
			free(source); source = NULL;
			if (duk_pcompile(g_duk, DUK_COMPILE_EVAL) != DUK_EXEC_SUCCESS)
				goto on_error;
			if (duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS)
				goto on_error;
		}
	}
	return true;

on_error:
	if (file != NULL) fclose(file);
	free(source);
	if (!duk_is_error(g_duk, -1))
		duk_push_error_object(g_duk, DUK_ERR_ERROR, "internal error");
	return false;
}

script_t*
compile_script(const lstring_t* source, bool is_cp1252, const char* fmt_name, ...)
{
	va_list ap;
	
	script_t* script;
	
	if (!(script = calloc(1, sizeof(script_t))))
		return NULL;
	
	// this wouldn't be necessary if Duktape duk_get_heapptr() would give us a strong reference, but alas,
	// we're stuck with this ugliness where we store the compiled function in the global stash so it doesn't
	// get eaten by the garbage collector.
	duk_push_global_stash(g_duk);
	if (!duk_get_prop_string(g_duk, -1, "scripts")) {
		duk_pop(g_duk);
		duk_push_array(g_duk); duk_put_prop_string(g_duk, -2, "scripts");
		duk_get_prop_string(g_duk, -1, "scripts");
	}
	script->id = s_next_id++;
	if (is_cp1252)
		duk_push_cstr_to_utf8(g_duk, lstr_cstr(source));
	else
		duk_push_lstring(g_duk, source->cstr, source->length);
	va_start(ap, fmt_name);
	duk_push_vsprintf(g_duk, fmt_name, ap);
	va_end(ap);
	duk_compile(g_duk, 0x0);
	duk_put_prop_index(g_duk, -2, script->id);
	duk_pop_2(g_duk);

	return ref_script(script);
}

script_t*
ref_script(script_t* script)
{
	++script->refcount;
	return script;
}

void
free_script(script_t* script)
{
	if (script == NULL || --script->refcount > 0)
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
	
	if (script->is_in_use && !allow_reentry)
		return;  // do nothing if an instance is already running
	was_in_use = script->is_in_use;

	// we need to ref the script in case it gets freed during execution,
	// as the owner may be destroyed in the process.
	ref_script(script);
	
	// retrieve the compiled script from the stash (so ugly...)
	duk_push_global_stash(g_duk);
	if (!duk_get_prop_string(g_duk, -1, "scripts")) {
		duk_pop(g_duk);
		duk_push_array(g_duk);
	}
	duk_get_prop_index(g_duk, -1, script->id);
	
	// execute the script
	script->is_in_use = true;
	duk_call(g_duk, 0);
	duk_pop_3(g_duk);
	script->is_in_use = was_in_use;

	free_script(script);
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
		script = compile_script(codestring, false, "%s", name);
		free_lstring(codestring);
	}
	else if (duk_is_null_or_undefined(ctx, index))
		return 0;
	else {
		duk_pop_2(ctx);
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "Script must be string, function, or null/undefined");
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

	return ref_script(script);
}
