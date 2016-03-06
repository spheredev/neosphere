#include "minisphere.h"
#include "script.h"

#include "api.h"
#include "debugger.h"
#include "utility.h"

struct script
{
	unsigned int  refcount;
	bool          is_in_use;
	duk_uarridx_t id;
};

static void      initialize_coffeescript (void);
static void      initialize_typescript   (void);
static script_t* script_from_js_function (void* heapptr);

static int       s_next_script_id = 0;

void
initialize_scripts(void)
{
	console_log(1, "initializing JS manager");
	
	duk_push_global_stash(g_duk);
	duk_push_array(g_duk);
	duk_put_prop_string(g_duk, -2, "scripts");
	duk_pop(g_duk);

	initialize_coffeescript();
	initialize_typescript();
}

void
shutdown_scripts(void)
{
	console_log(1, "shutting down JS manager");
}

bool
evaluate_script(const char* filename)
{
	const char*    extension;
	sfs_file_t*    file = NULL;
	path_t*        path;
	const char*    source_name;
	lstring_t*     source_text = NULL;
	char*          slurp;
	size_t         size;
	lstring_t*     transpiled_text;
	
	path = make_sfs_path(filename, NULL);
	source_name = get_source_name(path_cstr(path));
	if (!(slurp = sfs_fslurp(g_fs, filename, NULL, &size)))
		goto on_error;
	source_text = lstr_from_buf(slurp, size);
	free(slurp);

	// is it a CoffeeScript file? you know, I don't even like coffee.
	// now, Monster drinks on the other hand...
	extension = strrchr(filename, '.');
	if (extension != NULL && strcasecmp(extension, ".coffee") == 0) {
		if (!duk_get_global_string(g_duk, "CoffeeScript")) {
			duk_pop(g_duk);
			duk_error_ni(g_duk, -1, DUK_ERR_ERROR, "no CoffeeScript support (%s)", filename);
		}
		duk_get_prop_string(g_duk, -1, "compile");
		duk_push_lstring_t(g_duk, source_text);
		duk_push_object(g_duk);
		duk_push_string(g_duk, source_name);
		duk_put_prop_string(g_duk, -2, "filename");
		duk_push_true(g_duk);
		duk_put_prop_string(g_duk, -2, "bare");
		if (duk_pcall(g_duk, 2) != DUK_EXEC_SUCCESS)
			goto on_error;
		duk_remove(g_duk, -2);
		transpiled_text = duk_require_lstring_t(g_duk, -1);
		cache_source(source_name, transpiled_text);
		lstr_free(transpiled_text);
	}
	// TypeScript?
	else if (extension != NULL && strcasecmp(extension, ".ts") == 0) {
		if (!duk_get_global_string(g_duk, "ts")) {
			duk_pop(g_duk);
			duk_error_ni(g_duk, -1, DUK_ERR_ERROR, "no TypeScript support (%s)", filename);
		}
		duk_get_prop_string(g_duk, -1, "transpile");
		duk_push_lstring_t(g_duk, source_text);
		duk_push_object(g_duk);
		duk_push_boolean(g_duk, true);
		duk_put_prop_string(g_duk, -2, "noImplicitUseStrict");
		duk_push_string(g_duk, source_name);
		if (duk_pcall(g_duk, 3) != DUK_EXEC_SUCCESS)
			goto on_error;
		duk_remove(g_duk, -2);
		transpiled_text = duk_require_lstring_t(g_duk, -1);
		cache_source(source_name, transpiled_text);
		lstr_free(transpiled_text);
	}
	else
		duk_push_lstring_t(g_duk, source_text);

	// ready for launch in T-10...9...*munch*
	duk_push_string(g_duk, source_name);
	if (duk_pcompile(g_duk, DUK_COMPILE_EVAL) != DUK_EXEC_SUCCESS)
		goto on_error;
	if (duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS)
		goto on_error;
	
	lstr_free(source_text);
	path_free(path);
	return true;

on_error:
	lstr_free(source_text);
	path_free(path);
	if (!duk_is_error(g_duk, -1))
		duk_push_error_object(g_duk, DUK_ERR_ERROR, "script '%s' not found\n", filename);
	return false;
}

