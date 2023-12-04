/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "cell.h"
#include "module.h"

#include "fs.h"

struct module_ref
{
	module_type_t type;
	path_t*       path;
};

static bool js_require (int num_args, bool is_ctor, intptr_t magic);

static void          do_resolve_import (void);
static module_ref_t* find_module       (const char* specifier, const char* importer, const char* lib_dir_name, bool node_compatible);
static module_ref_t* load_package_json (const char* filename);
static void          push_new_require  (const char* module_id);
static module_type_t type_of_module    (const path_t* path, bool node_compatible);

static fs_t* s_fs;
static int   s_next_module_id = 1;
static bool  s_strict_imports;

void
modules_init(fs_t* fs, bool strict_imports)
{
	s_fs = fs;
	s_strict_imports = strict_imports;

	jsal_on_import_module(do_resolve_import);

	// initialize CommonJS cache and global require()
	jsal_push_hidden_stash();
	jsal_push_new_bare_object();
	jsal_put_prop_string(-2, "moduleCache");
	jsal_pop(1);

	jsal_push_global_object();
	push_new_require(NULL);
	jsal_to_propdesc_value(true, false, true);
	jsal_def_prop_string(-2, "require");
	jsal_pop(1);
}

bool
module_eval(const char* specifier, bool node_compatible)
{
	module_ref_t* ref;

	if (!(ref = module_resolve(specifier, NULL, node_compatible)))
		return false;
	return module_exec(ref);
}

module_ref_t*
module_resolve(const char* specifier, const char* importer, bool node_compatible)
{
	static const
	struct search_path
	{
		bool        esm_only;
		const char* path;
	}
	PATHS[] =
	{
		{ false, "$/node_modules" },
		{ true,  "#/cell_modules" },
		{ true,  "#/runtime" },
	};

	module_ref_t* ref = NULL;

	int i;

	// can't resolve a relative specifier if we don't know where it came from
	if (importer == NULL && (strncmp(specifier, "./", 2) == 0 || strncmp(specifier, "../", 3) == 0)) {
		jsal_push_new_error(JS_URI_ERROR, "Relative module specifier '%s' in non-module code", specifier);
		return false;
	}

	for (i = 0; i < sizeof PATHS / sizeof PATHS[0]; ++i) {
		if ((ref = find_module(specifier, importer, PATHS[i].path, node_compatible && !PATHS[i].esm_only)))
			break;  // short-circuit
	}
	if (ref == NULL) {
		jsal_push_new_error(JS_URI_ERROR, "Couldn't find JS module '%s'", specifier);
		return false;
	}
	return ref;
}

void
module_free(module_ref_t* it)
{
	path_free(it->path);
	free(it);
}

const char*
module_pathname(const module_ref_t* it)
{
	return path_cstr(it->path);
}

module_type_t
module_type(const module_ref_t* it)
{
	return it->type;
}

