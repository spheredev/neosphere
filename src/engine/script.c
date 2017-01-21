#include "minisphere.h"
#include "script.h"

#include "api.h"
#include "debugger.h"
#include "pegasus.h"
#include "utility.h"

struct script
{
	unsigned int  refcount;
	unsigned int  id;
	bool          is_in_use;
	void*         heapptr;
};

static int s_next_script_id = 0;

void
scripts_init(void)
{
	console_log(1, "initializing JS script manager");
}

void
scripts_uninit(void)
{
	console_log(1, "shutting down JS script manager");
}

bool
build_exec(const char* filename, bool as_module)
{
	duk_idx_t      duk_top;
	sfs_file_t*    file = NULL;
	path_t*        path = NULL;
	const char*    source_name;
	lstring_t*     source_text = NULL;
	char*          slurp;
	size_t         size;

	duk_top = duk_get_top(g_duk);
	
	if (as_module) {
		// the existence check here is needed because eval_module() will segfault if the
		// file doesn't exist.  it's an ugly hack, but a proper fix needs some refactoring
		// that I'm not up for right now.
		if (!sfs_fexist(g_fs, filename, NULL))
			goto on_error;
		if (!duk_pegasus_eval_module(g_duk, filename))
			goto on_error;
		return true;
	}
	else {
		path = fs_make_path(filename, NULL, false);
		source_name = get_source_name(path_cstr(path));
		if (!(slurp = sfs_fslurp(g_fs, filename, NULL, &size)))
			goto on_error;
		source_text = lstr_from_cp1252(slurp, size);
		free(slurp);

		// ready for launch in T-10...9...*munch*
		duk_push_lstring_t(g_duk, source_text);
		duk_push_string(g_duk, source_name);
		if (duk_pcompile(g_duk, DUK_COMPILE_EVAL) != DUK_EXEC_SUCCESS)
			goto on_error;
		if (duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS)
			goto on_error;

		lstr_free(source_text);
		path_free(path);
		return true;
	}

on_error:
	lstr_free(source_text);
	path_free(path);
	if (duk_get_top(g_duk) == duk_top)  // note: ugly hack, refactor
		duk_push_error_object(g_duk, DUK_ERR_ERROR, "script not found `%s`\n", filename);
	return false;
}

script_t*
script_new(const lstring_t* source, const char* fmt_name, ...)
{
	va_list    ap;
	void*      heapptr;
	lstring_t* name;
	script_t*  script;
	
	va_start(ap, fmt_name);
	name = lstr_vnewf(fmt_name, ap);
	va_end(ap);
	script = calloc(1, sizeof(script_t));

	console_log(3, "compiling script #%u as `%s`", s_next_script_id, lstr_cstr(name));
	
	// compile the source.  Duktape gives us a function back; save its heap pointer so
	// we can call the script later.
	duk_push_lstring_t(g_duk, source);
	duk_push_lstring_t(g_duk, name);
	duk_compile(g_duk, 0x0);
	heapptr = duk_ref_heapptr(g_duk, -1);
	duk_pop(g_duk);

	cache_source(lstr_cstr(name), source);
	lstr_free(name);

	script->id = s_next_script_id++;
	script->heapptr = heapptr;
	return script_ref(script);
}

script_t*
script_new_func(duk_context* ctx, duk_idx_t idx)
{
	void*     heapptr;
	script_t* script;

	idx = duk_require_normalize_index(ctx, idx);
	duk_require_function(ctx, idx);
	heapptr = duk_ref_heapptr(ctx, idx);

	if (!(script = calloc(1, sizeof(script_t))))
		return NULL;
	script->id = s_next_script_id++;
	script->heapptr = heapptr;
	return script_ref(script);
}

script_t*
script_ref(script_t* script)
{
	++script->refcount;
	return script;
}

void
script_free(script_t* script)
{
	if (script == NULL || --script->refcount > 0)
		return;
	
	console_log(3, "disposing script #%u no longer in use", script->id);

	duk_unref_heapptr(g_duk, script->heapptr);
	free(script);
}

void
script_run(script_t* script, bool allow_reentry)
{
	bool was_in_use;

	if (script == NULL)  // NULL is allowed, it's a no-op
		return;
	
	// check whether an instance of the script is already running.
	// if it is, but the script is reentrant, allow it.  otherwise, return early
	// to prevent multiple invocation.
	if (script->is_in_use && !allow_reentry) {
		console_log(3, "skipping execution of script #%u, already in use", script->id);
		return;
	}
	was_in_use = script->is_in_use;

	console_log(4, "executing script #%u", script->id);

	// ref the script in case it gets freed during execution.  the owner
	// may be destroyed in the process and we don't want to end up crashing.
	script_ref(script);
	
	// execute the script!
	script->is_in_use = true;
	duk_push_heapptr(g_duk, script->heapptr);
	duk_call(g_duk, 0);
	duk_pop(g_duk);
	script->is_in_use = was_in_use;

	script_free(script);
}
