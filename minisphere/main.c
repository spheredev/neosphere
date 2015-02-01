#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include "duktape.h"

#include "scriptfuncs.h"

ALLEGRO_DISPLAY*     g_display = NULL;
ALLEGRO_EVENT_QUEUE* g_events  = NULL;
duk_context*         g_duktape = NULL;

int
main(int argc, char** argv)
{
	// initialize JavaScript engine
	g_duktape = duk_create_heap_default();
	InitializeAPI(g_duktape);
	
	// create render window
	al_init();
	al_init_primitives_addon();
	g_display = al_create_display(320, 240);
	al_set_window_title(g_display, "minisphere");
	g_events = al_create_event_queue();
	al_register_event_source(g_events, al_get_display_event_source(g_display));
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_flip_display();

	// inject test script
	duk_peval_string_noresult(g_duktape, "function game() { while (true) { Rectangle(10, 10, 100, 100, CreateColor(0, 0, 128, 128)); Rectangle(80, 80, 100, 100, CreateColor(128, 0, 0, 128)); FlipScreen(); } }");
	
	// call game() function in script
	duk_push_global_object(g_duktape);
	duk_get_prop_string(g_duktape, -1, "game");
	duk_pcall(g_duktape, 0);
	duk_pop(g_duktape);
	duk_pop(g_duktape);
	
	// teardown
	al_destroy_display(g_display);
	al_destroy_event_queue(g_events);
	al_uninstall_system();
	duk_destroy_heap(g_duktape);
	return 0;
}
