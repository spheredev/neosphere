#include "cell.h"

#include "fs.h"

#include "tinydir.h"

static duk_ret_t js_game    (duk_context* ctx);
static duk_ret_t js_install (duk_context* ctx);

static bool wildcmp (const char* filename, const char* pattern);

int
fs_install(const char* pattern, const char* src_path, const char* dest_path, bool recursive)
{
	tinydir_dir  dir;
	tinydir_file file;
	bool         skip_entry;
	int          num_files = 0;
	char         path[1024];

	tinydir_open(&dir, src_path);
	while (dir.has_next) {
		tinydir_readfile(&dir, &file);
		tinydir_next(&dir);

		sprintf(path, "%s/%s", src_path, g_out_path);
		skip_entry = file.is_dir && !recursive
			|| strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0
			|| strstr(file.path, path) == file.path;

		if (skip_entry)
			continue;
		else if (!file.is_dir && wildcmp(file.name, pattern)) {
			if (dest_path != NULL)
				sprintf(path, "%s/%s/", g_out_path, dest_path);
			else
				sprintf(path, "%s/", g_out_path);
			fs_mkdir(path);
			strcat(path, file.name);
			if (tinydir_copy(file.path, path, true) == 0) {
				print_verbose("Copied file '%s' to '%s'\n", file.path, path);
				++num_files;
			}
		}
		else if (file.is_dir) {
			if (dest_path != NULL)
				sprintf(path, "%s/%s", dest_path, file.name);
			else
				sprintf(path, "%s", file.name);
			num_files += fs_install(pattern, file.path, path, recursive);
		}
	}
	tinydir_close(&dir);
	return num_files;
}

bool
fs_mkdir(const char* path)
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
wildcmp(const char *string, const char *pattern)
{
	const char* cp = NULL;
	const char* mp = NULL;
	bool        is_match = 0;
	char*       parse;
	char*       token;

	if (strchr(pattern, '|')) {
		parse = strdup(pattern);
		token = strtok(parse, "|");
		do {
			if (is_match |= wildcmp(string, token))
				break;
		} while (token = strtok(NULL, "|"));
		free(parse);
		return is_match;
	}
	else {
		while (*string != '\0' && *pattern != '*') {
			if (*pattern != *string && *pattern != '?')
				return false;
			++pattern;
			++string;
		}
		while (*string != '\0') {
			if (*pattern == '*') {
				if (*++pattern == '\0') return true;
				mp = pattern;
				cp = string + 1;
			}
			else if (*pattern == *string || *pattern == '?') {
				pattern++;
				string++;
			}
			else {
				pattern = mp;
				string = cp++;
			}
		}
		while (*pattern == '*')
			pattern++;
		return *pattern == '\0';
	}
}

void
init_fs_api(void)
{
	duk_push_c_function(g_duk, js_game, DUK_VARARGS);
	duk_put_global_string(g_duk, "game");
	duk_push_c_function(g_duk, js_install, DUK_VARARGS);
	duk_put_global_string(g_duk, "install");
}

static duk_ret_t
js_game(duk_context* ctx)
{
	FILE*       file;
	const char* name;
	const char* author;
	const char* description;
	char*       parse;
	const char* resolution;
	int         res_x, res_y;
	const char* script;
	char        sgm_path[1024];

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

	// write game.sgm
	fs_mkdir(g_out_path);
	sprintf(sgm_path, "%s/game.sgm", g_out_path);
	if (!(file = fopen(sgm_path, "wb")))
		duk_error(ctx, DUK_ERR_ERROR, "Failed to write game.sgm");
	fprintf(file, "name=%s\n", name);
	fprintf(file, "author=%s\n", author);
	fprintf(file, "description=%s\n", description);
	fprintf(file, "screen_width=%d\n", res_x);
	fprintf(file, "screen_height=%d\n", res_y);
	fprintf(file, "script=%s\n", script);
	fclose(file);
	printf("Wrote game.sgm '%s'\n", name);
	return 0;
}

static duk_ret_t
js_install(duk_context* ctx)
{
	// install(pattern, path, isRecursive);
	// Copies file(s) from source into target
	// Arguments:
	//     pattern:   A filename pattern, e.g. "*.js". Files matching the pattern
	//                will be copied to the destination directory.
	//     path:      Path to the destination, relative to the output directory.
	//     recursive: Optional. true to recursively copy files in subdirectories.
	//                (default: true)

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
	num_copied = fs_install(pattern, src_path, dest_path, is_recursive);
	if (num_copied > 0)
		printf("Installed %d files into '%s'\n", num_copied,
			dest_path != NULL ? dest_path : g_out_path);
	return 0;
}
