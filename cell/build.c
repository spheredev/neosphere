#include "cell.h"
#include "build.h"

#include "assets.h"
#include "spk_writer.h"
#include "tinydir.h"

#include <sys/stat.h>

struct build
{
	duk_context*  duk;
	path_t*       in_path;
	vector_t*     installs;
	path_t*       out_path;
	time_t        js_mtime;
	spk_writer_t* spk;
	path_t*       staging_path;
	vector_t*     targets;
	bool          want_source_map;
};

struct install
{
	const target_t* target;
	path_t*         path;
};

struct target
{
	asset_t* asset;
	path_t*  subpath;
};

static duk_ret_t js_api_install (duk_context* ctx);
static duk_ret_t js_api_files   (duk_context* ctx);
static duk_ret_t js_api_sgm     (duk_context* ctx);

static void process_add_files (build_t* build, const char* wildcard, const path_t* path, const path_t* subpath, bool recursive, vector_t* *inout_targets);
static bool process_install   (build_t* build, struct install* inst, bool *out_is_new);
static bool process_target    (build_t* build, const target_t* target, bool *out_is_new);

build_t*
new_build(const path_t* in_path, const path_t* out_path, bool want_source_map)
{
	build_t*    build = NULL;
	path_t*     path;
	struct stat sb;

	build = calloc(1, sizeof(build_t));

	// check for Cellscript.js in input directory
	path = path_rebase(path_new("Cellscript.js"), in_path);
	if (stat(path_cstr(path), &sb) != 0 || !(sb.st_mode & S_IFREG)) {
		fprintf(stderr, "[error] no Cellscript.js in input directory\n");
		return NULL;
	}
	build->js_mtime = sb.st_mtime;
	path_free(path);

	// initialize JavaScript environment
	build->duk = duk_create_heap_default();
	duk_push_global_stash(build->duk);
	duk_push_pointer(build->duk, build);
	duk_put_prop_string(build->duk, -2, "\xFF""environ");
	duk_pop(build->duk);

	// wire up JavaScript API
	duk_push_c_function(build->duk, js_api_install, DUK_VARARGS);
	duk_put_global_string(build->duk, "install");
	duk_push_c_function(build->duk, js_api_files, DUK_VARARGS);
	duk_put_global_string(build->duk, "files");
	duk_push_c_function(build->duk, js_api_sgm, DUK_VARARGS);
	duk_put_global_string(build->duk, "sgm");

	// set up build harness
	path_mkdir(out_path);
	if (path_filename_cstr(out_path)
		&& !(build->spk = spk_create(path_cstr(out_path))))
	{
		printf("[error] failed to create SPK '%s'", path_cstr(out_path));
		goto on_error;
	}
	build->want_source_map = want_source_map;
	build->targets = new_vector(sizeof(target_t*));
	build->installs = new_vector(sizeof(struct install));
	build->in_path = path_resolve(path_dup(in_path), NULL);
	build->out_path = path_resolve(path_dup(out_path), NULL);
	build->staging_path = path_rebase(path_new(".cell/"), build->in_path);
	path_mkdir(build->staging_path);
	
	printf("    Building '%s' as %s\n", path_cstr(build->out_path),
		build->spk ? "SPK" : "dist");
	printf("    Source path is '%s'\n\n", path_cstr(build->in_path));
	return build;

on_error:
	duk_destroy_heap(build->duk);
	free(build);
	return NULL;
}

void
free_build(build_t* build)
{
	struct install *p_inst;
	target_t*      *p_target;
	iter_t iter;

	if (build == NULL)
		return;
	
	duk_destroy_heap(build->duk);
	iter = iterate_vector(build->targets);
	while (p_target = next_vector_item(&iter)) {
		free_asset((*p_target)->asset);
		path_free((*p_target)->subpath);
		free(*p_target);
	}
	free_vector(build->targets);
	iter = iterate_vector(build->installs);
	while (p_inst = next_vector_item(&iter)) {
		path_free(p_inst->path);
	}
	free_vector(build->installs);
	path_free(build->in_path);
	path_free(build->out_path);
	spk_close(build->spk);
	free(build);
}

