#include "ssj.h"

#include "dvalue.h"

static bool parse_cmdline    (int argc, char* argv[], path_t* *out_game_path);
static void print_banner     (bool want_copyright);
static void print_cell_quote (void);

int
main(int argc, char* argv[])
{
	int     retval = EXIT_FAILURE;
	path_t* game_path;
	
	srand((unsigned int)time(NULL));
	if (!parse_cmdline(argc, argv, &game_path))
		goto shut_down;
	
	print_banner(true);
	printf("\n");
	
	initialize_client();
	session_new("127.0.0.1", 1208);
	
	retval = EXIT_SUCCESS;

shut_down:
	shutdown_client();
	path_free(game_path);
	return retval;
}

static bool
parse_cmdline(int argc, char* argv[], path_t* *out_game_path)
{
	const char* short_args;

	int    i;
	size_t i_arg;

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
				print_cell_quote();
				return false;
			}
			else {
				printf("ssj: error: unknown option '%s'\n", argv[i]);
				return false;
			}
		}
		else if (argv[i][0] == '-') {
			short_args = argv[i];
			for (i_arg = strlen(short_args) - 1; i_arg >= 1; --i_arg) {
				switch (short_args[i_arg]) {
				default:
					printf("ssj: error: unknown option '-%c'\n", short_args[i_arg]);
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

static void
print_cell_quote(void)
{
	static const char* const MESSAGES[] =
	{
		"I expected the end to be a little more dramatic...",
		"Don't you realize yet you're up against the perfect weapon?!",
		"Would you stop interfering!?",
		"You're all so anxious to die, aren't you? Well all you had to do WAS ASK!",
		"Why can't you people JUST STAY DOWN!!",
		"They just keep lining up to die!",
		"No chance! YOU HAVE NO CHANCE!!",
		"SAY GOODBYE!",
		"I WAS PERFECT...!",
	};

	printf("Release it--release everything! Remember all the pain he's caused, the people\n");
	printf("he's hurt--now MAKE THAT YOUR POWER!!\n\n");
	printf("    Cell says:\n    \"%s\"\n", MESSAGES[rand() % (sizeof MESSAGES / sizeof(const char*))]);
}
