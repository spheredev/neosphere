#include "minisphere.h"
#include "commonjs.h"

#include "api.h"

static duk_ret_t js_require (duk_context* ctx);

duk_int_t
duk_peval_module(const char* filename)
{
	// HERE BE DRAGONS!
	// this function is horrendous.  Duktape's stack-based API is powerful, but gets
	// very messy very quickly when dealing with object properties.  I tried to add
	// comments to hint at what's going on, but it's still likely to be confusing for
	// someone not familiar with the code.  proceed with caution.

	// notes:
	//     - the final value of `module.exports` is left on top of the Duktape value stack.
	//     - `module.id` is set to the given filename.  in order to guarantee proper cache
	//       behavior, the filename should be canonicalized first.
	//     - if the module code throws, the exception will propogate out of this call.

	size_t  code_size;
	char*   code_string;

	// is the requested module already in the cache?
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "moduleCache");
	if (duk_get_prop_string(g_duk, -1, filename)) {
		duk_remove(g_duk, -2);
		duk_remove(g_duk, -2);
		goto have_module;
	}
	else {
		duk_pop_3(g_duk);
	}

	// synthesize a function to wrap the module code.  this is the easiest way to
	// guarantee CommonJS semantics and matches the behavior of Node.js.
	code_string = sfs_fslurp(g_fs, filename, NULL, &code_size);
	duk_push_sprintf(g_duk, "(function(exports,require,module){%s})", code_string);
	duk_push_string(g_duk, filename);
	duk_compile(g_duk, DUK_COMPILE_EVAL);
	duk_call(g_duk, 0);
	free(code_string);

	// construct a `module` object for the new module
	duk_push_object(g_duk);
	duk_push_object(g_duk);
	duk_put_prop_string(g_duk, -2, "exports");  // module.exports = {}
	duk_push_string(g_duk, filename);
	duk_put_prop_string(g_duk, -2, "filename");  // module.filename
	duk_push_string(g_duk, filename);
	duk_put_prop_string(g_duk, -2, "id");  // module.id
	duk_push_false(g_duk);
	duk_put_prop_string(g_duk, -2, "loaded");  // module.loaded = false

	// cache the `module` object.  this is done in advance so that circular
	// requires don't topple the stack.
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "moduleCache");
	duk_dup(g_duk, -3);
	duk_put_prop_string(g_duk, -2, filename);
	duk_pop_2(g_duk);

	// move the `module` object above the function about to be called.
	// we'll need it afterwards.
	duk_insert(g_duk, -2);

	// set up to call the synthesized function
	duk_get_prop_string(g_duk, -2, "exports");  // exports
	duk_push_c_function(g_duk, js_require, DUK_VARARGS);  // require
	duk_push_string(g_duk, "cache");
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "moduleCache");
	duk_remove(g_duk, -2);
	duk_def_prop(g_duk, -3, DUK_DEFPROP_HAVE_VALUE);  // require.cache
	duk_push_string(g_duk, "id");
	duk_push_string(g_duk, filename);
	duk_def_prop(g_duk, -3, DUK_DEFPROP_HAVE_VALUE);  // require.id
	duk_dup(g_duk, -4);  // module

	// go, go, go!
	if (duk_pcall(g_duk, 3) != DUK_EXEC_SUCCESS) {
		// hm, the module threw an error during initialization.  the caller may
		// want to retry, so we'll remove it from the cache...
		duk_push_global_stash(g_duk);
		duk_get_prop_string(g_duk, -1, "moduleCache");
		duk_del_prop_string(g_duk, -1, filename);
		duk_pop_2(g_duk);
		duk_remove(g_duk, -2);  // ...and leave the error on the stack.
		return DUK_EXEC_ERROR;
	}
	duk_pop(g_duk);
	
	// set `module.loaded` to true
	duk_push_true(g_duk);
	duk_put_prop_string(g_duk, -2, "loaded");

have_module:
	// `module` should be on the stack, we need `module.exports`
	duk_get_prop_string(g_duk, -1, "exports");
	duk_remove(g_duk, -2);
	return DUK_EXEC_SUCCESS;
}

path_t*
cjs_resolve(const char* id, const char* origin, const char* sys_origin)
{
	const char* const filenames[] =
	{
		"%s",
		"%s.js",
		"%s/index.js",
	};
	
	path_t* origin_path;
	char*   filename;
	path_t* path;

	int i;

	if (strlen(id) >= 2 && (strncmp(id, "./", 2) == 0 || strncmp(id, "../", 3) == 0))
		// resolve module relative to calling module
		origin_path = path_new(origin);
	else
		// resolve module from designated module repository
		origin_path = path_new(sys_origin);
	
	for (i = 0; i < (int)(sizeof(filenames) / sizeof(filenames[0])); ++i) {
		path = path_dup(origin_path);
		filename = strnewf(filenames[i], id);
		path_strip(path);
		path_append(path, filename);
		path_collapse(path, true);
		free(filename);
		if (sfs_fexist(g_fs, path_cstr(path), NULL))
			return path;
		else
			path_free(path);
	}
	return NULL;
}



void
init_commonjs_api(void)
{
	api_register_function(g_duk, NULL, "require", js_require);

	duk_push_global_stash(g_duk);
	duk_push_object(g_duk);
	duk_put_prop_string(g_duk, -2, "moduleCache");
}

static duk_ret_t
js_require(duk_context* ctx)
{
	const char* caller_id;
	const char* id;
	path_t*     path;

	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, "id");
	caller_id = duk_get_string(ctx, -1);
	id = duk_require_string(ctx, 0);

	if (!(path = cjs_resolve(id, caller_id, "lib/"))
		&& !(path = cjs_resolve(id, caller_id, "#/modules/")))
	{
		duk_error_ni(g_duk, -1, DUK_ERR_REFERENCE_ERROR, "unable to resolve require `%s`", id);
	}
	if (duk_peval_module(path_cstr(path)) != DUK_EXEC_SUCCESS)
		duk_throw(ctx);
	return 1;
}
