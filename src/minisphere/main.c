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
static void draw_status_message (const char* text);
static bool find_startup_game   (path_t* *out_path);
static bool parse_command_line  (int argc, char* argv[], path_t* *out_game_path, bool *out_want_fullscreen, int *out_fullscreen, int *out_verbosity, bool *out_want_throttle, bool *out_want_debug);
static void print_banner        (bool want_copyright, bool want_deps);
static void print_usage         (void);
static void report_error        (const char* fmt, ...);
static bool verify_requirements (sandbox_t* fs);

static void on_duk_fatal (duk_context* ctx, duk_errcode_t code, const char* msg);

ALLEGRO_DISPLAY*     g_display = NULL;
duk_context*         g_duk = NULL;
ALLEGRO_EVENT_QUEUE* g_events = NULL;
sandbox_t*           g_fs = NULL;
path_t*              g_game_path = NULL;
path_t*              g_last_game_path = NULL;
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

	path_t*              browse_path;
	lstring_t*           dialog_name;
	duk_errcode_t        err_code;
	const char*          err_msg;
	ALLEGRO_FILECHOOSER* file_dlg;
	const char*          filename;
	bool                 have_shaders = false;
	image_t*             icon;
	int                  line_num;
	const path_t*        script_path;
	ALLEGRO_TRANSFORM    trans;
	int                  verbosity;
	bool                 want_debug;

	// parse the command line
	if (parse_command_line(argc, argv, &g_game_path,
		&s_is_fullscreen, &s_max_frameskip, &verbosity, &s_conserve_cpu, &want_debug))
	{
		initialize_console(verbosity);
	}
	else
		return EXIT_FAILURE;

	print_banner(true, false);
	printf("\n");

	// print out options
	console_log(1, "Parsing command line");
	console_log(1, "  Game path: %s", g_game_path != NULL ? path_cstr(g_game_path) : "<none provided>");
	console_log(1, "  Fullscreen: %s", s_is_fullscreen ? "ON" : "OFF");
	console_log(1, "  Frameskip limit: %d frames", s_max_frameskip);
	console_log(1, "  CPU throttle: %s", s_conserve_cpu ? "ON" : "OFF");
	console_log(1, "  Console verbosity: V%d", verbosity);
#if defined(MINISPHERE_SPHERUN)
	console_log(1, "  Debugger mode: %s", want_debug ? "Active" : "Passive");
#endif
	console_log(1, "");

	if (!initialize_engine())
		return EXIT_FAILURE;

	// set up jump points for script bailout
	console_log(1, "Setting up jump points for longjmp");
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
		console_log(1, "\nRestarting to launch new game");
		console_log(1, "  Path: %s", path_cstr(g_game_path));
		initialize_engine();
	}

	// locate the game manifest
	console_log(1, "Looking for a game to launch");
	if (g_game_path == NULL)
		// no game specified on command line, see if we have a startup game
		find_startup_game(&g_game_path);
	if (g_game_path != NULL)
		// user provided a path or startup game was found, attempt to load it
		g_fs = new_sandbox(path_cstr(g_game_path));
	else {
		// no game path provided and no startup game, let user find one
		browse_path = path_rebase(path_new("Sphere Games/"), homepath());
		path_mkdir(browse_path);
		dialog_name = lstr_newf("%s - Select a Sphere game to launch", PRODUCT_NAME);
		file_dlg = al_create_native_file_dialog(path_cstr(browse_path),
			lstr_cstr(dialog_name),
			"game.sgm;game.s2gm;*.spk", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
		al_show_native_file_dialog(NULL, file_dlg);
		lstr_free(dialog_name);
		path_free(browse_path);
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
			return EXIT_SUCCESS;
		}
	}
	if (g_fs == NULL) {
		// if after all that, we still don't have a valid sandbox pointer, bail out;
		// there's not much else we can do.
		al_show_native_message_box(NULL, "Unable to Load Game", path_cstr(g_game_path),
			"minisphere was unable to load the game manifest or it was not found.  Check to make sure the directory above exists and contains a valid Sphere game.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		exit_game(false);
	}
	if (!verify_requirements(g_fs))
		exit_game(false);

	// try to create a display. if we can't get a programmable pipeline, try again but
	// only request bare OpenGL. keep in mind that if this happens, shader support will be
	// disabled.
	console_log(1, "Creating Allegro rendering context");
	get_sgm_resolution(g_fs, &g_res_x, &g_res_y);
	g_scale_x = g_scale_y = (g_res_x <= 400 && g_res_y <= 300) ? 2.0 : 1.0;
	console_log(1, "  Resolution: %dx%d @ %.1fx", g_res_x, g_res_y, (double)g_scale_x);
#ifdef MINISPHERE_USE_SHADERS
	al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_PROGRAMMABLE_PIPELINE);
	if (g_display = al_create_display(g_res_x * g_scale_x, g_res_y * g_scale_y))
		have_shaders = true;
	else {
		al_set_new_display_flags(ALLEGRO_OPENGL);
		g_display = al_create_display(g_res_x * g_scale_x, g_res_y * g_scale_y);
	}
