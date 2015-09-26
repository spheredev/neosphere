#include "build.h"

#include "duktape.h"
#include "assets.h"
#include "spk_writer.h"
#include "tinydir.h"
#include "cell.h"

#include <sys/stat.h>

struct build
{
	duk_context*  duk;
	path_t*       in_path;
	vector_t*     installs;
	path_t*       out_path;
	spk_writer_t* spk;
	path_t*       stage_path;
	vector_t*     targets;
};

struct install
{
	const target_t* target;
	path_t*         path;
};

struct target
{
	asset_t* asset;
	path_t*  path;
};

static duk_ret_t js_api_install (duk_context* ctx);
static duk_ret_t js_api_files   (duk_context* ctx);
static duk_ret_t js_api_sgm     (duk_context* ctx);

static bool process_install (build_t* build, const struct install* inst, bool *out_is_new);
static bool process_target  (build_t* build, const target_t* target, bool *out_is_new);

build_t*
new_build(const path_t* in_path, const path_t* out_path)
{
	build_t* build = NULL;

	build = calloc(1, sizeof(build_t));

	// initialize JavaScript environment
	build->duk = duk_create_heap_default();
	duk_push_global_stash(build->duk);
	duk_push_pointer(build->duk, build);
	duk_put_prop_string(build->duk, -2, "\xFF""environ");
	duk_pop(build->duk);

	// wire up JavaScript API
	duk_push_c_function(build->duk, js_api_install, DUK_VARARGS);
	duk_put_global_string(build->duk, "install");
	/*duk_push_c_function(build->duk, js_api_files, DUK_VARARGS);
	duk_put_global_string(build->duk, "files");*/
	duk_push_c_function(build->duk, js_api_sgm, DUK_VARARGS);
	duk_put_global_string(build->duk, "sgm");

	// set up build harness
	build->targets = new_vector(sizeof(target_t*));
	build->installs = new_vector(sizeof(struct install));
	build->in_path = path_dup(in_path);
	build->out_path = path_dup(out_path);
	build->stage_path = path_rebase(path_new(".cell/"), build->in_path);
	path_mkdir(build->stage_path);
	if (path_filename_cstr(out_path))
		build->spk = spk_create(path_cstr(out_path));
	return build;
}

void
free_build(build_t* build)
{
	struct install *p_inst;
	target_t*      *p_target;
	iter_t iter;

	duk_destroy_heap(build->duk);
	iter = iterate_vector(build->targets);
	while (p_target = next_vector_item(&iter)) {
		free_asset((*p_target)->asset);
		path_free((*p_target)->path);
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
	struct stat sb;
	path_t*     script_path;

	script_path = path_rebase(path_new("cell.js"), build->in_path);
	if (stat(path_cstr(script_path), &sb) != 0 || !(sb.st_mode & S_IFREG)) {
		fprintf(stderr, "[err] no cell.js in input directory\n");
		return false;
	}

	// process build script
	printf("Processing cell.js rule '%s'\n", name);
	sprintf(func_name, "$%s", name);
	if (duk_peval_file(build->duk, path_cstr(script_path)) != 0) {
		path_free(script_path);
		printf("[js] %s\n", duk_safe_to_string(build->duk, -1));
		return false;
	}
	path_free(script_path);
	if (!duk_get_global_string(build->duk, func_name) || !duk_is_callable(build->duk, -1)) {
		printf("[err] no rule named '%s' in cell.js\n", name);
		return false;
	}
	if (duk_pcall(build->duk, 0) != 0) {
		printf("[js] %s\n", duk_safe_to_string(build->duk, -1));
		return false;
	}

	return true;
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
add_files(build_t* build, const path_t* pattern, bool recursive)
{
	return NULL;
}

target_t*
add_target(build_t* build, asset_t* asset, const path_t* path)
{
	target_t* target;
	
	target = calloc(1, sizeof(target_t));

	target->asset = asset;
	target->path = path != NULL ? path_dup(path) : path_new("./");
	push_back_vector(build->targets, &target);
	return target;
}

bool
run_build(build_t* build)
{
	bool        is_new;
	int         n_assets;

	struct install *p_inst;
	target_t*      *p_target;
	iter_t iter;

	if (get_vector_size(build->installs) == 0) {
		printf("[err] no assets staged for install\n");
		return false;
	}

	// build and install assets
	printf("Building assets...\n");
	n_assets = 0;
	iter = iterate_vector(build->targets);
	while (p_target = next_vector_item(&iter)) {
		if (!process_target(build, *p_target, &is_new))
			return false;
		if (is_new) ++n_assets;
	}
	if (n_assets > 0) printf("    %d asset(s) built\n", n_assets);
		else printf("    Up to date!\n");

	printf("Installing assets...\n");
	n_assets = 0;
	iter = iterate_vector(build->installs);
	while (p_inst = next_vector_item(&iter)) {
		if (!process_install(build, p_inst, &is_new))
			return false;
		if (is_new) ++n_assets;
	}
	if (n_assets > 0) printf("    %d asset(s) installed\n", n_assets);
		else printf("    Up to date!\n");
	
	printf("Success!\n\n");

	return true;
}

static bool
process_install(build_t* build, const struct install* inst, bool *out_is_new)
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
		path_cat(out_path, inst->target->path);
		path_append(out_path, path_filename_cstr(src_path), false);
		path_mkdir(out_path);
		fn_src = path_cstr(src_path);
		fn_dest = path_cstr(out_path);
		if (stat(fn_src, &sb_src) != 0) {
			fprintf(stderr, "error: failed to access '%s'\n", fn_src);
			return false;
		}
		if (stat(fn_dest, &sb_dest) != 0
			|| difftime(sb_src.st_mtime, sb_dest.st_mtime) > 0.0)
		{
			if (!(file_data = fslurp(fn_src, &file_size))
				|| !fspew(file_data, file_size, fn_dest))
			{
				free(file_data);
				fprintf(stderr, "error: failed to copy '%s' to '%s'\n",
					path_cstr(src_path), path_cstr(out_path));
				return false;
			}
			free(file_data);
			*out_is_new = true;
		}
	}
	else {
		out_path = path_cat(path_dup(inst->path), inst->target->path);
		path_collapse(out_path, true);
		path_append(out_path, path_filename_cstr(src_path), false);
		spk_pack(build->spk, path_cstr(src_path), path_cstr(out_path));
		*out_is_new = true;
	}
	if (*out_is_new)
		printf("    %s\n", path_cstr(out_path));
	path_free(out_path);
	return true;
}

static bool
process_target(build_t* build, const target_t* target, bool *out_is_new)
{
	return build_asset(target->asset, build->stage_path, out_is_new);
}


static duk_ret_t
js_api_install(duk_context* ctx)
{
	int       n_args;
	build_t*  build;
	path_t*   path = NULL;
	target_t* target;

	n_args = duk_get_top(ctx);
	target = duk_require_pointer(ctx, 0);
	if (n_args >= 2)
		path = path_new(duk_require_string(ctx, 1));
	duk_push_global_stash(ctx);
	build = (duk_get_prop_string(ctx, -1, "\xFF""environ"), duk_get_pointer(ctx, -1));
	duk_pop_2(ctx);

	add_install(build, target, path);
	path_free(path);
	return 0;
}

static duk_ret_t
js_api_files(duk_context* ctx)
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
	target = add_target(build, new_sgm_asset(manifest), NULL);
	duk_push_pointer(ctx, target);
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
	target = add_target(build, new_sgm_asset(manifest), NULL);
	duk_push_pointer(ctx, target);
	return 1;
}
