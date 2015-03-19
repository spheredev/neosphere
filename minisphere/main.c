#include "minisphere.h"

#include "api.h"
#include "bytearray.h"
#include "color.h"
#include "file.h"
#include "font.h"
#include "image.h"
#include "input.h"
#include "logger.h"
#include "map_engine.h"
#include "primitives.h"
#include "rawfile.h"
#include "rfn_handler.h"
#include "sockets.h"
#include "sound.h"
#include "spriteset.h"
#include "surface.h"
#include "windowstyle.h"

// enable visual styles (VC++)
#ifdef _MSC_VER
#pragma comment(linker, \
    "\"/manifestdependency:type='Win32' "\
    "name='Microsoft.Windows.Common-Controls' "\
    "version='6.0.0.0' "\
    "processorArchitecture='*' "\
    "publicKeyToken='6595b64144ccf1df' "\
    "language='*'\"")
#endif

static const int MAX_FRAME_SKIPS = 5;

static void initialize_engine (void);
static void shutdown_engine   (void);
static void queue_key         (int keycode);
static void toggle_fullscreen (void);

static void on_duk_fatal (duk_context* ctx, duk_errcode_t code, const char* msg);

static rect_t  s_clip_rect;
static int     s_current_fps;
static int     s_current_game_fps;
static int     s_frame_skips;
static bool    s_is_fullscreen = false;
static char*   s_last_game_path = NULL;
static double  s_last_flip_time;
static double  s_next_fps_poll_time;
static double  s_next_frame_time;
static int     s_num_flips;
static int     s_num_frames;
bool           s_skipping_frame = false;
static bool    s_show_fps = false;
static bool    s_take_snapshot = false;

ALLEGRO_DISPLAY*     g_display    = NULL;
duk_context*         g_duktape    = NULL;
ALLEGRO_EVENT_QUEUE* g_events     = NULL;
ALLEGRO_CONFIG*      g_game_conf  = NULL;
ALLEGRO_PATH*        g_game_path  = NULL;
key_queue_t          g_key_queue;
float                g_scale_x    = 1.0;
float                g_scale_y    = 1.0;
ALLEGRO_CONFIG*      g_sys_conf;
font_t*              g_sys_font   = NULL;
int                  g_res_x, g_res_y;