bool
module_exec(module_ref_t* it)
{
	// HERE BE DRAGONS!
	// this function is horrendous.  JSAL's stack-based API is powerful, but gets very
	// messy very quickly when dealing with object properties.  I tried to add comments
	// to illuminate what's going on, but it's still likely to be confusing for someone
	// not familiar with Lua or similar APIs.  proceed with caution.

	// notes:
	//     - the final value of 'module.exports' is left on top of the JSAL value stack.
	//     - 'module.id' is set to the given filename.  in order to guarantee proper cache
	//       behavior, the filename should be canonicalized first.
	//     - this is a protected call.  if the module being loaded throws, this function will
	//       return false and the error will be left on top of the stack for the caller to deal
	//       with.

	lstring_t*  code_string;
	path_t*     dir_path;
	bool        is_module_loaded;
	const char* pathname;
	size_t      source_size;
	char*       source;

	pathname = path_cstr(it->path);
	dir_path = path_strip(path_dup(it->path));

	// if loading as ESM, we can skip the whole CommonJS rigamarole.
	if (it->type == MODULE_ESM) {
		source = fs_fslurp(s_fs, pathname, &source_size);
		code_string = lstr_from_utf8(source, source_size, true);
		free(source);
		jsal_push_lstring_t(code_string);
		is_module_loaded = jsal_try_eval_module(pathname, NULL);
		lstr_free(code_string);
		if (!is_module_loaded)
			goto on_error;
		goto have_module;
	}

	// is the requested module already in the cache?
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	if (jsal_get_prop_string(-1, pathname)) {
		jsal_remove(-2);
		jsal_remove(-2);
		goto have_module;
	}
	else {
		jsal_pop(3);
	}

	source = fs_fslurp(s_fs, pathname, &source_size);
	code_string = lstr_from_utf8(source, source_size, true);
	free(source);

	// construct a CommonJS module object for the new module
	jsal_push_new_object();  // module object
	jsal_push_new_object();
	jsal_put_prop_string(-2, "exports");  // module.exports = {}
	jsal_push_string(pathname);
	jsal_put_prop_string(-2, "filename");  // module.filename
	jsal_push_string(pathname);
	jsal_put_prop_string(-2, "id");  // module.id
	jsal_push_boolean_false();
	jsal_put_prop_string(-2, "loaded");  // module.loaded = false
	push_new_require(pathname);
	jsal_put_prop_string(-2, "require");  // module.require

	// cache the module object in advance
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	jsal_dup(-3);
	jsal_put_prop_string(-2, pathname);
	jsal_pop(2);

	if (it->type == MODULE_JSON) {
		// JSON file, decode to JavaScript object
		jsal_push_lstring_t(code_string);
		lstr_free(code_string);
		if (!jsal_try_parse(-1))
			goto on_error;
		jsal_put_prop_string(-2, "exports");
	}
	else {
		// synthesize a function to wrap the module code.  this is the simplest way to
		// implement CommonJS semantics and matches the behavior of Node.js.
		jsal_push_sprintf("(function (exports, require, module, __filename, __dirname) {%s%s\n})",
			strncmp(lstr_cstr(code_string), "#!", 2) == 0 ? "//" : "",  // shebang?
			lstr_cstr(code_string));
		lstr_free(code_string);
		if (!jsal_try_compile(pathname))
			goto on_error;
		jsal_call(0);
		jsal_push_new_object();
		jsal_push_string("main");
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-2, "name");

		// go, go, go!
		jsal_get_prop_string(-2, "exports");    // this = exports
		jsal_get_prop_string(-3, "exports");    // exports
		jsal_get_prop_string(-4, "require");    // require
		jsal_dup(-5);                           // module
		jsal_push_string(pathname);             // __filename
		jsal_push_string(path_cstr(dir_path));  // __dirname
		if (!jsal_try_call_method(5))
			goto on_error;
		jsal_pop(1);
	}

	// module executed successfully, set 'module.loaded' to true
	jsal_push_boolean_true();
	jsal_put_prop_string(-2, "loaded");

have_module:
	if (it->type != MODULE_ESM) {
		// 'module` is on the stack; we need `module.exports'.
		jsal_get_prop_string(-1, "exports");
		jsal_replace(-2);
	}
	module_free(it);
	return true;

on_error:
	// note: it's assumed that at this point, the only thing(s) left in our portion of the
	//       value stack are the module object (for CommonJS) and the thrown error.
	if (it->type != MODULE_ESM) {
		jsal_push_hidden_stash();
		jsal_get_prop_string(-1, "moduleCache");
		jsal_del_prop_string(-1, pathname);
		jsal_pop(2);
		jsal_replace(-2);  // leave the error on the stack
	}
	module_free(it);
	return false;
}

