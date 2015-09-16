#include "cell.h"

static bool parse_cmdline    (int argc, char* argv[]);
static void print_cell_quote (void);
static void print_version    (bool want_all);

duk_context* g_duk = NULL;
bool         g_is_verbose = false;
char*        g_out_path = NULL;

char* s_target_name;

int
main(int argc, char* argv[])
{
	int         retval = EXIT_SUCCESS;
	const char* js_error_msg;
	char        js_target_name[256];

	srand((unsigned int)time(NULL));
	if (!parse_cmdline(argc, argv))
		return EXIT_FAILURE;

	if (g_is_verbose) {
		print_version(false);
		//printf("\n");
	}
	
	// initialize JavaScript environment
	g_duk = duk_create_heap_default();
	init_fs_api();

	// evaluate the build script
	if (duk_pcompile_file(g_duk, 0x0, "cell.js") != DUK_EXEC_SUCCESS
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
		printf("Processing Cell target '%s'\n", s_target_name);
		if (duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS) {
			fprintf(stderr, "FATAL: JS %s\n", duk_safe_to_string(g_duk, -1));
			retval = EXIT_FAILURE;
			goto shutdown;
		}
		printf("Success!\n");
	}
	else {
		fprintf(stderr, "cell: error: no target named '%s'\n", s_target_name);
		retval = EXIT_FAILURE;
		goto shutdown;
	}

shutdown:
	free(s_target_name);
	duk_destroy_heap(g_duk);
	return retval;
}

static bool
parse_cmdline(int argc, char* argv[])
{
	int num_targets = 0;
	
	int i, j;
	
	// establish compiler defaults
	g_out_path = strdup("dist");
	s_target_name = strdup("sphere");

	// validate and parse the command line
	for (i = 1; i < argc; ++i) {
		if (strstr(argv[i], "--") == argv[i]) {
			if (strcmp(argv[i], "--in") == 0) {
				if (++i >= argc) goto missing_argument;
				if (tinydir_chdir(argv[i]) != 0) {
					printf("cell: error: no such directory '%s'\n", argv[i]);
					return false;
				}
				++i;
			}
			else if (strcmp(argv[i], "--out") == 0) {
				if (++i >= argc) goto missing_argument;
				free(g_out_path);
				g_out_path = strdup(argv[i]);
			}
			else if (strcmp(argv[i], "--unforgivable") == 0) {
				print_cell_quote();
				return false;
			}
			else if (strcmp(argv[i], "--version") == 0) {
				print_version(true);
				return false;
			}
			else {
				printf("cell: error: unknown option '%s'\n", argv[i]);
				return false;
			}
		}
		else if (argv[i][0] == '-') {
			for (j = strlen(argv[i]) - 1; j >= 1; --j) {
				switch (argv[i][j]) {
				case 'v':
					g_is_verbose = true;
					break;
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
	printf("ERROR: No argument provided for '%s'.\n", argv[i]);
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
print_version(bool want_all)
{
	printf("Cell %s Sphere Game Compiler %s\n",
		CELL_VERSION,
		sizeof(void*) == 8 ? "x64" : "x86");
	if (want_all) {
		printf("A scriptable build engine for Sphere games.\n");
		printf("(c) 2015 Fat Cerberus\n\n");
		printf("    JS runtime: Duktape %s\n", DUK_GIT_DESCRIBE);
	}
}
