#include "cell.h"
#include "build.h"

#include "assets.h"
#include "spk_writer.h"
#include "tinydir.h"

struct build
{
	duk_context*  duktape;
	path_t*       in_path;
	vector_t*     installs;
	path_t*       out_path;
	time_t        js_mtime;
	int           last_warn_count;
	int           num_errors;
	int           num_warnings;
	char*         rule_name;
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
	unsigned int num_refs;
	asset_t*     asset;
	path_t*      subpath;
};

static duk_ret_t js_api_install (duk_context* ctx);
static duk_ret_t js_api_files   (duk_context* ctx);
static duk_ret_t js_api_s2gm    (duk_context* ctx);
static duk_ret_t js_api_sgm     (duk_context* ctx);

static int  compare_asset_names (const void* in_a, const void* in_b);
static void emit_begin_op       (build_t* build, const char* fmt, ...);
static void emit_end_op         (build_t* build, const char* fmt, ...);
static void emit_warning        (build_t* build, const char* fmt, ...);
static void emit_error          (build_t* build, const char* fmt, ...);
static void process_add_files   (build_t* build, const char* wildcard, const path_t* path, const path_t* subpath, bool recursive, vector_t* *inout_targets);
static bool process_install     (build_t* build, struct install* inst, bool *out_is_new);
static bool process_target      (build_t* build, const target_t* target, bool *out_is_new);
static void validate_targets    (build_t* build);

build_t*
build_new(const path_t* in_path, const path_t* out_path, bool want_source_map)
{
	build_t*    build = NULL;
	path_t*     path;
	struct stat sb;

	build = calloc(1, sizeof(build_t));

	// check for Cellscript.js in input directory
	path = path_rebase(path_new("Cellscript.js"), in_path);
	if (stat(path_cstr(path), &sb) != 0 || !(sb.st_mode & S_IFREG)) {
		fprintf(stderr, "error: internal: failed to stat Cellscript.js\n");
		return NULL;
	}
	build->js_mtime = sb.st_mtime;
	path_free(path);

	// initialize JavaScript environment
	build->duktape = duk_create_heap_default();
	duk_push_global_stash(build->duktape);
	duk_push_pointer(build->duktape, build);
	duk_put_prop_string(build->duktape, -2, "\xFF""environ");
	duk_pop(build->duktape);

	// wire up JavaScript API
	duk_push_c_function(build->duktape, js_api_install, DUK_VARARGS);
	duk_put_global_string(build->duktape, "install");
	duk_push_c_function(build->duktape, js_api_files, DUK_VARARGS);
	duk_put_global_string(build->duktape, "files");
	duk_push_c_function(build->duktape, js_api_s2gm, DUK_VARARGS);
	duk_put_global_string(build->duktape, "s2gm");
	duk_push_c_function(build->duktape, js_api_sgm, DUK_VARARGS);
	duk_put_global_string(build->duktape, "sgm");

	// set up build environment (ensure directory exists, etc.)
	path_mkdir(out_path);
	if (path_filename_cstr(out_path)) {
		build->spk = spk_create(path_cstr(out_path));
		if (build->spk == NULL) {
			fprintf(stderr, "ERROR: unable to create package '%s'\n", path_cstr(out_path));
			goto on_error;
		}
	}

	build->want_source_map = want_source_map;
	build->targets = vector_new(sizeof(target_t*));
	build->installs = vector_new(sizeof(struct install));
	build->in_path = path_resolve(path_dup(in_path), NULL);
	build->out_path = path_resolve(path_dup(out_path), NULL);
	build->staging_path = path_rebase(path_new(".cell/"), build->in_path);

	if (build->out_path == NULL) {
		fprintf(stderr, "ERROR: unable to create '%s'\n", path_cstr(out_path));
		goto on_error;
	}
	return build;

on_error:
	duk_destroy_heap(build->duktape);
	free(build);
	return NULL;
}

void
build_free(build_t* build)
{
	struct install *p_inst;
	target_t*      *p_target;
	iter_t iter;

	if (build == NULL)
		return;
	
	duk_destroy_heap(build->duktape);
	iter = vector_enum(build->targets);
	while (p_target = vector_next(&iter)) {
		asset_free((*p_target)->asset);
		path_free((*p_target)->subpath);
		free(*p_target);
	}
	vector_free(build->targets);
	iter = vector_enum(build->installs);
	while (p_inst = vector_next(&iter)) {
		path_free(p_inst->path);
	}
	vector_free(build->installs);
	path_free(build->staging_path);
	path_free(build->in_path);
	path_free(build->out_path);
	spk_close(build->spk);
	free(build->rule_name);
	free(build);
}