static void
do_resolve_import(void)
{
	/* [ module_name parent_specifier ] -> [ ... specifier url source ] */

	const char*   caller_id = NULL;
	const char*   pathname;
	module_ref_t* ref;
	char*         shim_name;
	lstring_t*    shim_source;
	char*         source;
	size_t        source_len;
	const char*   specifier;

	specifier = jsal_require_string(0);
	if (!jsal_is_null(1))
		caller_id = jsal_require_string(1);

	if (!(ref = module_resolve(specifier, caller_id, false)))
		jsal_throw();
	pathname = module_pathname(ref);
	if (module_type(ref) == MODULE_COMMONJS) {
		if (s_strict_imports)
			jsal_error(JS_TYPE_ERROR, "CommonJS import '%s' unsupported in strict imports mode", specifier);

		// ES module shim, allows 'import' to work with CommonJS modules
		shim_name = strnewf("%%/moduleShim-%d.js", s_next_module_id++);
		shim_source = lstr_newf(
			"/* ES module shim for CommonJS module */\n"
			"export default require(\"%s\");", pathname);
		jsal_push_string(shim_name);
		jsal_dup(-1);
		jsal_push_lstring_t(shim_source);
		free(shim_name);
		lstr_free(shim_source);
	}
	else {
		source = fs_fslurp(s_fs, pathname, &source_len);
		jsal_push_string(pathname);
		jsal_dup(-1);
		jsal_push_lstring(source, source_len);
		free(source);
	}
	module_free(ref);
}

static module_ref_t*
find_module(const char* specifier, const char* importer, const char* lib_dir_name, bool node_compatible)
{
	static const
	struct pattern
	{
		bool        esm_aware;
		bool        strict_aware;
		const char* name;
	}
	PATTERNS[] =
	{
		{ true,  true,  "%s" },
		{ true,  false, "%s.mjs" },
		{ true,  false, "%s.js" },
		{ true,  false, "%s.cjs" },
		{ false, false, "%s.json" },
		{ true,  false, "%s/package.json" },
		{ true,  false, "%s/index.mjs" },
		{ true,  false, "%s/index.js" },
		{ true,  false, "%s/index.cjs" },
		{ false, false, "%s/index.json" },
	};

	path_t*       base_path;
	char*         filename;
	path_t*       path;
	module_ref_t* ref;
	bool          strict_mode = false;

	int i;

	if (strncmp(specifier, "./", 2) == 0 || strncmp(specifier, "../", 3) == 0 || fs_is_prefix_path(s_fs, specifier)) {
		// relative-path specifier or SphereFS prefix, resolve from file system
		base_path = path_new(importer != NULL ? importer : "./");
		strict_mode = s_strict_imports && !node_compatible;
	}
	else {
		// bare specifier, resolve from designated module directory
		base_path = path_new_dir(lib_dir_name);
	}

	for (i = 0; i < sizeof PATTERNS / sizeof PATTERNS[0]; ++i) {
		if (!node_compatible && !PATTERNS[i].esm_aware)
			continue;
		filename = strnewf(PATTERNS[i].name, specifier);
		if (fs_is_prefix_path(s_fs, specifier)) {
			path = fs_full_path(filename, NULL);
		}
		else {
			path = path_dup(base_path);
			path_strip(path);
			path_append(path, filename);
			path_collapse(path, true);
		}
		free(filename);
		if (fs_fexist(s_fs, path_cstr(path))) {
			if (strict_mode && !PATTERNS[i].strict_aware)
				jsal_error(JS_URI_ERROR, "Abbreviated import '%s' unsupported in strict imports mode", specifier);
			if (strcmp(path_filename(path), "package.json") == 0) {
				if (!(ref = load_package_json(path_cstr(path))))
					goto next_filename;
				path_free(path);
				return ref;
			}
			else {
				if (!(ref = calloc(1, sizeof(module_ref_t))))
					return NULL;
				ref->path = path;
				ref->type = type_of_module(path, node_compatible);
				return ref;
			}
		}

	next_filename:
		path_free(path);
	}

	return NULL;
}

