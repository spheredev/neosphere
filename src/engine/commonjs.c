#include "minisphere.h"
#include "commonjs.h"

#include "api.h"
#include "transpile.h"

static duk_ret_t js_require (duk_context* ctx);

static void    duk_push_require_function (duk_context* ctx, const char* module_id);
static path_t* find_module               (const char* id, const char* origin, const char* sys_origin);
static path_t* load_package_json         (const char* filename);

bool
cjs_eval_module(const char* filename)
{
	// HERE BE DRAGONS!
	// this function is horrendous.  Duktape's stack-based API is powerful, but gets
	// very messy very quickly when dealing with object properties.  I tried to add
	// comments to illuminate what's going on, but it's still likely to be confusing for
	// someone not familiar with Duktape code.  proceed with caution.

	// notes:
	//     - the final value of `module.exports` is left on top of the Duktape value stack.
	//     - `module.id` is set to the given filename.  in order to guarantee proper cache
	//       behavior, the filename should be in canonical form.
	//     - as this is a protected call, if the module throws, the error will be caught
	//       and left on top of the stack for the caller to deal with.

	lstring_t* code_string;
	path_t*    dir_path;
	path_t*    file_path;
	size_t     source_size;
	char*      source;

	file_path = path_new(filename);
	dir_path = path_strip(path_dup(file_path));

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

	console_log(1, "initializing JS module `%s`", filename);

	source = sfs_fslurp(g_fs, filename, NULL, &source_size);
	code_string = lstr_from_buf(source, source_size);
	free(source);
	if (!transpile_to_js(&code_string, filename)) {
		lstr_free(code_string);
		return false;
	}

	// construct a module object for the new module
	duk_push_object(g_duk);  // module object
	duk_push_object(g_duk);
	duk_put_prop_string(g_duk, -2, "exports");  // module.exports = {}
	duk_push_string(g_duk, filename);
	duk_put_prop_string(g_duk, -2, "filename");  // module.filename
	duk_push_string(g_duk, filename);
	duk_put_prop_string(g_duk, -2, "id");  // module.id
	duk_push_false(g_duk);
	duk_put_prop_string(g_duk, -2, "loaded");  // module.loaded = false
	duk_push_require_function(g_duk, filename);
	duk_put_prop_string(g_duk, -2, "require");  // module.require

	// cache the module object in advance
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "moduleCache");
	duk_dup(g_duk, -3);
	duk_put_prop_string(g_duk, -2, filename);
	duk_pop_2(g_duk);

	if (strcmp(path_ext_cstr(file_path), ".json") == 0) {
		// JSON file, decode to JavaScript object
		duk_push_lstring_t(g_duk, code_string);
		lstr_free(code_string);
		if (duk_json_pdecode(g_duk) != DUK_EXEC_SUCCESS)
			goto on_error;
		duk_put_prop_string(g_duk, -2, "exports");
	}
	else {
		// synthesize a function to wrap the module code.  this is the easiest way to
		// guarantee CommonJS semantics and matches the behavior of Node.js.
		duk_push_string(g_duk, "(function main(exports, require, module, __filename, __dirname) { ");
		duk_push_lstring_t(g_duk, code_string);
		duk_push_string(g_duk, " })");
		duk_concat(g_duk, 3);
		duk_push_string(g_duk, filename);
		duk_compile(g_duk, DUK_COMPILE_EVAL);
		duk_call(g_duk, 0);
		lstr_free(code_string);

		// go, go, go!
		duk_get_prop_string(g_duk, -2, "exports");  // exports
		duk_get_prop_string(g_duk, -3, "require");  // require
		duk_dup(g_duk, -4);  // module
		duk_push_string(g_duk, filename);  // __filename
		duk_push_string(g_duk, path_cstr(dir_path));  // __dirname
		if (duk_pcall(g_duk, 5) != DUK_EXEC_SUCCESS)
			goto on_error;
		duk_pop(g_duk);
	}
	
	// module executed successfully, set `module.loaded` to true
	duk_push_true(g_duk);
	duk_put_prop_string(g_duk, -2, "loaded");

have_module:
	// `module` is on the stack, we need `module.exports`
	duk_get_prop_string(g_duk, -1, "exports");
	duk_remove(g_duk, -2);
	return true;

on_error:
	// note: it's assumed that at this point, the only things left in our portion of the
	//       Duktape stack are the module object and the thrown error.
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "moduleCache");
	duk_del_prop_string(g_duk, -1, filename);
	duk_pop_2(g_duk);
	duk_remove(g_duk, -2);  // leave the error on the stack
	return false;
}

