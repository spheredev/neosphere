#include "cell.h"
#include "script.h"

#include "api.h"
#include "build.h"
#include "fs.h"
#include "target.h"
#include "tool.h"
#include "utility.h"

static duk_bool_t eval_module       (duk_context* ctx, const char* filename);
static path_t*    find_module       (duk_context* ctx, const char* id, const char* origin, const char* sys_origin);
static build_t*   get_current_build (duk_context* js);
static void       initialize_api    (duk_context* ctx);
static duk_ret_t  install_target    (duk_context* ctx);
static path_t*    load_package_json (duk_context* ctx, const char* filename);
static void       make_file_targets (fs_t* fs, const char* wildcard, const path_t* path, const path_t* subdir, vector_t* targets, bool recursive);
static void       push_require      (duk_context* ctx, const char* module_id);

static duk_ret_t js_describe                (duk_context* ctx);
static duk_ret_t js_files                   (duk_context* ctx);
static duk_ret_t js_install                 (duk_context* ctx);
static duk_ret_t js_require                 (duk_context* ctx);
static duk_ret_t js_system_name             (duk_context* ctx);
static duk_ret_t js_system_version          (duk_context* ctx);
static duk_ret_t js_FS_exists               (duk_context* ctx);
static duk_ret_t js_FS_mkdir                (duk_context* ctx);
static duk_ret_t js_FS_open                 (duk_context* ctx);
static duk_ret_t js_FS_readFile             (duk_context* ctx);
static duk_ret_t js_FS_rename               (duk_context* ctx);
static duk_ret_t js_FS_resolve              (duk_context* ctx);
static duk_ret_t js_FS_rmdir                (duk_context* ctx);
static duk_ret_t js_FS_unlink               (duk_context* ctx);
static duk_ret_t js_FS_writeFile            (duk_context* ctx);
static duk_ret_t js_FileStream_finalize     (duk_context* ctx);
static duk_ret_t js_FileStream_get_position (duk_context* ctx);
static duk_ret_t js_FileStream_get_size     (duk_context* ctx);
static duk_ret_t js_FileStream_set_position (duk_context* ctx);
static duk_ret_t js_FileStream_close        (duk_context* ctx);
static duk_ret_t js_FileStream_read         (duk_context* ctx);
static duk_ret_t js_FileStream_write        (duk_context* ctx);
static duk_ret_t js_new_Tool                (duk_context* ctx);
static duk_ret_t js_Tool_finalize           (duk_context* ctx);
static duk_ret_t js_Tool_build              (duk_context* ctx);
static duk_ret_t js_Target_finalize         (duk_context* ctx);
static duk_ret_t js_Target_get_fileName     (duk_context* ctx);
static duk_ret_t js_Target_get_name         (duk_context* ctx);