bool
evaluate_rule(build_t* build, const char* name)
{
	char        func_name[255];
	path_t*     script_path;

	// process build script
	printf("Processing Cellscript.js rule '%s'... ", name);
	sprintf(func_name, "$%s", name);
	script_path = path_rebase(path_new("Cellscript.js"), build->in_path);
	if (duk_peval_file(build->duk, path_cstr(script_path)) != 0) {
		path_free(script_path);
		printf("\n[js] %s\n", duk_safe_to_string(build->duk, -1));
		return false;
	}
	path_free(script_path);
	if (!duk_get_global_string(build->duk, func_name) || !duk_is_callable(build->duk, -1)) {
		printf("\n[error] no rule named '%s' in Cellscript.js\n", name);
		return false;
	}
	if (duk_pcall(build->duk, 0) != 0) {
		printf("\n[js] %s\n", duk_safe_to_string(build->duk, -1));
		return false;
	}

	printf("OK.\n");
	return true;
}

vector_t*
add_files(build_t* build, const path_t* pattern, bool recursive)
{
	path_t*      path;
	vector_t*    targets;
	char*        wildcard;

	targets = new_vector(sizeof(target_t*));
	path = path_rebase(path_strip(path_dup(pattern)), build->in_path);
	wildcard = strdup(path_filename_cstr(pattern));
	process_add_files(build, wildcard, path, NULL, recursive, &targets);
	path_free(path);
	free(wildcard);
	return targets;
}

void
add_install(build_t* build, const target_t* target, const path_t* path)
{
	struct install inst;

	inst.target = target;
	inst.path = path != NULL ? path_dup(path) : path_new("./");
	push_back_vector(build->installs, &inst);
}

target_t*
add_target(build_t* build, asset_t* asset, const path_t* subpath)
{
	target_t* target;

	target = calloc(1, sizeof(target_t));

	target->asset = asset;
	target->subpath = subpath != NULL
		? path_dup(subpath)
		: path_new("./");
	push_back_vector(build->targets, &target);
	return target;
}

bool
run_build(build_t* build)
{
	bool        has_changed = false;
	bool        is_new;
	const char* json;
	duk_size_t  json_size;
	int         n_assets;
	path_t*     path;

	struct install *p_inst;
	target_t*      *p_target;
	iter_t iter;

	if (get_vector_size(build->installs) == 0) {
		printf("[error] no assets staged for install\n");
		return false;
	}

	// build and install assets
	printf("Building assets...");
	n_assets = 0;
	iter = iterate_vector(build->targets);
	while (p_target = next_vector_item(&iter)) {
		if (!process_target(build, *p_target, &is_new))
			return false;
		if (is_new) {
			if (n_assets == 0) printf("\n");
			printf("  %s\n", path_cstr(get_asset_path((*p_target)->asset)));
			++n_assets;
			has_changed = true;
		}
	}
	if (n_assets > 0) printf("  %d asset(s) built\n", n_assets);
		else printf(" Up to date.\n");

	printf("Installing assets...");
	n_assets = 0;
	iter = iterate_vector(build->installs);
	while (p_inst = next_vector_item(&iter)) {
		if (!process_install(build, p_inst, &is_new))
			return false;
		if (is_new) {
			if (n_assets == 0) printf("\n");
			printf("  %s\n", path_cstr(p_inst->path));
			++n_assets;
			has_changed = true;
		}
	}
	if (n_assets > 0) printf("  %d asset(s) installed\n", n_assets);
		else printf(" Up to date.\n");

	// generate source map
	if (build->want_source_map) {
		printf("Generating source map... ");
		duk_push_object(build->duk);
		iter = iterate_vector(build->installs);
		while (p_inst = next_vector_item(&iter)) {
			path = path_resolve(path_dup(get_asset_path(p_inst->target->asset)), build->in_path);
			duk_push_string(build->duk, path_cstr(path));
			duk_put_prop_string(build->duk, -2, path_cstr(p_inst->path));
		}
		duk_json_encode(build->duk, -1);
		json = duk_get_lstring(build->duk, -1, &json_size);
		path = path_rebase(path_new("sourcemap.json"),
			build->spk ? build->staging_path : build->out_path);
		path_mkdir(build->out_path);
		if (!fspew(json, json_size, path_cstr(path))) {
			printf("\n[error] failed to write source map");
			return false;
		}
		if (build->spk != NULL)
			spk_add_file(build->spk, path_cstr(path), path_filename_cstr(path));
		printf("OK.\n");
	}

	printf(has_changed ? "Success!\n" : "Everything's up to date!\n");

	return true;
}

