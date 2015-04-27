#include "minisphere.h"

#include "api.h"
#include "bytearray.h"
#include "color.h"
#include "file.h"
#include "font.h"
#include "galileo.h"
#include "image.h"
#include "input.h"
#include "logger.h"
#include "map_engine.h"
#include "primitives.h"
#include "rawfile.h"
#include "sockets.h"
#include "sound.h"
#include "spriteset.h"
#include "surface.h"
#include "windowstyle.h"

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

static void on_duk_fatal (duk_context* ctx, duk_errcode_t code, const char* msg);

ALLEGRO_DISPLAY*     g_display = NULL;
duk_context*         g_duk = NULL;
ALLEGRO_EVENT_QUEUE* g_events = NULL;
ALLEGRO_CONFIG*      g_game_conf = NULL;
ALLEGRO_PATH*        g_game_path = NULL;
char*                g_last_game_path = NULL;
float                g_scale_x = 1.0;
float                g_scale_y = 1.0;
ALLEGRO_CONFIG*      g_sys_conf;
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
static bool    s_take_snapshot = false;

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
	{ "An exception was thrown.", "minisphere takes exception to crappy games." }
};

int
main(int argc, char* argv[])
{
	ALLEGRO_USTR*        dialog_name;
	duk_errcode_t        err_code;
	const char*          err_msg;
	duk_int_t            exec_result;
	ALLEGRO_FILECHOOSER* file_dlg;
	const char*          file_path;
	const char*          filename;
	char*                game_path;
	ALLEGRO_BITMAP*      icon;
	char*                icon_path;
	int                  line_num;
	int                  max_skips;
	char*                p_strtol;
	char*                path;
	ALLEGRO_TRANSFORM    trans;
	
	int i;

	printf("%s (%i-bit)\n", ENGINE_NAME, sizeof(void*) * 8);
	printf("A lightweight Sphere-compatible game engine\n\n");
	
	if (!initialize_engine())
		return EXIT_FAILURE;
	
	// determine location of game.sgm and try to load it
	g_game_path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
	al_append_path_component(g_game_path, "startup");
	if (argc == 2 && argv[1][0] != '-') {
		// single non-switch argument passed, assume it's a game.sgm path or game directory
		al_destroy_path(g_game_path);
		g_game_path = al_create_path(argv[1]);
		if (strcasecmp(al_get_path_filename(g_game_path), "game.sgm") != 0) {
			al_destroy_path(g_game_path);
			g_game_path = al_create_path_for_directory(argv[1]);
		}
	}
	else {
		// more than one argument, perform full commandline parsing
		for (i = 1; i < argc; ++i) {
			if (strcmp(argv[i], "-game") == 0 || strcmp(argv[i], "--game") == 0 && i < argc - 1) {
				al_destroy_path(g_game_path);
				g_game_path = al_create_path(argv[i + 1]);
				if (strcasecmp(al_get_path_filename(g_game_path), "game.sgm") != 0) {
					al_destroy_path(g_game_path);
					g_game_path = al_create_path_for_directory(argv[i + 1]);
				}
			}
			if (strcmp(argv[i], "--frameskip") == 0 && i < argc - 1) {
				errno = 0; max_skips = strtol(argv[i + 1], &p_strtol, 10);
				if (errno != ERANGE && *p_strtol == '\0')
					set_max_frameskip(max_skips);
			}
			else if (strcmp(argv[i], "--no-throttle") == 0) {
				s_conserve_cpu = false;
			}
			else if (strcmp(argv[i], "--fullscreen") == 0) {
				s_is_fullscreen = true;
			}
			else if (strcmp(argv[i], "--windowed") == 0) {
				s_is_fullscreen = false;
			}
		}
	}
	
	// print out options
	printf("Finished parsing command line\n");
	printf("  SGM path: %s\n", al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP));
	printf("  Maximum consecutive frame skips: %i\n", s_max_frameskip);
	printf("  CPU throttle: %s\n", s_conserve_cpu ? "ON" : "OFF");
	
	// set up jump points for script bailout
	if (setjmp(s_jmp_exit)) {  // user closed window, script called Exit(), etc.
		shutdown_engine();
		if (g_last_game_path != NULL) {  // returning from ExecuteGame()?
			initialize_engine();
			g_game_path = al_create_path(g_last_game_path);
			if (strcasecmp(al_get_path_filename(g_game_path), "game.sgm") != 0) {
				al_destroy_path(g_game_path);
				g_game_path = al_create_path_for_directory(g_last_game_path);
			}
			free(g_last_game_path);
			g_last_game_path = NULL;
		}
		else {
			return EXIT_SUCCESS;
		}
	}
	if (setjmp(s_jmp_restart)) {  // script called RestartGame() or ExecuteGame()
		game_path = strdup(al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP));
		shutdown_engine();
		initialize_engine();
		g_game_path = al_create_path(game_path);
		free(game_path);
	}

	// locate game.sgm
	printf("Searching for SGM file...\n");
	al_set_path_filename(g_game_path, NULL);
	al_make_path_canonical(g_game_path);
	char* sgm_path = get_asset_path("game.sgm", NULL, false);
	g_game_conf = al_load_config_file(sgm_path);
	free(sgm_path);
	if (g_game_conf == NULL) {
		dialog_name = al_ustr_newf("%s - Where is game.sgm?", ENGINE_NAME);
		file_dlg = al_create_native_file_dialog(NULL, al_cstr(dialog_name), "game.sgm", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST);
		if (al_show_native_file_dialog(NULL, file_dlg)) {
			al_destroy_path(g_game_path);
			g_game_path = al_create_path(al_get_native_file_dialog_path(file_dlg, 0));
		}
		else {
			return EXIT_SUCCESS;
		}
		al_destroy_native_file_dialog(file_dlg);
		al_ustr_free(dialog_name);
		g_game_conf = al_load_config_file(al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP));
		if (g_game_conf == NULL) {
			al_show_native_message_box(NULL, "Unable to Load Game",
				al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP),
				"minisphere was unable to load game.sgm or it was not found.  Check to make sure the above directory exists and contains a valid Sphere game.",
				NULL, ALLEGRO_MESSAGEBOX_ERROR);
			return EXIT_FAILURE;
		}
	}
	printf("Found SGM: %s\n", al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP));

	// set up engine and create display window
	icon_path = get_asset_path("icon.png", NULL, false);
	icon = al_load_bitmap(icon_path);
	free(icon_path);
	g_res_x = atoi(al_get_config_value(g_game_conf, NULL, "screen_width"));
	g_res_y = atoi(al_get_config_value(g_game_conf, NULL, "screen_height"));
	g_scale_x = g_scale_y = (g_res_x <= 400 && g_res_y <= 300) ? 2.0 : 1.0;
	if (!(g_display = al_create_display(g_res_x * g_scale_x, g_res_y * g_scale_y))) {
		al_show_native_message_box(NULL, "Unable to Create Display", "minisphere was unable to create a display window.",
			"A display window is required for rendering. The engine cannot run without it and will now close.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		return EXIT_FAILURE;
	}
	al_identity_transform(&trans);
	al_scale_transform(&trans, g_scale_x, g_scale_y);
	al_use_transform(&trans);
	if (icon != NULL)
		al_set_display_icon(g_display, icon);
	al_set_window_title(g_display, al_get_config_value(g_game_conf, NULL, "name"));
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	g_events = al_create_event_queue();
	al_register_event_source(g_events, al_get_display_event_source(g_display));
	printf("Created render window\n");
	
	// attempt to locate and load system font
	if (g_sys_conf != NULL) {
		filename = al_get_config_value(g_sys_conf, NULL, "Font");
		path = get_sys_asset_path(filename, "system");
		g_sys_font = load_font(path);
		free(path);
	}
	if (g_sys_font == NULL) {
		al_show_native_message_box(g_display, "No System Font Available", "A system font is required.",
			"minisphere was unable to locate the system font or it failed to load.  As a usable font is necessary for proper operation of the engine, minisphere will now close.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		return EXIT_FAILURE;
	}
	printf("Loaded system font\n");

	// display loading message, scripts may take a bit to compile
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	draw_status_message("loading...");
	al_flip_display();
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));

	// switch to fullscreen if necessary and initialize clipping
	if (s_is_fullscreen)
		toggle_fullscreen();
	set_clip_rectangle(new_rect(0, 0, g_res_x, g_res_y));

	al_hide_mouse_cursor(g_display);
	
	// load startup script
	printf("Starting up...\n");
	path = get_asset_path(al_get_config_value(g_game_conf, NULL, "script"), "scripts", false);
	exec_result = duk_pcompile_file(g_duk, 0x0, path);
	free(path);
	if (exec_result != DUK_EXEC_SUCCESS) goto on_js_error;
	if (duk_pcall(g_duk, 0) != DUK_EXEC_SUCCESS) goto on_js_error;
	duk_pop(g_duk);

	// initialize timing variables
	s_next_fps_poll_time = al_get_time() + 1.0;
	s_num_frames = s_num_flips = 0;
	s_current_fps = s_current_game_fps = 0;
	s_next_frame_time = s_last_flip_time = al_get_time();

	// call game() function in script
	duk_push_global_object(g_duk);
	duk_get_prop_string(g_duk, -1, "game");
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
	file_path = duk_get_string(g_duk, -1);
	if (file_path != NULL) {
		filename = strrchr(file_path, ALLEGRO_NATIVE_PATH_SEP);
		filename = filename != NULL ? filename + 1 : file_path;
		fprintf(stderr, "JS Error: %s:%i - %s", filename, line_num, err_msg);
		if (err_msg[strlen(err_msg) - 1] != '\n')
			duk_push_sprintf(g_duk, "`%s`, line: %i | %s", filename, line_num, err_msg);
		else
			duk_push_string(g_duk, err_msg);
	}
	else {
		fprintf(stderr, "JS Error: %s", err_msg);
		duk_push_string(g_duk, err_msg);
	}
	duk_fatal(g_duk, err_code, duk_get_string(g_duk, -1));
}

