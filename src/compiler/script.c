#include "cell.h"
#include "script.h"

#include "api.h"
#include "build.h"
#include "fs.h"
#include "target.h"
#include "tool.h"
#include "utility.h"

static build_t*  get_current_build (duk_context* js);
static duk_ret_t install_target    (duk_context* ctx);
static void      make_file_targets (fs_t* fs, const char* wildcard, const path_t* path, const path_t* subdir, vector_t* targets, bool recursive);

static duk_ret_t js_files           (duk_context* ctx);
static duk_ret_t js_install         (duk_context* ctx);
static duk_ret_t js_metadata        (duk_context* ctx);
static duk_ret_t js_system_name     (duk_context* ctx);
static duk_ret_t js_system_version  (duk_context* ctx);
static duk_ret_t js_new_Tool        (duk_context* ctx);
static duk_ret_t js_Tool_finalize   (duk_context* ctx);
static duk_ret_t js_Tool_build      (duk_context* ctx);
static duk_ret_t js_Target_finalize (duk_context* ctx);
static duk_ret_t js_Target_get_name (duk_context* ctx);
static duk_ret_t js_Target_get_path (duk_context* ctx);

bool
script_eval(build_t* build)
{
	void*        file_data;
	const char*  filename;
	size_t       file_size;
	tool_t*      install_tool;
	duk_context* js_env;
	int          line_number;
	lstring_t*   source;

	if (!(file_data = fs_fslurp(build_fs(build), "Cellscript.js", &file_size))) {
		printf("ERROR: unable to open Cellscript.js, does it exist?\n");
		return false;
	}
	source = lstr_from_cp1252(file_data, file_size);
	free(file_data);

	printf("evaluating Cellscript.js...\n");
	
	// note: no fatal error handler set here.  if a JavaScript exception is thrown
	//       and nothing catches it, Cell will crash.
	js_env = duk_create_heap_default();

	// initialize the Cellscript API
	api_init(js_env);
	api_define_function(js_env, NULL, "files", js_files);
	api_define_function(js_env, NULL, "install", js_install);
	api_define_function(js_env, NULL, "metadata", js_metadata);
	api_define_function(js_env, "system", "name", js_system_name);
	api_define_function(js_env, "system", "version", js_system_version);
	api_define_class(js_env, "Target", NULL, js_Target_finalize);
	api_define_property(js_env, "Target", "name", js_Target_get_name, NULL);
	api_define_property(js_env, "Target", "path", js_Target_get_path, NULL);
	api_define_class(js_env, "Tool", js_new_Tool, js_Tool_finalize);
	api_define_method(js_env, "Tool", "build", js_Tool_build);

	// stash the build pointer for easier access by API calls
	duk_push_global_stash(js_env);
	duk_push_pointer(js_env, build);
	duk_put_prop_string(js_env, -2, "buildPtr");
	duk_pop(js_env);

	duk_get_global_string(js_env, "install");
	duk_push_c_function(js_env, install_target, DUK_VARARGS);
	install_tool = tool_new(js_env, -1, "install");
	duk_push_class_obj(js_env, "Tool", install_tool);
	duk_replace(js_env, -2);
	duk_put_prop_string(js_env, -2, "\xFF" "tool");
	duk_pop(js_env);
	
	// execute the Cellscript
	duk_push_lstring(js_env, lstr_cstr(source), lstr_len(source));
	duk_push_string(js_env, "Cellscript.js");
	duk_compile(js_env, 0x0);
	if (duk_pcall(js_env, 0) != DUK_EXEC_SUCCESS) {
		duk_get_prop_string(js_env, -1, "fileName");
		filename = duk_safe_to_string(js_env, -1);
		duk_get_prop_string(js_env, -2, "lineNumber");
		line_number = duk_get_int(js_env, -1);
		duk_dup(js_env, -3);
		duk_to_string(js_env, -1);
		printf("    %s\n", duk_get_string(js_env, -1));
		printf("    @ [%s:%d]\n", filename, line_number);
		duk_pop_3(js_env);
	}
	duk_pop(js_env);
	
	//duk_destroy_heap(js_env);
	lstr_free(source);

	return true;
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

	if (vector_len(targets) == 0) {
		printf("    warn: '%s' matches 0 files\n", pattern);
	}

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
js_metadata(duk_context* ctx)
{
	return 0;
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
js_Target_finalize(duk_context* ctx)
{
	target_t* target;

	target = duk_require_class_obj(ctx, 0, "Target");

	target_free(target);
	return 0;
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
js_Target_get_path(duk_context* ctx)
{
	target_t* target;

	duk_push_this(ctx);
	target = duk_require_class_obj(ctx, -1, "Target");
	
	duk_push_string(ctx, path_cstr(target_path(target)));
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
	duk_uarridx_t length;
	path_t*       name;
	path_t*       out_path;
	target_t*     source;
	target_t*     target;
	tool_t*       tool;

	duk_uarridx_t i;

	duk_push_this(ctx);
	tool = duk_require_class_obj(ctx, -1, "Tool");
	out_path = path_new(duk_require_string(ctx, 0));
	if (duk_is_array(ctx, 1))
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "array expected (argument 2)");

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

	duk_push_class_obj(ctx, "Target", target);
	return 1;
}
