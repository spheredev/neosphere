#include "cell.h"
#include "engine.h"

static bool parse_cmdline    (int argc, char* argv[]);
static void print_banner     (bool want_copyright, bool want_deps);
static void print_cell_quote (void);

duk_context* g_duk = NULL;
bool         g_is_verbose = false;
path_t*      g_in_path = NULL;
path_t*      g_out_path = NULL;
bool         g_want_dry_run = false;
bool         g_want_package = false;
bool         g_want_source_map = false;

char* s_target_name;

int
main(int argc, char* argv[])
{
	path_t*     cell_js_path;
	int         retval = EXIT_SUCCESS;
	const char* js_error_msg;
	char        js_target_name[256];

	srand((unsigned int)time(NULL));
	if (!parse_cmdline(argc, argv))
		return EXIT_FAILURE;

	// initialize JavaScript environment
	g_duk = duk_create_heap_default();
	initialize_js_api();

	// evaluate the build script
	cell_js_path = path_rebase(path_new("cell.js"), g_in_path);
	if (duk_pcompile_file(g_duk, 0x0, path_cstr(cell_js_path)) != DUK_EXEC_SUCCESS
		|| duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS)
	{
		js_error_msg = duk_safe_to_string(g_duk, -1);
		if (strstr(js_error_msg, "no sourcecode"))
			fprintf(stderr, "cell: error: no cell.js in input directory\n");
		else
			fprintf(stderr, "FATAL: JS %s", js_error_msg);
		
		retval = EXIT_FAILURE;
		goto shutdown;
	}

	sprintf(js_target_name, "_%s", s_target_name);
	if (duk_get_global_string(g_duk, js_target_name) && duk_is_callable(g_duk, -1)) {
		print_banner(true, false);
		printf("\n");
		printf("Processing Cell target '%s'\n", s_target_name);
		initialize_engine();
		if (duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS) {
			fprintf(stderr, "FATAL: JS %s\n", duk_safe_to_string(g_duk, -1));
			retval = EXIT_FAILURE;
			goto shutdown;
		}
		run_build();
	}
	else {
		fprintf(stderr, "cell: error: no target named '%s'\n", s_target_name);
		retval = EXIT_FAILURE;
		goto shutdown;
	}

shutdown:
	path_free(cell_js_path);
	free(s_target_name);
	duk_destroy_heap(g_duk);
	return retval;
}

void
print_verbose(const char* fmt, ...)
{
	va_list ap;

	if (!g_is_verbose)
		return;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

static bool
parse_cmdline(int argc, char* argv[])
{
	int num_targets = 0;
	
	int i, j;
	
	// establish compiler defaults
	g_in_path = path_new("./");
	g_out_path = path_new("dist/");
	s_target_name = strdup("sphere");

	// validate and parse the command line
	for (i = 1; i < argc; ++i) {
		if (strstr(argv[i], "--") == argv[i]) {
			if (strcmp(argv[i], "--help") == 0) {
				print_banner(false, false);
				printf("\n");
				printf("USAGE: cell [--in <path>] [--out <path>] [target]\n");
				printf("       cell [options] [target]\n");
				printf("\nOPTIONS:\n");
				printf("   --in <path>            Sets the input directory, Cell looks for sources in\n");
				printf("                          the current working directory by default\n");
				printf("   --out <path>           Sets the output directory, 'dist' if not provided\n");
				printf("   --version              Prints the Cell compiler version and exits\n");
				printf("   --make-package, -p     Makes a Sphere package (.spk) for the compiled game\n");
				printf("   --source-map, -m       Generates a source map, which maps compiled assets to\n");
				printf("                          their original sources in the input directory; useful\n");
				printf("                          for debugging\n");
				printf("   --verbose, -v          Be verbose, print lots of details\n");
				return false;
			}
			else if (strcmp(argv[i], "--version") == 0) {
				print_banner(true, true);
				return false;
			}
			else if (strcmp(argv[i], "--in") == 0) {
				if (++i >= argc) goto missing_argument;
				path_free(g_in_path);
				g_in_path = path_new_dir(argv[i]);
			}
			else if (strcmp(argv[i], "--out") == 0) {
				if (++i >= argc) goto missing_argument;
				path_free(g_out_path);
				g_out_path = path_new_dir(argv[i]);
			}
			else if (strcmp(argv[i], "--dry-run") == 0) {
				g_want_dry_run = true;
			}
			else if (strcmp(argv[i], "--explode") == 0) {
				print_cell_quote();
				return false;
			}
			else if (strcmp(argv[i], "--make-package") == 0) {
				g_want_package = true;
			}
			else if (strcmp(argv[i], "--source-map") == 0) {
				g_want_source_map = true;
			}
			else if (strcmp(argv[i], "--verbose") == 0) {
				g_is_verbose = true;
			}
			else {
				printf("cell: error: unknown option '%s'\n", argv[i]);
				return false;
			}
		}
		else if (argv[i][0] == '-') {
			for (j = strlen(argv[i]) - 1; j >= 1; --j) {
				switch (argv[i][j]) {
				case 'm': g_want_source_map = true; break;
				case 'p': g_want_package = true; break;
				case 'v': g_is_verbose = true; break;
				default:
					printf("cell: error: unknown option '-%c'\n", argv[i][j]);
					return false;
				}
			}
		}
		else {
			free(s_target_name);
			s_target_name = strdup(argv[i]);
		}

	}
	return true;

missing_argument:
	printf("cell: error: no argument provided for '%s'\n", argv[i - 1]);
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
	};

	printf("Cell seems to be going through some sort of transformation...\n");
	printf("He's pumping himself up like a balloon!\n\n");
	printf("    Cell says:\n    \"%s\"\n", MESSAGES[rand() % (sizeof MESSAGES / sizeof(const char*))]);
}

static void
print_banner(bool want_copyright, bool want_deps)
{
	printf("Cell %s Sphere Game Compiler %s\n", CELL_VERSION, sizeof(void*) == 8 ? "x64" : "x86");
	if (want_copyright) {
		printf("A scriptable build engine for Sphere games.\n");
		printf("(c) 2015 Fat Cerberus\n");
	}
	if (want_deps) {
		printf("\n");
		printf("    Cell:    %s %s\n", CELL_VERSION, sizeof(void*) == 8 ? "x64" : "x86");
		printf("    Duktape: %s\n", DUK_GIT_DESCRIBE);
	}
}
