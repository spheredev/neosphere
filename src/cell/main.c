/**
 *  Cell, the Sphere packaging compiler
 *  Copyright (c) 2015-2017, Fat Cerberus
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

#include "build.h"
#include "jsal.h"

static bool parse_cmdline    (int argc, char* argv[]);
static void print_cell_quote (void);
static void print_banner     (bool want_copyright, bool want_deps);
static void print_usage      (void);

static path_t* s_in_path;
static path_t* s_out_path;
static path_t* s_package_path;
static bool    s_want_clean;
static bool    s_want_rebuild;
static bool    s_want_source_map;

int
main(int argc, char* argv[])
{
	build_t* build = NULL;
	int      retval = EXIT_FAILURE;

	srand((unsigned int)time(NULL));
	js_init();

	// parse the command line
	if (!parse_cmdline(argc, argv))
		goto shutdown;

	print_banner(true, false);
	printf("\n");

	build = build_new(s_in_path, s_out_path);
	if (!build_eval(build, "Cellscript.mjs") && !build_eval(build, "Cellscript.js"))
		goto shutdown;
	if (s_want_clean)
		build_clean(build);
	else {
		if (!build_run(build, s_want_source_map, s_want_rebuild))
			goto shutdown;
		if (s_package_path != NULL)
			build_package(build, path_cstr(s_package_path));
	}
	retval = EXIT_SUCCESS;

shutdown:
	build_free(build);
	path_free(s_in_path);
	path_free(s_out_path);
	js_uninit();
	return retval;
}

static bool
parse_cmdline(int argc, char* argv[])
{
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
	s_want_clean = false;
	s_want_rebuild = false;
	s_want_source_map = false;

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
				if (++i >= argc) goto missing_argument;
				path_free(s_package_path);
				s_package_path = path_new(argv[i]);
				if (path_filename(s_package_path) == NULL) {
					printf("cell: `%s` argument cannot be a directory\n", argv[i - 1]);
					return false;
				}
				have_in_dir = true;
			}
			else if (strcmp(argv[i], "--out-dir") == 0) {
				if (++i >= argc) goto missing_argument;
				path_free(s_out_path);
				s_out_path = path_new_dir(argv[i]);
			}
			else if (strcmp(argv[i], "--debug") == 0) {
				s_want_source_map = true;
				have_in_dir = true;
			}
			else {
				printf("cell: unknown option `%s`\n", argv[i]);
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
						printf("cell: `%s` argument cannot be a directory\n", short_args);
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
					s_want_source_map = true;
					have_in_dir = true;
					break;
				default:
					printf("cell: unknown option `-%c`\n", short_args[i_arg]);
					return false;
				}
			}
		}
		else {
			printf("cell: unexpected argument `%s`\n", argv[i]);
			return false;
		}
	}

	// validate command line
	if (s_out_path == NULL)
		s_out_path = path_new("dist/");

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
	printf("cell: `%s` requires an argument\n", argv[i - 1]);
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
	printf("%s %s Sphere packaging compiler (%s)\n", COMPILER_NAME, VERSION_NAME,
		sizeof(void*) == 8 ? "x64" : "x86");
	if (want_copyright) {
		printf("the JavaScript-powered build engine for Sphere\n");
		printf("(c) 2015-2017 Fat Cerberus\n");
	}
	if (want_deps) {
		printf("\n");
		printf("    Duktape: v%ld.%ld.%ld    zlib: v%s\n",
			DUK_VERSION / 10000, DUK_VERSION / 100 % 100, DUK_VERSION % 100,
			zlibVersion());
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
	printf("   -i, --in-dir    Set the input directory.  If not provided, Cell will look \n");
	printf("                   for sources in the current working directory.             \n");
	printf("   -o, --out-dir   Set the output directory.  If not provided, the output    \n");
	printf("                   directory defaults to './dist'.                           \n");
	printf("   -p, --package   Build a Sphere game package (.spk).                       \n");
	printf("   -r, --rebuild   Rebuild all targets, even when already up to date.        \n");
	printf("   -c, --clean     Clean up all artifacts from the previous build.           \n");
	printf("   -d, --debug     Include a debugging map in the compiled game which maps   \n");
	printf("                   compiled targets to their corresponding source files.     \n");
	printf("       --version   Print the version number of Cell and its dependencies.    \n");
	printf("       --help      Print this help text.                                     \n");
}