static void
process_add_files(build_t* build, const char* wildcard, const path_t* path, const path_t* subpath, bool recursive, vector_t* *inout_targets)
{
	tinydir_dir  dir;
	tinydir_file file_info;
	path_t*      file_path = NULL;
	path_t*      full_path;
	path_t*      new_subpath;
	target_t*    target;

	full_path = subpath != NULL
		? path_cat(path_dup(path), subpath)
		: path_dup(path);
	tinydir_open(&dir, path_cstr(full_path));
	dir.n_files;
	while (dir.has_next) {
		tinydir_readfile(&dir, &file_info);
		tinydir_next(&dir);
		
		path_free(file_path);
		file_path = file_info.is_dir
			? path_new_dir(file_info.path)
			: path_new(file_info.path);
		if (!path_resolve(file_path, NULL)) continue;
		if (strcmp(file_info.name, ".") == 0 || strcmp(file_info.name, "..") == 0)
			continue;
		if (!wildcmp(file_info.name, wildcard) && file_info.is_reg) continue;
		if (path_cmp(file_path, build->staging_path)) continue;
		
		if (file_info.is_reg) {
			target = add_target(build, new_file_asset(file_path), subpath);
			push_back_vector(*inout_targets, &target);
		}
		else if (file_info.is_dir && recursive) {
			new_subpath = subpath != NULL
				? path_append_dir(path_dup(subpath), file_info.name)
				: path_new_dir(file_info.name);
			process_add_files(build, wildcard, path, new_subpath, recursive, inout_targets);
			path_free(new_subpath);
		}
	}
	path_free(file_path);
	tinydir_close(&dir);
}

static bool
process_install(build_t* build, struct install* inst, bool *out_is_new)
{
	void*         file_data = NULL;
	size_t        file_size;
	const char*   fn_dest;
	const char*   fn_src;
	path_t*       out_path;
	struct stat   sb_src, sb_dest;
	const path_t* src_path;

	*out_is_new = false;
	if (!(src_path = get_asset_path(inst->target->asset)))
		return false;

	if (build->spk == NULL) {
		out_path = path_cat(path_dup(build->out_path), inst->path);
		path_cat(out_path, inst->target->subpath);
		path_append(out_path, path_filename_cstr(src_path));
		path_mkdir(out_path);
		fn_src = path_cstr(src_path);
		fn_dest = path_cstr(out_path);
		if (stat(fn_src, &sb_src) != 0) {
			fprintf(stderr, "[error] failed to access '%s'\n", fn_src);
			return false;
		}
		if (stat(fn_dest, &sb_dest) != 0
			|| difftime(sb_src.st_mtime, sb_dest.st_mtime) > 0.0)
		{
			path_mkdir(out_path);
			if (!(file_data = fslurp(fn_src, &file_size))
				|| !fspew(file_data, file_size, fn_dest))
			{
				free(file_data);
				fprintf(stderr, "[error] failed to copy '%s' to '%s'\n",
					path_cstr(src_path), path_cstr(out_path));
				return false;
			}
			free(file_data);
			*out_is_new = true;
		}
		path_free(inst->path);
		inst->path = path_resolve(out_path, build->out_path);
	}
	else {
		out_path = path_cat(path_dup(inst->path), inst->target->subpath);
		path_collapse(out_path, true);
		path_append(out_path, path_filename_cstr(src_path));
		spk_add_file(build->spk, path_cstr(src_path), path_cstr(out_path));
		path_free(inst->path);
		inst->path = path_dup(out_path);
		*out_is_new = true;
	}
	return true;
}