bool
is_skipped_frame(void)
{
	return s_skipping_frame;
}

char*
get_asset_path(const char* path, const char* base_dir, bool allow_mkdir)
{
	bool is_homed = (strstr(path, "~/") == path || strstr(path, "~\\") == path);
	ALLEGRO_PATH* base_path = al_create_path_for_directory(base_dir);
	al_rebase_path(g_game_path, base_path);
	if (allow_mkdir) {
		const char* dir_path = al_path_cstr(base_path, ALLEGRO_NATIVE_PATH_SEP);
		al_make_directory(dir_path);
	}
	ALLEGRO_PATH* asset_path = al_create_path(is_homed ? &path[2] : path);
	bool is_absolute = al_get_path_num_components(asset_path) > 0
		&& strcmp(al_get_path_component(asset_path, 0), "") == 0;
	char* out_path = NULL;
	if (!is_absolute) {
		al_rebase_path(is_homed ? g_game_path : base_path, asset_path);
		al_make_path_canonical(asset_path);
		out_path = strdup(al_path_cstr(asset_path, ALLEGRO_NATIVE_PATH_SEP));
	}
	al_destroy_path(asset_path);
	al_destroy_path(base_path);
	return out_path;
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

char*
get_sys_asset_path(const char* path, const char* base_dir)
{
	bool is_homed = (strstr(path, "~/") == path || strstr(path, "~\\") == path);
	ALLEGRO_PATH* base_path = al_create_path_for_directory(base_dir);
	ALLEGRO_PATH* system_path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
	al_rebase_path(system_path, base_path);
	ALLEGRO_PATH* asset_path = al_create_path(is_homed ? &path[2] : path);
	bool is_absolute = al_get_path_num_components(asset_path) > 0
		&& strcmp(al_get_path_component(asset_path, 0), "") == 0;
	char* out_path = NULL;
	if (!is_absolute) {
		al_rebase_path(is_homed ? system_path : base_path, asset_path);
		al_make_path_canonical(asset_path);
		out_path = strdup(al_path_cstr(asset_path, ALLEGRO_NATIVE_PATH_SEP));
	}
	al_destroy_path(asset_path);
	al_destroy_path(system_path);
	al_destroy_path(base_path);
	return out_path;
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
do_events(void)
{
	ALLEGRO_EVENT event;

	dyad_update();
	update_input();
	update_sounds();

	// process Allegro events
	while (al_get_next_event(g_events, &event)) {
		switch (event.type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			exit_game(true);
		}
	}
}

noreturn
exit_game(bool is_shutdown)
{
	if (is_shutdown) {
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
	char*             path;
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
		if (s_take_snapshot) {
			snapshot = al_clone_bitmap(al_get_backbuffer(g_display));
			sprintf(filename, "snapshot-%li.png", (long)time(NULL));
			path = get_asset_path(filename, "snapshots", true);
			al_save_bitmap(path, snapshot);
			al_destroy_bitmap(snapshot);
			free(path);
			s_take_snapshot = false;
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
	s_take_snapshot = true;
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
	char* path;
	
	srand(time(NULL));
	
	// initialize Allegro
	if (!al_init())
		goto on_error;
	if (!al_init_native_dialog_addon()) goto on_error;
	if (!al_init_primitives_addon()) goto on_error;
	if (!al_init_image_addon()) goto on_error;

	// initialize networking
	dyad_init();
	dyad_setUpdateTimeout(0.001);

	// load system configuraton
	path = get_sys_asset_path("system.ini", "system");
	g_sys_conf = al_load_config_file(path);
	free(path);

	initialize_input();
	initialize_map_engine();
	initialize_sound();

	// initialize JavaScript API
	if (!(g_duk = duk_create_heap(NULL, NULL, NULL, NULL, &on_duk_fatal)))
		goto on_error;
	printf("Created Duktape context\n");
	initialize_api(g_duk);
	init_bytearray_api();
	init_color_api();
	init_file_api();
	init_font_api(g_duk);
	init_galileo_api();
	init_image_api(g_duk);
	init_input_api();
	init_logging_api();
	init_map_engine_api(g_duk);
	init_primitives_api();
	init_rawfile_api();
	init_sockets_api();
	init_sound_api();
	init_spriteset_api(g_duk);
	init_surface_api();
	init_windowstyle_api();
	printf("Initialized Sphere API\n");
	return true;

on_error:
	al_show_native_message_box(NULL, "Unable to Start", "Engine initialized failed.",
		"One or more engine components failed to initialize properly. minisphere cannot continue in this state and will now close.",
		NULL, ALLEGRO_MESSAGEBOX_ERROR);
	return false;
}

static void
shutdown_engine(void)
{
	shutdown_map_engine();
	duk_destroy_heap(g_duk);
	dyad_shutdown();
	shutdown_sound();
	shutdown_input();
	al_destroy_display(g_display);
	al_destroy_event_queue(g_events);
	al_destroy_config(g_game_conf);
	al_destroy_path(g_game_path);
	if (g_sys_conf != NULL) al_destroy_config(g_sys_conf);
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