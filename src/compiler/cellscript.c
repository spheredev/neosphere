#include "cell.h"
#include "cellscript.h"

#include "api.h"
#include "target.h"
#include "tinydir.h"
#include "tool.h"

static void list_files (const char* wildcard, const path_t* path, const path_t* subdir, vector_t* targets, bool recursive);

static duk_ret_t js_files           (duk_context* ctx);
static duk_ret_t js_system_name     (duk_context* ctx);
static duk_ret_t js_system_version  (duk_context* ctx);
static duk_ret_t js_Target_finalize (duk_context* ctx);
static duk_ret_t js_Target_get_path (duk_context* ctx);

void
cell_api_init(duk_context* ctx)
{
	api_init(ctx);

	api_define_function(ctx, NULL, "files", js_files);
	
	api_define_function(ctx, "system", "name", js_system_name);
	api_define_function(ctx, "system", "version", js_system_version);

	api_define_class(ctx, "Target", NULL, js_Target_finalize);
	api_define_property(ctx, "Target", "path", js_Target_get_path, NULL);
}

static void
list_files(const char* wildcard, const path_t* path, const path_t* subdir, vector_t* targets, bool recursive)
{
	// note: 'targets' should be a vector_t initialized to sizeof(target_t*).

	tinydir_dir  dir_info;
	tinydir_file file_info;
	path_t*      file_path;
	path_t*      name;
	target_t*    target;

	tinydir_open(&dir_info, path_cstr(path));
	while (dir_info.has_next) {
		tinydir_readfile(&dir_info, &file_info);
		tinydir_next(&dir_info);
		if (file_info.is_dir && recursive) {
			name = path_new_dir(file_info.name);
			file_path = path_new_dir(file_info.path);
			if (subdir != NULL)
				path_rebase(name, subdir);
			list_files(wildcard, file_path, name, targets, true);
			path_free(file_path);
			path_free(name);
		}
		else if (file_info.is_reg && wildcmp(file_info.name, wildcard)) {
			name = path_new(file_info.name);
			file_path = path_new(file_info.path);
			if (subdir != NULL)
				path_rebase(name, subdir);
			target = target_new(name, file_path, NULL);
			vector_push(targets, &target);
			path_free(file_path);
			path_free(name);
		}
	}
}

static duk_ret_t
js_files(duk_context* ctx)
{
	int         num_args;
	const char* pattern;
	path_t*     path;
	bool        recursive = false;
	vector_t*   targets;
	char*       wildcard;

	iter_t iter;
	target_t* *p;

	num_args = duk_get_top(ctx);
	pattern = duk_require_string(ctx, 0);
	if (num_args >= 2)
		recursive = duk_require_boolean(ctx, 1);

	// extract the wildcard, if any, from the given path.  after this,
	// 'path' is guaranteed to refer to a directory.
	path = path_new(pattern);
	if (!path_is_file(path))
		wildcard = strdup("*");
	else {
		wildcard = strdup(path_filename(path));
		path_strip(path);
	}
	
	// this is potentially recursive, so we defer to list_files() to construct
	// the targets.
	targets = vector_new(sizeof(target_t*));
	list_files(wildcard, path, NULL, targets, true);
	free(wildcard);

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
js_Target_get_path(duk_context* ctx)
{
	target_t* target;

	duk_push_this(ctx);
	target = duk_require_class_obj(ctx, -1, "Target");
	
	duk_push_string(ctx, path_cstr(target_path(target)));
	return 1;
}
