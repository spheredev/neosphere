#include "ssj.h"

#include <dyad.h>
#include "session.h"

struct cmdline
{
	path_t* path;
	bool    want_run;
};

static struct cmdline* parse_cmdline    (int argc, char* argv[], int *out_retval);
static void            free_cmdline     (struct cmdline* obj);
static void            print_cell_quote (void);
static void            print_banner     (bool want_copyright, bool want_deps);
static void            print_usage      (void);

int
main(int argc, char* argv[])
{
	struct cmdline* cmdline;
	int             retval;
	session_t*      session;

	if (!(cmdline = parse_cmdline(argc, argv, &retval)))
		return retval;

	print_banner(true, false);
	printf("\n");

	if (cmdline->path != NULL && !launch_minisphere(cmdline->path))
		return EXIT_FAILURE;

	sessions_init();

	if (!(session = new_session("127.0.0.1", 1208)))
		return EXIT_FAILURE;
	if (cmdline->want_run)
		execute_next(session, EXEC_RESUME);
	run_session(session);
	end_session(session);

	sessions_deinit();

	free_cmdline(cmdline);
	return EXIT_SUCCESS;
}

bool
launch_minisphere(path_t* game_path)
{
#if defined(_WIN32)
	char*   command_line;
	HMODULE h_module;
	TCHAR   pathname[MAX_PATH];

	printf("starting %s... ", path_cstr(game_path));
	fflush(stdout);
	h_module = GetModuleHandle(NULL);
	GetModuleFileName(h_module, pathname, MAX_PATH);
	PathRemoveFileSpec(pathname);
	SetCurrentDirectory(pathname);
	if (!PathFileExists(TEXT(".\\spherun.exe")))
		goto on_error;
	else {
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(STARTUPINFOA));
		si.cb = sizeof(STARTUPINFOA);
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		si.wShowWindow = SW_HIDE;
		command_line = strnewf("./spherun.exe --debug \"%s\"", path_cstr(game_path));
		CreateProcessA(NULL, command_line, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		free(command_line);
		printf("OK.\n");
		return true;
	}
#else
	path_t*     path;
	char        pathname[PATH_MAX];
	ssize_t     pathname_len;
	struct stat stat_buf;
	
	printf("starting %s... ", path_cstr(game_path));
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
		if (fork() != 0)
			path_free(path);
		else {
			// suppress minisphere's stdout. this is kind of a hack for now; eventually
			// I'd like to intermingle the engine's output with SSJ's, like in native
			// debuggers e.g. GDB.
			dup2(open("/dev/null", O_WRONLY), STDOUT_FILENO);
			dup2(open("/dev/null", O_WRONLY), STDERR_FILENO);
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

char*
strnewf(const char* fmt, ...)
{
	va_list ap, apc;
	char* buffer;
	int   buf_size;

	va_start(ap, fmt);
	va_copy(apc, ap);
	buf_size = vsnprintf(NULL, 0, fmt, apc) + 1;
	va_end(apc);
	buffer = malloc(buf_size);
	va_copy(apc, ap);
	vsnprintf(buffer, buf_size, fmt, apc);
	va_end(apc);
	va_end(ap);
	return buffer;
}

static void
free_cmdline(struct cmdline* obj)
{
	path_free(obj->path);
	free(obj);
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
			else if (strcmp(argv[i], "--run") == 0)
				cmdline->want_run = true;
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
				case 'r':
					cmdline->want_run = true;
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

	srand((unsigned int)time(NULL));
	printf("Release it, Gohan--release everything! Remember all the pain he's caused, the\n");
	printf("people he's hurt--now MAKE THAT YOUR POWER!!\n\n");
	printf("    Cell says:\n    \"%s\"\n", MESSAGES[rand() % (sizeof MESSAGES / sizeof(const char*))]);
}

static void
print_banner(bool want_copyright, bool want_deps)
{
	printf("SSJ %s minisphere CLI debugger (%s)\n", VERSION_NAME, sizeof(void*) == 8 ? "x64" : "x86");
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
	printf("   ssj [--run] <game-path>\n");
	printf("   ssj -c [--run]\n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("   -c, --connect   Connect to a target which has already been started.  If no  \n");
	printf("                   connection can be made within 30 seconds, SSJ will exit.    \n");
	printf("   -r, --run       Prevent SSJ from pausing execution on attach.  When starting\n");
	printf("                   a new instance, begin execution immediately.                \n");
	printf("       --version   Print the version number of SSJ and its dependencies.       \n");
	printf("       --help      Print this help text.                                       \n");
}