bool
build_is_ok(const build_t* build, int *out_n_errors, int *out_n_warnings)
{
	if (out_n_errors) *out_n_errors = build->num_errors;
	if (out_n_warnings) *out_n_warnings = build->num_warnings;
	return build->num_errors == 0;
}

target_t*
build_add_asset(build_t* build, asset_t* asset, const path_t* subpath)
{
	target_t* target;

	target = calloc(1, sizeof(target_t));

	target->asset = asset;
	target->subpath = subpath != NULL
		? path_dup(subpath)
		: path_new("./");
	vector_push(build->targets, &target);
	return target;
}

vector_t*
build_add_files(build_t* build, const path_t* pattern, bool recursive)
{
	path_t*   path;
	vector_t* targets;
	char*     wildcard;

	targets = vector_new(sizeof(target_t*));
	path = path_rebase(path_strip(path_dup(pattern)), build->in_path);
	wildcard = strdup(path_filename_cstr(pattern));
	process_add_files(build, wildcard, path, NULL, recursive, &targets);
	path_free(path);
	free(wildcard);
	return targets;
}

void
build_install(build_t* build, const target_t* target, const path_t* path)
{
	struct install inst;

	inst.target = target;
	inst.path = path != NULL ? path_dup(path) : path_new("./");
	vector_push(build->installs, &inst);
}

bool
build_prime(build_t* build, const char* rule_name)
{
	char    func_name[255];
	path_t* script_path;

	build->rule_name = strdup(rule_name);

	// process build script
	emit_begin_op(build, "processing Cellscript.js rule '%s'", rule_name);
	sprintf(func_name, "$%s", rule_name);
	script_path = path_rebase(path_new("Cellscript.js"), build->in_path);
	if (duk_peval_file(build->duktape, path_cstr(script_path)) != 0) {
		path_free(script_path);
		emit_error(build, "JS: %s", duk_safe_to_string(build->duktape, -1));
		goto on_error;
	}
	path_free(script_path);
	if (duk_get_global_string(build->duktape, func_name) && duk_is_callable(build->duktape, -1)) {
		if (duk_pcall(build->duktape, 0) != 0) {
			emit_error(build, "JS: %s", duk_safe_to_string(build->duktape, -1));
			goto on_error;
		}
	}
	else {
		emit_error(build, "no Cellscript rule named '%s'", rule_name);
		goto on_error;
	}

	validate_targets(build);
	if (build->num_errors)
		goto on_error;

	path_append_dir(build->staging_path, rule_name);
	emit_end_op(build, "OK.");
	return true;

on_error:
	printf("\n");
	return false;
}

bool
build_run(build_t* build)
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

	if (vector_len(build->installs) == 0) {
		emit_error(build, "no assets staged for install");
		return false;
	}

	// build and install assets
	printf("compiling assets...");
	path_mkdir(build->staging_path);
	n_assets = 0;
	iter = vector_enum(build->targets);
	while (p_target = vector_next(&iter)) {
		if (!process_target(build, *p_target, &is_new))
			return false;
		if (is_new) {
			if (n_assets == 0) printf("\n");
			printf("    %s\n", path_cstr(asset_object_path((*p_target)->asset)));
			++n_assets;
			has_changed = true;
		}
	}
	if (n_assets > 0) printf("    %d asset(s) compiled\n", n_assets);
		else printf(" up-to-date.\n");

	printf("installing assets... ");
	n_assets = 0;
	iter = vector_enum(build->installs);
	while (p_inst = vector_next(&iter)) {
		if (!process_install(build, p_inst, &is_new))
			return false;
		if (is_new) {
			if (n_assets == 0) printf("\n");
			printf("    %s\n", path_cstr(p_inst->path));
			++n_assets;
			has_changed = true;
		}
	}
	if (n_assets > 0) printf("    %d asset(s) installed\n", n_assets);
		else printf(" up-to-date.\n");

	// generate source map
	if (build->want_source_map) {
		printf("generating source map... ");
		duk_push_object(build->duktape);
		duk_push_string(build->duktape, path_cstr(build->in_path));
		duk_put_prop_string(build->duktape, -2, "origin");
		duk_push_object(build->duktape);
		iter = vector_enum(build->installs);
		while (p_inst = vector_next(&iter)) {
			path = path_resolve(path_dup(asset_object_path(p_inst->target->asset)), build->in_path);
			duk_push_string(build->duktape, path_cstr(path));
			duk_put_prop_string(build->duktape, -2, path_cstr(p_inst->path));
			path_free(path);
		}
		duk_put_prop_string(build->duktape, -2, "fileMap");
		duk_json_encode(build->duktape, -1);
		json = duk_get_lstring(build->duktape, -1, &json_size);
		path = path_rebase(path_new("sourcemap.json"),
			build->spk != NULL ? build->staging_path : build->out_path);
		path_mkdir(build->out_path);
		if (!fspew(json, json_size, path_cstr(path))) {
			path_free(path);
			fprintf(stderr, "\nERROR: failed to write source map\n");
			return false;
		}
		if (build->spk != NULL)
			spk_add_file(build->spk, path_cstr(path), path_filename_cstr(path));
		path_free(path);
		printf("OK.\n");
	}
	else if (build->spk == NULL) {
		// non-debug build, remove any existing source map
		path = path_rebase(path_new("sourcemap.json"), build->out_path);
		unlink(path_cstr(path));
		path_free(path);
	}

	printf("%s -> %s\n", build->rule_name, path_cstr(build->out_path));
	printf("    %d errors, %d warnings\n", build->num_errors, build->num_warnings);

	return true;
}