int
main(int argc, char** argv)
{
	ALLEGRO_USTR*        dialog_name;
	duk_errcode_t        err_code;
	duk_int_t            exec_result;
	ALLEGRO_FILECHOOSER* file_dlg;
	const char*          filename;
	char*                game_path;
	ALLEGRO_BITMAP*      icon;
	char*                icon_path;
	char*                path;
	ALLEGRO_TRANSFORM    trans;
	
	int i;

	initialize_engine();
	
	// determine location of game.sgm and try to load it
	g_game_path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
	al_append_path_component(g_game_path, "startup");
	if (argc == 2 && argv[1][0] != '-') {
		// single non-switch argument passed, assume it's an .sgm file or game directory
		al_destroy_path(g_game_path);
		g_game_path = al_create_path(argv[1]);
		if (strcasecmp(al_get_path_extension(g_game_path), ".sgm") != 0) {
			al_destroy_path(g_game_path);
			g_game_path = al_create_path_for_directory(argv[1]);
		}
	}
	else {
		// more than one argument, perform full commandline parsing
		for (i = 1; i < argc; ++i) {
			if (strcmp(argv[i], "-game") == 0 && i < argc - 1) {
				al_destroy_path(g_game_path);
				g_game_path = al_create_path(argv[i + 1]);
				if (strcasecmp(al_get_path_extension(g_game_path), ".sgm") != 0) {
					al_destroy_path(g_game_path);
					g_game_path = al_create_path_for_directory(argv[i + 1]);
				}
			}
			else if (strcmp(argv[i], "-fullscreen") == 0) {
				s_is_fullscreen = true;
			}
			else if (strcmp(argv[i], "-windowed") == 0) {
				s_is_fullscreen = false;
			}
		}
	}
	
startup:
	al_set_path_filename(g_game_path, NULL);
	al_make_path_canonical(g_game_path);
	char* sgm_path = get_asset_path("game.sgm", NULL, false);
	g_game_conf = al_load_config_file(sgm_path);
	free(sgm_path);
	if (g_game_conf == NULL) {
		dialog_name = al_ustr_newf("%s - Where is game.sgm?", ENGINE_VERSION_NAME);
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

	// set up engine and create display window
	icon_path = get_asset_path("game-icon.png", NULL, false);
	icon = al_load_bitmap(icon_path);
	free(icon_path);
	al_register_font_loader(".rfn", &al_load_rfn_font);
	al_reserve_samples(8);
	al_set_mixer_gain(al_get_default_mixer(), 1.0);
	g_res_x = atoi(al_get_config_value(g_game_conf, NULL, "screen_width"));
	g_res_y = atoi(al_get_config_value(g_game_conf, NULL, "screen_height"));
	g_scale_x = g_scale_y = (g_res_x <= 400 && g_res_y <= 300) ? 2.0 : 1.0;
	g_display = al_create_display(g_res_x * g_scale_x, g_res_y * g_scale_y);
	al_identity_transform(&trans);
	al_scale_transform(&trans, g_scale_x, g_scale_y);
	al_use_transform(&trans);
	if (icon != NULL) al_set_display_icon(g_display, icon);
	al_set_window_title(g_display, al_get_config_value(g_game_conf, NULL, "name"));
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	g_events = al_create_event_queue();
	al_register_event_source(g_events, al_get_display_event_source(g_display));
	al_register_event_source(g_events, al_get_keyboard_event_source());
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_flip_display();

	// attempt to locate and load system font
	if (g_sys_conf != NULL) {
		filename = al_get_config_value(g_sys_conf, NULL, "Font");
		path = get_sys_asset_path(filename, "system");
		g_sys_font = load_font(path, 8);
		free(path);
	}
	if (g_sys_font == NULL) {
		al_show_native_message_box(g_display, "No System Font Available", "A system font is required.",
			"minisphere was unable to locate the system font or it failed to load.  As a usable font is necessary for proper operation of the engine, minisphere will now close.",
			NULL, ALLEGRO_MESSAGEBOX_ERROR);
		return EXIT_FAILURE;
	}

	al_hide_mouse_cursor(g_display);
	
	// switch to fullscreen if necessary and initialize clipping
	if (s_is_fullscreen) toggle_fullscreen();
	set_clip_rectangle(new_rect(0, 0, g_res_x, g_res_y));

	// load startup script
	path = get_asset_path(al_get_config_value(g_game_conf, NULL, "script"), "scripts", false);
	exec_result = duk_pcompile_file(g_duktape, 0x0, path);
	free(path);
	if (exec_result != DUK_EXEC_SUCCESS) goto on_js_error;
	if (duk_pcall(g_duktape, 0) != DUK_EXEC_SUCCESS) goto on_js_error;
	duk_pop(g_duktape);

	// initialize timing variables
	s_next_fps_poll_time = al_get_time() + 1.0;
	s_num_frames = s_num_flips = 0;
	s_current_fps = s_current_game_fps = 0;
	s_next_frame_time = s_last_flip_time = al_get_time();

	// call game() function in script
	duk_push_global_object(g_duktape);
	duk_get_prop_string(g_duktape, -1, "game");
	if (duk_pcall(g_duktape, 0) != DUK_EXEC_SUCCESS)
		goto on_js_error;
	duk_pop(g_duktape);
	duk_pop(g_duktape);
	
teardown:
	// are we returning from an ExecuteGame() call?
	if (s_last_game_path != NULL) {
		shutdown_engine();
		initialize_engine();
		g_game_path = al_create_path_for_directory(s_last_game_path);
		free(s_last_game_path); s_last_game_path = NULL;
		goto startup;
	}
	
	// otherwise shut down, we're done here
	shutdown_engine();
	return EXIT_SUCCESS;

on_js_error:
	err_code = duk_get_error_code(g_duktape, -1);
	duk_dup(g_duktape, -1);
	const char* err_msg = duk_safe_to_string(g_duktape, -1);
	if (err_code != DUK_ERR_ERROR || strstr(err_msg, "Error: @") != err_msg) {
		// this is an legitimate exception, handle accordingly
		al_show_mouse_cursor(g_display);
		duk_get_prop_string(g_duktape, -2, "lineNumber");
		duk_int_t line_num = duk_get_int(g_duktape, -1);
		duk_pop(g_duktape);
		duk_get_prop_string(g_duktape, -2, "fileName");
		const char* file_path = duk_get_string(g_duktape, -1);
		if (file_path != NULL) {
			char* file_name = strrchr(file_path, ALLEGRO_NATIVE_PATH_SEP);
			file_name = file_name != NULL ? file_name + 1 : file_path;
			duk_push_sprintf(g_duktape, "%s (line: %i)\n\n%s", file_name, line_num, err_msg);
		}
		else {
			duk_push_string(g_duktape, err_msg);
		}
		duk_fatal(g_duktape, err_code, duk_get_string(g_duktape, -1));
	}
	else if (strstr(err_msg, "Error: @restart") == err_msg) {
		// RestartGame() was called, reset engine and start again
		game_path = strdup(al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP));
		shutdown_engine();
		initialize_engine();
		g_game_path = al_create_path(game_path);
		free(game_path);
		goto startup;
	}
	else if (strstr(err_msg, "Error: @exec ") == err_msg) {
		// ExecuteGame() was called, clear and and prep for new session
		s_last_game_path = strdup(al_path_cstr(g_game_path, ALLEGRO_NATIVE_PATH_SEP));
		filename = err_msg + 13;
		path = get_sys_asset_path(filename, "games");
		shutdown_engine();
		initialize_engine();
		g_game_path = al_create_path(path);
		if (strcasecmp(al_get_path_extension(g_game_path), ".sgm") != 0) {
			al_destroy_path(g_game_path);
			g_game_path = al_create_path_for_directory(path);
		}
		al_set_path_filename(g_game_path, NULL);
		free(path);
		goto startup;
	}
	
	// exception was an intentional bailout (window close, Exit(), etc.), shut down normally
	goto teardown;
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

bool
do_events(void)
{
	ALLEGRO_EVENT event;

	dyad_update();

	// update global input state
	update_input();

	// process Allegro events
	while (al_get_next_event(g_events, &event)) {
		switch (event.type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			return false;
		case ALLEGRO_EVENT_KEY_CHAR:
			switch (event.keyboard.keycode) {
			case ALLEGRO_KEY_ENTER:
				if (event.keyboard.modifiers & ALLEGRO_KEYMOD_ALT
					|| event.keyboard.modifiers & ALLEGRO_KEYMOD_ALTGR)
				{
					toggle_fullscreen();
				}
				else {
					queue_key(event.keyboard.keycode);
				}
				break;
			case ALLEGRO_KEY_F10:
				toggle_fullscreen();
				break;
			case ALLEGRO_KEY_F11:
				s_show_fps = !s_show_fps;
				break;
			case ALLEGRO_KEY_F12:
				s_take_snapshot = true;
				break;
			default:
				queue_key(event.keyboard.keycode);
				break;
			}
		}
	}
	return true;
}

bool
flip_screen(int framerate)
{
	char              filename[50];
	char              fps_text[20];
	bool              is_backbuffer_valid;
	char*             path;
	ALLEGRO_BITMAP*   snapshot;
	ALLEGRO_TRANSFORM trans;
	int               x, y;

	is_backbuffer_valid = !s_skipping_frame;
	if (is_backbuffer_valid) {
		++s_num_flips;
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
			draw_text(g_sys_font, al_map_rgba(0, 0, 0, 128), x + 51, y + 3, TEXT_ALIGN_CENTER, fps_text);
			draw_text(g_sys_font, al_map_rgba(255, 255, 255, 128), x + 50, y + 2, TEXT_ALIGN_CENTER, fps_text);
			al_scale_transform(&trans, g_scale_x, g_scale_y);
			al_use_transform(&trans);
		}
		al_flip_display();
		s_last_flip_time = al_get_time();
		s_frame_skips = 0;
	}
	else {
		++s_frame_skips;
	}
	if (framerate > 0) {
		s_skipping_frame = s_frame_skips < MAX_FRAME_SKIPS && s_last_flip_time > s_next_frame_time;
		if (s_next_frame_time > al_get_time()) {
			al_wait_for_event_timed(g_events, NULL, s_next_frame_time - al_get_time());
		}
		do {
			if (!do_events()) return false;
		} while (al_get_time() < s_next_frame_time);
		s_next_frame_time += 1.0 / framerate;
	}
	else {
		s_skipping_frame = false;
		if (!do_events()) return false;
	}
	if (!is_backbuffer_valid && !s_skipping_frame)  // did we just finish skipping frames?
		s_next_frame_time = al_get_time() + 1.0 / framerate;
	++s_num_frames;
	if (al_get_time() >= s_next_fps_poll_time) {
		s_current_fps = s_num_flips;
		s_current_game_fps = s_num_frames;
		s_num_flips = 0;
		s_num_frames = 0;
		s_next_fps_poll_time = al_get_time() + 1.0;
	}
	if (!s_skipping_frame) al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	return true;
}

