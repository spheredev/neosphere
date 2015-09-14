#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "duktape.h"
#include "tinydir.h"

#define CELL_VERSION "2.0.0"

static duk_ret_t js_game    (duk_context* ctx);
static duk_ret_t js_install (duk_context* ctx);

static bool wildcmp (const char* filename, const char* pattern);

static duk_context* s_duk = NULL;
static bool         s_want_spk = false;

int
main(int argc, char* argv[])
{
	// yes, these are entirely necessary. :o)
	static const char* const MESSAGES[] =
	{
		"Cell seems to be going through some sort of transformation...!",
		"He's pumping himself up like a balloon!",
		"This is the end for you!",
		"Very soon, I am going to explode. And when I do...",
		"Careful now! I wouldn't attack me if I were you...",
		"I'm quite volatile, and the slightest jolt could set me off!",
		"One minute left! There's nothing you can do now!",
		"If only you'd finished me off a little bit sooner...",
		"Ten more seconds, and the Earth will be gone!",
		"Let's just call our little match a draw, shall we?",
	};
	
	const char* js_error_msg;
	int         num_messages;
	int         retval = EXIT_SUCCESS;
	const char* target_name;

	srand((unsigned int)time(NULL));
	num_messages = sizeof MESSAGES / sizeof(const char*);
	
	printf("Cell %s  (c) 2015 Fat Cerberus\n", CELL_VERSION);
	printf("%s\n", MESSAGES[rand() % num_messages]);
	
	// initialize JavaScript environment
	s_duk = duk_create_heap_default();
	duk_push_c_function(s_duk, js_game, DUK_VARARGS);
	duk_push_c_function(s_duk, js_install, DUK_VARARGS);
	duk_put_global_string(s_duk, "install");
	duk_put_global_string(s_duk, "game");

	// evaluate the build script
	if (duk_pcompile_file(s_duk, 0x0, "cell.js") != DUK_EXEC_SUCCESS
		|| duk_pcall(s_duk, 0) != DUK_EXEC_SUCCESS)
	{
		js_error_msg = duk_safe_to_string(s_duk, -1);
		if (strstr(js_error_msg, "no sourcecode"))
			fprintf(stderr, "ERROR: cell.js was not found.\n");
		else
			fprintf(stderr, "ERROR: JS error `%s`", js_error_msg);
		
		retval = EXIT_FAILURE;
		goto shutdown;
	}

	target_name = argc > 1 ? argv[1] : "make";
	if (duk_get_global_string(s_duk, target_name) && duk_is_callable(s_duk, -1)) {
		printf("Processing Cell target '%s'\n", target_name);
		if (duk_pcall(s_duk, 0) != DUK_EXEC_SUCCESS) {
			fprintf(stderr, "ERROR: JS error `%s`\n", duk_safe_to_string(s_duk, -1));
			retval = EXIT_FAILURE;
			goto shutdown;
		}
		printf("Success!\n");
	}
	else {
		fprintf(stderr, "ERROR: No target named '%s'\n", target_name);
		retval = EXIT_FAILURE;
		goto shutdown;
	}

shutdown:
	duk_destroy_heap(s_duk);
	return retval;
}

int
install(const char* pattern, const char* src_path, const char* dest_path, bool recursive)
{
	tinydir_dir  dir;
	tinydir_file file;
	int          num_files = 0;
	char         path[1024];

	tinydir_open(&dir, src_path);
	while (dir.has_next) {
		tinydir_readfile(&dir, &file);
		if (!file.is_dir && wildcmp(file.name, pattern)) {
			sprintf(path, "./dist/%s/", dest_path);
			CreateDirectoryA(path, NULL);
			strcat(path, file.name);
			if (CopyFileA(file.path, path, TRUE))
				++num_files;
		}
		else if (file.is_dir && recursive
			&& strcmp(file.name, ".") != 0 && strcmp(file.name, "..") != 0
			&& strcmp(file.path, "./dist") != 0)
		{
			sprintf(path, "%s/%s", dest_path, file.name);
			install(pattern, file.path, path, recursive);
		}
		tinydir_next(&dir);
	}
	tinydir_close(&dir);
	if (num_files > 0)
		printf("Installed %d file(s) to ./dist/%s\n", num_files, dest_path);
	return num_files;
}

static bool
wildcmp(const char *string, const char *pattern)
{
	const char* cp = NULL;
	const char* mp = NULL;

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
	char*        pattern;
	const char*  src_path;

	n_args = duk_get_top(ctx);
	pattern = strdup(duk_require_string(ctx, 0));
	dest_path = n_args >= 2 ? duk_require_string(ctx, 1) : ".";
	is_recursive = n_args >= 3 ? duk_is_valid_index(ctx, 2) : true;
	
	if (last_slash = strrchr(pattern, '/'))
		*last_slash = '\0';
	src_path = last_slash != NULL ? pattern : ".";
	pattern = last_slash != NULL ? last_slash + 1 : pattern;
	install(pattern, src_path, dest_path, is_recursive);
	return 0;
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
	if (!(file = fopen("dist/game.sgm", "wb")))
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
