#include "build.h"

#include "cell.h"
#include "spk_writer.h"
#include "tinydir.h"

static duk_ret_t js_game    (duk_context* ctx);
static duk_ret_t js_install (duk_context* ctx);

static void add_asset  (const path_t* source_path, const path_t* dest_path);
static int  do_install (const char* pattern, const path_t* src_path, const path_t* dest_path, bool recursive);
static bool fspew      (const void* buffer, size_t size, const char* filename);
static bool mkdir_r    (const char* path);
static bool wildcmp    (const char* filename, const char* pattern);

struct game
{
	char* name;
	char* author;
	char* description;
	int   width;
	int   height;
	char* script_path;
};

struct asset
{
	path_t* source_path;
	path_t* path;
};

static vector_t*   s_assets;
static struct game s_game_info;

void
initialize_engine(void)
{
	memset(&s_game_info, 0, sizeof(struct game));
	s_assets = new_vector(sizeof(struct asset));
}

void
shutdown_engine(void)
{
	struct asset* source;
	
	iter_t iter;

	iter = iterate_vector(s_assets);
	while (source = next_vector_item(&iter)) {
		free(source->source_path);
		free(source->path);
	}
	free_vector(s_assets);
}

bool
run_build()
{
	FILE*          file;
	const char*    json;
	int            num_files = 0;
	path_t*        path;
	spk_writer_t*  spk = NULL;
	struct asset*  asset;
	
	iter_t iter;
	
	if (s_game_info.name == NULL) {
		printf("FATAL: missing game() directive\n");
		return false;
	}
	
	// copy staged files
	printf("Installing assets... ");
	if (get_vector_size(s_assets) == 0)
		printf("warning: no files installed\n");
	if (g_want_source_map)
		duk_push_object(g_duk);
	iter = iterate_vector(s_assets);
	while (asset = next_vector_item(&iter)) {
		path = path_strip(path_dup(asset->path));
		mkdir_r(path_cstr(path));
		path_free(path);
		if (tinydir_copy(path_cstr(asset->source_path), path_cstr(asset->path), true) == 0) {
			++num_files;
			print_verbose("  '%s' => '%s'\n",
				path_cstr(asset->source_path),
				path_cstr(asset->path));
		}
		path = path_resolve(path_dup(asset->path), g_out_path);
		if (g_want_source_map) {
			duk_push_string(g_duk, path_cstr(path));
			duk_put_prop_string(g_duk, -2, path_cstr(asset->source_path));
		}
		path_free(path);
	}
	if (num_files > 0)
		printf("%d asset(s)\n", num_files);
	else
		printf("Up to date!\n");
	
	if (g_want_source_map) {
		printf("Writing source map... ");
		path = path_rebase(path_new("sourcemap.json"), g_out_path);
		duk_get_global_string(g_duk, "JSON");
		duk_get_prop_string(g_duk, -1, "stringify");
		duk_dup(g_duk, -3);
		duk_push_null(g_duk);
		duk_push_string(g_duk, "\t");
		duk_call(g_duk, 3);
		json = duk_get_string(g_duk, -1);
		fspew(json, strlen(json), path_cstr(path));
		duk_pop_3(g_duk);
		add_asset(NULL, path);
		path_free(path);
		printf("Done!\n");
	}
	
	// write game.sgm
	printf("Writing game manifest... ");
	mkdir_r(path_cstr(g_out_path));
	path = path_rebase(path_new("game.sgm"), g_out_path);
	if (!(file = fopen(path_cstr(path), "wb")))
		fprintf(stderr, "FATAL: failed to write game manifest");
	fprintf(file, "name=%s\n", s_game_info.name);
	fprintf(file, "author=%s\n", s_game_info.author);
	fprintf(file, "description=%s\n", s_game_info.description);
	fprintf(file, "screen_width=%d\n", s_game_info.width);
	fprintf(file, "screen_height=%d\n", s_game_info.height);
	fprintf(file, "script=%s\n", s_game_info.script_path);
	fclose(file);
	add_asset(NULL, path);
	path_free(path);
	printf("Done!\n");
	
	if (g_want_package) {
		printf("Packaging... ");
		path = path_rebase(path_new("game.spk"), g_out_path);
		spk = spk_create(path_cstr(path));
		path_free(path);
		iter = iterate_vector(s_assets);
		while (asset = next_vector_item(&iter)) {
			path = path_resolve(path_dup(asset->path), g_out_path);
			spk_pack(spk, path_cstr(asset->path), path_cstr(path));
			path_free(path);
		}
		spk_close(spk);
		printf("%zu asset(s)\n", get_vector_size(s_assets));
	}
	
	printf("Success!\n\n");
	
	path = path_resolve(path_dup(g_out_path), NULL);
	printf("Sphere game built in '%s'\n", path_cstr(path));
	path_free(path);
	printf("         Title: %s\n", s_game_info.name);
	printf("        Author: %s\n", s_game_info.author);
	printf("    Resolution: %dx%d\n", s_game_info.width, s_game_info.height);

	return true;
}