bool
script_eval(build_t* build)
{
	const char*  filename;
	tool_t*      install_tool;
	duk_context* js;
	int          line_number;

	js = build_js_realm(build);

	// stash the build pointer for easier access by API calls
	duk_push_global_stash(js);
	duk_push_pointer(js, build);
	duk_put_prop_string(js, -2, "buildPtr");
	duk_pop(js);

	// initialize CommonJS cache and global require()
	duk_push_global_stash(js);
	dukrub_push_bare_object(js);
	duk_put_prop_string(js, -2, "moduleCache");
	duk_pop(js);

	duk_push_global_object(js);
	duk_push_string(js, "require");
	push_require(js, NULL);
	duk_def_prop(js, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_SET_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);

	// polyfill for ECMAScript 2015
	if (fs_fexist(build_fs(build), "#/polyfill.js") && !eval_module(js, "#/polyfill.js"))
		return false;

	// initialize the Cellscript API
	api_init(js);
	api_define_function(js, NULL, "describe", js_describe);
	api_define_function(js, NULL, "files", js_files);
	api_define_function(js, NULL, "install", js_install);
	
	api_define_function(js, "system", "name", js_system_name);
	api_define_function(js, "system", "version", js_system_version);
	
	api_define_function(js, "FS", "exists", js_FS_exists);
	api_define_function(js, "FS", "open", js_FS_open);
	api_define_function(js, "FS", "mkdir", js_FS_mkdir);
	api_define_function(js, "FS", "readFile", js_FS_readFile);
	api_define_function(js, "FS", "rename", js_FS_rename);
	api_define_function(js, "FS", "resolve", js_FS_resolve);
	api_define_function(js, "FS", "rmdir", js_FS_rmdir);
	api_define_function(js, "FS", "unlink", js_FS_unlink);
	api_define_function(js, "FS", "writeFile", js_FS_writeFile);

	api_define_class(js, "FileStream", NULL, js_FileStream_finalize);
	api_define_property(js, "FileStream", "position", js_FileStream_get_position, js_FileStream_set_position);
	api_define_property(js, "FileStream", "size", js_FileStream_get_size, NULL);
	api_define_method(js, "FileStream", "close", js_FileStream_close);
	api_define_method(js, "FileStream", "read", js_FileStream_read);
	api_define_method(js, "FileStream", "write", js_FileStream_write);

	api_define_class(js, "Target", NULL, js_Target_finalize);
	api_define_property(js, "Target", "fileName", js_Target_get_fileName, NULL);
	api_define_property(js, "Target", "name", js_Target_get_name, NULL);
	
	api_define_class(js, "Tool", js_new_Tool, js_Tool_finalize);
	api_define_method(js, "Tool", "build", js_Tool_build);

	duk_get_global_string(js, "install");
	duk_push_c_function(js, install_target, DUK_VARARGS);
	install_tool = tool_new(js, -1, "installing");
	duk_push_class_obj(js, "Tool", install_tool);
	duk_replace(js, -2);
	duk_put_prop_string(js, -2, "\xFF" "tool");
	duk_pop(js);
	
	// execute the Cellscript
	printf("evaluating Cellscript.js...\n");
	if (!eval_module(js, "Cellscript.js")) {
		duk_get_prop_string(js, -1, "fileName");
		filename = duk_safe_to_string(js, -1);
		duk_get_prop_string(js, -2, "lineNumber");
		line_number = duk_get_int(js, -1);
		duk_dup(js, -3);
		duk_to_string(js, -1);
		printf("\n");
		printf("    %s\n", duk_get_string(js, -1));
		printf("    @ [%s:%d]\n", filename, line_number);
		duk_pop_3(js);
	}
	duk_pop(js);
	
	return true;
}

