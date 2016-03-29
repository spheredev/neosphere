#include "minisphere.h"
#include "api.h"
#include "async.h"
#include "audialis.h"
#include "debugger.h"
#include "galileo.h"
#include "input.h"
#include "map_engine.h"
#include "rng.h"
#include "spriteset.h"

#include <libmng.h>

// enable Windows visual styles (MSVC)
#ifdef _MSC_VER
#pragma comment(linker, \
    "\"/manifestdependency:type='Win32' "\
    "name='Microsoft.Windows.Common-Controls' "\
    "version='6.0.0.0' "\
    "processorArchitecture='*' "\
    "publicKeyToken='6595b64144ccf1df' "\
    "language='*'\"")
#endif

static bool initialize_engine   (void);
static void shutdown_engine     (void);
static bool find_startup_game   (path_t* *out_path);
static bool parse_command_line  (int argc, char* argv[], path_t* *out_game_path, bool *out_want_fullscreen, int *out_fullscreen, int *out_verbosity, bool *out_want_throttle, bool *out_want_debug);
static void print_banner        (bool want_copyright, bool want_deps);
static void print_usage         (void);
static void report_error        (const char* fmt, ...);
static bool verify_requirements (sandbox_t* fs);

static void on_duk_fatal (duk_context* ctx, duk_errcode_t code, const char* msg);

duk_context*         g_duk = NULL;
ALLEGRO_EVENT_QUEUE* g_events = NULL;
int                  g_framerate = 0;
sandbox_t*           g_fs = NULL;
path_t*              g_game_path = NULL;
path_t*              g_last_game_path = NULL;
screen_t*            g_screen = NULL;
kev_file_t*          g_sys_conf;
font_t*              g_sys_font = NULL;
int                  g_res_x, g_res_y;

static jmp_buf s_jmp_exit;
static jmp_buf s_jmp_restart;

static const char* const ERROR_TEXT[][2] =
{
	{ "*munch*", "a hunger-pig just devoured your game!" },
	{ "*CRASH!*", "it's an 812-car pileup!" },
	{ "so, um... a funny thing happened...", "...on the way to the boss..." },
	{ "here's the deal.", "the game encountered an error." },
	{ "this game sucks!", "or maybe it's just the programmer..." },
	{ "cows eat kitties. pigs don't eat cows.", "they just get \"replaced\" by them." },
	{ "hey look, a squirrel!", "I wonder if IT'S responsible for this." },
	{ "sorry. it's just...", "...well, this is a trainwreck of a game." },
	{ "you better run, and you better hide...", "...'cause a big fat hawk just ate that guy!" },
	{ "an exception was thrown.", "minisphere takes exception to sucky games." },
	{ "honk. HONK. honk. HONK. :o)", "there's a clown behind you." },
};

