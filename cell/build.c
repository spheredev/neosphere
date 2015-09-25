#include "build.h"

#include "assets.h"
#include "spk_writer.h"
#include "tinydir.h"
#include "cell.h"

static duk_ret_t js_file    (duk_context* ctx);
static duk_ret_t js_install (duk_context* ctx);

static int  do_install (const char* pattern, const path_t* src_path, const path_t* dest_path, bool recursive);
static bool fspew      (const void* buffer, size_t size, const char* filename);
static bool wildcmp    (const char* filename, const char* pattern);

struct install
{
	asset_t* asset;
	path_t*  path;
};

static vector_t* s_installs;

void
initialize_engine(void)
{
	s_installs = new_vector(sizeof(struct install));
}

void
shutdown_engine(void)
{
	struct install* *p_inst;
	iter_t iter;

	iter = iterate_vector(s_installs);
	while (p_inst = next_vector_item(&iter)) {
		free_asset((*p_inst)->asset);
		path_free((*p_inst)->path);
	}
	free_vector(s_installs);
}

bool
run_build()
{
	int num_installs = 0;
	
	struct install *p_inst;
	iter_t iter;
	
	if (get_vector_size(s_installs) == 0) {
		printf("FATAL: no assets staged for install\n");
		return false;
	}
	
	printf("Building assets... ");
	iter = iterate_vector(s_installs);
	while (p_inst = next_vector_item(&iter)) {
		if (build_asset(p_inst->asset))
			++num_installs;
	}
	if (num_installs > 0)
		printf("%d built\n", num_installs);
	else
		printf("Up to date!\n");
	
	printf("Installing assets... ");
	iter = iterate_vector(s_installs);
	while (p_inst = next_vector_item(&iter)) {
		if (install_asset(p_inst->asset, p_inst->path))
			++num_installs;
	}
	if (num_installs > 0)
		printf("%d installed\n", num_installs);
	else
		printf("Up to date!\n");
	
	printf("Success!\n\n");
	
	return true;
}

static int
do_install(const char* pattern, const path_t* src_path, const path_t* dest_path, bool is_recursive)
{
	tinydir_dir    dir;
	tinydir_file   file;
	path_t*        in_path;
	struct install inst;
	path_t*        out_path;
	bool           skip_entry;
	int            num_files = 0;

	tinydir_open(&dir, path_cstr(src_path));
	while (dir.has_next) {
		tinydir_readfile(&dir, &file);
		tinydir_next(&dir);

		skip_entry = file.is_dir && !is_recursive
			|| strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0
			|| strstr(file.path, path_cstr(g_out_path)) == file.path;
		if (skip_entry)
			continue;

		in_path = path_append(path_dup(src_path), file.name, file.is_dir);
		out_path = path_append(path_dup(dest_path), file.name, file.is_dir);
		if (!file.is_dir && wildcmp(file.name, pattern)) {
			inst.asset = new_file_asset(in_path);
			inst.path = path_dup(out_path);
			push_back_vector(s_installs, &inst);
			++num_files;
		}
		else if (file.is_dir) {
			num_files += do_install(pattern, in_path, out_path, is_recursive);
		}
		path_free(in_path);
		path_free(out_path);
	}
	tinydir_close(&dir);
	return num_files;
}

static bool
fspew(const void* buffer, size_t size, const char* filename)
{
	FILE* file = NULL;

	if (!(file = fopen(filename, "wb")))
		return false;
	fwrite(buffer, size, 1, file);
	fclose(file);
	return true;
}

static bool
wildcmp(const char *filename, const char *pattern)
{
	const char* cp = NULL;
	const char* mp = NULL;
	bool        is_match = 0;
	char*       parse;
	char*       token;

	if (strchr(pattern, '|')) {
		// if there are any pipe character in the input, split at the pipes
		// and run a check on each filter in the list. if any filter matches, we
		// return true.
		parse = strdup(pattern);
		token = strtok(parse, "|");
		do {
			if (is_match |= wildcmp(filename, token))
				break;
		} while (token = strtok(NULL, "|"));
		free(parse);
		return is_match;
	}
	else {
		// check filename against the specified filter string
		while (*filename != '\0' && *pattern != '*') {
			if (*pattern != *filename && *pattern != '?')
				return false;
			++pattern;
			++filename;
		}
		while (*filename != '\0') {
			if (*pattern == '*') {
				if (*++pattern == '\0') return true;
				mp = pattern;
				cp = filename + 1;
			}
			else if (*pattern == *filename || *pattern == '?') {
				pattern++;
				filename++;
			}
			else {
				pattern = mp;
				filename = cp++;
			}
		}
		while (*pattern == '*')
			pattern++;
		return *pattern == '\0';
	}
}

void
initialize_js_api(void)
{
	duk_push_c_function(g_duk, js_file, DUK_VARARGS);
	duk_put_global_string(g_duk, "file");
	duk_push_c_function(g_duk, js_install, DUK_VARARGS);
	duk_put_global_string(g_duk, "install");
}

static duk_ret_t
js_game(duk_context* ctx)
{
	const char*   name;
	const char*   author;
	const char*   description;
	char*         parse;
	const char*   resolution;
	int           res_x, res_y;
	const path_t* script_path;

	duk_require_object_coercible(ctx, 0);
	name = (duk_get_prop_string(ctx, 0, "name"), duk_require_string(ctx, -1));
	author = (duk_get_prop_string(ctx, 0, "author"), duk_require_string(ctx, -1));
	description = (duk_get_prop_string(ctx, 0, "description"), duk_require_string(ctx, -1));
	resolution = (duk_get_prop_string(ctx, 0, "resolution"), duk_require_string(ctx, -1));
	script_path = path_new((duk_get_prop_string(ctx, 0, "script"), duk_require_string(ctx, -1)));

	// parse screen resolution
	parse = strdup(resolution);
	res_x = atoi(strtok(parse, "x"));
	res_y = atoi(strtok(NULL, "x"));

	return 0;
}

static duk_ret_t
js_file(duk_context* ctx)
{
	// file(filename, isRecursive);
	// Returns a file asset, which copies a file as-is into the target when installed.
	// Arguments:
	//     pattern:     A filename pattern, e.g. "*.js". Files matching the pattern
	//                  will be copied to the destination directory.
	//     path:        Path to the destination, relative to the output directory.
	//     isRecursive: Optional. true to recursively copy files in subdirectories.
	//                  (default: true)

	asset_t* asset;
	path_t*  src_path;

	src_path = path_new(duk_require_string(ctx, 0));
	asset = new_file_asset(src_path);
	path_free(src_path);
	duk_push_pointer(ctx, asset);
	return 1;
}

static duk_ret_t
js_install(duk_context* ctx)
{
	duk_idx_t      n_args;
	struct install inst;
	duk_size_t     num_assets;
	path_t*        path;

	duk_size_t i;

	n_args = duk_get_top(ctx);
	path = path_new_dir(n_args >= 2 ? duk_require_string(ctx, 1) : "./");
	if (!duk_is_array(ctx, 0)) {
		inst.asset = duk_require_pointer(ctx, 0);
		inst.path = path_dup(path);
		push_back_vector(s_installs, &inst);
	}
	else {
		num_assets = duk_get_length(ctx, 0);
		for (i = 0; i < num_assets; ++i) {
			duk_get_prop_index(ctx, 0, i);
			inst.asset = duk_require_pointer(ctx, -1);
			duk_pop(ctx);
			inst.path = path_dup(path);
			push_back_vector(s_installs, &inst);
		}
	}
	return 0;
}