script_t*
compile_script(const lstring_t* source, const char* fmt_name, ...)
{
	va_list    ap;
	lstring_t* name;
	script_t*  script;
	
	va_start(ap, fmt_name);
	name = lstr_vnewf(fmt_name, ap);
	va_end(ap);
	script = calloc(1, sizeof(script_t));

	console_log(3, "compiling script #%u as '%s'", s_next_script_id, lstr_cstr(name));
	
	// this wouldn't be necessary if Duktape duk_get_heapptr() gave us a strong reference.
	// instead we get this ugliness where the compiled function is saved in the global stash
	// so it doesn't get eaten by the garbage collector. yummy!
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "scripts");
	duk_push_lstring_t(g_duk, source);
	duk_push_lstring_t(g_duk, name);
	duk_compile(g_duk, 0x0);
	duk_put_prop_index(g_duk, -2, s_next_script_id);
	duk_pop_2(g_duk);

	cache_source(lstr_cstr(name), source);
	lstr_free(name);

	script->id = s_next_script_id++;
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
	
	console_log(3, "disposing script #%u no longer in use", script->id);
	
	// unstash the compiled function, it's now safe to GC
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "scripts");
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
	
	// check whether an instance of the script is already running.
	// if it is, but the script is reentrant, allow it. otherwise, return early
	// to prevent multiple invocation.
	if (script->is_in_use && !allow_reentry) {
		console_log(3, "skipping execution of script #%u, already in use", script->id);
		return;
	}
	was_in_use = script->is_in_use;

	console_log(4, "executing script #%u", script->id);

	// ref the script in case it gets freed during execution. the owner
	// may be destroyed in the process and we don't want to end up crashing.
	ref_script(script);
	
	// get the compiled script from the stash and run it. so dumb...
	script->is_in_use = true;
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "scripts");
	duk_get_prop_index(g_duk, -1, script->id);
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
		script = compile_script(codestring, "%s", name);
		lstr_free(codestring);
	}
	else if (duk_is_null_or_undefined(ctx, index))
		return NULL;
	else
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "script must be string, function, or null/undefined");
	return script;
}

static void
initialize_coffeescript(void)
{
	if (sfs_fexist(g_fs, "~sys/coffee-script.js", NULL)) {
		if (evaluate_script("~sys/coffee-script.js")) {
			if (!duk_get_global_string(g_duk, "CoffeeScript")) {
				duk_pop_2(g_duk);
				console_log(1, "    'CoffeeScript' not defined");
				goto on_error;
			}
			duk_get_prop_string(g_duk, -1, "VERSION");
			console_log(1, "    CoffeeScript %s", duk_get_string(g_duk, -1));
			duk_pop_3(g_duk);
		}
		else {
			console_log(1, "    error evaluating coffee-script.js");
			console_log(1, "    %s", duk_to_string(g_duk, -1));
			duk_pop(g_duk);
			goto on_error;
		}
	}
	else {
		console_log(1, "  coffee-script.js is missing");
		goto on_error;
	}
	return;

on_error:
	console_log(1, "    CoffeeScript support not enabled");
}

static void
initialize_typescript(void)
{
	if (sfs_fexist(g_fs, "~sys/typescriptServices.js", NULL)) {
		if (evaluate_script("~sys/typescriptServices.js")) {
			if (!duk_get_global_string(g_duk, "ts")) {
				duk_pop_2(g_duk);
				console_log(1, "    'ts' not defined");
				goto on_error;
			}
			duk_get_prop_string(g_duk, -1, "version");
			console_log(1, "    TypeScript %s", duk_get_string(g_duk, -1));
			duk_pop_3(g_duk);
		}
		else {
			console_log(1, "    error evaluating typescriptServices.js");
			console_log(1, "    %s", duk_to_string(g_duk, -1));
			duk_pop(g_duk);
			goto on_error;
		}
	}
	else {
		console_log(1, "  typescriptServices.js is missing");
		goto on_error;
	}
	return;

on_error:
	console_log(1, "    TypeScript support not enabled");
}

static script_t*
script_from_js_function(void* heapptr)
{
	script_t* script;

	if (!(script = calloc(1, sizeof(script_t))))
		return NULL;

	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "scripts");
	duk_push_heapptr(g_duk, heapptr);
	duk_put_prop_index(g_duk, -2, s_next_script_id);
	duk_pop_2(g_duk);

	script->id = s_next_script_id++;
	return ref_script(script);
}