static void
emit_begin_op(build_t* build, const char* fmt, ...)
{
	va_list ap;

	build->last_warn_count = build->num_warnings;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	printf("... ");
	va_end(ap);
}

static void
emit_end_op(build_t* build, const char* fmt, ...)
{
	va_list ap;

	if (build->num_warnings > build->last_warn_count)
		printf("\n");
	else {
		va_start(ap, fmt);
		vprintf(fmt, ap);
		printf("\n");
		va_end(ap);
	}
}

static void
emit_warning(build_t* build, const char* fmt, ...)
{
	va_list ap;

	++build->num_warnings;
	va_start(ap, fmt);
	printf("\n  warning: ");
	vprintf(fmt, ap);
	va_end(ap);
}

static void
emit_error(build_t* build, const char* fmt, ...)
{
	va_list ap;

	++build->num_errors;
	va_start(ap, fmt);
	printf("\n  ERROR: ");
	vprintf(fmt, ap);
	va_end(ap);
}

static void
validate_targets(build_t* build)
{
	const path_t* name;
	int           num_dups = 0;
	const path_t* prev_name = NULL;
	vector_t*     targets;

	iter_t iter;
	target_t* *p_target;

	// check for asset name conflicts
	targets = vector_sort(vector_dup(build->targets), compare_asset_names);
	iter = vector_enum(build->targets);
	while (p_target = vector_next(&iter)) {
		if (!(name = asset_name((*p_target)->asset)))
			continue;
		if (!(*p_target)->num_refs == 0)
			continue;
		if (prev_name != NULL && path_cmp(name, prev_name))
			++num_dups;
		else {
			if (num_dups > 0)
				emit_error(build, "'%s' %d-way asset conflict", path_cstr(name), num_dups + 1);
			num_dups = 0;
		}
		prev_name = name;
	}
	if (num_dups > 0)
		emit_error(build, "'%s' %d-way asset conflict", path_cstr(prev_name), num_dups + 1);
	vector_free(targets);
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
	path_free(full_path);
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
			target = build_add_asset(build, asset_new_file(file_path), subpath);
			vector_push(*inout_targets, &target);
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
	if (!(src_path = asset_object_path(inst->target->asset)))
		return false;

	if (build->spk == NULL) {
		out_path = path_cat(path_dup(build->out_path), inst->path);
		path_cat(out_path, inst->target->subpath);
		path_append(out_path, path_filename_cstr(src_path));
		path_mkdir(out_path);
		fn_src = path_cstr(src_path);
		fn_dest = path_cstr(out_path);
		if (stat(fn_src, &sb_src) != 0) {
			emit_error(build, "failed to access '%s'", fn_src);
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
				emit_error(build, "failed to copy '%s' to '%s'", path_cstr(src_path), path_cstr(out_path));
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
		inst->path = out_path;
		*out_is_new = true;
	}
	return true;
}

static bool
process_target(build_t* build, const target_t* target, bool *out_is_new)
{
	return target->num_refs == 0
		|| asset_build(target->asset, build->staging_path, out_is_new);
}

static int
compare_asset_names(const void* in_a, const void* in_b)
{
	const path_t* path_a;
	const path_t* path_b;

	path_a = asset_name((*(target_t**)in_a)->asset);
	path_b = asset_name((*(target_t**)in_b)->asset);
	return path_a == path_b ? 0
		: path_a == NULL ? 1
		: path_b == NULL ? -1
		: strcmp(path_cstr(path_a), path_cstr(path_b));
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
		++target->num_refs;
		build_install(build, target, path);
	}
	else {
		n_targets = duk_get_length(ctx, 0);
		for (i = 0; i < n_targets; ++i) {
			duk_get_prop_index(ctx, 0, (duk_uarridx_t)i);
			target = duk_require_pointer(ctx, -1);
			++target->num_refs;
			build_install(build, target, path);
			duk_pop(ctx);
		}
	}
	path_free(path);
	return 0;
}

