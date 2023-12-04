/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
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
 *  * Neither the name of Spherical nor the names of its contributors may be
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

#include "neosphere.h"

#if defined(NEOSPHERE_MNG_SUPPORT)
#include <libmng.h>
#endif
#include <zlib.h>

#include "api.h"
#include "audio.h"
#include "debugger.h"
#include "dispatch.h"
#include "dyad.h"
#include "event_loop.h"
#include "galileo.h"
#include "input.h"
#include "jsal.h"
#include "map_engine.h"
#include "module.h"
#include "pegasus.h"
#include "profiler.h"
#include "sockets.h"
#include "spriteset.h"
#include "vanilla.h"

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

game_t*   g_game = NULL;
double    g_idle_time = 0.0;
js_ref_t* g_main_object = NULL;
screen_t* g_screen = NULL;
uint32_t  g_tick_count = 0;

enum fullscreen_mode
{
	FULLSCREEN_AUTO,
	FULLSCREEN_ON,
	FULLSCREEN_OFF,
};

static void on_enqueue_js_job   (void);
static bool on_reject_promise   (void);
static void on_socket_idle      (void);
static bool initialize_engine   (void);
static void shutdown_engine     (void);
static bool find_startup_game   (path_t* *out_path);
static bool parse_command_line  (int argc, char* argv[], path_t* *out_game_path, int *out_fullscreen, int *out_frameskip, int *out_verbosity, ssj_mode_t *out_ssj_mode, bool *out_retro_mode, int *out_extras_offset);
static void print_banner        (bool want_copyright, bool want_deps);
static void print_usage         (void);
static void report_error        (const char* fmt, ...);
static void show_error_screen   (const char* message);

static int                  s_event_loop_version;
static ALLEGRO_EVENT_QUEUE* s_event_queue = NULL;
static path_t*              s_game_path = NULL;
static path_t*              s_last_game_path = NULL;
static bool                 s_restart_game = false;

static const char* const ERROR_TEXT[][2] =
{
	{ "*MUNCH*", "pretty sure you just got eaten by a pig" },
	{ "*crash*", "it's an 812-car pileup" },
	{ "so, um, a funny thing happened", "on the way to the boss" },
	{ "here's the deal", "the game encountered an error" },
	{ "this game sucks", "or maybe it's just the programmer" },
	{ "cows eat kitties but pigs don't eat cows", "they just get 'replaced' by them" },
	{ "hey look, a squirrel", "it's probably responsible for all this" },
	{ "sorry, it's just", "this is a trainwreck of a game" },
	{ "you better run, and you better hide", "'cause a big fat hawk just ate that guy" },
	{ "an exception was thrown", "neoSphere takes exception to sucky games" },
	{ "honk. HONK. honk. HONK. :o)", "there's a clown behind you" },
	{ "this game has over nine thousand errors", "what, 9000, there's no way that can be right" },
	{ "a wild ERROR appeared and used CRASH", "it's super effective" },
	{ "cat... alligator... why not adopt both?", "that can't possibly end badly" },
};

