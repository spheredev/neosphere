#include "duktape.h"
#include <SFML/System.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>

#include "scriptfuncs.h"

duk_context*    g_duktape = 0;
sfRenderWindow* g_window  = 0;

int
main(int argc, char* argv[])
{
	// initialize JavaScript engine
	g_duktape = duk_create_heap_default();
	InitializeAPI(g_duktape);
	
	// create render window
	sfVideoMode videoMode;
	videoMode.width = 640;
	videoMode.height = 480;
	videoMode.bitsPerPixel = 32;
	g_window = sfRenderWindow_create(videoMode, "minisphere", sfDefaultStyle & ~sfResize, 0);
	sfRenderWindow_clear(g_window, sfColor_fromRGB(0, 0, 0));
	
	// inject test script
	duk_peval_string_noresult(g_duktape, "function game() { while (true) { Rectangle(10, 10, 110, 110, CreateColor(0, 128, 0, 255)); FlipScreen(); } }");
	
	// call game() function in script
	duk_push_global_object(g_duktape);
	duk_get_prop_string(g_duktape, -1, "game");
	duk_pcall(g_duktape, 0);
	duk_pop(g_duktape);
	duk_pop(g_duktape);
	
	// teardown
	duk_destroy_heap(g_duktape);
	return 0;
}
