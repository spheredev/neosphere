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
#include "jsal.h"

enum mode
{
	MODE_BUILD,
	MODE_CLEAN,
	MODE_PACK,
};

static bool parse_command_line (int argc, char* argv[]);
static void print_banner       (bool want_copyright, bool want_deps);
static void print_cell_quote   (void);
static void print_usage        (void);

static bool      s_debug_build;
static path_t*   s_in_path;
static enum mode s_mode;
static path_t*   s_out_path;
static path_t*   s_package_path;
static bool      s_want_rebuild;

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
	switch (s_mode) {
	case MODE_BUILD:
	case MODE_PACK:
		if (!build_run(build, s_debug_build, s_want_rebuild))
			goto shutdown;
		if (s_mode == MODE_PACK)
			build_package(build, path_cstr(s_package_path));
		break;
	case MODE_CLEAN:
		build_clean(build);
		break;
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
parse_command_line(int argc, char* argv[])
{
	int         args_index = 1;
	const char* command;
	bool        have_debug_flag = false;
	bool        have_in_dir = false;
	path_t*     js_path;
	path_t*     mjs_path;
	int         num_targets = 0;
	const char* short_args;

	int    i;
	size_t i_arg;

	// establish default options
	s_mode = MODE_BUILD;
	s_in_path = path_new("./");
	s_out_path = NULL;
	s_package_path = NULL;
	s_want_rebuild = false;
	s_debug_build = false;

	if (argc >= 2 && argv[1][0] != '-') {
		args_index = 2;
		command = argv[1];
		if (strcmp(command, "build") == 0 || strcmp(command, "b") == 0) {
			s_mode = MODE_BUILD;
			have_in_dir = true;
		}
		else if (strcmp(command, "clean") == 0 || strcmp(command, "c") == 0) {
			s_mode = MODE_CLEAN;
		}
		else if (strcmp(command, "pack") == 0 || strcmp(command, "p") == 0) {
			s_mode = MODE_PACK;
		}
		else if (strcmp(command, "version") == 0 || strcmp(command, "v") == 0) {
			print_banner(true, true);
			return false;
		}
		else if (strcmp(command, "explode") == 0) {
			print_cell_quote();
			return false;
		}
		else if (strcmp(command, "help") == 0 || strcmp(command, "h") == 0) {
			print_usage();
			return false;
		}
		else {
			printf("cell: '%s' is not a valid Cell command\n", command);
			return false;
		}
	}
	
	// validate and parse the command line
	for (i = args_index; i < argc; ++i) {
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
				if (++i >= argc)
					goto missing_argument;
				path_free(s_in_path);
				s_in_path = path_new_dir(argv[i]);
				have_in_dir = true;
			}
			else if (strcmp(argv[i], "--out-dir") == 0) {
				if (++i >= argc)
					goto missing_argument;
				path_free(s_out_path);
				s_out_path = path_new_dir(argv[i]);
			}
			else if (strcmp(argv[i], "--rebuild") == 0) {
				s_want_rebuild = true;
				have_in_dir = true;
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
					if (++i >= argc)
						goto missing_argument;
					path_free(s_in_path);
					s_in_path = path_new_dir(argv[i]);
					have_in_dir = true;
					break;
				case 'o':
					if (++i >= argc)
						goto missing_argument;
					path_free(s_out_path);
					s_out_path = path_new_dir(argv[i]);
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
		else if (s_mode == MODE_PACK && s_package_path == NULL) {
			s_package_path = path_new(argv[i]);
			if (path_filename(s_package_path) == NULL) {
				printf("cell: 'cell pack' target '%s' cannot be a directory\n", argv[i - 1]);
				return false;
			}
			have_in_dir = true;
		} else {
			printf("cell: unexpected argument '%s'\n", argv[i]);
			return false;
		}
	}

	// validate command line
	if (s_out_path == NULL)
		s_out_path = path_new("dist/");
	if (!have_debug_flag)
		s_debug_build = s_mode == MODE_PACK;
	if (s_mode == MODE_PACK && s_package_path == NULL) {
		printf("cell: no SPK filename was provided for 'cell %s'\n", command);
		return false;
	}

	// check if a Cellscript exists
	mjs_path = path_rebase(path_new("Cellscript.mjs"), s_in_path);
	js_path = path_rebase(path_new("Cellscript.js"), s_in_path);
	if (!path_resolve(mjs_path, NULL) && !path_resolve(js_path, NULL)) {
		path_free(mjs_path);
		path_free(js_path);
		if (have_in_dir)
			printf("cell: no Cellscript found in source directory '%s'.\n", path_cstr(s_in_path));
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
	printf("   cell build [options]\n");
	printf("   cell clean [options]\n");
	printf("   cell pack [options] <packfile-name>\n");
	printf("   cell help\n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("   -i  --in-dir    Set the input directory (default is current working dir)  \n");
	printf("   -o  --out-dir   Set the output directory (default is './dist')            \n");
	printf("\n");
	printf("   for build/pack:\n");
	printf("   -r  --rebuild   Rebuild all targets, even those already up to date        \n");
	printf("   -d  --debug     Include debugging information for use with SSj or SSj Blue\n");
	printf("       --release   Build for distribution, without any debugging information \n");
}