int
main(int argc, char* argv[])
{
	// HERE BE DRAGONS!
	// as the oldest function in the neoSphere codebase by definition, this has become
	// something of a hairball over time, and likely quite fragile.  don't be surprised if
	// attempting to edit it causes something to break. :o)

	int                  api_level;
	int                  api_version;
	lstring_t*           dialog_name;
	int                  error_column = 0;
	int                  error_line = 0;
	const char*          error_source;
	const char*          error_stack = NULL;
	const char*          error_text;
	const char*          error_url = NULL;
	jmp_buf              exit_label;
	ALLEGRO_FILECHOOSER* file_dialog;
	int                  fullscreen_mode;
	int                  game_args_offset;
	image_t*             icon;
	size2_t              resolution;
	jmp_buf              restart_label;
	bool                 retro_mode;
	const path_t*        script_path;
	ssj_mode_t           ssj_mode;
	int                  target_api_level;
	int                  use_frameskip;
	int                  use_verbosity;
#if defined(_WIN32)
	HANDLE               h_stdout;
	DWORD                handle_mode;
#endif

	int i;

#if defined(_WIN32)
	// enable ANSI colors (note: requires Windows 10+)
	h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleMode(h_stdout, &handle_mode);
	handle_mode |= 0x0004;  // ENABLE_VIRTUAL_TERMINAL_PROCESSING
	SetConsoleMode(h_stdout, handle_mode);
#endif

	// parse the command line
	if (parse_command_line(argc, argv, &s_game_path,
		&fullscreen_mode, &use_frameskip, &use_verbosity, &ssj_mode, &retro_mode,
		&game_args_offset))
	{
		if (ssj_mode == SSJ_ACTIVE)
			fullscreen_mode = FULLSCREEN_OFF;
		console_init(use_verbosity);
	}
	else {
		return EXIT_FAILURE;
	}

	print_banner(true, false);
	printf("\n");

	// print out options
	console_log(1, "parsing command line arguments");
	console_log(1, "    game path: %s", s_game_path != NULL ? path_cstr(s_game_path) : "<none provided>");
	console_log(1, "    retrograde API: %s", retro_mode ? "yes" : "no");
	console_log(1, "    fullscreen: %s",
		fullscreen_mode == FULLSCREEN_ON ? "on"
			: fullscreen_mode == FULLSCREEN_OFF ? "off"
			: "auto");
	console_log(1, "    frameskip limit: %d frames", use_frameskip);
	console_log(1, "    console verbosity: V%d", use_verbosity);
#if defined(NEOSPHERE_SPHERUN)
	console_log(1, "    debugger mode: %s",
		ssj_mode == SSJ_ACTIVE ? "active"
			: ssj_mode == SSJ_PASSIVE ? "passive"
			: "disabled");
#endif
	console_log(1, "");

	if (!initialize_engine())
		return EXIT_FAILURE;

	// set up jump points for script bailout
	console_log(1, "setting up jump points for longjmp");
	if (setjmp(exit_label) != 0) {
		// JS code called Exit(), user closed engine, etc.
		if (g_screen != NULL) {
			fullscreen_mode = screen_get_fullscreen(g_screen)
				? FULLSCREEN_ON : FULLSCREEN_OFF;
		}
		shutdown_engine();
		if (s_last_game_path != NULL) {  // returning from ExecuteGame()?
			if (!initialize_engine()) {
				path_free(s_last_game_path);
				return EXIT_FAILURE;
			}
			s_game_path = s_last_game_path;
			s_last_game_path = NULL;
		}
		else {
			return EXIT_SUCCESS;
		}
	}
	if (setjmp(restart_label) != 0) {
		// JS code called either RestartGame() or ExecuteGame()
		fullscreen_mode = screen_get_fullscreen(g_screen)
			? FULLSCREEN_ON : FULLSCREEN_OFF;
		shutdown_engine();
		console_log(1, "\nrestarting to launch new game");
		console_log(1, "    path: %s", path_cstr(s_game_path));
		screen_free(g_screen);
		if (!initialize_engine())
			return EXIT_FAILURE;
	}

	// locate the game manifest
	console_log(1, "searching for a game to launch");
	if (s_game_path == NULL) {
		// no game specified on command line, see if we have a startup game
		find_startup_game(&s_game_path);
	}
	if (s_game_path != NULL) {
		// user provided a path or startup game was found, attempt to load it
		g_game = game_open(path_cstr(s_game_path));
	}
	else {
		// no game path provided and no startup game, let user find one
		dialog_name = lstr_newf("%s - Select a Sphere game to launch", SPHERE_ENGINE_NAME);
		file_dialog = al_create_native_file_dialog(path_cstr(home_path()),
			lstr_cstr(dialog_name),
			"game.sgm;game.json;*.spk", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
		al_show_native_file_dialog(NULL, file_dialog);
		lstr_free(dialog_name);
		if (al_get_native_file_dialog_count(file_dialog) > 0) {
			path_free(s_game_path);
			s_game_path = path_new(al_get_native_file_dialog_path(file_dialog, 0));
			g_game = game_open(path_cstr(s_game_path));
			al_destroy_native_file_dialog(file_dialog);
		}
		else {
			// user cancelled the dialog box
			al_destroy_native_file_dialog(file_dialog);
			return EXIT_SUCCESS;
		}
	}

	if (g_game == NULL) {
		// if after all that, we still don't have a valid game_t pointer, bail out;
		// there's not much else we can do.
#if !defined(NEOSPHERE_SPHERUN)
		al_show_native_message_box(NULL, "Unable to Load Game", path_cstr(s_game_path),
			"Either the engine couldn't read the game manifest or a manifest file was not found.  Check that the directory listed above contains a valid Sphere game manifest file.\n\n"
			"For Sphere game developers:\nUsing SpheRun to start the game from the command line may yield more insight.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
#endif
		console_error("no valid Sphere manifest at '%s'", path_cstr(s_game_path));
		longjmp(exit_label, 1);
	}

	if (game_version(g_game) > SPHERE_API_VERSION || game_api_level(g_game) > SPHERE_API_LEVEL) {
#if !defined(NEOSPHERE_SPHERUN)
		al_show_native_message_box(NULL, "Unable to Start Game", game_name(g_game),
			"This game was developed for a newer version of the Sphere API than your version of neoSphere ("SPHERE_VERSION") supports.  "
			"You'll need to use a newer version of neoSphere to play this game.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
#else
		console_error("game requires at least Sphere v%d level %d API",
			game_version(g_game), game_api_level(g_game));
#endif
		longjmp(exit_label, 1);
	}

	// set up the render context ("screen") so we can draw stuff
	resolution = game_resolution(g_game);
	if (!(icon = image_load("@/icon.png")))
		icon = image_load("#/icon.png");
	g_screen = screen_new(game_name(g_game), icon, resolution, use_frameskip, game_default_font(g_game));
	if (g_screen == NULL) {
		al_show_native_message_box(NULL, "Unable to Create Render Context", "The engine couldn't create a render context.",
			"Your hardware may be too old to run neoSphere, or there could be a problem with the drivers on this system.  Check that your graphics drivers in particular are fully installed and up-to-date.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		return EXIT_FAILURE;
	}

	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	s_event_queue = al_create_event_queue();
	al_register_event_source(s_event_queue,
		al_get_display_event_source(screen_display(g_screen)));
	attach_input_display();
	kb_load_keymap();
	
	// in retrograde mode, only provide access to functions up to the targeted
	// API level, nothing newer.
	target_api_level = game_api_level(g_game);
	if (retro_mode) {
		api_version = game_version(g_game);
		api_level = target_api_level;
	}
	else {
		api_version = SPHERE_API_VERSION;
		api_level = SPHERE_API_LEVEL;
	}
	
	api_init(target_api_level <= 3);
	modules_init(target_api_level);
	if (api_version == 1 || (api_version == 2 && target_api_level < 4))
		vanilla_init();
	if (api_version >= 2)
		pegasus_init(api_level, target_api_level);

	// switch to fullscreen if necessary and initialize clipping
	if (fullscreen_mode == FULLSCREEN_ON || (fullscreen_mode == FULLSCREEN_AUTO && game_fullscreen(g_game)))
		screen_toggle_fullscreen(g_screen);

	// enable the SSj debug server, wait for a connection if requested.
#if defined(NEOSPHERE_SPHERUN)
	if (ssj_mode == SSJ_ACTIVE) {
		al_clear_to_color(al_map_rgba(0, 0, 0, 255));
		screen_draw_status(g_screen, "waiting for debugger...", mk_color(255, 255, 255, 255));
		al_flip_display();
		al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	}
	debugger_init(ssj_mode, false);

	if (ssj_mode == SSJ_OFF)
		profiler_init();
#endif

	s_event_loop_version = 1;
	s_restart_game = false;

	// evaluate the main script (v1) or module (v2)
	script_path = game_script_path(g_game);
	api_version = game_version(g_game);
	if (api_version >= 2) {
		// Sphere v2 mode: call the default export, if there is one.  the module ready
		// callback deals with actually calling it.
		if (path_extension_is(script_path, ".cjs") && target_api_level >= 4) {
			jsal_push_new_error(JS_TYPE_ERROR, "CommonJS main '%s' unsupported when targeting API 4+", path_cstr(script_path));
			goto on_js_error;
		}
		if (!module_eval(path_cstr(script_path), false))
			goto on_js_error;
	}
	else {
		// Sphere v1 mode: call the global game() function.  note that, in contrast to
		// Sphere 1.x, it's not an error if this function doesn't exist.
		if (!script_eval(path_cstr(script_path)))
			goto on_js_error;
		jsal_get_global_string("game");
		if (jsal_is_function(-1)) {
			for (i = game_args_offset; i < argc; ++i)
				jsal_push_string(argv[i]);
			if (!jsal_try_call(argc - game_args_offset))
				goto on_js_error;
		}
		jsal_pop(2);
	}

	// start up the event loop.  we can do this even in compatibility mode:
	// the event loop terminates when there are no pending jobs or promises to settle,
	// and neither one was available in Sphere 1.x.
	if (!events_run_main_loop())
		goto on_js_error;

#if defined(NEOSPHERE_SPHERUN)
	if (ssj_mode == SSJ_OFF)
		profiler_uninit();
#endif

	if (s_restart_game)
		longjmp(restart_label, 1);

	longjmp(exit_label, 1);

on_js_error:
	jsal_dup(-1);
	error_text = jsal_to_string(-1);
	if (jsal_is_error(-2)) {
		jsal_get_prop_string(-2, "column");
		jsal_get_prop_string(-3, "line");
		jsal_get_prop_string(-4, "source");
		jsal_get_prop_string(-5, "stack");
		jsal_get_prop_string(-6, "url");
		if (jsal_is_number(-5))
			error_column = jsal_get_int(-5) + 1;
		if (jsal_is_number(-4))
			error_line = jsal_get_int(-4) + 1;
		error_source = jsal_get_string(-3);
		error_stack = jsal_get_string(-2);
		error_url = jsal_get_string(-1);
		if (error_stack != NULL)
			error_text = error_stack;
	}
	if (error_url != NULL && error_line > 0) {
		fprintf(stderr, "GAME CRASH: error at '%s':%d:%d\n", error_url, error_line, error_column);
		fprintf(stderr, "%s\n", error_text);
		fprintf(stderr, "   %d %s\n", error_line, error_source);
	}
	else if (error_url != NULL) {
		fprintf(stderr, "GAME CRASH: error in '%s'\n", error_url);
		fprintf(stderr, "%s\n", error_text);
	}
	else {
		fprintf(stderr, "GAME CRASH: uncaught JavaScript exception\n");
		fprintf(stderr, "%s\n", error_text);
	}
	if (error_url != NULL && error_line > 0)
		jsal_push_sprintf("error at '%s':%d:%d\n\n%s\n", error_url, error_line, error_column, error_text);
	else if (error_url != NULL)
		jsal_push_sprintf("error in '%s'\n\n%s\n", error_url, error_text);
	else
		jsal_push_sprintf("uncaught JavaScript exception\n\n%s\n", error_text);
	show_error_screen(jsal_get_string(-1));
	longjmp(exit_label, 1);
}

void
sphere_abort(const char* message)
{
	// note: this won't exit immediately.  it merely cancels Dispatch jobs and
	//       disables the JavaScript VM so that control will fall through the event
	//       loop naturally.

	if (message != NULL)
		show_error_screen(message);
	dispatch_cancel_all(true, true);
	jsal_enable_vm(false);
}

void
sphere_change_game(const char* pathname)
{
	// store the old game path so we can relaunch when the chained game exits
	s_last_game_path = path_dup(game_path(g_game));

	s_game_path = path_new(pathname);
	sphere_restart();
}

void
sphere_exit(bool allow_game_change)
{
	if (!allow_game_change) {
		path_free(s_last_game_path);
		s_last_game_path = NULL;
	}
	dispatch_cancel_all(true, false);
	if (s_event_loop_version < 2)
		jsal_enable_vm(false);
}

void
sphere_heartbeat(bool in_event_loop, int api_version)
{
	// IMPORTANT: if `in_event_loop` is false, we MUST avoid doing anything that causes
	//            JavaScript code to run, otherwise bad things will happen.

	ALLEGRO_EVENT event;
#if defined(NEOSPHERE_SPHERUN)
	double        start_time;
#endif

#if defined(NEOSPHERE_SPHERUN)
	if (in_event_loop)
		start_time = al_get_time();
#endif

	sockets_update();

	if (in_event_loop) {
#if defined(NEOSPHERE_SPHERUN)
		debugger_update();
#endif
		s_event_loop_version = api_version;
		jsal_update(true);
	}

	update_input();
	audio_update();

	// check if the user closed the game window
	while (al_get_next_event(s_event_queue, &event)) {
		switch (event.type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			sphere_exit(false);
			break;
		}
	}

#if defined(NEOSPHERE_SPHERUN)
	// only count a heartbeat as idle time if it occurs as part of event loop processing.
	// most critically, screen_flip()--all of which is already idle time itself--generates benign
	// heartbeats while waiting for the next frame so we want to avoid counting that time twice.
	if (in_event_loop)
		g_idle_time += al_get_time() - start_time;
#endif
}

void
sphere_restart(void)
{
	s_restart_game = true;
	sphere_exit(true);
}

void
sphere_sleep(double time)
{
	double end_time;
	double time_left;

	end_time = al_get_time() + time;
	do {
		time_left = end_time - al_get_time();
		if (time_left > 0.0)
			al_wait_for_event_timed(s_event_queue, NULL, time_left);
		sphere_heartbeat(false, 0);
	} while (al_get_time() < end_time);
}

static void
on_enqueue_js_job(void)
{
	script_t* script;

	script = script_new_function(0);
	dispatch_defer(script, 0, JOB_ON_TICK, true);
}

static void
on_module_complete(const char* specifier, bool has_error)
{
	// temporary hack: if there's an exception, JSAL puts it on the stack.  but since it also converts any exceptions
	// we throw into uncaught promise rejections, we can just rethrow it.
	if (has_error)
		jsal_throw();

	// in Sphere v2 mode, the main script is loaded as a module.  check for a default export
	// and `new` it if possible, then call obj.start().
	if (strcmp(specifier, path_cstr(game_script_path(g_game))) == 0) {
		jsal_get_prop_string(0, "default");
		if (jsal_is_async_function(-1)) {
			// async functions aren't constructible, so call those normally.
			jsal_call(0);
		}
		else if (jsal_is_function(-1)) {
			jsal_construct(0);
			g_main_object = jsal_ref(-1);
			jsal_get_prop_string(-1, "start");
			jsal_pull(-2);
			if (jsal_vm_enabled() && jsal_is_function(-2))
				jsal_call_method(0);
		}
		jsal_pop(2);
	}
}

static bool
on_reject_promise(void)
{
	int         error_column;
	int         error_line;
	const char* error_source;
	const char* error_stack;
	const char* error_text;
	const char* error_url = NULL;

	jsal_dup(0);
	error_text = jsal_to_string(-1);
	if (jsal_is_error(1)) {
		jsal_get_prop_string(1, "column");
		jsal_get_prop_string(1, "line");
		jsal_get_prop_string(1, "source");
		jsal_get_prop_string(1, "stack");
		jsal_get_prop_string(1, "url");
		error_column = jsal_get_int(-5) + 1;
		error_line = jsal_get_int(-4) + 1;
		error_source = jsal_get_string(-3);
		error_stack = jsal_get_string(-2);
		error_url = jsal_get_string(-1);
		if (error_stack != NULL)
			error_text = error_stack;
	}
	if (error_url != NULL) {
		fprintf(stderr, "EXCEPTION: unhandled JS promise rejection at %s:%d:%d.\n", error_url, error_line, error_column);
		fprintf(stderr, "%s\n", error_text);
		fprintf(stderr, "   %d %s\n", error_line, error_source);
	}
	else {
		fprintf(stderr, "EXCEPTION: unhandled JS promise rejection.\n");
		fprintf(stderr, "%s\n", error_text);
	}
	return false;
}

static void
on_socket_idle(void)
{
	sphere_sleep(0.05);
}

static bool
initialize_engine(void)
{
	uint32_t al_version;

	srand(time(NULL));

	// initialize Allegro
	al_version = al_get_allegro_version();
	console_log(1, "initializing Allegro %u.%u.%u.%u",
		al_version >> 24, (al_version >> 16) & 0xFF, (al_version >> 8) & 0xFF,
		(al_version & 0xFF) - 1);
	al_set_app_name("neoSphere");
	if (!al_init())
		goto on_error;
	if (!al_init_native_dialog_addon())
		goto on_error;
	if (!al_init_primitives_addon())
		goto on_error;
	if (!al_init_image_addon())
		goto on_error;
	if (!al_init_font_addon())
		goto on_error;
	if (!al_init_ttf_addon())
		goto on_error;

	// initialize networking
	console_log(1, "initializing Dyad %s", dyad_getVersion());
	dyad_init();
	dyad_setUpdateTimeout(0.0);

	// initialize JavaScript
	console_log(1, "initializing JavaScript");
	if (!jsal_init())
		goto on_error;
	jsal_on_module_complete(on_module_complete);
	jsal_on_enqueue_job(on_enqueue_js_job);
	jsal_on_reject_promise(on_reject_promise);

	// initialize engine components
	dispatch_init();
	events_init();
	galileo_init();
	audio_init();
	initialize_input();
	sockets_init(on_socket_idle);
	spritesets_init();
	map_engine_init();
	scripts_init();

	return true;

on_error:
	al_show_native_message_box(NULL, "Unable to Start", "Does your car turn over in the morning?",
		"neoSphere was unable to initialize one or more engine components.  The engine cannot continue in this state and will now close.",
		NULL, ALLEGRO_MESSAGEBOX_ERROR);
	return false;
}

static void
shutdown_engine(void)
{
	jsal_enable_vm(true);
	
	kb_save_keymap();

#if defined(NEOSPHERE_SPHERUN)
	debugger_uninit();
#endif

	game_unref(g_game);
	g_game = NULL;

	map_engine_uninit();
	shutdown_input();
	scripts_uninit();
	sockets_uninit();

	console_log(1, "shutting down JavaScript");
	jsal_uninit();

	console_log(1, "shutting down Dyad");
	dyad_shutdown();

	spritesets_uninit();
	audio_uninit();
	galileo_uninit();
	events_uninit();
	dispatch_uninit();

	console_log(1, "shutting down Allegro");
	screen_free(g_screen);
	g_screen = NULL;
	if (s_event_queue != NULL)
		al_destroy_event_queue(s_event_queue);
	s_event_queue = NULL;
	al_uninstall_system();
}

static bool
find_startup_game(path_t* *out_path)
{
	const char* const MANIFEST_PATHS[] =
	{
		"startup/game.json",
		"dist/game.json",
		"startup/game.sgm",
		"dist/game.sgm",
	};

	ALLEGRO_FS_ENTRY* engine_dir;
	const char*       file_ext;
	const char*       filename;
	ALLEGRO_FS_ENTRY* fse;
	int               n_spk_files = 0;

	int i;

	// prefer a bundled startup game over an SPK if one exists
	for (i = 0; i < sizeof MANIFEST_PATHS / sizeof MANIFEST_PATHS[0]; ++i) {
		*out_path = path_rebase(path_new(MANIFEST_PATHS[i]), assets_path());
		if (al_filename_exists(path_cstr(*out_path)))
			return true;  // found a startup game
		path_free(*out_path);
	}

	// check for a single bundled SPK package
	*out_path = path_dup(assets_path());
	engine_dir = al_create_fs_entry(path_cstr(*out_path));
	al_open_directory(engine_dir);
	while ((fse = al_read_directory(engine_dir))) {
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
	*out_path = path_rebase(path_new("system/startup.spk"), assets_path());
	if (al_filename_exists(path_cstr(*out_path)))
		return true;
	path_free(*out_path);
	*out_path = path_rebase(path_new("../share/sphere/system/startup.spk"), assets_path());
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
	path_t* *out_game_path, int *out_fullscreen, int *out_frameskip,
	int *out_verbosity, ssj_mode_t *out_ssj_mode, bool *out_retro_mode,
	int *out_extras_offset)
{
	bool parse_options = true;

	int i, j;

	// establish default settings
	*out_extras_offset = argc;
	*out_fullscreen = FULLSCREEN_AUTO;
	*out_frameskip = 20;
	*out_game_path = NULL;
	*out_retro_mode = false;
	*out_ssj_mode = SSJ_PASSIVE;
	*out_verbosity = 0;

	// process command line arguments
	for (i = 1; i < argc; ++i) {
		if (strstr(argv[i], "--") == argv[i] && parse_options) {
			if (strcmp(argv[i], "--") == 0)
				parse_options = false;
			else if (strcmp(argv[i], "--frameskip") == 0) {
				if (++i >= argc)
					goto missing_argument;
				*out_frameskip = atoi(argv[i]);
			}
			else if (strcmp(argv[i], "--fullscreen") == 0) {
				*out_fullscreen = FULLSCREEN_ON;
			}
			else if (strcmp(argv[i], "--windowed") == 0) {
				*out_fullscreen = FULLSCREEN_OFF;
			}
#if defined(NEOSPHERE_SPHERUN)
			else if (strcmp(argv[i], "--version") == 0) {
				print_banner(true, true);
				return false;
			}
			else if (strcmp(argv[i], "--help") == 0) {
				print_usage();
				return false;
			}
			else if (strcmp(argv[i], "--debug") == 0) {
				*out_ssj_mode = SSJ_ACTIVE;
			}
			else if (strcmp(argv[i], "--retro") == 0) {
				*out_retro_mode = true;
			}
			else if (strcmp(argv[i], "--profile") == 0) {
				*out_ssj_mode = SSJ_OFF;
			}
			else if (strcmp(argv[i], "--verbose") == 0) {
				if (++i >= argc)
					goto missing_argument;
				*out_verbosity = atoi(argv[i]);
			}
			else {
				report_error("unrecognized option '%s'\n", argv[i]);
				return false;
			}
#else
			else if (strcmp(argv[i], "--verbose") == 0) {
				++i;
			}
#endif
		}
		else if (argv[i][0] == '-' && parse_options) {
			for (j = 1; j < (int)strlen(argv[i]); ++j) {
				switch (argv[i][j]) {
				case '0': case '1': case '2': case '3': case '4':
					*out_verbosity = argv[i][j] - '0';
					break;
				case 'd':
					*out_ssj_mode = SSJ_ACTIVE;
					break;
				case 'h':
					print_usage();
					return false;
				case 'p':
					*out_ssj_mode = SSJ_OFF;
					break;
				case 'r':
					*out_retro_mode = true;
					break;
				case 'v':
					print_banner(true, true);
					return false;
				default:
					report_error("unrecognized option '-%c'\n", argv[i][j]);
					return false;
				}
			}
		}
		else {
			if (*out_game_path == NULL) {
				*out_game_path = path_new(argv[i]);
				if (!path_resolve(*out_game_path, NULL)) {
					report_error("pathname not found '%s'\n", path_cstr(*out_game_path));
					path_free(*out_game_path);
					*out_game_path = NULL;
					return false;
				}
				*out_extras_offset = i + 1;
				parse_options = false;
			}
		}
	}

#if defined(NEOSPHERE_SPHERUN)
	if (*out_game_path == NULL) {
		print_usage();
		return false;
	}
#endif

	return true;

missing_argument:
	report_error("missing argument for option '%s'\n", argv[i - 1]);
	return false;
}

static void
print_banner(bool want_copyright, bool want_deps)
{
	char*    al_version;
	uint32_t al_version_id;

	printf("%s %s JS game engine\n", SPHERE_ENGINE_NAME, SPHERE_VERSION);
	if (want_copyright) {
		printf("a lightweight JavaScript-powered game engine\n");
		printf("(c) 2024 Fat Cerberus\n");
	}
	if (want_deps) {
		al_version_id = al_get_allegro_version();
		al_version = strnewf("%d.%d.%d.%d", al_version_id >> 24,
			(al_version_id >> 16) & 0xFF, (al_version_id >> 8) & 0xFF,
			(al_version_id & 0xFF) - 1);
		printf("\n");
		printf("    Allegro: v%-8s\n", al_version);
#if defined(NEOSPHERE_MNG_SUPPORT)
		printf("     libmng: v%s\n", mng_version_text());
#endif
		printf("     Dyad.c: v%-8s\n", dyad_getVersion());
		printf("       zlib: v%s\n", zlibVersion());
		free(al_version);
	}
}

static void
print_usage(void)
{
	print_banner(true, false);
	printf("\n");
	printf("USAGE:\n");
	printf("   spherun [--fullscreen | --windowed] [--frameskip <n>] [--debug | --profile]\n");
	printf("           [--retro] [--verbose <n>] <game_path> [<game_args>]                \n");
	printf("\n");
	printf("OPTIONS:\n");
	printf("       --fullscreen   Start the game in fullscreen mode                       \n");
	printf("       --windowed     Start the game in windowed mode (default for SpheRun)   \n");
	printf("       --frameskip    Set the maximum number of consecutive frames to skip    \n");
	printf("   -d  --debug        Wait 30 seconds for an SSj/Ki debugger to connect       \n");
	printf("   -p  --profile      Enable the profiler for this session (disables debugger)\n");
	printf("   -r  --retro        Emulate the game's targeted API level (retrograde mode) \n");
	printf("       --verbose      Set the engine's verbosity level from 0 to 4            \n");
	printf("   -v  --version      Show which version of neoSphere is installed            \n");
	printf("   -h  --help         Show this help text                                     \n");
	printf("\n");
	printf("NOTE:\n");
	printf("   spherun(1) is used to execute Sphere games in a development environment. If\n");
	printf("   your intent is simply to play a game, use neosphere(1) instead.           \n");
}

static void
report_error(const char* fmt, ...)
{
	va_list ap;

	lstring_t* error_text;

	va_start(ap, fmt);
	error_text = lstr_vnewf(fmt, ap);
	va_end(ap);
#if defined(NEOSPHERE_SPHERUN)
	fprintf(stderr, "spherun: ERROR: %s", lstr_cstr(error_text));
#else
	al_show_native_message_box(NULL,
		"neoSphere", "An error occurred starting the engine.", lstr_cstr(error_text),
		NULL, ALLEGRO_MESSAGEBOX_ERROR);
#endif
	lstr_free(error_text);
}

static void
show_error_screen(const char* message)
{
	image_t*               backbuffer;
	blend_op_t*            blend_op;
	bool                   debugging;
	wraptext_t*            error_info;
	font_t*                font;
	bool                   is_copied = false;
	bool                   is_finished;
	int                    frames_till_close;
	ALLEGRO_KEYBOARD_STATE keyboard;
	const char*            line_text;
	int                    num_lines;
	transform_t*           projection;
	size2_t                resolution;
	const char*            subtitle;
	const char*            title;
	int                    title_index;

	int i;

	title_index = rand() % (sizeof ERROR_TEXT / sizeof(const char*) / 2);
	title = ERROR_TEXT[title_index][0];
	subtitle = ERROR_TEXT[title_index][1];
	if (!(font = game_default_font(g_game)))
		goto show_error_box;

	// word-wrap the error message to fit inside the error box
	resolution = screen_size(g_screen);
	if (!(error_info = font_wrap(font, message, resolution.width - 84)))
		goto show_error_box;
	num_lines = wraptext_len(error_info);

	// reset the projection to pixel-perfect orthographic, the blend and depth-test modes to default, switch to
	// the default shader, and disable clipping.  no JavaScript code will execute past this point, so we won't
	// step on any toes.
	screen_unskip_frame(g_screen);
	backbuffer = screen_backbuffer(g_screen);
	blend_op = blend_op_new_sym(BLEND_OP_ADD, BLEND_ALPHA, BLEND_INV_ALPHA);
	projection = transform_new();
	transform_orthographic(projection, 0, 0, resolution.width, resolution.height, -1.0f, 1.0f);
	image_render_to(backbuffer, NULL);
	shader_use(NULL, true);
	image_set_scissor(backbuffer, mk_rect(0, 0, resolution.width, resolution.height));
	image_set_blend_op(backbuffer, blend_op);
	image_set_depth_op(backbuffer, DEPTH_PASS);
	image_set_transform(screen_backbuffer(g_screen), projection);
	blend_op_unref(blend_op);
	transform_unref(projection);

	is_finished = false;
	debugging = debugger_attached();
	frames_till_close = 30;
	while (!is_finished) {
		al_draw_filled_rounded_rectangle(32, 48, resolution.width - 32, resolution.height - 32, 5, 5, al_map_rgba(48, 16, 16, 255));
		font_set_mask(font, mk_color(0, 0, 0, 255));
		font_draw_text(font, resolution.width / 2 + 1, 11, TEXT_ALIGN_CENTER, title);
		font_draw_text(font, resolution.width / 2 + 1, 23, TEXT_ALIGN_CENTER, subtitle);
		font_set_mask(font, mk_color(192, 192, 192, 255));
		font_draw_text(font, resolution.width / 2, 10, TEXT_ALIGN_CENTER, title);
		font_draw_text(font, resolution.width / 2, 22, TEXT_ALIGN_CENTER, subtitle);
		for (i = 0; i < num_lines; ++i) {
			line_text = wraptext_line(error_info, i);
			font_set_mask(font, mk_color(16, 0, 0, 255));
			font_draw_text(font,
				resolution.width / 2 + 1, 59 + i * font_height(font),
				TEXT_ALIGN_CENTER, line_text);
			font_set_mask(font, mk_color(192, 192, 192, 255));
			font_draw_text(font,
				resolution.width / 2, 58 + i * font_height(font),
				TEXT_ALIGN_CENTER, line_text);
		}
		if (frames_till_close <= 0) {
			font_set_mask(font, mk_color(255, 255, 192, 255));
			font_draw_text(font,
				resolution.width / 2, resolution.height - 10 - font_height(font),
				TEXT_ALIGN_CENTER,
				is_copied ? "[space]/[esc] to close" : "[ctrl+c] to copy  [space]/[esc] to close");
		}
		debugger_update();
		screen_flip(g_screen, 30, true);
		if (frames_till_close <= 0) {
			al_get_keyboard_state(&keyboard);
			is_finished = al_key_down(&keyboard, ALLEGRO_KEY_ESCAPE)
				|| al_key_down(&keyboard, ALLEGRO_KEY_SPACE)
				|| debugging && !debugger_attached();

			// if Ctrl+C is pressed, copy the error message and location to clipboard
			if ((al_key_down(&keyboard, ALLEGRO_KEY_LCTRL) || al_key_down(&keyboard, ALLEGRO_KEY_RCTRL))
				&& al_key_down(&keyboard, ALLEGRO_KEY_C))
			{
				is_copied = true;
				al_set_clipboard_text(screen_display(g_screen), message);
			}
		}
		else {
			--frames_till_close;
		}
	}
	wraptext_free(error_info);

	return;

show_error_box:
	// use a native message box only as a last resort
	al_show_native_message_box(NULL, "Script Error",
		"neoSphere encountered an error during JavaScript execution.",
		message, NULL, ALLEGRO_MESSAGEBOX_ERROR);
}
