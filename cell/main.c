#include "cell.h"

static bool parse_cmdline (int argc, char* argv[]);

duk_context* g_duk = NULL;
char*        g_out_path = NULL;

char* s_target_name;

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
	
	int         retval = EXIT_SUCCESS;
	const char* js_error_msg;
	char        js_target_name[256];

	printf("Cell %s Sphere Game Compiler %s\n", CELL_VERSION,
		sizeof(void*) == 8 ? "x64" : "x86");
	printf("%s\n", MESSAGES[rand() % (sizeof MESSAGES / sizeof(const char*))]);
	printf("(c) 2015 Fat Cerberus\n\n");
	
	srand((unsigned int)time(NULL));
	if (!parse_cmdline(argc, argv))
		return EXIT_FAILURE;

	// initialize JavaScript environment
	g_duk = duk_create_heap_default();
	init_fs_api();

	// evaluate the build script
	if (duk_pcompile_file(g_duk, 0x0, "cell.js") != DUK_EXEC_SUCCESS
		|| duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS)
	{
		js_error_msg = duk_safe_to_string(g_duk, -1);
		if (strstr(js_error_msg, "no sourcecode"))
			fprintf(stderr, "ERROR: No cell.js in current directory.\n");
		else
			fprintf(stderr, "ERROR: JS %s", js_error_msg);
		
		retval = EXIT_FAILURE;
		goto shutdown;
	}

	sprintf(js_target_name, "_%s", s_target_name);
	if (duk_get_global_string(g_duk, js_target_name) && duk_is_callable(g_duk, -1)) {
		printf("Processing Cell target '%s'\n", s_target_name);
		if (duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS) {
			fprintf(stderr, "ERROR: JS %s\n", duk_safe_to_string(g_duk, -1));
			retval = EXIT_FAILURE;
			goto shutdown;
		}
		printf("Success!\n");
	}
	else {
		fprintf(stderr, "ERROR: No such target '%s'.\n", s_target_name);
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
	
	int i;
	
	// establish compiler defaults
	g_out_path = strdup("dist");
	s_target_name = strdup("sphere");

	// validate and parse the command line
	for (i = 1; i < argc; ++i) {
		if (strstr(argv[i], "--") == argv[i]) {
			if (strcmp(argv[i], "--out") == 0) {
				if (i >= argc - 1) goto missing_argument;
				free(g_out_path);
				g_out_path = strdup(argv[i + 1]);
				++i;
			}
			else {
				printf("ERROR: Unknown option '%s'.\n", argv[i]);
				return false;
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