static void
add_asset(const path_t* source_path, const path_t* dest_path)
{
	struct asset source;

	source.source_path = source_path 
		? path_dup(source_path)
		: NULL;
	source.path = path_dup(dest_path);
	push_back_vector(s_assets, &source);
}

static int
do_install(const char* pattern, const path_t* src_path, const path_t* dest_path, bool is_recursive)
{
	tinydir_dir  dir;
	tinydir_file file;
	path_t*      in_path;
	path_t*      out_path;
	bool         skip_entry;
	int          num_files = 0;

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
			add_asset(in_path, out_path);
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
mkdir_r(const char* path)
{
	char  parent[1024] = "";
	char* parse;
	char* token;

	parse = strdup(path);
	token = strtok(parse, "/");
	do {
		strcat(parent, token);
		strcat(parent, "/");
		if (tinydir_mkdir(parent) != 0)
			goto on_error;
	} while (token = strtok(NULL, "/"));
	free(parse);
	return true;

on_error:
	free(parse);
	return false;
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
	duk_push_c_function(g_duk, js_game, DUK_VARARGS);
	duk_put_global_string(g_duk, "game");
	duk_push_c_function(g_duk, js_install, DUK_VARARGS);
	duk_put_global_string(g_duk, "install");
}

static duk_ret_t
js_game(duk_context* ctx)
{
	const char* name;
	const char* author;
	const char* description;
	char*       parse;
	const char* resolution;
	int         res_x, res_y;
	const char* script;

	duk_require_object_coercible(ctx, 0);
	name = (duk_get_prop_string(ctx, 0, "name"), duk_require_string(ctx, -1));
	author = (duk_get_prop_string(ctx, 0, "author"), duk_require_string(ctx, -1));
	description = (duk_get_prop_string(ctx, 0, "description"), duk_require_string(ctx, -1));
	resolution = (duk_get_prop_string(ctx, 0, "resolution"), duk_require_string(ctx, -1));
	script = (duk_get_prop_string(ctx, 0, "script"), duk_require_string(ctx, -1));

	// parse screen resolution
	parse = strdup(resolution);
	res_x = atoi(strtok(parse, "x"));
	res_y = atoi(strtok(NULL, "x"));

	free(s_game_info.name);
	free(s_game_info.author);
	free(s_game_info.description);
	free(s_game_info.script_path);
	s_game_info.name = strdup(name);
	s_game_info.author = strdup(author);
	s_game_info.description = strdup(description);
	s_game_info.width = res_x;
	s_game_info.height = res_y;
	s_game_info.script_path = strdup(script);
	return 0;
}

static duk_ret_t
js_install(duk_context* ctx)
{
	// install(pattern, path, isRecursive);
	// Copies file(s) from source into target
	// Arguments:
	//     pattern:     A filename pattern, e.g. "*.js". Files matching the pattern
	//                  will be copied to the destination directory.
	//     path:        Path to the destination, relative to the output directory.
	//     isRecursive: Optional. true to recursively copy files in subdirectories.
	//                  (default: true)

	path_t*     dest_path;
	const char* dest_pathname;
	bool        is_recursive;
	char*       last_slash;
	int         n_args;
	int         num_copied;
	const char* pattern;
	char*       src_filter;
	path_t*     src_path;
	char*       src_pathname;

	n_args = duk_get_top(ctx);
	src_filter = strdup(duk_require_string(ctx, 0));
	dest_pathname = n_args >= 2 ? duk_require_string(ctx, 1) : ".";
	is_recursive = n_args >= 3 ? duk_is_valid_index(ctx, 2) : true;

	if (last_slash = strrchr(src_filter, '/'))
		*last_slash = '\0';
	src_pathname = last_slash != NULL ? src_filter : "./";
	pattern = last_slash != NULL ? last_slash + 1 : src_filter;
	src_path = path_rebase(path_new_dir(src_pathname), g_in_path);
	dest_path = path_rebase(path_new_dir(dest_pathname), g_out_path);
	num_copied = do_install(pattern, src_path, dest_path, is_recursive);
	if (num_copied > 0)
		print_verbose("Staging %d files for install in '%s'\n",
			num_copied,
			path_cstr(dest_path));
	path_free(src_path);
	path_free(dest_path);
	free(src_filter);
	return 0;
}