void
unskip_frame(void)
{
	s_skipping_frame = false;
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
}

void
al_draw_tinted_tiled_bitmap(ALLEGRO_BITMAP* bitmap, ALLEGRO_COLOR tint, float x, float y, float width, float height)
{
	ALLEGRO_VERTEX v[] = {
		{ x, y, 0, 0, 0, tint },
		{ x + width, y, 0, width, 0, tint },
		{ x, y + height, 0, 0, height, tint },
		{ x + width, y + height, 0, width, height, tint }
	};
	int w = al_get_bitmap_width(bitmap);
	int h = al_get_bitmap_height(bitmap);
	al_draw_prim(v, NULL, bitmap, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
}

ALLEGRO_BITMAP*
al_fread_bitmap(ALLEGRO_FILE* file, int width, int height)
{
	ALLEGRO_BITMAP*        bitmap = NULL;
	uint8_t*               line_ptr;
	ALLEGRO_LOCKED_REGION* lock = NULL;
	int                    y;

	if ((bitmap = al_create_bitmap(width, height)) == NULL)
		goto on_error;
	if ((lock = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY)) == NULL)
		goto on_error;
	size_t line_size = width * 4;
	for (y = 0; y < height; ++y) {
		line_ptr = (uint8_t*)lock->data + y * lock->pitch;
		if (al_fread(file, line_ptr, line_size) != line_size)
			goto on_error;
	}
	al_unlock_bitmap(bitmap);
	return bitmap;

on_error:
	if (lock != NULL) al_unlock_bitmap(bitmap);
	if (bitmap != NULL) al_destroy_bitmap(bitmap);
	return NULL;
}