static void
duk_push_require_function(duk_context* ctx, const char* module_id)
{
	duk_push_c_function(ctx, js_require, 1);
	duk_push_string(ctx, "name");
	duk_push_string(ctx, "require");
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);  // require.name
	duk_push_string(g_duk, "cache");
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "moduleCache");
	duk_remove(g_duk, -2);
	duk_def_prop(g_duk, -3, DUK_DEFPROP_HAVE_VALUE);  // require.cache
	if (module_id != NULL) {
		duk_push_string(g_duk, "id");
		duk_push_string(g_duk, module_id);
		duk_def_prop(g_duk, -3, DUK_DEFPROP_HAVE_VALUE);  // require.id
	}
}

static path_t*
find_module(const char* id, const char* origin, const char* sys_origin)
{
	const char* const filenames[] =
	{
		"%s",
		"%s.js",
		"%s.ts",
		"%s.coffee",
		"%s.json",
		"%s/package.json",
		"%s/index.js",
		"%s/index.ts",
		"%s/index.coffee",
		"%s/index.json",
	};

	path_t*   origin_path;
	char*     filename;
	path_t*   main_path;
	path_t*   path;

	int i;

	if (strncmp(id, "./", 2) == 0 || strncmp(id, "../", 3) == 0)
		// resolve module relative to calling module
		origin_path = path_new(origin != NULL ? origin : "./");
	else
		// resolve module from designated module repository
		origin_path = path_new(sys_origin);

	for (i = 0; i < (int)(sizeof(filenames) / sizeof(filenames[0])); ++i) {
		filename = strnewf(filenames[i], id);
		if (strncmp(id, "@/", 2) == 0 || strncmp(id, "~/", 2) == 0 || strncmp(id, "#/", 2) == 0)
			path = path_new("./");
		else
			path = path_dup(origin_path);
		path_strip(path);
		path_append(path, filename);
		path_collapse(path, true);
		free(filename);
		if (sfs_fexist(g_fs, path_cstr(path), NULL)) {
			if (strcmp(path_filename_cstr(path), "package.json") != 0)
				return path;
			else {
				if (!(main_path = load_package_json(path_cstr(path))))
					goto next_filename;
				if (sfs_fexist(g_fs, path_cstr(main_path), NULL)) {
					path_free(path);
					return main_path;
				}
			}
		}

	next_filename:
		path_free(path);
	}

	return NULL;
}

static path_t*
load_package_json(const char* filename)
{
	duk_idx_t duk_top;
	char*     json;
	size_t    json_size;
	path_t*   path;
	
	duk_top = duk_get_top(g_duk);
	if (!(json = sfs_fslurp(g_fs, filename, NULL, &json_size)))
		goto on_error;
	duk_push_lstring(g_duk, json, json_size);
	free(json);
	if (duk_json_pdecode(g_duk) != DUK_EXEC_SUCCESS)
		goto on_error;
	if (!duk_is_object_coercible(g_duk, -1))
		goto on_error;
	duk_get_prop_string(g_duk, -1, "main");
	if (!duk_is_string(g_duk, -1))
		goto on_error;
	path = path_strip(path_new(filename));
	path_append(path, duk_get_string(g_duk, -1));
	path_collapse(path, true);
	if (!sfs_fexist(g_fs, path_cstr(path), NULL))
		goto on_error;
	return path;

on_error:
	duk_set_top(g_duk, duk_top);
	return NULL;
}

void
init_commonjs_api(void)
{
	const path_t* script_path;

	duk_push_global_stash(g_duk);
	duk_push_object(g_duk);
	duk_put_prop_string(g_duk, -2, "moduleCache");
	duk_pop(g_duk);
	
	script_path = get_sgm_script_path(g_fs);
	duk_push_global_object(g_duk);
	duk_push_string(g_duk, "require");
	duk_push_require_function(g_duk, NULL);
	duk_def_prop(g_duk, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_SET_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
}

static duk_ret_t
js_require(duk_context* ctx)
{
	const char* id;
	const char* parent_id = NULL;
	path_t*     path;

	duk_push_current_function(ctx);
	if (duk_get_prop_string(ctx, -1, "id"))
		parent_id = duk_get_string(ctx, -1);
	id = duk_require_string(ctx, 0);

	if (parent_id == NULL && (strncmp(id, "./", 2) == 0 || strncmp(id, "../", 3) == 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "illegal relative require in global code");
	if (!(path = find_module(id, parent_id, "lib/")) && !(path = find_module(id, parent_id, "#/modules/")))
		duk_error_ni(g_duk, -1, DUK_ERR_REFERENCE_ERROR, "unable to resolve require `%s`", id);
	if (!cjs_eval_module(path_cstr(path)))
		duk_throw(ctx);
	return 1;
}
