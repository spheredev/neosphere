#include "ssj.h"

#include "client.h"

static bool parse_cmdline (int argc, char* argv[], path_t* *out_game_path);
static void print_banner  (bool want_copyright);

int
main(int argc, char* argv[])
{
	int     retval = EXIT_FAILURE;
	path_t* game_path;
	
	if (!parse_cmdline(argc, argv, &game_path))
		goto shut_down;
	
	print_banner(true);
	initialize_client();
	
	retval = EXIT_SUCCESS;

shut_down:
	shutdown_client();
	path_free(game_path);
	return retval;
}

static void
print_banner(bool want_copyright)
{
	printf("SSJ %s Sphere Game Debugger %s\n", VERSION_NAME, sizeof(void*) == 8 ? "x64" : "x86");
	if (want_copyright) {
		printf("A powerful JavaScript debugger for minisphere\n");
		printf("(c) 2016 Fat Cerberus\n");
	}
}

static bool
parse_cmdline(int argc, char* argv[], path_t* *out_game_path)
{
	const char* short_args;

	int i, j;

	*out_game_path = NULL;
	
	// validate and parse the command line
	for (i = 1; i < argc; ++i) {
		if (strstr(argv[i], "--") == argv[i]) {
			if (strcmp(argv[i], "--help") == 0) {
				print_banner(false);
				printf("\n");
				printf("USAGE: ssj\n");
				printf("       ssj [options] [<game_path>]\n");
				printf("\n");
				printf("OPTIONS:\n");
				printf("       --version          Prints the SSJ debugger version and exits.           \n");
				return false;
			}
			else if (strcmp(argv[i], "--version") == 0) {
				print_banner(true);
				return false;
			}
			else if (strcmp(argv[i], "--explode") == 0) {
				printf("Release it, release everything! Remember all the pain he's caused, the people\n");
				printf("he's hurt--now MAKE THAT YOUR POWER!\n");
				return false;
			}
			else {
				printf("ssj: error: unknown option '%s'\n", argv[i]);
				return false;
			}
		}
		else if (argv[i][0] == '-') {
			short_args = argv[i];
			for (j = strlen(short_args) - 1; j >= 1; --j) {
				switch (short_args[j]) {
				default:
					printf("ssj: error: unknown option '-%c'\n", short_args[j]);
					return false;
				}
			}
		}
		else {
			path_free(*out_game_path);
			*out_game_path = path_new(argv[i]);
		}
	}

	return true;

missing_argument:
	printf("ssj: error: no argument provided for '%s'\n", argv[i - 1]);
	return false;
}
