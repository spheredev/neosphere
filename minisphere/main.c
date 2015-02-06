#include "minisphere.h"
#include "sphere_api.h"

static void handle_js_error();
static void do_teardown();

static void on_duk_fatal(duk_context* ctx, duk_errcode_t code, const char* msg);

ALLEGRO_DISPLAY*     g_display = NULL;
duk_context*         g_duktape  = NULL;
ALLEGRO_EVENT_QUEUE* g_events   = NULL;
ALLEGRO_FONT*        g_sys_font = NULL;

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
	duk_int_t exec_result;
	
	// initialize JavaScript engine
	g_duktape = duk_create_heap(NULL, NULL, NULL, NULL, &on_duk_fatal);
	init_sphere_api(g_duktape);
	
	// initialize Allegro and create display window
	al_init();
	al_init_native_dialog_addon();
	al_init_primitives_addon();
	al_init_font_addon();
	al_init_ttf_addon();
	g_sys_font = al_load_ttf_font("consola.ttf", 8, 0x0);
	g_display = al_create_display(320, 240);
	al_set_window_title(g_display, "minisphere");
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	g_events = al_create_event_queue();
	al_register_event_source(g_events, al_get_display_event_source(g_display));
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_flip_display();

	// inject test script
	duk_push_string(g_duktape, "function game() { while (true) { Rectangle(10, 10, 100, 100, CreateColor(0, 0, 255, 128)); OutlinedRectangle(10, 10, 100, 100, CreateColor(255, 0, 0, 128), 3); FlipScreen(); } }");
	duk_push_string(g_duktape, "main.js");
	exec_result = duk_pcompile(g_duktape, 0x0);
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
	do_teardown();
	return 0;
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

void
do_teardown(void)
{
	al_destroy_display(g_display);
	al_destroy_event_queue(g_events);
	al_destroy_font(g_sys_font);
	al_uninstall_system();
	duk_destroy_heap(g_duktape);
}

void
on_duk_fatal(duk_context* ctx, duk_errcode_t code, const char* msg)
{
	al_show_native_message_box(g_display, "Script Error", msg, NULL, NULL, ALLEGRO_MESSAGEBOX_ERROR);
	do_teardown();
	exit(0);
}