static duk_bool_t
eval_module(duk_context* ctx, const char* filename)
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
	//     - this is a protected call.  if the module being loaded throws, the error will be
	//       caught and left on top of the stack for the caller to deal with.

	lstring_t* code_string;
	path_t*    dir_path;
	path_t*    file_path;
	fs_t*      fs;
	size_t     source_size;
	char*      source;

	fs = build_fs(get_current_build(ctx));
	
	file_path = path_new(filename);
	dir_path = path_strip(path_dup(file_path));

	// is the requested module already in the cache?
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "moduleCache");
	if (duk_get_prop_string(ctx, -1, filename)) {
		duk_remove(ctx, -2);
		duk_remove(ctx, -2);
		goto have_module;
	}
	else {
		duk_pop_3(ctx);
	}

	source = fs_fslurp(fs, filename, &source_size);
	code_string = lstr_from_cp1252(source, source_size);
	free(source);

	// construct a module object for the new module
	duk_push_object(ctx);  // module object
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, "exports");  // module.exports = {}
	duk_push_string(ctx, filename);
	duk_put_prop_string(ctx, -2, "filename");  // module.filename
	duk_push_string(ctx, filename);
	duk_put_prop_string(ctx, -2, "id");  // module.id
	duk_push_false(ctx);
	duk_put_prop_string(ctx, -2, "loaded");  // module.loaded = false
	push_require(ctx, filename);
	duk_put_prop_string(ctx, -2, "require");  // module.require

	// cache the module object in advance
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "moduleCache");
	duk_dup(ctx, -3);
	duk_put_prop_string(ctx, -2, filename);
	duk_pop_2(ctx);

	if (strcmp(path_extension(file_path), ".json") == 0) {
		// JSON file, decode to JavaScript object
		duk_push_lstring_t(ctx, code_string);
		lstr_free(code_string);
		if (duk_json_pdecode(ctx) != DUK_EXEC_SUCCESS)
			goto on_error;
		duk_put_prop_string(ctx, -2, "exports");
	}
	else {
		// synthesize a function to wrap the module code.  this is the simplest way to
		// implement CommonJS semantics and matches the behavior of Node.js.
		duk_push_string(ctx, "(function(exports, require, module, __filename, __dirname) { ");
		duk_push_lstring_t(ctx, code_string);
		duk_push_string(ctx, " })");
		duk_concat(ctx, 3);
		duk_push_string(ctx, filename);
		if (duk_pcompile(ctx, DUK_COMPILE_EVAL) != DUK_EXEC_SUCCESS)
			goto on_error;
		duk_call(ctx, 0);
		duk_push_string(ctx, "name");
		duk_push_string(ctx, "main");
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_FORCE);
		lstr_free(code_string);

		// go, go, go!
		duk_get_prop_string(ctx, -2, "exports");    // exports
		duk_get_prop_string(ctx, -3, "require");    // require
		duk_dup(ctx, -4);                           // module
		duk_push_string(ctx, filename);             // __filename
		duk_push_string(ctx, path_cstr(dir_path));  // __dirname
		if (duk_pcall(ctx, 5) != DUK_EXEC_SUCCESS)
			goto on_error;
		duk_pop(ctx);
	}

	// module executed successfully, set `module.loaded` to true
	duk_push_true(ctx);
	duk_put_prop_string(ctx, -2, "loaded");

have_module:
	// `module` is on the stack, we need `module.exports`
	duk_get_prop_string(ctx, -1, "exports");
	duk_remove(ctx, -2);
	return 1;

on_error:
	// note: it's assumed that at this point, the only things left in our portion of the
	//       Duktape stack are the module object and the thrown error.
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "moduleCache");
	duk_del_prop_string(ctx, -1, filename);
	duk_pop_2(ctx);
	duk_remove(ctx, -2);  // leave the error on the stack
	return 0;
}

