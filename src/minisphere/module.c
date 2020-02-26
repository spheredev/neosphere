/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2020, Fat Cerberus
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
 *  * Neither the name of miniSphere nor the names of its contributors may be
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

#include "minisphere.h"
#include "module.h"

struct module_ref
{
	module_type_t type;
	path_t*       path;
};

static module_ref_t* load_package_json (const char* filename);

module_ref_t*
module_find(const char* specifier, const char* origin, const char* lib_dir_name, bool node_compatible)
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
		{ false, false, "%s/package.json" },
		{ true,  false, "%s/index.mjs" },
		{ true,  false, "%s/index.js" },
		{ true,  false, "%s/index.cjs" },
		{ false, false, "%s/index.json" },
	};

	char*         filename;
	path_t*       origin_path;
	path_t*       path;
	module_ref_t* ref;
	bool          strict_mode = false;

	int i;

	if (strncmp(specifier, "./", 2) == 0 || strncmp(specifier, "../", 3) == 0 || game_is_prefix_path(g_game, specifier)) {
		// relative-path specifier or SphereFS prefix, resolve from file system
		origin_path = path_new(origin != NULL ? origin : "./");
		strict_mode = game_strict_imports(g_game) && !node_compatible;
	}
	else {
		// bare specifier, resolve module from designated module repository
		origin_path = path_new_dir(lib_dir_name);
	}

	for (i = 0; i < sizeof PATTERNS / sizeof PATTERNS[0]; ++i) {
		if (!node_compatible && !PATTERNS[i].esm_aware)
			continue;
		filename = strnewf(PATTERNS[i].name, specifier);
		if (game_is_prefix_path(g_game, specifier)) {
			path = game_full_path(g_game, filename, NULL, false);
		}
		else {
			path = path_dup(origin_path);
			path_strip(path);
			path_append(path, filename);
			path_collapse(path, true);
		}
		free(filename);
		if (game_file_exists(g_game, path_cstr(path))) {
			if (strict_mode && !PATTERNS[i].strict_aware)
				jsal_error(JS_URI_ERROR, "Partial specifier in import '%s' unsupported with strictImports", specifier);
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
				ref->type = node_compatible && !path_extension_is(path, ".mjs")
					? MODULE_COMMONJS : MODULE_ESM;
				return ref;
			}
		}

	next_filename:
		path_free(path);
	}

	return NULL;
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

static module_ref_t*
load_package_json(const char* filename)
{
	char*         json;
	size_t        json_size;
	path_t*       path;
	module_ref_t* ref;
	const char*   specifier = NULL;
	int           stack_top;
	module_type_t type = MODULE_COMMONJS;

	stack_top = jsal_get_top();
	if (!(json = game_read_file(g_game, filename, &json_size)))
		goto on_error;
	jsal_push_lstring(json, json_size);
	free(json);
	if (!jsal_try_parse(-1))
		goto on_error;
	if (!jsal_is_object_coercible(-1))
		goto on_error;
	jsal_get_prop_string(-1, "module");
	jsal_get_prop_string(-2, "main");
	if (jsal_is_string(-2)) {
		specifier = jsal_require_string(-2);
		type = MODULE_ESM;
	}
	else if (jsal_is_string(-1)) {
		specifier = jsal_require_string(-1);
	}
	if (specifier == NULL)
		goto on_error;
	
	// check that the module exists and verify the load type
	path = path_strip(path_new(filename));
	path_append(path, specifier);
	path_collapse(path, true);
	if (!game_file_exists(g_game, path_cstr(path)))
		goto on_error;
	if (path_extension_is(path, ".mjs"))
		type = MODULE_ESM;  // treat .mjs as ESM, always
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
