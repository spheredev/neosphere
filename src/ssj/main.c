#include "ssj.h"

#include <dyad.h>
#include "remote.h"
#include "session.h"

struct cmdline
{
	path_t* path;
	bool    want_pause;
};

static struct cmdline* parse_cmdline    (int argc, char* argv[], int *out_retval);
static void            print_cell_quote (void);
static void            print_banner     (bool want_copyright, bool want_deps);
static void            print_usage      (void);

int
main(int argc, char* argv[])
{
	struct cmdline* cmdline;
	int             retval;
	session_t*      session;

	srand((unsigned int)time(NULL));
	if (!(cmdline = parse_cmdline(argc, argv, &retval)))
		return retval;

	print_banner(true, false);
	printf("\n");

	if (cmdline->path != NULL && !launch_minisphere(cmdline->path))
		return EXIT_FAILURE;

	initialize_client();
	if (!(session = new_session("127.0.0.1", 1208)))
		return EXIT_FAILURE;
	if (!cmdline->want_pause)
		execute_next(session, EXEC_RESUME);
	run_session(session);
	shutdown_client();
	return EXIT_SUCCESS;
}

bool
launch_minisphere(path_t* game_path)
{
#if defined(_WIN32)
	lstring_t* command;
	HMODULE    h_module;
	TCHAR      pathname[MAX_PATH];

	printf("Starting '%s'... ", path_cstr(game_path));
	fflush(stdout);
	h_module = GetModuleHandle(NULL);
	GetModuleFileName(h_module, pathname, MAX_PATH);
	PathRemoveFileSpec(pathname);
	SetCurrentDirectory(pathname);
	if (!PathFileExists(TEXT(".\\spherun.exe")))
		goto on_error;
	else {
		command = lstr_newf("start ./spherun.exe --debug \"%s\"", path_cstr(game_path));
		system(lstr_cstr(command));
		lstr_free(command);
		printf("OK.\n");
		return true;
	}
#else
	path_t*     path;
	char        pathname[PATH_MAX];
	ssize_t     pathname_len;
	struct stat stat_buf;
	
	printf("Starting '%s'... ", path_cstr(game_path));
	fflush(stdout);
	memset(pathname, 0, sizeof pathname);
	pathname_len = readlink("/proc/self/exe", pathname, PATH_MAX);
	if (pathname_len == -1 || pathname_len == PATH_MAX)
		goto on_error;
	path = path_strip(path_new(pathname));
	if (chdir(path_cstr(path)) != 0) goto on_error;
	path_append(path, "spherun");
	if (stat(path_cstr(path), &stat_buf) != 0)
		goto on_error;
	else {
		if (fork() == 0) {
			// suppress minisphere's stdout. this is kind of a hack for now; eventually
			// I'd like to intermingle the engine's output with SSJ's, like in native
			// debuggers e.g. GDB.
			dup2(open("/dev/null", O_WRONLY), STDOUT_FILENO);
			execlp("./spherun", "./spherun", "--debug", path_cstr(game_path), NULL);
		}
		printf("OK.\n");
		return true;
	}
#endif

on_error:
	printf("Error!\n");
	return false;
}

static struct cmdline*
parse_cmdline(int argc, char* argv[], int *out_retval)
{
	struct cmdline* cmdline;
	bool            have_target = false;
	const char*     short_args;

	int    i;
	size_t i_arg;

	// parse the command line
	cmdline = calloc(1, sizeof(struct cmdline));
	*out_retval = EXIT_SUCCESS;
	for (i = 1; i < argc; ++i) {
		if (strstr(argv[i], "--") == argv[i]) {
			if (strcmp(argv[i], "--help") == 0) {
				print_usage();
				goto on_output_only;
			}
			else if (strcmp(argv[i], "--version") == 0) {
				print_banner(true, true);
				goto on_output_only;
			}
			else if (strcmp(argv[i], "--explode") == 0) {
				print_cell_quote();
				goto on_output_only;
			}
			else if (strcmp(argv[i], "--connect") == 0)
				have_target = true;
			else if (strcmp(argv[i], "--pause") == 0)
				cmdline->want_pause = true;
			else {
				printf("ssj: error: unknown option '%s'\n", argv[i]);
				goto on_output_only;
			}
		}
		else if (argv[i][0] == '-') {
			short_args = argv[i];
			for (i_arg = strlen(short_args) - 1; i_arg >= 1; --i_arg) {
				switch (short_args[i_arg]) {
				case 'c':
					have_target = true;
					break;
				case 'p':
					cmdline->want_pause = true;
					break;
				default:
					printf("ssj: error: unknown option '-%c'\n", short_args[i_arg]);
					*out_retval = EXIT_FAILURE;
					goto on_output_only;
				}
			}
		}
		else {
			path_free(cmdline->path);
			cmdline->path = path_resolve(path_new(argv[i]), NULL);
			if (cmdline->path == NULL) {
				printf("ssj: error: cannot resolve pathname '%s'\n", argv[i]);
				*out_retval = EXIT_FAILURE;
				goto on_output_only;
			}
			have_target = true;
		}
	}

	// sanity-check command line parameters
	if (!have_target) {
		print_usage();
		goto on_output_only;
	}

	// we're good!
	return cmdline;

on_output_only:
	free(cmdline);
	return NULL;
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

static void
print_banner(bool want_copyright, bool want_deps)
{
	printf("SSJ %s Sphere game debugger %s\n", VERSION_NAME, sizeof(void*) == 8 ? "x64" : "x86");
	if (want_copyright) {
		printf("A powerful JavaScript debugger for minisphere\n");
		printf("(c) 2016 Fat Cerberus\n");
	}
	if (want_deps) {
		printf("\n");
		printf("   Dyad.c: v%s\n", dyad_getVersion());
	}
}

static void
print_usage(void)
{
	print_banner(true, false);
	printf("\n");
	printf("USAGE:\n");
	printf("   ssj [options] <game-path>\n");
	printf("   ssj -c [options]\n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("       --version   Prints the SSJ debugger version.                            \n");
	printf("       --help      Prints this help text.                                      \n");
	printf("   -c, --connect   Attempts to attach to a target already running. If the      \n");
	printf("                   connection attempt fails, SSJ will exit.                    \n");
	printf("   -p, --pause     Pauses execution on attach, or, when debugging a new        \n");
	printf("                   instance, pauses before any code is executed. This allows   \n");
	printf("                   setting up breakpoints, etc. in advance.                    \n");
}
