#include "minisphere.h"
#include "sphere_api.h"

static void on_duk_fatal (duk_context* ctx, duk_errcode_t code, const char* msg);

static void  handle_js_error ();
static void  shutdown_engine ();

ALLEGRO_DISPLAY*     g_display         = NULL;
duk_context*         g_duktape         = NULL;
ALLEGRO_EVENT_QUEUE* g_events          = NULL;
char                 g_game_path[1024] = "";
ALLEGRO_FONT*        g_sys_font        = NULL;

// enable visual styles (VC++)
#pragma comment(linker, \
    "\"/manifestdependency:type='Win32' "\
    "name='Microsoft.Windows.Common-Controls' "\
    "version='6.0.0.0' "\
    "processorArchitecture='*' "\
    "publicKeyToken='6595b64144ccf1df' "\
    "language='*'\"")

int
main(int argc, char** argv)
{
	if (argc > 2 && strncmp(argv[1], "-game", 5) == 0) {
		strncpy(g_game_path, argv[2], 1024);
		g_game_path[1023] = '\0';
	}
	else {
		getcwd(g_game_path, sizeof g_game_path);
	}

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
	g_sys_font = al_load_ttf_font("consola.ttf", 8, 0x0);
	g_display = al_create_display(320, 240);
	al_set_window_title(g_display, "minisphere");
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	g_events = al_create_event_queue();
	al_register_event_source(g_events, al_get_display_event_source(g_display));
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_flip_display();

	// load startup script
	duk_int_t exec_result;
	char* script_path = normalize_path("main.js", "scripts");
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

void
on_duk_fatal(duk_context* ctx, duk_errcode_t code, const char* msg)
{
	al_show_native_message_box(g_display, "Script Error", msg, NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
	shutdown_engine();
	exit(0);
}

void
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
		char* file_name = strrchr(file_path, '\\');
		file_name = file_name != NULL ? (file_name + 1) : file_path;
		duk_pop(g_duktape);
		duk_push_sprintf(g_duktape, "%s (line %d)\n%s", file_name, (int)line_num, err_msg);
		duk_fatal(g_duktape, err_code, duk_get_string(g_duktape, -1));
	}
}

char*
normalize_path(const char* path, const char* base_dir)
{
	char* norm_path = strdup(path);
	size_t path_len = strlen(norm_path);
	for (char* c = norm_path; *c != '\0'; ++c) {
		if (*c == '\\') *c = '/';
	}
	if (norm_path[0] == '/' || norm_path[1] == ':')
		// absolute path - not allowed
		return NULL;
	bool is_homed = (strstr(norm_path, "~/") == norm_path);
	char* out_path = NULL;
	if (is_homed) {
		size_t buf_size = strlen(g_game_path) + strlen(norm_path + 2) + 2;
		out_path = malloc(buf_size);
		sprintf(out_path, "%s/%s", g_game_path, norm_path + 2);
	}
	else {
		size_t buf_size = strlen(g_game_path) + strlen(base_dir) + strlen(norm_path) + 3;
		out_path = malloc(buf_size);
		sprintf(out_path, "%s/%s/%s", g_game_path, base_dir, norm_path);
	}
	free(norm_path);
	return out_path;
}

void
shutdown_engine(void)
{
	al_uninstall_audio();
	al_destroy_display(g_display);
	al_destroy_event_queue(g_events);
	al_destroy_font(g_sys_font);
	al_uninstall_system();
	duk_destroy_heap(g_duktape);
}