static duk_ret_t
js_api_files(duk_context* ctx)
{
	int           n_args;
	build_t*      build;
	duk_uarridx_t idx = 0;
	bool          is_recursive;
	path_t*       pattern;
	vector_t*     targets;

	target_t** p_target;
	iter_t iter;

	n_args = duk_get_top(ctx);
	pattern = path_new(duk_require_string(ctx, 0));
	if (path_filename_cstr(pattern) == NULL)
		path_append(pattern, "*");
	is_recursive = n_args >= 2 ? duk_require_boolean(ctx, 1) : false;
	duk_push_global_stash(ctx);
	build = (duk_get_prop_string(ctx, -1, "\xFF""environ"), duk_get_pointer(ctx, -1));
	duk_pop_2(ctx);

	targets = build_add_files(build, pattern, is_recursive);
	if (vector_len(targets) == 0)
		emit_warning(build, "files(): no files match '%s'", path_cstr(pattern));
	duk_push_array(ctx);
	iter = vector_enum(targets);
	while (p_target = vector_next(&iter)) {
		duk_push_pointer(ctx, *p_target);
		duk_put_prop_index(ctx, -2, idx++);
	}
	vector_free(targets);
	path_free(pattern);
	return 1;
}

static duk_ret_t
js_api_sgm(duk_context* ctx)
{
	build_t*   build;
	sgm_info_t manifest;
	char*      token;
	char*      next_token;
	char*      res_string;
	target_t*  target;

	duk_require_object_coercible(ctx, 0);
	duk_push_global_stash(ctx);
	build = (duk_get_prop_string(ctx, -1, "\xFF""environ"), duk_get_pointer(ctx, -1));
	duk_pop_2(ctx);

	duk_get_prop_string(ctx, 0, "name");
	duk_get_prop_string(ctx, 0, "author");
	duk_get_prop_string(ctx, 0, "summary");
	duk_get_prop_string(ctx, 0, "resolution");
	duk_get_prop_string(ctx, 0, "script");
	strcpy(manifest.name, duk_require_string(ctx, -5));
	strcpy(manifest.author, duk_require_string(ctx, -4));
	strcpy(manifest.description, duk_require_string(ctx, -3));
	strcpy(manifest.script, duk_require_string(ctx, -1));
	
	// parse the screen resolution string
	res_string = strdup(duk_require_string(ctx, -2));
	token = strtok_r(res_string, "x", &next_token);
	manifest.width = atoi(token);
	if (!(token = strtok_r(NULL, "x", &next_token)))
		duk_error(ctx, DUK_ERR_TYPE_ERROR, "sgm(): malformed resolution '%s'", duk_require_string(ctx, -2));
	manifest.height = atoi(token);
	if (strtok_r(NULL, "x", &next_token) || manifest.width <= 0 || manifest.height <= 0)
		duk_error(ctx, DUK_ERR_TYPE_ERROR, "sgm(): malformed resolution '%s'", duk_require_string(ctx, -2));
	free(res_string);
	target = build_add_asset(build, asset_new_sgm(manifest, build->js_mtime), NULL);
	duk_push_pointer(ctx, target);
	return 1;
}

static duk_ret_t
js_api_s2gm(duk_context* ctx)
{
	build_t*    build;
	const char* json;
	path_t*     name;
	target_t*   target;

	duk_require_object_coercible(ctx, 0);
	duk_push_global_stash(ctx);
	build = (duk_get_prop_string(ctx, -1, "\xFF""environ"), duk_get_pointer(ctx, -1));
	duk_pop_2(ctx);

	if (!duk_get_prop_string(ctx, 0, "author"))
		emit_warning(build, "s2gm(): 'author' field is missing");
	if (!duk_get_prop_string(ctx, 0, "summary"))
		emit_warning(build, "s2gm(): 'summary' field is missing");
	duk_pop_2(ctx);

	json = duk_json_encode(ctx, 0);
	name = path_new("game.s2gm");
	target = build_add_asset(build,
		asset_new_raw(name, json, strlen(json), build->js_mtime),
		NULL);
	path_free(name);
	duk_push_pointer(ctx, target);
	return 1;
}
