#include "minisphere.h"
#include "sphere_api.h"

static void on_duk_fatal    (duk_context* ctx, duk_errcode_t code, const char* msg);
static void handle_js_error ();
static void shutdown_engine ();

ALLEGRO_DISPLAY*     g_display         = NULL;
duk_context*         g_duktape         = NULL;
ALLEGRO_EVENT_QUEUE* g_events          = NULL;
ALLEGRO_PATH*        g_game_path       = NULL;
ALLEGRO_FONT*        g_sys_font        = NULL;

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

int
main(int argc, char** argv)
{
	char* game_dir = al_get_current_directory();
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-game") == 0 && i < argc - 1) {
			game_dir = al_realloc(game_dir, strlen(argv[i + 1]) + 1);
			strcpy(game_dir, argv[i + 1]);
		}
	}
	g_game_path = al_create_path(game_dir);
	if (strcmp(al_get_path_filename(g_game_path), "game.sgm") != 0) {
		al_destroy_path(g_game_path);
		g_game_path = al_create_path_for_directory(game_dir);
	}
	al_make_path_canonical(g_game_path);
	al_free(game_dir);

	// initialize JavaScript engine
	g_duktape = duk_create_heap(NULL, NULL, NULL, NULL, &on_duk_fatal);
	init_sphere_api(g_duktape);
	
	// initialize Allegro and create display window
	al_init();
	al_init_native_dialog_addon();
	al_init_primitives_addon();
	al_init_font_addon();
	al_init_ttf_addon();
	al_install_audio();
	al_init_acodec_addon();
	al_reserve_samples(8);
	al_set_mixer_gain(al_get_default_mixer(), 1.0);
	g_sys_font = al_load_font("consola.ttf", 8, 0x0);
	g_display = al_create_display(320, 240);
	al_set_window_title(g_display, "minisphere");
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	g_events = al_create_event_queue();
	al_register_event_source(g_events, al_get_display_event_source(g_display));
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_flip_display();

	// load startup script
	duk_int_t exec_result;
	char* script_path = get_asset_path("main.js", "scripts");
	exec_result = duk_pcompile_file(g_duktape, 0x0, script_path);
	free(script_path);
	if (exec_result != DUK_EXEC_SUCCESS) {
		handle_js_error();
	}
	exec_result = duk_pcall(g_duktape, 0);
	if (exec_result != DUK_EXEC_SUCCESS) {
		handle_js_error();
	}
	duk_pop(g_duktape);

	// call game() function in script
	duk_push_global_object(g_duktape);
	duk_get_prop_string(g_duktape, -1, "game");
	exec_result = duk_pcall(g_duktape, 0);
	if (exec_result != DUK_EXEC_SUCCESS) {
		handle_js_error();
	}
	duk_pop(g_duktape);
	duk_pop(g_duktape);
	
	// teardown
	shutdown_engine();
	return 0;
}

char*
get_asset_path(const char* path, const char* base_dir)
{
	bool is_homed = (strstr(path, "~/") == path || strstr(path, "~\\") == path);
	ALLEGRO_PATH* base_path = al_create_path_for_directory(base_dir);
	al_rebase_path(g_game_path, base_path);
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

static void
on_duk_fatal(duk_context* ctx, duk_errcode_t code, const char* msg)
{
	al_show_native_message_box(g_display, "Script Error", msg, NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
	shutdown_engine();
	exit(0);
}

static void
handle_js_error()
{
	duk_errcode_t err_code = duk_get_error_code(g_duktape, -1);
	duk_dup(g_duktape, -1);
	const char* err_msg = duk_safe_to_string(g_duktape, -1);
	if (err_code != DUK_ERR_ERROR || strcmp(err_msg, "Error: !exit") != 0) {
		duk_get_prop_string(g_duktape, -2, "lineNumber");
		duk_int_t line_num = duk_get_int(g_duktape, -1);
		duk_pop(g_duktape);
		duk_get_prop_string(g_duktape, -2, "fileName");
		const char* file_path = duk_get_string(g_duktape, -1);
		char* file_name = strrchr(file_path, '/');
		file_name = file_name != NULL ? (file_name + 1) : file_path;
		duk_pop(g_duktape);
		duk_push_sprintf(g_duktape, "%s (line %d)\n%s", file_name, (int)line_num, err_msg);
		duk_fatal(g_duktape, err_code, duk_get_string(g_duktape, -1));
	}
}

static void
shutdown_engine(void)
{
	al_uninstall_audio();
	al_destroy_display(g_display);
	al_destroy_event_queue(g_events);
	al_destroy_font(g_sys_font);
	al_destroy_path(g_game_path);
	al_uninstall_system();
	duk_destroy_heap(g_duktape);
}
