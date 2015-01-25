#include "duktape.h"
#include <SFML/System.h>
#include <SFML/Window.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>

extern sfRenderWindow* g_window;

int
duk_FlipScreen(duk_context* ctx)
{
	sfRenderWindow_display(g_window);
	sfRenderWindow_clear(g_window, sfColor_fromRGB(0, 0, 0));
	sfEvent event;
	while (sfRenderWindow_pollEvent(g_window, &event)) {
		if (event.type == sfEvtClosed) {
			sfRenderWindow_close(g_window);
			duk_push_string(ctx, "");
			duk_throw(ctx);
		}
	}
	return 0;
}