static path_t*
find_module(duk_context* ctx, const char* id, const char* origin, const char* sys_origin)
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
	fs_t*     fs;
	path_t*   main_path;
	path_t*   path;

	int i;

	fs = build_fs(get_current_build(ctx));
	
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
		if (fs_fexist(fs, path_cstr(path))) {
			if (strcmp(path_filename(path), "package.json") != 0)
				return path;
			else {
				if (!(main_path = load_package_json(ctx, path_cstr(path))))
					goto next_filename;
				if (fs_fexist(fs, path_cstr(main_path))) {
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

static build_t*
get_current_build(duk_context* js)
{
	build_t* build;

	duk_push_global_stash(js);
	duk_get_prop_string(js, -1, "buildPtr");
	build = duk_get_pointer(js, -1);
	duk_pop_2(js);
	return build;
}

static void
initialize_api(duk_context* ctx)
{

}

static duk_ret_t
install_target(duk_context* ctx)
{
	// note: install targets never have more than one source because an individual
	//       target is constructed for each file installed.

	build_t*    build;
	int         result;
	const char* source_path;
	const char* target_path;

	build = get_current_build(ctx);
	target_path = duk_require_string(ctx, 0);
	duk_get_prop_index(ctx, 1, 0);
	source_path = duk_require_string(ctx, -1);

	result = fs_fcopy(build_fs(build), target_path, source_path, true);
	duk_push_boolean(ctx, result == 0);
	return 1;
}

static path_t*
load_package_json(duk_context* ctx, const char* filename)
{
	duk_idx_t duk_top;
	fs_t*     fs;
	char*     json;
	size_t    json_size;
	path_t*   path;

	fs = build_fs(get_current_build(ctx));
	
	duk_top = duk_get_top(ctx);
	if (!(json = fs_fslurp(fs, filename, &json_size)))
		goto on_error;
	duk_push_lstring(ctx, json, json_size);
	free(json);
	if (duk_json_pdecode(ctx) != DUK_EXEC_SUCCESS)
		goto on_error;
	if (!duk_is_object_coercible(ctx, -1))
		goto on_error;
	duk_get_prop_string(ctx, -1, "main");
	if (!duk_is_string(ctx, -1))
		goto on_error;
	path = path_strip(path_new(filename));
	path_append(path, duk_get_string(ctx, -1));
	path_collapse(path, true);
	if (!fs_fexist(fs, path_cstr(path)))
		goto on_error;
	return path;

on_error:
	duk_set_top(ctx, duk_top);
	return NULL;
}

static void
make_file_targets(fs_t* fs, const char* wildcard, const path_t* path, const path_t* subdir, vector_t* targets, bool recursive)
{
	// note: 'targets' should be a vector_t initialized to sizeof(target_t*).

	path_t*      file_path;
	bool         ignore_dir;
	vector_t*    list;
	path_t*      name;
	target_t*    target;

	iter_t iter;
	path_t* *p_path;

	list = fs_list_dir(fs, path_cstr(path));
	
	iter = vector_enum(list);
	while (p_path = vector_next(&iter)) {
		ignore_dir = fs_is_game_dir(fs, path_cstr(*p_path))
			&& path_num_hops(path) > 0
			&& !path_hop_cmp(path, 0, "@");
		if (!path_is_file(*p_path) && !ignore_dir && recursive) {
			name = path_new_dir(path_hop(*p_path, path_num_hops(*p_path) - 1));
			file_path = path_dup(*p_path);
			if (subdir != NULL)
				path_rebase(name, subdir);
			make_file_targets(fs, wildcard, file_path, name, targets, true);
			path_free(file_path);
			path_free(name);
		}
		else if (path_is_file(*p_path) && wildcmp(path_filename(*p_path), wildcard)) {
			name = path_new(path_filename(*p_path));
			file_path = path_dup(*p_path);
			if (subdir != NULL)
				path_rebase(name, subdir);
			target = target_new(name, file_path, NULL);
			vector_push(targets, &target);
			path_free(file_path);
			path_free(name);
		}
	}

	iter = vector_enum(list);
	while (p_path = vector_next(&iter))
		path_free(*p_path);
	vector_free(list);
}

static void
push_require(duk_context* ctx, const char* module_id)
{
	duk_push_c_function(ctx, js_require, 1);
	duk_push_string(ctx, "name");
	duk_push_string(ctx, "require");
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);  // require.name
	duk_push_string(ctx, "cache");
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "moduleCache");
	duk_remove(ctx, -2);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);  // require.cache
	if (module_id != NULL) {
		duk_push_string(ctx, "id");
		duk_push_string(ctx, module_id);
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);  // require.id
	}
}

static duk_ret_t
js_describe(duk_context* ctx)
{
	const char* title;

	title = duk_require_string(ctx, 0);
	duk_require_object_coercible(ctx, 1);

	duk_push_global_stash(ctx);
	duk_dup(ctx, 1);
	duk_dup(ctx, 0);
	duk_put_prop_string(ctx, -2, "name");
	duk_put_prop_string(ctx, -2, "descriptor");

	return 0;
}