static void
initialize_engine(void)
{
	char* path;
	
	// initialize Allegro
	al_init();
	al_init_native_dialog_addon();
	al_init_primitives_addon();
	al_init_image_addon();
	al_init_font_addon();
	al_init_ttf_addon();
	al_install_audio();
	al_init_acodec_addon();
	al_install_keyboard();
	al_install_mouse();
	al_install_joystick();

	// load system configuraton
	path = get_sys_asset_path("system.ini", "system");
	g_sys_conf = al_load_config_file(path);
	free(path);
	
	// initialize networking
	dyad_init();
	dyad_setUpdateTimeout(0.001);
	
	// initialize JavaScript API
	g_duktape = duk_create_heap(NULL, NULL, NULL, NULL, &on_duk_fatal);
	init_api(g_duktape);
	init_bytearray_api();
	init_color_api();
	init_file_api();
	init_font_api(g_duktape);
	init_image_api(g_duktape);
	init_input_api();
	init_logging_api();
	init_map_engine_api(g_duktape);
	init_primitives_api();
	init_rawfile_api();
	init_sockets_api();
	init_sound_api();
	init_spriteset_api(g_duktape);
	init_surface_api();
	init_windowstyle_api();
	
	init_map_engine();
}

static void
queue_key(int keycode)
{
	int key_index;
	
	if (g_key_queue.num_keys < 255) {
		key_index = g_key_queue.num_keys;
		++g_key_queue.num_keys;
		g_key_queue.keys[key_index] = keycode;
	}
}

static void
shutdown_engine(void)
{
	shutdown_map_engine();
	duk_destroy_heap(g_duktape);
	dyad_shutdown();
	al_uninstall_audio();
	al_destroy_display(g_display);
	al_destroy_event_queue(g_events);
	al_destroy_config(g_game_conf);
	al_destroy_path(g_game_path);
	if (g_sys_conf != NULL) al_destroy_config(g_sys_conf);
	al_uninstall_system();
}

static void
toggle_fullscreen(void)
{
	int                  flags;
	ALLEGRO_MONITOR_INFO monitor;
	ALLEGRO_TRANSFORM    transform;
	
	flags = al_get_display_flags(g_display);
	if (flags & ALLEGRO_FULLSCREEN_WINDOW) {
		// switch from fullscreen to windowed, resizing the display manually to work
		// around an Allegro bug
		s_is_fullscreen = false;
		al_set_display_flag(g_display, ALLEGRO_FULLSCREEN_WINDOW, false);
		g_scale_x = g_scale_y = (g_res_x <= 400 || g_res_y <= 300) ? 2.0 : 1.0;
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

static void
on_duk_fatal(duk_context* ctx, duk_errcode_t code, const char* msg)
{
	al_show_native_message_box(g_display, "Script Error", msg, NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
	shutdown_engine();
	exit(0);
}
