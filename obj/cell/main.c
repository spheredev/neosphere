#include "cell.h"

duk_context* g_duk = NULL;

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
	
	printf("Cell %s Sphere Game Compiler %s\n", CELL_VERSION, sizeof(void*) == 4 ? "x86" : "x64");
	printf("%s\n", MESSAGES[rand() % num_messages]);
	printf("(c) 2015 Fat Cerberus\n\n");
	
	// initialize JavaScript environment
	g_duk = duk_create_heap_default();
	init_basics_api();

	// evaluate the build script
	if (duk_pcompile_file(g_duk, 0x0, "cell.js") != DUK_EXEC_SUCCESS
		|| duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS)
	{
		js_error_msg = duk_safe_to_string(g_duk, -1);
		if (strstr(js_error_msg, "no sourcecode"))
			fprintf(stderr, "ERROR: cell.js was not found.\n");
		else
			fprintf(stderr, "ERROR: JS error `%s`", js_error_msg);
		
		retval = EXIT_FAILURE;
		goto shutdown;
	}

	target_name = argc > 1 ? argv[1] : "make";
	if (duk_get_global_string(g_duk, target_name) && duk_is_callable(g_duk, -1)) {
		printf("Processing Cell target '%s'\n", target_name);
		if (duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS) {
			fprintf(stderr, "ERROR: JS error `%s`\n", duk_safe_to_string(g_duk, -1));
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
	duk_destroy_heap(g_duk);
	return retval;
}