static duk_ret_t
js_files(duk_context* ctx)
{
	build_t*    build;
	int         num_args;
	const char* pattern;
	path_t*     path;
	bool        recursive = false;
	vector_t*   targets;
	char*       wildcard;

	iter_t iter;
	target_t* *p;

	build = get_current_build(ctx);
	num_args = duk_get_top(ctx);
	pattern = duk_require_string(ctx, 0);
	if (num_args >= 2)
		recursive = duk_require_boolean(ctx, 1);

	// extract the wildcard, if any, from the given path
	path = path_new(pattern);
	if (!path_is_file(path))
		wildcard = strdup("*");
	else {
		wildcard = strdup(path_filename(path));
		path_strip(path);
	}

	// this is potentially recursive, so we defer to make_file_targets() to construct
	// the targets.  note: 'path' should always be a directory at this point.
	targets = vector_new(sizeof(target_t*));
	make_file_targets(build_fs(build), wildcard, path, NULL, targets, recursive);
	free(wildcard);

	if (vector_len(targets) == 0)
		build_emit_warning(build, "'%s' matches 0 files", pattern);

	// return all the newly constructed targets as an array.
	duk_push_array(ctx);
	iter = vector_enum(targets);
	while (p = vector_next(&iter)) {
		duk_push_class_obj(ctx, "Target", *p);
		duk_put_prop_index(ctx, -2, (duk_uarridx_t)iter.index);
	}
	return 1;
}

static duk_ret_t
js_install(duk_context* ctx)
{
	build_t*      build;
	path_t*       dest_path;
	duk_uarridx_t length;
	target_t*     source;
	path_t*       name;
	path_t*       path;
	target_t*     target;
	tool_t*       tool;

	duk_uarridx_t i;

	build = get_current_build(ctx);
	
	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "tool");
	tool = duk_require_class_obj(ctx, -1, "Tool");
	dest_path = path_new_dir(duk_require_string(ctx, 0));
	
	if (duk_is_array(ctx, 1)) {
		length = (duk_uarridx_t)duk_get_length(ctx, 1);
		for (i = 0; i < length; ++i) {
			duk_get_prop_index(ctx, 1, i);
			source = duk_require_class_obj(ctx, -1, "Target");
			name = path_dup(target_name(source));
			path = path_rebase(path_dup(name), dest_path);
			target = target_new(name, path, tool);
			target_add_source(target, source);
			build_add_target(build, target);
			target_free(target);
			duk_pop(ctx);
		}
	}
	else {
		source = duk_require_class_obj(ctx, 1, "Target");
		name = path_dup(target_name(source));
		path = path_rebase(path_dup(name), dest_path);
		target = target_new(name, path, tool);
		target_add_source(target, source);
		build_add_target(build, target);
		target_free(target);
	}
	return 0;
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
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "relative require not allowed in global code");
	if (!(path = find_module(ctx, id, parent_id, "lib/")) && !(path = find_module(ctx, id, parent_id, "#/cell_modules/")))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "module not found `%s`", id);
	if (!eval_module(ctx, path_cstr(path)))
		duk_throw(ctx);
	return 1;
}

static duk_ret_t
js_system_name(duk_context* ctx)
{
	duk_push_string(ctx, COMPILER_NAME);
	return 1;
}

static duk_ret_t
js_system_version(duk_context* ctx)
{
	duk_push_string(ctx, VERSION_NAME);
	return 1;
}

static duk_ret_t
js_FS_exists(duk_context* ctx)
{
	const char* filename;
	fs_t*       fs;

	filename = duk_require_path(ctx, 0);

	fs = build_fs(get_current_build(ctx));
	duk_push_boolean(ctx, fs_fexist(fs, filename));
	return 1;
}

static duk_ret_t
js_FS_mkdir(duk_context* ctx)
{
	fs_t*       fs;
	const char* name;

	name = duk_require_path(ctx, 0);

	fs = build_fs(get_current_build(ctx));
	if (fs_mkdir(fs, name) != 0)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "directory creation failed");
	return 0;
}