int
main(int argc, char* argv[])
{
	// HERE BE DRAGONS!
	// as the oldest function in the minisphere codebase by definition, this has become
	// something of a hairball over time, and likely quite fragile.  don't be surprised if
	// attempting to edit it causes something to break. :o)

	path_t*              games_path;
	lstring_t*           dialog_name;
	duk_errcode_t        err_code;
	const char*          err_msg;
	ALLEGRO_FILECHOOSER* file_dlg;
	const char*          filename;
	bool                 have_shaders = false;
	image_t*             icon;
	int                  line_num;
	const path_t*        script_path;
	bool                 use_conserve_cpu;
	int                  use_frameskip;
	bool                 use_fullscreen;
	int                  use_verbosity;
	bool                 want_debug;

	// parse the command line
	if (parse_command_line(argc, argv, &g_game_path,
		&use_fullscreen, &use_frameskip, &use_verbosity, &use_conserve_cpu, &want_debug))
	{
		initialize_console(use_verbosity);
	}
	else
		return EXIT_FAILURE;

	print_banner(true, false);
	printf("\n");

	// print out options
	console_log(1, "parsing command line");
	console_log(1, "    game path: %s", g_game_path != NULL ? path_cstr(g_game_path) : "<none provided>");
	console_log(1, "    fullscreen: %s", use_fullscreen ? "on" : "off");
	console_log(1, "    frameskip limit: %d frames", use_frameskip);
	console_log(1, "    sleep when idle: %s", use_conserve_cpu ? "yes" : "no");
	console_log(1, "    console verbosity: V%d", use_verbosity);
#if defined(MINISPHERE_SPHERUN)
	console_log(1, "    debugger mode: %s", want_debug ? "active" : "passive");
#endif
	console_log(1, "");

	if (!initialize_engine())
		return EXIT_FAILURE;

	// set up jump points for script bailout
	console_log(1, "setting up jump points for longjmp");
	if (setjmp(s_jmp_exit)) {  // user closed window, script called Exit(), etc.
		shutdown_engine();
		if (g_last_game_path != NULL) {  // returning from ExecuteGame()?
			initialize_engine();
			g_game_path = g_last_game_path;
			g_last_game_path = NULL;
		}
		else {
			return EXIT_SUCCESS;
		}
	}
	if (setjmp(s_jmp_restart)) {  // script called RestartGame() or ExecuteGame()
		shutdown_engine();
		console_log(1, "\nrestarting to launch new game");
		console_log(1, "    path: %s", path_cstr(g_game_path));
		initialize_engine();
	}

	// locate the game manifest
	console_log(1, "searching for a game to launch");
	games_path = path_rebase(path_new("Sphere 2.0/games/"), homepath());
	path_mkdir(games_path);
	if (g_game_path == NULL)
		// no game specified on command line, see if we have a startup game
		find_startup_game(&g_game_path);
	if (g_game_path != NULL)
		// user provided a path or startup game was found, attempt to load it
		g_fs = new_sandbox(path_cstr(g_game_path));
	else {
		// no game path provided and no startup game, let user find one
		dialog_name = lstr_newf("%s - Select a Sphere game to launch", PRODUCT_NAME);
		file_dlg = al_create_native_file_dialog(path_cstr(games_path),
			lstr_cstr(dialog_name),
			"game.sgm;game.s2gm;*.spk", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
		al_show_native_file_dialog(NULL, file_dlg);
		lstr_free(dialog_name);
		if (al_get_native_file_dialog_count(file_dlg) > 0) {
			path_free(g_game_path);
			g_game_path = path_new(al_get_native_file_dialog_path(file_dlg, 0));
			g_fs = new_sandbox(path_cstr(g_game_path));
			al_destroy_native_file_dialog(file_dlg);
		}
		else {
			// user clicked Cancel; as this is a valid action, we return
			// success, not failure.
			al_destroy_native_file_dialog(file_dlg);
			path_free(games_path);
			return EXIT_SUCCESS;
		}
	}
	path_free(games_path);

	if (g_fs == NULL) {
		// if after all that, we still don't have a valid sandbox pointer, bail out;
		// there's not much else we can do.
#if !defined(MINISPHERE_SPHERUN)
		al_show_native_message_box(NULL, "Unable to Load Game", path_cstr(g_game_path),
			"minisphere was unable to load the game manifest or it was not found.  Check to make sure the directory above exists and contains a valid Sphere game.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
#endif
		exit_game(false);
	}
	if (!verify_requirements(g_fs))
		exit_game(false);

	// try to create a display. if we can't get a programmable pipeline, try again but
	// only request bare OpenGL. keep in mind that if this happens, shader support will be
	// disabled.
	get_sgm_resolution(g_fs, &g_res_x, &g_res_y);
	if (!(icon = load_image("~sgm/icon.png")))
		icon = load_image("~sys/icon.png");
	g_screen = screen_new(get_sgm_name(g_fs), icon, g_res_x, g_res_y, use_frameskip, !use_conserve_cpu);
	if (g_screen == NULL) {
		al_show_native_message_box(NULL, "Unable to Create Render Context", "minisphere was unable to create a render context.",
			"Your hardware may be too old to run minisphere, or there is a driver problem on this system.  Check that your graphics drivers are installed and up-to-date.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		return EXIT_FAILURE;
	}
	
	al_set_new_bitmap_flags(ALLEGRO_NO_PREMULTIPLIED_ALPHA);
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	g_events = al_create_event_queue();
	al_register_event_source(g_events,
		al_get_display_event_source(screen_display(g_screen)));
	attach_input_display();
	load_key_map();

	// initialize shader support
	initialize_shaders(have_shaders);

	// attempt to locate and load system font
	console_log(1, "loading system default font");
	if (g_sys_conf != NULL) {
		filename = read_string_rec(g_sys_conf, "Font", "system.rfn");
		g_sys_font = load_font(systempath(filename));
	}
	if (g_sys_font == NULL) {
		al_show_native_message_box(screen_display(g_screen), "No System Font Available", "A system font is required.",
			"minisphere was unable to locate the system font or it failed to load.  As a usable font is necessary for correct operation, minisphere will now close.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		return EXIT_FAILURE;
	}

	// switch to fullscreen if necessary and initialize clipping
	if (use_fullscreen)
		screen_toggle_fullscreen(g_screen);

	// display loading message, scripts may take a bit to compile
	if (want_debug) {
		al_clear_to_color(al_map_rgba(0, 0, 0, 255));
		screen_draw_status(g_screen, "waiting for SSJ...");
		al_flip_display();
		al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	}

	// enable debugging support
#if defined(MINISPHERE_SPHERUN)
	initialize_debugger(want_debug, false);
#endif

	// display loading message, scripts may take a bit to compile
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	screen_draw_status(g_screen, "starting up...");
	al_flip_display();
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));

	// evaluate startup script
	screen_show_mouse(g_screen, false);
	script_path = get_sgm_script_path(g_fs);
	if (!evaluate_script(path_cstr(script_path)))
		goto on_js_error;
	duk_pop(g_duk);

	// call game() function in script
	duk_get_global_string(g_duk, "game");
	if (duk_is_callable(g_duk, -1) && duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS)
		goto on_js_error;
	duk_pop(g_duk);
	exit_game(false);

on_js_error:
	err_code = duk_get_error_code(g_duk, -1);
	duk_dup(g_duk, -1);
	err_msg = duk_safe_to_string(g_duk, -1);
	screen_show_mouse(g_screen, true);
	duk_get_prop_string(g_duk, -2, "lineNumber");
	line_num = duk_get_int(g_duk, -1);
	duk_pop(g_duk);
	duk_get_prop_string(g_duk, -2, "fileName");
	filename = duk_get_string(g_duk, -1);
	if (filename != NULL) {
		fprintf(stderr, "Unhandled JS exception caught by engine\n  [%s:%d] %s\n", filename, line_num, err_msg);
		if (err_msg[strlen(err_msg) - 1] != '\n')
			duk_push_sprintf(g_duk, "%s:%d\n\n%s\n ", filename, line_num, err_msg);
		else
			duk_push_sprintf(g_duk, "%s\n ", err_msg);
	}
	else {
		fprintf(stderr, "Unhandled JS error caught by engine.\n%s\n", err_msg);
		duk_push_string(g_duk, err_msg);
	}
	duk_fatal(g_duk, err_code, duk_get_string(g_duk, -1));
}

void
delay(double time)
{
	double end_time;
	double time_left;

	end_time = al_get_time() + time;
	do {
		time_left = end_time - al_get_time();
		if (time_left > 0.001)  // engine may stall with < 1ms timeout
			al_wait_for_event_timed(g_events, NULL, time_left);
		do_events();
	} while (al_get_time() < end_time);
}

void
do_events(void)
{
	ALLEGRO_EVENT event;

	dyad_update();

#if defined(MINISPHERE_SPHERUN)
	update_debugger();
#endif

	update_async();
	update_input();
	update_audialis();

	// process Allegro events
	while (al_get_next_event(g_events, &event)) {
		switch (event.type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			exit_game(true);
		}
	}
}

noreturn
exit_game(bool force_shutdown)
{
	if (force_shutdown) {
		free(g_last_game_path);
		g_last_game_path = NULL;
	}
	longjmp(s_jmp_exit, 1);
}

noreturn
restart_engine(void)
{
	longjmp(s_jmp_restart, 1);
}

static void
on_duk_fatal(duk_context* ctx, duk_errcode_t code, const char* msg)
{
	wraptext_t*            error_info;
	bool                   is_copied = true;
	bool                   is_finished;
	int                    frames_till_close;
	ALLEGRO_KEYBOARD_STATE keyboard;
	const char*            line_text;
	int                    num_lines;
	const char*            subtitle;
	const char*            title;
	int                    title_index;

	int i;

#ifdef MINISPHERE_USE_CLIPBOARD
	is_copied = false;
#endif

	title_index = rand() % (sizeof(ERROR_TEXT) / sizeof(const char*) / 2);
	title = ERROR_TEXT[title_index][0];
	subtitle = ERROR_TEXT[title_index][1];
	if (g_sys_font == NULL)
		goto show_error_box;

	// create wraptext from error message
	if (!(error_info = word_wrap_text(g_sys_font, msg, g_res_x - 84)))
		goto show_error_box;
	num_lines = get_wraptext_line_count(error_info);

	// show error in-engine, Sphere 1.x style
	screen_unskip_frame(g_screen);
	is_finished = false;
	frames_till_close = 30;
	while (!is_finished) {
		al_draw_filled_rounded_rectangle(32, 48, g_res_x - 32, g_res_y - 32, 5, 5, al_map_rgba(16, 16, 16, 255));
		draw_text(g_sys_font, rgba(0, 0, 0, 255), g_res_x / 2 + 1, 11, TEXT_ALIGN_CENTER, title);
		draw_text(g_sys_font, rgba(255, 255, 255, 255), g_res_x / 2, 10, TEXT_ALIGN_CENTER, title);
		draw_text(g_sys_font, rgba(0, 0, 0, 255), g_res_x / 2 + 1, 23, TEXT_ALIGN_CENTER, subtitle);
		draw_text(g_sys_font, rgba(255, 255, 255, 255), g_res_x / 2, 22, TEXT_ALIGN_CENTER, subtitle);
		for (i = 0; i < num_lines; ++i) {
			line_text = get_wraptext_line(error_info, i);
			draw_text(g_sys_font, rgba(0, 0, 0, 255),
				g_res_x / 2 + 1, 59 + i * get_font_line_height(g_sys_font),
				TEXT_ALIGN_CENTER, line_text);
			draw_text(g_sys_font, rgba(192, 192, 192, 255),
				g_res_x / 2, 58 + i * get_font_line_height(g_sys_font),
				TEXT_ALIGN_CENTER, line_text);
		}
		if (frames_till_close <= 0) {
			draw_text(g_sys_font, rgba(255, 255, 255, 255), g_res_x / 2, g_res_y - 10 - get_font_line_height(g_sys_font),
				TEXT_ALIGN_CENTER,
				is_copied ? "[Space]/[Esc] to close" : "[Ctrl+C] to copy, [Space]/[Esc] to close");
		}
		screen_flip(g_screen, 30);
		if (frames_till_close <= 0) {
			al_get_keyboard_state(&keyboard);
			is_finished = al_key_down(&keyboard, ALLEGRO_KEY_ESCAPE)
				|| al_key_down(&keyboard, ALLEGRO_KEY_SPACE);

			// if Ctrl+C is pressed, copy the error message and location to clipboard
#ifdef MINISPHERE_USE_CLIPBOARD
			if ((al_key_down(&keyboard, ALLEGRO_KEY_LCTRL) || al_key_down(&keyboard, ALLEGRO_KEY_RCTRL))
			    && al_key_down(&keyboard, ALLEGRO_KEY_C))
			{
				is_copied = true;
				al_set_clipboard_text(screen_display(g_screen), msg);
			}
#endif
		}
		else {
			--frames_till_close;
		}
	}
	free_wraptext(error_info);
	shutdown_engine();
	exit(EXIT_SUCCESS);

show_error_box:
	// use a native message box only as a last resort
	al_show_native_message_box(screen_display(g_screen), "Script Error",
		"minisphere encountered an error during game execution.",
		msg, NULL, ALLEGRO_MESSAGEBOX_ERROR);
	shutdown_engine();
	exit(EXIT_SUCCESS);
}

static bool
initialize_engine(void)
{
	uint32_t al_version;

	srand(time(NULL));

	// initialize Allegro
	al_version = al_get_allegro_version();
	console_log(1, "initializing Allegro (%d.%d.%d)",
		al_version >> 24, (al_version >> 16) & 0xFF, (al_version >> 8) & 0xFF);
	al_set_org_name("Fat Cerberus");
	al_set_app_name("minisphere");
	if (!al_init())
		goto on_error;
	if (!al_init_native_dialog_addon()) goto on_error;
	if (!al_init_primitives_addon()) goto on_error;
	if (!al_init_image_addon()) goto on_error;

	// initialize networking
	console_log(1, "initializing Dyad (%s)", dyad_getVersion());
	dyad_init();
	dyad_setUpdateTimeout(0.0);

	// initialize JavaScript
	console_log(1, "initializing Duktape (%s)", DUK_GIT_DESCRIBE);
	if (!(g_duk = duk_create_heap(NULL, NULL, NULL, NULL, &on_duk_fatal)))
		goto on_error;

	// load system configuraton
	console_log(1, "loading system configuration");
	g_sys_conf = open_kev_file(NULL, "~sys/system.ini");

	// initialize engine components
	initialize_async();
	initialize_rng();
	initialize_galileo();
	initialize_audialis();
	initialize_input();
	initialize_spritesets();
	initialize_map_engine();
	initialize_scripts();

	// register the Sphere API
	initialize_api(g_duk);

	return true;

on_error:
	al_show_native_message_box(screen_display(g_screen), "Unable to Start", "Engine initialized failed.",
		"One or more components failed to initialize properly. minisphere cannot continue in this state and will now close.",
		NULL, ALLEGRO_MESSAGEBOX_ERROR);
	return false;
}

static void
shutdown_engine(void)
{
	save_key_map();

#if defined(MINISPHERE_SPHERUN)
	shutdown_debugger();
#endif

	shutdown_map_engine();
	shutdown_input();
	shutdown_scripts();

	console_log(1, "shutting down Duktape");
	duk_destroy_heap(g_duk);

	console_log(1, "shutting down Dyad");
	dyad_shutdown();

	shutdown_spritesets();
	shutdown_audialis();
	shutdown_galileo();
	shutdown_async();

	console_log(1, "shutting down Allegro");
	screen_free(g_screen);
	g_screen = NULL;
	if (g_events != NULL)
		al_destroy_event_queue(g_events);
	g_events = NULL;
	free_sandbox(g_fs);
	g_fs = NULL;
	if (g_sys_conf != NULL)
		close_kev_file(g_sys_conf);
	g_sys_conf = NULL;
	al_uninstall_system();
}

static bool
find_startup_game(path_t* *out_path)
{
	ALLEGRO_FS_ENTRY* engine_dir;
	const char*       file_ext;
	const char*       filename;
	ALLEGRO_FS_ENTRY* fse;
	int               n_spk_files = 0;

	// prefer startup game alongside engine if one exists
	*out_path = path_rebase(path_new("startup/game.sgm"), enginepath());
	if (al_filename_exists(path_cstr(*out_path)))
		return true;
	path_free(*out_path);

	// check for single SPK package alongside engine
	*out_path = path_dup(enginepath());
	engine_dir = al_create_fs_entry(path_cstr(*out_path));
	al_open_directory(engine_dir);
	while (fse = al_read_directory(engine_dir)) {
		filename = al_get_fs_entry_name(fse);
		file_ext = strrchr(filename, '.');
		if (file_ext != NULL && strcmp(file_ext, ".spk") == 0) {
			if (++n_spk_files == 1)
				*out_path = path_new(filename);
		}
		al_destroy_fs_entry(fse);
	}
	al_close_directory(engine_dir);
	if (n_spk_files == 1)
		return true;  // found an SPK

	// as a last resort, use the default startup game
	*out_path = path_rebase(path_new("system/startup.spk"), enginepath());
	if (al_filename_exists(path_cstr(*out_path)))
		return true;
	path_free(*out_path);
	*out_path = path_rebase(path_new("../share/minisphere/system/startup.spk"), enginepath());
	if (al_filename_exists(path_cstr(*out_path)))
		return true;
	path_free(*out_path);
	*out_path = NULL;
	
	// if we reached this point, no suitable startup game was found.
	path_free(*out_path);
	*out_path = NULL;
	return false;
}

static bool
parse_command_line(
	int argc, char* argv[],
	path_t* *out_game_path, bool *out_want_fullscreen, int *out_frameskip,
	int *out_verbosity, bool *out_want_throttle, bool *out_want_debug)
{
	bool parse_options = true;

	int i;

	// establish default settings
#if defined(MINISPHERE_SPHERUN)
	*out_want_fullscreen = false;
#else
	*out_want_fullscreen = true;
	#endif

	*out_game_path = NULL;
	*out_frameskip = 5;
	*out_verbosity = 0;
	*out_want_throttle = true;
	*out_want_debug = false;

	// process command line arguments
	for (i = 1; i < argc; ++i) {
		if (strstr(argv[i], "--") == argv[i] && parse_options) {
			if (strcmp(argv[i], "--") == 0)
				parse_options = false;
			else if (strcmp(argv[i], "--frameskip") == 0) {
				if (++i >= argc) goto missing_argument;
				*out_frameskip = atoi(argv[i]);
			}
			else if (strcmp(argv[i], "--no-sleep") == 0) {
				*out_want_throttle = false;
			}
			else if (strcmp(argv[i], "--fullscreen") == 0) {
				*out_want_fullscreen = true;
			}
			else if (strcmp(argv[i], "--window") == 0) {
				*out_want_fullscreen = false;
			}
#if defined(MINISPHERE_SPHERUN)
			else if (strcmp(argv[i], "--version") == 0) {
				print_banner(true, true);
				return false;
			}
			else if (strcmp(argv[i], "--help") == 0) {
				print_usage();
				return false;
			}
			else if (strcmp(argv[i], "--debug") == 0) {
				*out_want_debug = true;
			}
			else if (strcmp(argv[i], "--verbose") == 0) {
				if (++i >= argc) goto missing_argument;
				*out_verbosity = atoi(argv[i]);
			}
			else {
				report_error("unrecognized option `%s`\n", argv[i]);
				return false;
			}
#else
			else if (strcmp(argv[i], "--verbose") == 0)
				++i;
#endif
		}
		else if (argv[i][0] == '-' && parse_options) {
			if (strcmp(argv[i], "-game") == 0 || strcmp(argv[i], "-package") == 0) {
				// -game and -package are provided for backwards compatibility; to minisphere
				// the two options are equivalent.
				if (++i >= argc) goto missing_argument;
				if (*out_game_path != NULL) {
					report_error("more than one game specified on command line\n");
					return false;
				}
				*out_game_path = path_new(argv[i]);
			}
#if defined(MINISPHERE_SPHERUN)
			else if (strcmp(argv[i], "-v") == 0) {
				if (++i >= argc) goto missing_argument;
				*out_verbosity = atoi(argv[i]);
			}
			else {
				report_error("unrecognized option `%s`\n", argv[i]);
				return false;
			}
#else
			else if (strcmp(argv[i], "-v") == 0)
				++i;
#endif
		}
		else {
			if (*out_game_path != NULL) {
				report_error("more than one game specified on command line\n");
				return false;
			}
			*out_game_path = path_new(argv[i]);
		}
	}

#if defined(MINISPHERE_SPHERUN)
	if (*out_game_path == NULL) {
		print_usage();
		return false;
	}
#endif
	
	return true;

missing_argument:
	report_error("missing argument for option `%s`\n", argv[i - 1]);
	return false;
}

static void
print_banner(bool want_copyright, bool want_deps)
{
	lstring_t* al_version;
	uint32_t   al_version_id;
	
	printf("%s Sphere Game Engine (%s)\n", PRODUCT_NAME, sizeof(void*) == 4 ? "x86" : "x64");
	if (want_copyright) {
		printf("a lightweight JavaScript-based 2D game engine\n");
		printf("(c) 2015-2016 Fat Cerberus\n");
	}
	if (want_deps) {
		al_version_id = al_get_allegro_version();
		al_version = lstr_newf("%d.%d.%d.%d", al_version_id >> 24,
			(al_version_id >> 16) & 0xFF, (al_version_id >> 8) & 0xFF,
			(al_version_id & 0xFF) - 1);
		printf("\n");
		printf("    Allegro: v%-8s    libmng: v%s\n", lstr_cstr(al_version), mng_version_text());
		printf("     Dyad.c: v%-8s      zlib: v%s\n", dyad_getVersion(), zlibVersion());
		printf("    Duktape: %s\n", DUK_GIT_DESCRIBE);
	}
}

static void
print_usage(void)
{
	print_banner(true, false);
	printf("\n");
	printf("USAGE:\n");
	printf("   spherun [--fullscreen | --window] [--frameskip <n>] [--no-sleep] [--debug]\n");
	printf("           [--verbose <n>] <game_path>                                       \n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("       --fullscreen   Start the engine in fullscreen mode.                   \n");
	printf("       --window       Start the engine in windowed mode. This is the default.\n");
	printf("       --frameskip    Set the maximum number of consecutive frames to skip.  \n");
	printf("       --no-sleep     Prevent the engine from sleeping between frames.       \n");
	printf("       --debug        Wait 30 seconds for the debugger to attach. If nothing \n");
	printf("                      attaches, minisphere will exit.                        \n");
	printf("   -v, --verbose      Set the engine's verbosity level. Valid levels are 0-4.\n");
	printf("                      Higher levels are more verbose. The default is 0, which\n");
	printf("                      only shows game output.                                \n");
	printf("       --version      Show the version number of the installed minisphere.   \n");
	printf("       --help         Show this help text.                                   \n");
	printf("\n");
	printf("NOTE:\n");
	printf("   spherun(1) is used to execute Sphere games in a development environment. If\n");
	printf("   your intent is simply to play a game, use minisphere(1) instead.           \n");
}

static void
report_error(const char* fmt, ...)
{
	va_list ap;

	lstring_t* error_text;

	va_start(ap, fmt);
	error_text = lstr_vnewf(fmt, ap);
	va_end(ap);
#if defined(MINISPHERE_SPHERUN)
	fprintf(stderr, "spherun: ERROR: %s", lstr_cstr(error_text));
#else
	al_show_native_message_box(screen_display(g_screen),
		"minisphere", "An error occurred starting the engine.", lstr_cstr(error_text),
		NULL, ALLEGRO_MESSAGEBOX_ERROR);
#endif
	lstr_free(error_text);
}

static bool
verify_requirements(sandbox_t* fs)
{
	// NOTE: before calling this function, the Sphere API must already have been
	//       initialized using initialize_api().

	const char* extension_name;
	lstring_t*  message;
	const char* recommendation = NULL;

	duk_size_t i;

	duk_push_lstring_t(g_duk, get_game_manifest(g_fs));
	duk_json_decode(g_duk, -1);

	if (duk_get_prop_string(g_duk, -1, "minimumPlatform")) {
		if (duk_get_prop_string(g_duk, -1, "recommend")) {
			if (duk_is_string(g_duk, -1))
				recommendation = duk_get_string(g_duk, -1);
			duk_pop(g_duk);
		}

		// check for minimum API version
		if (duk_get_prop_string(g_duk, -1, "apiVersion")) {
			if (duk_is_number(g_duk, -1)) {
				if (duk_get_number(g_duk, -1) > get_api_version())
					goto is_unsupported;
			}
			duk_pop(g_duk);
		}

		// check API extensions
		if (duk_get_prop_string(g_duk, -1, "extensions")) {
			if (duk_is_array(g_duk, -1))
				for (i = 0; i < duk_get_length(g_duk, -1); ++i) {
					duk_get_prop_index(g_duk, -1, (duk_uarridx_t)i);
					extension_name = duk_get_string(g_duk, -1);
					duk_pop(g_duk);
					if (extension_name != NULL && !have_api_extension(extension_name))
						goto is_unsupported;
				}
			duk_pop(g_duk);
		}
		duk_pop(g_duk);
	}
	duk_pop(g_duk);
	return true;

is_unsupported:
	if (recommendation != NULL) {
		message = lstr_newf(
			"A feature needed by this game is not supported in %s.  You may need to use a later version of minisphere or a different engine to play this game."
			"\n\nThe game developer recommends using %s.", PRODUCT_NAME, recommendation);
	}
	else {
		message = lstr_newf(
			"A feature needed by this game is not supported in %s.  You may need to use a later version of minisphere or a different engine to play this game."
			"\n\nNo specific recommendation was provided by the game developer.", PRODUCT_NAME);
	}
	al_show_native_message_box(screen_display(g_screen), "Unsupported Engine", path_cstr(g_game_path), lstr_cstr(message), NULL, ALLEGRO_MESSAGEBOX_ERROR);
	return false;
}