static bool
process_target(build_t* build, const target_t* target, bool *out_is_new)
{
	return build_asset(target->asset, build->staging_path, out_is_new);
}


static duk_ret_t
js_api_install(duk_context* ctx)
{
	build_t*   build;
	int        n_args;
	duk_size_t n_targets;
	path_t*    path = NULL;
	target_t*  target;

	size_t i;

	n_args = duk_get_top(ctx);
	if (n_args >= 2)
		path = path_new_dir(duk_require_string(ctx, 1));
	duk_push_global_stash(ctx);
	build = (duk_get_prop_string(ctx, -1, "\xFF""environ"), duk_get_pointer(ctx, -1));
	duk_pop_2(ctx);
	
	if (!duk_is_array(ctx, 0)) {
		target = duk_require_pointer(ctx, 0);
		add_install(build, target, path);
	}
	else {
		n_targets = duk_get_length(ctx, 0);
		for (i = 0; i < n_targets; ++i) {
			duk_get_prop_index(ctx, 0, (duk_uarridx_t)i);
			add_install(build, duk_require_pointer(ctx, -1), path);
			duk_pop(ctx);
		}
	}
	path_free(path);
	return 0;
}

static duk_ret_t
js_api_files(duk_context* ctx)
{
	build_t*      build;
	duk_uarridx_t idx = 0;
	int           n_args;
	path_t*       pattern;
	bool          recursive;
	vector_t*     targets;

	target_t** p_target;
	iter_t iter;

	n_args = duk_get_top(ctx);
	pattern = path_new(duk_require_string(ctx, 0));
	if (path_filename_cstr(pattern) == NULL)
		path_append(pattern, "*");
	recursive = n_args >= 2 ? duk_require_boolean(ctx, 1) : true;
	duk_push_global_stash(ctx);
	build = (duk_get_prop_string(ctx, -1, "\xFF""environ"), duk_get_pointer(ctx, -1));
	duk_pop_2(ctx);

	targets = add_files(build, pattern, recursive);
	duk_push_array(ctx);
	iter = iterate_vector(targets);
	while (p_target = next_vector_item(&iter)) {
		duk_push_pointer(ctx, *p_target);
		duk_put_prop_index(ctx, -2, idx++);
	}
	free_vector(targets);
	return 1;
}

static duk_ret_t
js_api_sgm(duk_context* ctx)
{
	build_t*   build;
	sgm_info_t manifest;
	target_t*  target;

	duk_require_object_coercible(ctx, 0);
	duk_push_global_stash(ctx);
	build = (duk_get_prop_string(ctx, -1, "\xFF""environ"), duk_get_pointer(ctx, -1));
	duk_pop_2(ctx);

	duk_get_prop_string(ctx, 0, "name");
	duk_get_prop_string(ctx, 0, "author");
	duk_get_prop_string(ctx, 0, "description");
	duk_get_prop_string(ctx, 0, "script");
	strcpy(manifest.name, duk_require_string(ctx, -4));
	strcpy(manifest.author, duk_require_string(ctx, -3));
	strcpy(manifest.description, duk_require_string(ctx, -2));
	strcpy(manifest.script, duk_require_string(ctx, -1));
	manifest.width = 320; manifest.height = 240;
	target = add_target(build, new_sgm_asset(manifest, build->js_mtime), NULL);
	duk_push_pointer(ctx, target);
	return 1;
}