static duk_ret_t
js_FS_open(duk_context* ctx)
{
	FILE*       file;
	const char* filename;
	fs_t*       fs;
	const char* mode;

	filename = duk_require_path(ctx, 0);
	mode = duk_require_string(ctx, 1);
	
	fs = build_fs(get_current_build(ctx));
	if (!(file = fs_fopen(fs, filename, mode)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot open file '%s'", filename);
	duk_push_class_obj(ctx, "FileStream", file);
	return 1;
}

static duk_ret_t
js_FS_readFile(duk_context* ctx)
{
	void*       buffer;
	void*       file_data;
	fs_t*       fs;
	const char* name;
	size_t      size;

	name = duk_require_path(ctx, 0);

	fs = build_fs(get_current_build(ctx));
	if (!(file_data = fs_fslurp(fs, name, &size)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "read file failed");
	buffer = duk_push_fixed_buffer(ctx, size);
	memcpy(buffer, file_data, size);
	free(file_data);

	duk_push_buffer_object(ctx, -1, 0, size, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_FS_rename(duk_context* ctx)
{
	fs_t*       fs;
	const char* name1;
	const char* name2;

	name1 = duk_require_path(ctx, 0);
	name2 = duk_require_path(ctx, 1);

	fs = build_fs(get_current_build(ctx));
	if (!fs_rename(fs, name1, name2))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "rename failed", name1, name2);
	return 0;
}

static duk_ret_t
js_FS_resolve(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0);

	duk_push_string(ctx, filename);
	return 1;
}

static duk_ret_t
js_FS_rmdir(duk_context* ctx)
{
	fs_t*       fs;
	const char* name;

	name = duk_require_path(ctx, 0);

	fs = build_fs(get_current_build(ctx));
	if (!fs_rmdir(fs, name))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "directory removal failed");
	return 0;
}

static duk_ret_t
js_FS_unlink(duk_context* ctx)
{
	const char* filename;
	fs_t*       fs;

	filename = duk_require_path(ctx, 0);

	fs = build_fs(get_current_build(ctx));
	if (!fs_unlink(fs, filename))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unlink failed", filename);
	return 0;
}

static duk_ret_t
js_FS_writeFile(duk_context* ctx)
{
	void*       file_data;
	fs_t*       fs;
	const char* name;
	size_t      size;

	name = duk_require_path(ctx, 0);
	file_data = duk_require_buffer_data(ctx, 1, &size);

	fs = build_fs(get_current_build(ctx));
	if (!fs_fspew(fs, name, file_data, size))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "write file failed");
	return 0;
}

static duk_ret_t
js_FileStream_finalize(duk_context* ctx)
{
	FILE* file;

	file = duk_require_class_obj(ctx, 0, "FileStream");

	if (file != NULL)
		fclose(file);
	return 0;
}

static duk_ret_t
js_FileStream_get_position(duk_context* ctx)
{
	FILE* file;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "FileStream");

	duk_push_number(ctx, ftell(file));
	return 1;
}

static duk_ret_t
js_FileStream_get_size(duk_context* ctx)
{
	FILE* file;
	long  file_pos;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "FileStream");

	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "stream is closed");
	file_pos = ftell(file);
	fseek(file, 0, SEEK_END);
	duk_push_number(ctx, ftell(file));
	fseek(file, file_pos, SEEK_SET);
	return 1;
}

static duk_ret_t
js_FileStream_set_position(duk_context* ctx)
{
	FILE* file;
	long  new_pos;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "FileStream");
	new_pos = duk_require_int(ctx, 0);

	if (new_pos < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid file position");
	fseek(file, new_pos, SEEK_SET);
	return 0;
}

static duk_ret_t
js_FileStream_close(duk_context* ctx)
{
	FILE* file;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "FileStream");

	duk_push_pointer(ctx, NULL);
	duk_put_prop_string(ctx, -2, "\xFF" "udata");
	fclose(file);
	return 0;
}