static module_ref_t*
load_package_json(const char* filename)
{
	size_t        json_size;
	char*         json_text;
	path_t*       path;
	module_ref_t* ref;
	const char*   specifier = NULL;
	int           stack_top;
	module_type_t type = MODULE_COMMONJS;
	const char*   type_string;

	stack_top = jsal_get_top();
	if (!(json_text = fs_fslurp(s_fs, filename, &json_size)))
		goto on_error;
	jsal_push_lstring(json_text, json_size);
	free(json_text);
	if (!jsal_try_parse(-1))
		goto on_error;
	if (!jsal_is_object_coercible(-1))
		goto on_error;
	jsal_get_prop_string(-1, "type");
	jsal_get_prop_string(-2, "main");
	type_string = jsal_is_string(-2) ? jsal_get_string(-2) : "commonjs";
	if (jsal_is_string(-1)) {
		specifier = jsal_require_string(-1);
		type = strcmp(type_string, "module") == 0 ? MODULE_ESM
			: MODULE_COMMONJS;
	}
	if (specifier == NULL)
		goto on_error;

	// check that the module exists and verify the load type
	path = path_strip(path_new(filename));
	path_append(path, specifier);
	path_collapse(path, true);
	if (!fs_fexist(s_fs, path_cstr(path)))
		goto on_error;
	if (path_extension_is(path, ".cjs"))
		type = MODULE_COMMONJS;  // treat `.cjs` as CommonJS, always
	if (path_extension_is(path, ".mjs"))
		type = MODULE_ESM;  // treat `.mjs` as ESM, always
	if (path_extension_is(path, ".json"))
		type = MODULE_JSON;
	
	jsal_set_top(stack_top);
	if (!(ref = calloc(1, sizeof(module_ref_t))))
		goto on_error;
	ref->path = path;
	ref->type = type;
	return ref;

on_error:
	jsal_set_top(stack_top);
	return NULL;
}

static void
push_new_require(const char* module_id)
{
	jsal_push_new_function(js_require, "require", 1, false, 0);

	// assign 'require.cache'
	jsal_push_new_object();
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	jsal_remove(-2);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "cache");

	if (module_id != NULL) {
		jsal_push_new_object();
		jsal_push_string(module_id);
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-2, "id");  // require.id
	}
}

static module_type_t
type_of_module(const path_t* path, bool node_compatible)
{
	// IMPORTANT: `path` is assumed to be canonical, including SphereFS prefix.

	int           hop_index;
	path_t*       json_path;
	size_t        json_size;
	char*         json_text;
	module_type_t type;
	const char*   type_string;

	if (node_compatible)
		type = MODULE_COMMONJS;
	else
		type = MODULE_ESM;

	// if extension is `.cjs` or `.mjs`, we don't need to look at `package.json`
	if (path_extension_is(path, ".mjs"))
		return MODULE_ESM;
	if (path_extension_is(path, ".cjs"))
		return MODULE_COMMONJS;
	if (path_extension_is(path, ".json"))
		return MODULE_JSON;

	// for `.js`, find the nearest uplevel `package.json` to determine module type.
	// if no package manifest is found, the default module type depends on whether we're
	// in Node.js compatibility mode or not (see above).
	if (path_extension_is(path, ".js")) {
		json_path = path_dup(path);
		path_strip(json_path);
		path_append(json_path, "package.json");
		hop_index = path_num_hops(json_path) - 1;
		while (hop_index >= 0) {
			if ((json_text = fs_fslurp(s_fs, path_cstr(json_path), &json_size))) {
				jsal_push_lstring(json_text, json_size);
				free(json_text);
				if (!jsal_try_parse(-1) || !jsal_is_object_coercible(-1)) {
					jsal_pop(1);
					continue;  // invalid `package.json`, move on
				}
				jsal_get_prop_string(-1, "type");
				type_string = jsal_is_string(-1) ? jsal_get_string(-1) : "commonjs";
				type = strcmp(type_string, "module") == 0 ? MODULE_ESM
					: MODULE_COMMONJS;
				jsal_pop(2);
				break;  // we're done here
			}
			path_remove_hop(json_path, hop_index--);
		}
		path_free(json_path);
	}

	return type;
}

static bool
js_require(int num_args, bool is_ctor, intptr_t magic)
{
	const char*   caller_id = NULL;
	module_ref_t* ref;
	const char*   specifier;

	specifier = jsal_require_string(0);

	jsal_push_callee();
	if (jsal_get_prop_string(-1, "id"))
		caller_id = jsal_get_string(-1);

	if (s_strict_imports)
		jsal_error(JS_TYPE_ERROR, "require() unsupported in strict imports mode");

	if (!(ref = module_resolve(specifier, caller_id, true)))
		jsal_throw();
	if (!module_exec(ref))
		jsal_throw();
	return true;
}
