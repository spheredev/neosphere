/**
 *  Cell, the Sphere packaging compiler
 *  Copyright (c) 2015-2018, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "cell.h"

#include <png.h>
#include <zlib.h>
#include "build.h"
#include "path.h"
#include "jsal.h"
#include "utility.h"

static bool generate_project   (char* path);
static bool parse_command_line (int argc, char* argv[]);
static void print_banner       (bool want_copyright, bool want_deps);
static void print_cell_quote   (void);
static void print_usage        (void);

static bool    s_debug_build;
static path_t* s_in_path;
static path_t* s_out_path;
static path_t* s_package_path;
static bool    s_want_clean;
static bool    s_want_rebuild;

int
main(int argc, char* argv[])
{
	build_t* build = NULL;
	int      retval = EXIT_FAILURE;

	srand((unsigned int)time(NULL));
	jsal_init();

	// parse the command line
	if (!parse_command_line(argc, argv))
		goto shutdown;

	print_banner(true, false);
	printf("\n");

	build = build_new(s_in_path, s_out_path);
	if (!build_eval(build, "$/Cellscript.mjs") && !build_eval(build, "$/Cellscript.js"))
		goto shutdown;
	if (s_want_clean)
		build_clean(build);
	else {
		if (!build_run(build, s_debug_build, s_want_rebuild))
			goto shutdown;
		if (s_package_path != NULL)
			build_package(build, path_cstr(s_package_path));
	}
	retval = EXIT_SUCCESS;

shutdown:
	build_free(build);
	path_free(s_in_path);
	path_free(s_out_path);
	jsal_uninit();
	return retval;
}

static bool
generate_project(char* path)
{
	char* project_name;
	char* author;
	char* width;
	char* height;
	char* summary;
	path_t* app_path;
	path_t* project_path;
	path_t* template_path;
	path_t* tmpl_path;
	path_t* mjs_path;
	FILE* mjs_file = NULL;
	char* tmpl_str;
	char* mjs_str;
	bool success = false;

	printf("Enter game name: ");
	project_name = malloc(256);
	fgets(project_name, 256, stdin);
	project_name[strcspn(project_name, "\r\n")] = 0;

	printf("Enter author: ");
	author = malloc(256);
	fgets(author, 256, stdin);
	author[strcspn(author, "\r\n")] = 0;

	printf("Enter screen width: ");
	width = malloc(256);
	fgets(width, 256, stdin);
	width[strcspn(width, "\r\n")] = 0;

	printf("Enter screen height: ");
	height = malloc(256);
	fgets(height, 256, stdin);
	height[strcspn(height, "\r\n")] = 0;

	printf("Enter game summary: ");
	summary = malloc(256);
	fgets(summary, 256, stdin);
	summary[strcspn(summary, "\r\n")] = 0;

	project_path = path_new_dir(path);
	path_mkdir(project_path);

	app_path = path_new_self();
	template_path = path_rebase(path_new("template/"), app_path);
	if (!path_resolve(template_path, NULL)) {
		path_free(template_path);
		template_path = path_rebase(path_new("../share/minisphere/template"), app_path);
		path_resolve(template_path, NULL);
	}

	if (!dircopy(path_cstr(template_path), path)) {
		printf("Failed copying directory\n");
		goto cleanup;
	}

	tmpl_path = path_rebase(path_new("./Cellscript.mjs.tmpl"), project_path);
	path_resolve(tmpl_path, NULL);
	mjs_path = path_rebase(path_new("./Cellscript.mjs"), project_path);
	path_resolve(mjs_path, NULL);

	mjs_file = fopen(path_cstr(mjs_path), "w");
	if (mjs_file == NULL)
		goto cleanup;

	size_t size;
	tmpl_str = (char*)fslurp(path_cstr(tmpl_path), &size);

	size_t openbrk = strstr(tmpl_str, "{{") - tmpl_str;
	size_t pos0 = strstr(tmpl_str, "{0}") - tmpl_str;
	size_t pos1 = strstr(tmpl_str, "{1}") - tmpl_str;
	size_t pos2 = strstr(tmpl_str, "{2}") - tmpl_str;
	size_t pos3 = strstr(tmpl_str, "{3}") - tmpl_str;
	size_t pos4 = strstr(tmpl_str, "{4}") - tmpl_str;
	size_t closebrk = strstr(tmpl_str, "}}") - tmpl_str;

	fwrite(tmpl_str, openbrk, 1, mjs_file);
	fwrite(tmpl_str + openbrk + 1, pos0 - openbrk - 1, 1, mjs_file);
	
	fwrite(project_name, strlen(project_name), 1, mjs_file);
	fwrite(tmpl_str + pos0 + 3, pos1 - pos0 - 3, 1, mjs_file);

	fwrite(author, strlen(author), 1, mjs_file);
	fwrite(tmpl_str + pos1 + 3, pos2 - pos1 - 3, 1, mjs_file);

	fwrite(summary, strlen(summary), 1, mjs_file);
	fwrite(tmpl_str + pos2 + 3, pos3 - pos2 - 3, 1, mjs_file);

	fwrite(width, strlen(width), 1, mjs_file);
	fwrite(tmpl_str + pos3 + 3, pos4 - pos3 - 3, 1, mjs_file);

	fwrite(height, strlen(height), 1, mjs_file);
	fwrite(tmpl_str + pos4 + 3, closebrk - pos4 - 3, 1, mjs_file);

	fwrite(tmpl_str + closebrk + 1, size - closebrk - 1, 1, mjs_file);

	fclose(mjs_file);
	success = (remove(path_cstr(tmpl_path)) == 0);

cleanup: 
	free(project_name);
	free(author);
	free(width);
	free(height);
	free(summary);
	free(mjs_str);
	free(tmpl_str);
	path_free(app_path);
	path_free(project_path);
	path_free(template_path);
	path_free(tmpl_path);
	path_free(mjs_path);
	return success;
}

static bool
parse_command_line(int argc, char* argv[])
{
	bool        have_debug_flag = false;
	bool        have_in_dir = false;
	path_t*     js_path;
	path_t*     mjs_path;
	int         num_targets = 0;
	const char* short_args;

	int    i;
	size_t i_arg;

	// establish default options
	s_in_path = path_new("./");
	s_out_path = NULL;
	s_package_path = NULL;
	s_want_clean = false;
	s_want_rebuild = false;
	s_debug_build = false;

	// validate and parse the command line
	for (i = 1; i < argc; ++i) {
		if (strstr(argv[i], "--") == argv[i]) {
			if (strcmp(argv[i], "--help") == 0) {
				print_usage();
				return false;
			}
			else if (strcmp(argv[i], "--version") == 0) {
				print_banner(true, true);
				return false;
			}
			else if (strcmp(argv[i], "--init") == 0) {
				printf("Generating new project\n");
				if (++i >= argc) goto missing_argument;
				if (fexist(argv[i])) {
					printf("cell: directory '%s' already exists\n", argv[i]);
					return false;
				}
				if (!generate_project(argv[i])) {
					printf("Error generating new project\n");
				}
				return false;
			}
			else if (strcmp(argv[i], "--in-dir") == 0) {
				if (++i >= argc) goto missing_argument;
				path_free(s_in_path);
				s_in_path = path_new_dir(argv[i]);
				have_in_dir = true;
			}
			else if (strcmp(argv[i], "--explode") == 0) {
				print_cell_quote();
				return false;
			}
			else if (strcmp(argv[i], "--clean") == 0) {
				s_want_clean = true;
				have_in_dir = true;
			}
			else if (strcmp(argv[i], "--rebuild") == 0) {
				s_want_rebuild = true;
				have_in_dir = true;
			}
			else if (strcmp(argv[i], "--package") == 0) {
				if (++i >= argc)
					goto missing_argument;
				path_free(s_package_path);
				s_package_path = path_new(argv[i]);
				if (path_filename(s_package_path) == NULL) {
					printf("cell: '%s' argument cannot be a directory\n", argv[i - 1]);
					return false;
				}
				have_in_dir = true;
			}
			else if (strcmp(argv[i], "--out-dir") == 0) {
				if (++i >= argc)
					goto missing_argument;
				path_free(s_out_path);
				s_out_path = path_new_dir(argv[i]);
			}
			else if (strcmp(argv[i], "--debug") == 0) {
				if (have_debug_flag && !s_debug_build) {
					printf("cell: illegal command line, both '--debug' and '--release' specified\n");
					return false;
				}
				s_debug_build = true;
				have_debug_flag = true;
				have_in_dir = true;
			}
			else if (strcmp(argv[i], "--release") == 0) {
				if (have_debug_flag && s_debug_build) {
					printf("cell: illegal command line, both '--debug' and '--release' specified\n");
					return false;
				}
				s_debug_build = false;
				have_debug_flag = true;
				have_in_dir = true;
			}
			else {
				printf("cell: unknown option '%s'\n", argv[i]);
				return false;
			}
		}
		else if (argv[i][0] == '-') {
			short_args = argv[i];
			for (i_arg = strlen(short_args) - 1; i_arg >= 1; --i_arg) {
				switch (short_args[i_arg]) {
				case 'i':
					if (++i >= argc) goto missing_argument;
					path_free(s_in_path);
					s_in_path = path_new_dir(argv[i]);
					have_in_dir = true;
					break;
				case 'o':
					if (++i >= argc) goto missing_argument;
					path_free(s_out_path);
					s_out_path = path_new_dir(argv[i]);
					break;
				case 'p':
					if (++i >= argc) goto missing_argument;
					path_free(s_package_path);
					s_package_path = path_new(argv[i]);
					if (path_filename(s_package_path) == NULL) {
						printf("cell: '%s' argument cannot be a directory\n", short_args);
						return false;
					}
					have_in_dir = true;
					break;
				case 'c':
					s_want_clean = true;
					have_in_dir = true;
					break;
				case 'r':
					s_want_rebuild = true;
					have_in_dir = true;
					break;
				case 'd':
					if (have_debug_flag && !s_debug_build) {
						printf("cell: illegal command line, both '--debug' and '--release' specified\n");
						return false;
					}
					s_debug_build = true;
					have_debug_flag = true;
					have_in_dir = true;
					break;
				case 'v':
					print_banner(true, true);
					return false;
				default:
					printf("cell: unknown option '-%c'\n", short_args[i_arg]);
					return false;
				}
			}
		}
		else {
			printf("cell: unexpected argument '%s'\n", argv[i]);
			return false;
		}
	}

	// validate command line
	if (s_out_path == NULL)
		s_out_path = path_new("dist/");
	if (!have_debug_flag)
		s_debug_build = s_package_path == NULL;

	// check if a Cellscript exists
	mjs_path = path_rebase(path_new("Cellscript.mjs"), s_in_path);
	js_path = path_rebase(path_new("Cellscript.js"), s_in_path);
	if (!path_resolve(mjs_path, NULL) && !path_resolve(js_path, NULL)) {
		path_free(mjs_path);
		path_free(js_path);
		if (have_in_dir)
			printf("no Cellscript found in source directory.\n");
		else
			print_usage();
		return false;
	}

	path_free(mjs_path);
	path_free(js_path);
	return true;

missing_argument:
	printf("cell: '%s' requires an argument\n", argv[i - 1]);
	return false;
}

static void
print_cell_quote(void)
{
	// yes, these are entirely necessary. :o)
	static const char* const MESSAGES[] =
	{
		"This is the end for you!",
		"Very soon, I am going to explode. And when I do...",
		"Careful now! I wouldn't attack me if I were you...",
		"I'm quite volatile, and the slightest jolt could set me off!",
		"One minute left! There's nothing you can do now!",
		"If only you'd finished me off a little bit sooner...",
		"Ten more seconds, and the Earth will be gone!",
		"Let's just call our little match a draw, shall we?",
		"Watch out! You might make me explode!",
	};

	printf("Cell seems to be going through some sort of transformation...\n");
	printf("He's pumping himself up like a balloon!\n\n");
	printf("    Cell says:\n    \"%s\"\n", MESSAGES[rand() % (sizeof MESSAGES / sizeof(const char*))]);
}

static void
print_banner(bool want_copyright, bool want_deps)
{
	png_uint_32 png_version;
	char*       png_version_string;
	
	printf("%s %s Sphere packaging compiler (%s)\n", SPHERE_COMPILER_NAME, SPHERE_VERSION,
		sizeof(void*) == 8 ? "x64" : "x86");
	if (want_copyright) {
		printf("the JavaScript-powered build engine for Sphere\n");
		printf("(c) 2015-2018 Fat Cerberus\n");
	}
	if (want_deps) {
		png_version = png_access_version_number();
		png_version_string = strnewf("%d.%d.%d",
			png_version / 10000,
			png_version / 100 % 100,
			png_version % 100);
		printf("\n");
		printf("    libpng: v%-8s   zlib: v%s\n", png_version_string, zlibVersion());
	}
}

static void
print_usage(void)
{
	print_banner(true, false);
	printf("\n");
	printf("USAGE:\n");
	printf("   cell [-i <in-dir>] [-o <out-dir>] [-p <out-pkgfile>] [options]\n");
	printf("   cell -c [-i <in-dir>] [-o <out-dir>]\n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("   -i  --in-dir    Set the input directory (default is current working dir)  \n");
	printf("   -o  --out-dir   Set the output directory (default is './dist')            \n");
	printf("   -p  --package   Create an SPK game package with the result of the build   \n");
	printf("   -r  --rebuild   Rebuild all targets, even those already up to date        \n");
	printf("   -c  --clean     Clean up all artifacts from the previous build            \n");
	printf("   -d  --debug     Include debugging information for use with SSj or SSj Blue\n");
	printf("       --release   Build for distribution, without any debugging information \n");
	printf("       --init      Create a new project in the given path using a template   \n");
	printf("   -v  --version   Print the version number of Cell and its dependencies.    \n");
	printf("       --help      Print this help text.                                     \n");
}
