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
static void draw_status_message (const char* text);
static bool find_startup_game   (ALLEGRO_PATH* *out_path);

static void on_duk_fatal (duk_context* ctx, duk_errcode_t code, const char* msg);

ALLEGRO_DISPLAY*     g_display = NULL;
duk_context*         g_duk = NULL;
ALLEGRO_EVENT_QUEUE* g_events = NULL;
sandbox_t*           g_fs = NULL;
ALLEGRO_PATH*        g_game_path = NULL;
char*                g_last_game_path = NULL;
float                g_scale_x = 1.0;
float                g_scale_y = 1.0;
kev_file_t*          g_sys_conf;
font_t*              g_sys_font = NULL;
int                  g_res_x, g_res_y;

static rect_t  s_clip_rect;
static bool    s_conserve_cpu = true;
static int     s_current_fps;
static int     s_current_game_fps;
static int     s_frame_skips;
static bool    s_is_fullscreen = false;
static jmp_buf s_jmp_exit;
static jmp_buf s_jmp_restart;
static double  s_last_flip_time;
static int     s_max_frameskip = 5;
static double  s_next_fps_poll_time;
static double  s_next_frame_time;
static int     s_num_flips;
static int     s_num_frames;
bool           s_skipping_frame = false;
static bool    s_show_fps = false;
static bool    s_want_snapshot = false;

static const char* const ERROR_TEXT[][2] =
{
	{ "*munch*", "A hunger-pig just devoured your game!" },
	{ "*CRASH!*", "It's an 812-car pileup!" },
	{ "So, um... a funny thing happened...", "...on the way to the boss..." },
	{ "Here's the deal.", "The game encountered an error." },
	{ "This game sucks!", "Or maybe it's just the programmer..." },
	{ "Cows eat kitties. Pigs don't eat cows.", "They just get \"replaced\" by them." },
	{ "Hey look, a squirrel!", "I wonder if IT'S responsible for this." },
	{ "Sorry. It's just...", "...well, this is a trainwreck of a game." },
	{ "You better run, and you better hide...", "...'cause a big fat hawk just ate that guy!" },
	{ "An exception was thrown.", "minisphere takes exception to sucky games." },
	{ "honk. HONK. honk. HONK. :o)", "There's a clown behind you." },
};