static duk_ret_t
js_FileStream_read(duk_context* ctx)
{
	// FileStream:read([numBytes]);
	// Reads data from the stream, returning it in an ArrayBuffer.
	// Arguments:
	//     numBytes: Optional. The number of bytes to read. If not provided, the
	//               entire file is read.

	int    argc;
	void*  buffer;
	FILE*  file;
	int    num_bytes;
	long   pos;

	argc = duk_get_top(ctx);
	num_bytes = argc >= 1 ? duk_require_int(ctx, 0) : 0;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "FileStream");
	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "stream is closed");
	if (argc < 1) {  // if no arguments, read entire file back to front
		pos = ftell(file);
		num_bytes = (fseek(file, 0, SEEK_END), ftell(file));
		fseek(file, 0, SEEK_SET);
	}
	if (num_bytes < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid read size");
	buffer = duk_push_fixed_buffer(ctx, num_bytes);
	num_bytes = (int)fread(buffer, 1, num_bytes, file);
	if (argc < 1)  // reset file position after whole-file read
		fseek(file, pos, SEEK_SET);
	duk_push_buffer_object(ctx, -1, 0, num_bytes, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_FileStream_write(duk_context* ctx)
{
	const void* data;
	FILE*       file;
	duk_size_t  num_bytes;

	duk_push_this(ctx);
	file = duk_require_class_obj(ctx, -1, "FileStream");
	data = duk_require_buffer_data(ctx, 0, &num_bytes);

	if (file == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "stream is closed");
	if (fwrite(data, 1, num_bytes, file) != num_bytes)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "file write failed");
	return 0;
}

static duk_ret_t
js_Target_finalize(duk_context* ctx)
{
	target_t* target;

	target = duk_require_class_obj(ctx, 0, "Target");

	target_free(target);
	return 0;
}

static duk_ret_t
js_Target_get_fileName(duk_context* ctx)
{
	target_t* target;

	duk_push_this(ctx);
	target = duk_require_class_obj(ctx, -1, "Target");

	duk_push_string(ctx, path_cstr(target_path(target)));
	return 1;
}

static duk_ret_t
js_Target_get_name(duk_context* ctx)
{
	target_t* target;

	duk_push_this(ctx);
	target = duk_require_class_obj(ctx, -1, "Target");

	duk_push_string(ctx, path_cstr(target_name(target)));
	return 1;
}

static duk_ret_t
js_new_Tool(duk_context* ctx)
{
	int         num_args;
	tool_t*     tool;
	const char* verb = "build";

	num_args = duk_get_top(ctx);
	if (!duk_is_constructor_call(ctx))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "constructor requires 'new'");
	duk_require_function(ctx, 0);
	if (num_args >= 2)
		verb = duk_require_string(ctx, 1);

	tool = tool_new(ctx, 0, verb);

	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "Tool", tool);
	return 1;
}

static duk_ret_t
js_Tool_finalize(duk_context* ctx)
{
	tool_t* tool;

	tool = duk_require_class_obj(ctx, 0, "Tool");

	tool_free(tool);
	return 0;
}

static duk_ret_t
js_Tool_build(duk_context* ctx)
{
	build_t*      build;
	duk_uarridx_t length;
	path_t*       name;
	path_t*       out_path;
	target_t*     source;
	target_t*     target;
	tool_t*       tool;

	duk_uarridx_t i;

	build = get_current_build(ctx);

	duk_push_this(ctx);
	tool = duk_require_class_obj(ctx, -1, "Tool");
	out_path = path_new(duk_require_string(ctx, 0));
	if (!duk_is_array(ctx, 1))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "array expected (argument 3)");

	name = path_new(path_filename(out_path));
	target = target_new(name, out_path, tool);
	length = (duk_uarridx_t)duk_get_length(ctx, 1);
	for (i = 0; i < length; ++i) {
		duk_get_prop_index(ctx, 1, i);
		source = duk_require_class_obj(ctx, -1, "Target");
		target_add_source(target, source);
		duk_pop(ctx);
	}
	path_free(out_path);
	build_add_target(build, target);

	duk_push_class_obj(ctx, "Target", target);
	return 1;
}