#else
	al_set_new_display_flags(ALLEGRO_OPENGL);
	g_display = al_create_display(g_res_x * g_scale_x, g_res_y * g_scale_y);
#endif
	if (g_display == NULL) {
		al_show_native_message_box(NULL, "Unable to Create Render Context", "minisphere was unable to create a rendering context.",
			"Your hardware is either too old to run minisphere, or there is a driver problem on this system.  Check that all drivers are installed and up-to-date.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		return EXIT_FAILURE;
	}
	console_log(1, "  Shaders: %s", have_shaders ? "Enabled" : "Unsupported");
	al_set_window_title(g_display, get_sgm_name(g_fs));
	al_set_new_bitmap_flags(ALLEGRO_NO_PREMULTIPLIED_ALPHA | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
	if (!(icon = load_image("~sgm/icon.png")))
		icon = load_image("~sys/icon.png");
	if (icon != NULL)
		rescale_image(icon, 32, 32);
	al_set_new_bitmap_flags(ALLEGRO_NO_PREMULTIPLIED_ALPHA);
	al_identity_transform(&trans);
	al_scale_transform(&trans, g_scale_x, g_scale_y);
	al_use_transform(&trans);
	if (icon != NULL)
		al_set_display_icon(g_display, get_image_bitmap(icon));
	free_image(icon);
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	g_events = al_create_event_queue();
	al_register_event_source(g_events, al_get_display_event_source(g_display));
	attach_input_display();
	load_key_map();

	// initialize shader support
	initialize_shaders(have_shaders);

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
#if defined(MINISPHERE_SPHERUN)
	initialize_debugger(want_debug, false);
#endif

	console_log(0, ">>> Engine started!");

	// display loading message, scripts may take a bit to compile
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	draw_status_message("starting up...");
	al_flip_display();
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));

	// initialize timing variables
	s_next_fps_poll_time = al_get_time() + 1.0;
	s_num_frames = s_num_flips = 0;
	s_current_fps = s_current_game_fps = 0;
	s_next_frame_time = s_last_flip_time = al_get_time();

	// evaluate startup script
	al_hide_mouse_cursor(g_display);
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
	al_show_mouse_cursor(g_display);
	duk_get_prop_string(g_duk, -2, "lineNumber");
	line_num = duk_get_int(g_duk, -1);
	duk_pop(g_duk);
	duk_get_prop_string(g_duk, -2, "fileName");
	filename = duk_get_string(g_duk, -1);
	if (filename != NULL) {
		fprintf(stderr, "Unhandled JS exception caught by engine\n  [%s:%d] %s\n", filename, line_num, err_msg);
		if (err_msg[strlen(err_msg) - 1] != '\n')
			duk_push_sprintf(g_duk, "%s:%d\n\n%s\n", filename, line_num, err_msg);
		else
			duk_push_sprintf(g_duk, "%s\n", err_msg);
	}
	else {
		fprintf(stderr, "Unhandled JS error caught by engine.\n%s\n", err_msg);
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
set_resolution(int width, int height)
{
	ALLEGRO_MONITOR_INFO monitor;
	ALLEGRO_TRANSFORM    transform;

	g_res_x = width;
	g_res_y = height;

	if (!s_is_fullscreen) {
		g_scale_x = (g_res_x <= 400 || g_res_y <= 300) ? 2.0 : 1.0;
		g_scale_y = g_scale_x;
		al_resize_display(g_display, g_res_x * g_scale_x, g_res_y * g_scale_y);
		al_get_monitor_info(0, &monitor);
		al_set_window_position(g_display,
			(monitor.x1 + monitor.x2) / 2 - g_res_x * g_scale_x / 2,
			(monitor.y1 + monitor.y2) / 2 - g_res_y * g_scale_y / 2);
	}
	else {
		g_scale_x = al_get_display_width(g_display) / (float)g_res_x;
		g_scale_y = al_get_display_height(g_display) / (float)g_res_y;
	}

	al_identity_transform(&transform);
	al_scale_transform(&transform, g_scale_x, g_scale_y);
	al_use_transform(&transform);
	set_clip_rectangle(new_rect(0, 0, g_res_x, g_res_y));

	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	al_flip_display();
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

void
flip_screen(int framerate)
{
	char              filename[50];
	char              fps_text[20];
	bool              is_backbuffer_valid;
	ALLEGRO_STATE     old_state;
	path_t*           path;
	const char*       pathstr;
	ALLEGRO_BITMAP*   snapshot;
	double            time_left;
	ALLEGRO_TRANSFORM trans;
	int               x, y;

	// update FPS with 1s granularity
	if (al_get_time() >= s_next_fps_poll_time) {
		s_current_fps = s_num_flips;
		s_current_game_fps = s_num_frames;
		s_num_flips = 0;
		s_num_frames = 0;
		s_next_fps_poll_time = al_get_time() + 1.0;
	}

	// flip the backbuffer, unless the preceeding frame was skipped
	is_backbuffer_valid = !s_skipping_frame;
	if (is_backbuffer_valid) {
		if (s_want_snapshot) {
			al_store_state(&old_state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
			al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA);
			snapshot = al_clone_bitmap(al_get_backbuffer(g_display));
			al_restore_state(&old_state);
			path = path_rebase(path_new("minisphere/Screenshots/"), homepath());
			path_mkdir(path);
			do {
				path_strip(path);
				sprintf(filename, "SS_%s.png", rng_string(10));
				path_append(path, filename);
				pathstr = path_cstr(path);
			} while (al_filename_exists(pathstr));
			al_save_bitmap(pathstr, snapshot);
			al_destroy_bitmap(snapshot);
			path_free(path);
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

	// if framerate is nonzero and we're backed up on frames, skip frames until we
	// catch up. there is a cap on consecutive frameskips to avoid the situation where
	// the engine "can't catch up" (due to a slow machine, overloaded CPU, etc.). better
	// that we lag instead of never rendering anything at all.
	if (framerate > 0) {
		s_skipping_frame = s_frame_skips < s_max_frameskip && s_last_flip_time > s_next_frame_time;
		do {  // kill time while we wait for the next frame
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
	if (!s_skipping_frame)
		al_clear_to_color(al_map_rgba(0, 0, 0, 255));
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
	set_clip_rectangle(s_clip_rect);
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
				TEXT_ALIGN_CENTER,
				is_copied ? "[Space]/[Esc] to close" : "[Ctrl+C] to copy, [Space]/[Esc] to close");
		}
		flip_screen(30);
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
				al_set_clipboard_text(g_display, msg);
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
	al_show_native_message_box(g_display, "Script Error",
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
	console_log(1, "Initializing Allegro (%d.%d.%d)",
		al_version >> 24, (al_version >> 16) & 0xFF, (al_version >> 8) & 0xFF);
	al_set_org_name("Fat Cerberus");
	al_set_app_name("minisphere");
	if (!al_init())
		goto on_error;
	if (!al_init_native_dialog_addon()) goto on_error;
	if (!al_init_primitives_addon()) goto on_error;
	if (!al_init_image_addon()) goto on_error;

	// initialize networking
	console_log(1, "Initializing Dyad");
	dyad_init();
	dyad_setUpdateTimeout(0.0);

	// load system configuraton
	console_log(1, "Loading system configuration");
	g_sys_conf = open_kev_file(NULL, "~sys/system.ini");

	// initialize engine components
	initialize_async();
	initialize_rng();
	initialize_galileo();
	initialize_audialis();
	initialize_input();
	initialize_spritesets();
	initialize_map_engine();

	// initialize JavaScript
	console_log(1, "Initializing JavaScript");
	if (!(g_duk = duk_create_heap(NULL, NULL, NULL, NULL, &on_duk_fatal)))
		goto on_error;
	console_log(1, "  Duktape %s", DUK_GIT_DESCRIBE);
	initialize_scripts();

	// register the Sphere API
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

#if defined(MINISPHERE_SPHERUN)
	shutdown_debugger();
#endif

	shutdown_map_engine();
	shutdown_input();

	console_log(1, "Shutting down Duktape");
	duk_destroy_heap(g_duk);

	console_log(1, "Shutting down Dyad");
	dyad_shutdown();

	shutdown_spritesets();
	shutdown_audialis();
	shutdown_galileo();
	shutdown_async();

	console_log(1, "Shutting down Allegro");
	if (g_display != NULL)
		al_destroy_display(g_display);
	g_display = NULL;
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
find_startup_game(path_t* *out_path)
{
	ALLEGRO_FS_ENTRY* engine_dir;
	const char*       file_ext;
	const char*       filename;
	ALLEGRO_FS_ENTRY* fse;
	int               n_spk_files = 0;

	// prefer startup game if one exists
	*out_path = path_rebase(path_new("startup/game.sgm"), enginepath());
	if (al_filename_exists(path_cstr(*out_path)))
		return true;
	path_free(*out_path);
	*out_path = NULL;

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
				report_error("unrecognized option '%s'\n", argv[i]);
				return false;
			}
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
			else {
				report_error("unrecognized option '%s'\n", argv[i]);
				return false;
			}
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
	report_error("missing argument for option '%s'\n", argv[i - 1]);
	return false;
}

static void
print_banner(bool want_copyright, bool want_deps)
{
	uint32_t al_version;
	char     version_string[64];
	
	printf("%s JS game engine (%s)\n", PRODUCT_NAME, sizeof(void*) == 4 ? "x86" : "x64");
	if (want_copyright) {
		printf("A lightweight Sphere-compatible game engine\n");
		printf("(c) 2016 Fat Cerberus\n");
	}
	if (want_deps) {
		al_version = al_get_allegro_version();
		sprintf(version_string, "%d.%d.%d", al_version >> 24, (al_version >> 16) & 0xFF,
			(al_version >> 8) & 0xFF);
		printf("\n");
		printf("   Allegro: v%-12s libmng: v%s\n", version_string, mng_version_text());
		printf("   Duktape: %-13s   zlib: v%s\n", DUK_GIT_DESCRIBE, zlibVersion());
		printf("    Dyad.c: v%-12s\n", dyad_getVersion());
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
	al_show_native_message_box(NULL,
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
	al_show_native_message_box(NULL, "Unsupported Engine", path_cstr(g_game_path), lstr_cstr(message), NULL, ALLEGRO_MESSAGEBOX_ERROR);
	return false;
}
