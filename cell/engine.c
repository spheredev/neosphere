#include "cell.h"

#include "engine.h"

#include "tinydir.h"

static duk_ret_t js_game    (duk_context* ctx);
static duk_ret_t js_install (duk_context* ctx);

static void add_source      (const lstring_t* source_path, const lstring_t* dest_path);
static bool fspew           (const void* buffer, size_t size, const char* filename);
static int  handle_install  (const char* pattern, const char* src_path, const char* dest_path, bool recursive);
static bool mkdir_recursive (const char* path);
static bool wildcmp         (const char* filename, const char* pattern);

struct game
{
	char* name;
	char* author;
	char* description;
	int   width;
	int   height;
	char* script_path;
};

struct source
{
	lstring_t* source_path;
	lstring_t* dest_path;
};

static struct game s_game_info;
static vector_t*   s_files;

void
initialize_engine(void)
{
	memset(&s_game_info, 0, sizeof(struct game));
	s_files = new_vector(sizeof(struct source));
}

void
shutdown_engine(void)
{
	struct source* source;
	
	iter_t iter;

	iter = iterate_vector(s_files);
	while (source = next_vector_item(&iter)) {
		free(source->source_path);
		free(source->dest_path);
	}
	free_vector(s_files);
}

bool
run_build()
{
	FILE*          file;
	char           filename[CELL_PATH_MAX];
	const char*    json;
	int            num_files = 0;
	struct source* source;
	
	iter_t iter;
	
	if (s_game_info.name == NULL) {
		printf("FATAL: missing game() directive\n");
		return false;
	}
	
	printf("Compiling Sphere game\n");
	printf("    Name: %s\n", s_game_info.name);
	printf("    Author: %s\n", s_game_info.author);
	printf("    Resolution: %dx%d\n", s_game_info.width, s_game_info.height);
	
	// copy staged files
	printf("Installing files... ");
	if (get_vector_size(s_files) == 0)
		printf("warning: no files installed\n");
	if (g_want_source_map)
		duk_push_object(g_duk);
	iter = iterate_vector(s_files);
	while (source = next_vector_item(&iter)) {
		if (tinydir_copy(lstr_cstr(source->source_path), lstr_cstr(source->dest_path), true) == 0) {
			++num_files;
			print_verbose("Copied '%s' to '%s'\n", source->source_path, source->dest_path);
		}
		if (g_want_source_map) {
			duk_push_string(g_duk, lstr_cstr(source->source_path));
			duk_put_prop_string(g_duk, -2, lstr_cstr(source->dest_path));
		}
	}
	printf("%d copied\n", num_files);
	if (g_want_source_map) {
		printf("Writing source map... ");
		sprintf(filename, "%s/sourcemap.json", g_out_path);
		duk_get_global_string(g_duk, "JSON");
		duk_get_prop_string(g_duk, -1, "stringify");
		duk_dup(g_duk, -3);
		duk_push_null(g_duk);
		duk_push_string(g_duk, "\t");
		duk_call(g_duk, 3);
		json = duk_get_string(g_duk, -1);
		fspew(json, strlen(json), filename);
		duk_pop_3(g_duk);
		printf("Done!\n");
	}
	
	// write game.sgm
	printf("Writing game manifest... ");
	mkdir_recursive(g_out_path);
	sprintf(filename, "%s/game.sgm", g_out_path);
	if (!(file = fopen(filename, "wb")))
		fprintf(stderr, "FATAL: failed to write game manifest");
	fprintf(file, "name=%s\n", s_game_info.name);
	fprintf(file, "author=%s\n", s_game_info.author);
	fprintf(file, "description=%s\n", s_game_info.description);
	fprintf(file, "screen_width=%d\n", s_game_info.width);
	fprintf(file, "screen_height=%d\n", s_game_info.height);
	fprintf(file, "script=%s\n", s_game_info.script_path);
	fclose(file);
	printf("Done!\n");
	
	printf("Success!\n");
	return true;
}

static void
add_source(const lstring_t* source_path, const lstring_t* dest_path)
{
	struct source source;

	source.source_path = lstr_dup(source_path);
	source.dest_path = lstr_dup(dest_path);
	push_back_vector(s_files, &source);
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

static int
handle_install(const char* pattern, const char* src_path, const char* dest_path, bool is_recursive)
{
	tinydir_dir  dir;
	tinydir_file file;
	lstring_t*   in_path = NULL;
	lstring_t*   out_path = NULL;
	bool         skip_entry;
	int          num_files = 0;
	char         path[CELL_PATH_MAX];

	tinydir_open(&dir, src_path);
	while (dir.has_next) {
		tinydir_readfile(&dir, &file);
		tinydir_next(&dir);

		sprintf(path, "%s/%s", src_path, g_out_path);
		skip_entry = file.is_dir && !is_recursive
			|| strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0
			|| strstr(file.path, path) == file.path;

		if (skip_entry)
			continue;
		else if (!file.is_dir && wildcmp(file.name, pattern)) {
			in_path = src_path ? lstr_newf("%s/%s", src_path, file.name) : lstr_newf("%s", file.name);
			out_path = dest_path ? lstr_newf("%s/%s", dest_path, file.name) : lstr_newf("%s", file.name);
			add_source(in_path, out_path);
			lstr_free(in_path);
			lstr_free(out_path);
			++num_files;
		}
		else if (file.is_dir) {
			if (dest_path != NULL)
				sprintf(path, "%s/%s", dest_path, file.name);
			else
				sprintf(path, "%s", file.name);
			num_files += handle_install(pattern, file.path, path, is_recursive);
		}
	}
	tinydir_close(&dir);
	return num_files;
}

static bool
mkdir_recursive(const char* path)
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

	const char*  dest_path;
	bool         is_recursive;
	char*        last_slash;
	int          n_args;
	int          num_copied;
	char*        pattern;
	const char*  src_path;

	n_args = duk_get_top(ctx);
	pattern = strdup(duk_require_string(ctx, 0));
	dest_path = n_args >= 2 ? duk_require_string(ctx, 1) : NULL;
	is_recursive = n_args >= 3 ? duk_is_valid_index(ctx, 2) : true;

	if (last_slash = strrchr(pattern, '/'))
		*last_slash = '\0';
	src_path = last_slash != NULL ? pattern : ".";
	pattern = last_slash != NULL ? last_slash + 1 : pattern;
	num_copied = handle_install(pattern, src_path, dest_path, is_recursive);
	if (num_copied > 0)
		print_verbose("Staging %d files for install in '%s'\n",
			num_copied,
			dest_path != NULL ? dest_path : g_out_path);
	return 0;
}