int
main(int argc, char* argv[])
{
	ALLEGRO_PATH*        browse_path;
	lstring_t*           dialog_name;
	duk_errcode_t        err_code;
	const char*          err_msg;
	ALLEGRO_FILECHOOSER* file_dlg;
	const char*          filename;
	ALLEGRO_FS_ENTRY*    games_dir;
	const char*          games_dirname;
	char*                game_path = NULL;
	image_t*             icon;
	int                  line_num;
	int                  max_skips;
	const path_t*        script_path;
	ALLEGRO_TRANSFORM    trans;
	int                  verbosity;
	bool                 want_debug = false;
	char                 *p_strtol;

	int i;

	printf("%s %s\n", ENGINE_NAME, sizeof(void*) == 4 ? "x86" : "x64");
	printf("A lightweight Sphere-compatible game engine\n");
	printf("(c) 2015 Fat Cerberus\n\n");
	
	// parse the command line, rejecting any unrecognized options
	printf("Parsing command line\n");
	if (argc == 2 && argv[1][0] != '-') {
		// single non-switch argument passed, assume it's a game path
		game_path = strdup(argv[1]);
	}
	else {
		// more than one argument, perform full command line parsing
		for (i = 1; i < argc; ++i) {
			if ((strcmp(argv[i], "--game") == 0
				|| strcmp(argv[i], "-game") == 0 || strcmp(argv[i], "-package") == 0)
				&& i < argc - 1)
			{
				// '-game' and '-package' options supported for backward compatibility with
				// Sphere 1.x. Note that they are functionality identical; new_sandbox() will
				// automatically detect the correct type.
				++i;
				if (game_path == NULL)
					game_path = strdup(argv[i]);
				else {
					printf("  More than one game was passed on command line\n");
					return EXIT_FAILURE;
				}
			}
			else if (strcmp(argv[i], "--debug") == 0) {
				#ifndef MINISPHERE_REDIST
				want_debug = true;
				#else
				al_show_native_message_box(NULL,
					"No Debugging Support", "Debugging is not enabled for this build.",
					"'--debug' was passed on the command line. The redistributable engine is not enabled for debugging. Please use minisphere Console if you need the debugger.",
					NULL, ALLEGRO_MESSAGEBOX_ERROR);
				return EXIT_FAILURE;
				#endif
			}
			else if (strcmp(argv[i], "--log-level") == 0 && i < argc - 1) {
				errno = 0; verbosity = strtol(argv[i + 1], &p_strtol, 10);
				if (errno != ERANGE && *p_strtol == '\0')
					set_log_verbosity(verbosity);
				else {
					printf("  Invalid logging level '%s'\n", argv[i + 1]);
					return EXIT_FAILURE;
				}
			}
			else if (strcmp(argv[i], "--frameskip") == 0 && i < argc - 1) {
				errno = 0; max_skips = strtol(argv[i + 1], &p_strtol, 10);
				if (errno != ERANGE && *p_strtol == '\0')
					set_max_frameskip(max_skips);
				else {
					printf("  Invalid frameskip '%s'\n", argv[i + 1]);
					return EXIT_FAILURE;
				}
			}
			else if (strcmp(argv[i], "--no-throttle") == 0)
				s_conserve_cpu = false;
			else if (strcmp(argv[i], "--fullscreen") == 0)
				s_is_fullscreen = true;
			else if (strcmp(argv[i], "--windowed") == 0)
				s_is_fullscreen = false;
			else {
				printf("  Error parsing command line token '%s'\n", argv[i]);
				return EXIT_FAILURE;
			}
		}
	}
	
	// print out options
	printf("  Game path: %s\n", game_path != NULL ? game_path : "<none provided>");
	printf("  Frameskip limit: %i frames\n", s_max_frameskip);
	printf("  CPU throttle: %s\n", s_conserve_cpu ? "ON" : "OFF");
	printf("  Console verbosity: V%i\n", get_log_verbosity());
	
	#ifndef MINISPHERE_REDIST
	printf("  Debugger mode: %s\n", want_debug ? "Active" : "Passive");
	#endif
	
	printf("\n");

	if (!initialize_engine())
		return EXIT_FAILURE;

	// set up jump points for script bailout
	console_log(1, "Setting up jump points for longjmp");
	if (setjmp(s_jmp_exit)) {  // user closed window, script called Exit(), etc.
		shutdown_engine();
		if (g_last_game_path != NULL) {  // returning from ExecuteGame()?
			initialize_engine();
			g_game_path = al_create_path(g_last_game_path);
			free(g_last_game_path);
			g_last_game_path = NULL;
		}
		else {
			return EXIT_SUCCESS;
		}
	}
	if (setjmp(s_jmp_restart)) {  // script called RestartGame() or ExecuteGame()
		console_log(0, "Restarting to launch new game");
		game_path = strdup(al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP));
		console_log(1, "  Game: %s", game_path);
		shutdown_engine();
		printf("\n");
		initialize_engine();
		g_game_path = al_create_path(game_path);
		free(game_path);
	}

	// locate the game manifest
	console_log(0, "Looking for a game to launch");
	g_game_path = game_path != NULL ? al_create_path(game_path) : NULL;
	if (g_game_path == NULL)
		// no game specified on command line, see if we have a startup game
		find_startup_game(&g_game_path);
	if (g_game_path != NULL)
		// user provided a path or startup game was found, attempt to load it
		g_fs = new_sandbox(al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP));
	else {
		// no game path provided and no startup game, let user find manifest
		browse_path = al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
		al_append_path_component(browse_path, "Sphere Games");
		games_dirname = al_path_cstr(browse_path, ALLEGRO_NATIVE_PATH_SEP);
		al_make_directory(games_dirname);
		games_dir = al_create_fs_entry(games_dirname);
		if (!al_open_directory(games_dir))
			games_dirname = NULL;
		al_close_directory(games_dir);
		al_destroy_fs_entry(games_dir);
		dialog_name = lstr_newf("%s - Where is your Sphere game?", ENGINE_NAME);
		file_dlg = al_create_native_file_dialog(games_dirname, lstr_cstr(dialog_name),
			"game.sgm;game.s2gm;*.spk", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
		al_show_native_file_dialog(NULL, file_dlg);
		lstr_free(dialog_name);
		al_destroy_path(browse_path);
		if (al_get_native_file_dialog_count(file_dlg) > 0) {
			al_destroy_path(g_game_path);
			g_game_path = al_create_path(al_get_native_file_dialog_path(file_dlg, 0));
			g_fs = new_sandbox(al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP));
			al_destroy_native_file_dialog(file_dlg);
		}
		else {
			// user clicked Cancel; as this is a valid action, we return
			// success, not failure.
			al_destroy_native_file_dialog(file_dlg);
			return EXIT_SUCCESS;
		}
	}
	if (g_fs == NULL) {
		// if after all that, we still don't have a valid sandbox pointer, bail out
		// because there's not much else we can do.
		al_show_native_message_box(NULL, "Unable to Load Game",
			al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP),
			"minisphere was unable to load the game manifest or it was not found.  Check to make sure the directory above exists and contains a valid Sphere game.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		
		// in case we reached this point via ExecuteGame()
		exit_game(false);
	}
	
	get_sgm_metrics(g_fs, &g_res_x, &g_res_y);

	// set up engine and create display window
	console_log(1, "Creating render window");
	g_scale_x = g_scale_y = (g_res_x <= 400 && g_res_y <= 300) ? 2.0 : 1.0;
	al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_PROGRAMMABLE_PIPELINE);
	if (!(g_display = al_create_display(g_res_x * g_scale_x, g_res_y * g_scale_y))) {
		al_show_native_message_box(NULL, "Unable to Create Display", "minisphere was unable to create a display window.",
			"A display window is required for rendering. The engine cannot run without it and will now close.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		return EXIT_FAILURE;
	}
	al_set_new_bitmap_flags(ALLEGRO_NO_PREMULTIPLIED_ALPHA | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
	if (!(icon = load_image("~sgm/icon.png", true)))
		icon = load_image("~sys/icon.png", true);
	rescale_image(icon, 32, 32);
	al_set_new_bitmap_flags(ALLEGRO_NO_PREMULTIPLIED_ALPHA);
	al_identity_transform(&trans);
	al_scale_transform(&trans, g_scale_x, g_scale_y);
	al_use_transform(&trans);
	if (icon != NULL)
		al_set_display_icon(g_display, get_image_bitmap(icon));
	free_image(icon);
	al_set_window_title(g_display, get_sgm_name(g_fs));
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	g_events = al_create_event_queue();
	al_register_event_source(g_events, al_get_display_event_source(g_display));
	attach_input_display();
	load_key_map();

	// initialize shader support
	initialize_shaders();
	
	// attempt to locate and load system font
	console_log(1, "Loading system font");
	if (g_sys_conf != NULL) {
		filename = read_string_rec(g_sys_conf, "Font", "system.rfn");
		g_sys_font = load_font(syspath(filename));
	}
	if (g_sys_font == NULL) {
		al_show_native_message_box(g_display, "No System Font Available", "A system font is required.",
			"minisphere was unable to locate the system font or it failed to load.  As a usable font is necessary for correct operation, minisphere will now close.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		return EXIT_FAILURE;
	}

	// switch to fullscreen if necessary and initialize clipping
	if (s_is_fullscreen)
		toggle_fullscreen();
	set_clip_rectangle(new_rect(0, 0, g_res_x, g_res_y));

	// display loading message, scripts may take a bit to compile
	if (want_debug) {
		al_clear_to_color(al_map_rgba(0, 0, 0, 255));
		draw_status_message("waiting for debugger...");
		al_flip_display();
		al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	}

	// enable debugging support
	#ifndef MINISPHERE_REDIST
	initialize_debugger(want_debug, false);
	#endif

	// display loading message, scripts may take a bit to compile
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	draw_status_message("starting up...");
	al_flip_display();
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	
	// load startup script
	console_log(0, "Starting up game");
	al_hide_mouse_cursor(g_display);
	script_path = get_sgm_script_path(g_fs);
	if (!try_evaluate_file(path_cstr(script_path), true))
		goto on_js_error;
	duk_pop(g_duk);

	// initialize timing variables
	s_next_fps_poll_time = al_get_time() + 1.0;
	s_num_frames = s_num_flips = 0;
	s_current_fps = s_current_game_fps = 0;
	s_next_frame_time = s_last_flip_time = al_get_time();

	// call game() function in script
	duk_push_global_object(g_duk);
	duk_get_prop_string(g_duk, -1, "game");
	if (!duk_is_callable(g_duk, -1)) {
		script_path = get_sgm_script_path(g_fs);
		duk_push_error_object_raw(g_duk, DUK_ERR_SYNTAX_ERROR,
			path_cstr(script_path), 0, "game() is not defined");
		goto on_js_error;
	}
	if (duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS)
		goto on_js_error;
	duk_pop(g_duk);
	duk_pop(g_duk);
	
	exit_game(false);

on_js_error:
	err_code = duk_get_error_code(g_duk, -1);
	duk_dup(g_duk, -1);
	err_msg = duk_safe_to_string(g_duk, -1);
	al_show_mouse_cursor(g_display);
	duk_get_prop_string(g_duk, -2, "lineNumber");
	line_num = duk_get_int(g_duk, -1);
	duk_pop(g_duk);
	duk_get_prop_string(g_duk, -2, "fileName");
	filename = duk_get_string(g_duk, -1);
	if (filename != NULL) {
		fprintf(stderr, "JS Error: %s:%i - %s\n", filename, line_num, err_msg);
		if (err_msg[strlen(err_msg) - 1] != '\n')
			duk_push_sprintf(g_duk, "'%s' (line: %i)\n\n%s", filename, line_num, err_msg);
		else
			duk_push_string(g_duk, err_msg);
	}
	else {
		fprintf(stderr, "JS Error: %s\n", err_msg);
		duk_push_string(g_duk, err_msg);
	}
	duk_fatal(g_duk, err_code, duk_get_string(g_duk, -1));
}

bool
is_skipped_frame(void)
{
	return s_skipping_frame;
}

rect_t
get_clip_rectangle(void)
{
	return s_clip_rect;
}

int
get_max_frameskip(void)
{
	return s_max_frameskip;
}

void
set_clip_rectangle(rect_t clip)
{
	s_clip_rect = clip;
	clip.x1 *= g_scale_x; clip.y1 *= g_scale_y;
	clip.x2 *= g_scale_x; clip.y2 *= g_scale_y;
	al_set_clipping_rectangle(clip.x1, clip.y1, clip.x2 - clip.x1, clip.y2 - clip.y1);
}

void
set_max_frameskip(int frames)
{
	s_max_frameskip = frames >= 0 ? frames : 0;
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
	
	#ifndef MINISPHERE_REDIST
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

void
flip_screen(int framerate)
{
	char              filename[50];
	char              fps_text[20];
	bool              is_backbuffer_valid;
	ALLEGRO_STATE     old_state;
	ALLEGRO_PATH*     path;
	const char*       pathstr;
	ALLEGRO_BITMAP*   snapshot;
	double            time_left;
	ALLEGRO_TRANSFORM trans;
	int               x, y;

	if (al_get_time() >= s_next_fps_poll_time) {
		s_current_fps = s_num_flips;
		s_current_game_fps = s_num_frames;
		s_num_flips = 0;
		s_num_frames = 0;
		s_next_fps_poll_time = al_get_time() + 1.0;
	}
	is_backbuffer_valid = !s_skipping_frame;
	if (is_backbuffer_valid) {
		if (s_want_snapshot) {
			al_store_state(&old_state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
			al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA);
			snapshot = al_clone_bitmap(al_get_backbuffer(g_display));
			al_restore_state(&old_state);
			path = al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
			al_append_path_component(path, "minisphere");
			al_append_path_component(path, "Screenshots");
			al_make_directory(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
			do {
				sprintf(filename, "SS_%s.png", rng_string(10));
				al_set_path_filename(path, filename);
				pathstr = al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP);
			} while (al_filename_exists(pathstr));
			al_save_bitmap(pathstr, snapshot);
			al_destroy_bitmap(snapshot);
			al_destroy_path(path);
			s_want_snapshot = false;
		}
		if (s_show_fps) {
			if (framerate > 0) sprintf(fps_text, "%i/%i fps", s_current_fps, s_current_game_fps);
			else sprintf(fps_text, "%i fps", s_current_fps);
			al_identity_transform(&trans);
			al_use_transform(&trans);
			x = al_get_display_width(g_display) - 108;
			y = 8;
			al_draw_filled_rounded_rectangle(x, y, x + 100, y + 16, 4, 4, al_map_rgba(0, 0, 0, 128));
			draw_text(g_sys_font, rgba(0, 0, 0, 128), x + 51, y + 3, TEXT_ALIGN_CENTER, fps_text);
			draw_text(g_sys_font, rgba(255, 255, 255, 128), x + 50, y + 2, TEXT_ALIGN_CENTER, fps_text);
			al_scale_transform(&trans, g_scale_x, g_scale_y);
			al_use_transform(&trans);
		}
		al_flip_display();
		s_last_flip_time = al_get_time();
		s_frame_skips = 0;
		++s_num_flips;
	}
	else {
		++s_frame_skips;
	}
	if (framerate > 0) {
		s_skipping_frame = s_frame_skips < s_max_frameskip && s_last_flip_time > s_next_frame_time;
		do {
			time_left = s_next_frame_time - al_get_time();
			if (s_conserve_cpu && time_left > 0.001)  // engine may stall with < 1ms timeout
				al_wait_for_event_timed(g_events, NULL, time_left);
			do_events();
		} while (al_get_time() < s_next_frame_time);
		if (!is_backbuffer_valid && !s_skipping_frame)  // did we just finish skipping frames?
			s_next_frame_time = al_get_time() + 1.0 / framerate;
		else
			s_next_frame_time += 1.0 / framerate;
	}
	else {
		s_skipping_frame = false;
		do_events();
		s_next_frame_time = al_get_time();
	}
	++s_num_frames;
	if (!s_skipping_frame) al_clear_to_color(al_map_rgba(0, 0, 0, 255));
}

noreturn
restart_engine(void)
{
	longjmp(s_jmp_restart, 1);
}

void
take_screenshot(void)
{
	s_want_snapshot = true;
}

void
toggle_fps_display(void)
{
	s_show_fps = !s_show_fps;
}

void
toggle_fullscreen(void)
{
	int                  flags;
	ALLEGRO_MONITOR_INFO monitor;
	ALLEGRO_TRANSFORM    transform;

	flags = al_get_display_flags(g_display);
	if (flags & ALLEGRO_FULLSCREEN_WINDOW) {
		// switch from fullscreen to windowed
		s_is_fullscreen = false;
		al_set_display_flag(g_display, ALLEGRO_FULLSCREEN_WINDOW, false);
		g_scale_x = g_scale_y = (g_res_x <= 400 || g_res_y <= 300) ? 2.0 : 1.0;
		
		// we have to resize and reposition manually because Allegro is bugged
		// and gives us a 0x0 window when switching out of fullscreen
		al_resize_display(g_display, g_res_x * g_scale_x, g_res_y * g_scale_y);
		al_get_monitor_info(0, &monitor);
		al_set_window_position(g_display,
			(monitor.x1 + monitor.x2) / 2 - g_res_x * g_scale_x / 2,
			(monitor.y1 + monitor.y2) / 2 - g_res_y * g_scale_y / 2);
		
		// strange but true: this fixes an Allegro bug where OpenGL displays get
		// stuck in limbo when switching out of fullscreen.
		al_destroy_bitmap(al_clone_bitmap(al_get_backbuffer(g_display)));
	}
	else {
		// switch from windowed to fullscreen
		s_is_fullscreen = true;
		al_set_display_flag(g_display, ALLEGRO_FULLSCREEN_WINDOW, true);
		g_scale_x = al_get_display_width(g_display) / (float)g_res_x;
		g_scale_y = al_get_display_height(g_display) / (float)g_res_y;
	}
	al_identity_transform(&transform);
	al_scale_transform(&transform, g_scale_x, g_scale_y);
	al_use_transform(&transform);
}

void
unskip_frame(void)
{
	s_skipping_frame = false;
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
}

static void
on_duk_fatal(duk_context* ctx, duk_errcode_t code, const char* msg)
{
	wraptext_t*            error_info;
	bool                   is_finished;
	int                    frames_till_close;
	ALLEGRO_KEYBOARD_STATE keyboard;
	const char*            line_text;
	int                    num_lines;
	const char*            subtitle;
	const char*            title;
	int                    title_index;

	int i;
	
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
	unskip_frame();
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
				TEXT_ALIGN_CENTER, "Press space bar or [Esc] to close.");
		}
		flip_screen(30);
		if (frames_till_close <= 0) {
			al_get_keyboard_state(&keyboard);
			is_finished = al_key_down(&keyboard, ALLEGRO_KEY_ESCAPE)
				|| al_key_down(&keyboard, ALLEGRO_KEY_SPACE);
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
	al_show_native_message_box(g_display, "Script Error",
		"minisphere encountered an error during game execution.", 
		msg, NULL, ALLEGRO_MESSAGEBOX_ERROR);
	shutdown_engine();
	exit(EXIT_SUCCESS);
}

static bool
initialize_engine(void)
{
	srand(time(NULL));
	
	// initialize Allegro
	console_log(0, "Initializing Allegro");
	if (!al_init())
		goto on_error;
	if (!al_init_native_dialog_addon()) goto on_error;
	if (!al_init_primitives_addon()) goto on_error;
	if (!al_init_image_addon()) goto on_error;

	// initialize networking
	console_log(0, "Initializing Dyad");
	dyad_init();
	dyad_setUpdateTimeout(0.001);

	// load system configuraton
	console_log(1, "Loading system configuration");
	g_sys_conf = open_kev_file("~sys/system.ini");

	initialize_async();
	initialize_rng();
	initialize_galileo();
	initialize_audialis();
	initialize_input();
	initialize_spritesets();
	initialize_map_engine();

	// initialize JavaScript
	console_log(0, "Initializing JavaScript");
	if (!(g_duk = duk_create_heap(NULL, NULL, NULL, NULL, &on_duk_fatal)))
		goto on_error;
	console_log(1, "  Duktape %s", DUK_GIT_DESCRIBE);
	initialize_scripts();

	// initialize Sphere API
	initialize_api(g_duk);
	
	return true;

on_error:
	al_show_native_message_box(NULL, "Unable to Start", "Engine initialized failed.",
		"One or more components failed to initialize properly. minisphere cannot continue in this state and will now close.",
		NULL, ALLEGRO_MESSAGEBOX_ERROR);
	return false;
}

static void
shutdown_engine(void)
{
	save_key_map();
	
	#ifndef MINISPHERE_REDIST
	shutdown_debugger();
	#endif
	
	shutdown_map_engine();
	shutdown_input();
	
	console_log(0, "Shutting down Duktape");
	duk_destroy_heap(g_duk);
	
	console_log(0, "Shutting down Dyad");
	dyad_shutdown();
	
	shutdown_spritesets();
	shutdown_audialis();
	shutdown_galileo();
	
	console_log(0, "Shutting down Allegro");
	al_destroy_display(g_display); g_display = NULL;
	al_destroy_event_queue(g_events); g_events = NULL;
	al_destroy_path(g_game_path); g_game_path = NULL;
	free_sandbox(g_fs); g_fs = NULL;
	if (g_sys_conf != NULL)
		close_kev_file(g_sys_conf);
	g_sys_conf = NULL;
	al_uninstall_system();
}

static void
draw_status_message(const char* text)
{
	ALLEGRO_TRANSFORM old_transform;
	ALLEGRO_TRANSFORM transform;
	int               width = get_text_width(g_sys_font, text) + 20;
	int               height = get_font_line_height(g_sys_font) + 10;
	int               w_screen = al_get_display_width(g_display);
	int               h_screen = al_get_display_height(g_display);

	al_copy_transform(&old_transform, al_get_current_transform());
	al_identity_transform(&transform);
	al_use_transform(&transform);
	al_draw_filled_rounded_rectangle(
		w_screen - 16 - width, h_screen - 16 - height, w_screen - 16, h_screen - 16,
		4, 4, al_map_rgba(16, 16, 16, 255));
	draw_text(g_sys_font, rgba(0, 0, 0, 255), w_screen - 16 - width / 2 + 1, h_screen - 16 - height + 6, TEXT_ALIGN_CENTER, text);
	draw_text(g_sys_font, rgba(255, 255, 255, 255), w_screen - 16 - width / 2, h_screen - 16 - height + 5, TEXT_ALIGN_CENTER, text);
	al_use_transform(&old_transform);
}

static bool
find_startup_game(ALLEGRO_PATH* *out_path)
{
	ALLEGRO_FS_ENTRY* engine_dir;
	const char*       file_ext;
	const char*       filename;
	ALLEGRO_FS_ENTRY* fse;
	int               n_spk_files = 0;
	
	// prefer startup game if one exists
	*out_path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
	al_append_path_component(*out_path, "startup");
	al_set_path_filename(*out_path, "game.sgm");
	if (al_filename_exists(al_path_cstr(*out_path, ALLEGRO_NATIVE_PATH_SEP)))
		return true;
	al_destroy_path(*out_path);
	*out_path = NULL;
	
	// check for single SPK package alongside engine
	*out_path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
	engine_dir = al_create_fs_entry(al_path_cstr(*out_path, ALLEGRO_NATIVE_PATH_SEP));
	al_open_directory(engine_dir);
	while (fse = al_read_directory(engine_dir)) {
		filename = al_get_fs_entry_name(fse);
		file_ext = strrchr(filename, '.');
		if (file_ext != NULL && strcmp(file_ext, ".spk") == 0) {
			if (++n_spk_files == 1)
				*out_path = al_create_path(filename);
		}
		al_destroy_fs_entry(fse);
	}
	al_close_directory(engine_dir);
	if (n_spk_files == 1)
		return true;
	al_destroy_path(*out_path);
	*out_path = NULL;
	return false;
}
