#include "duktape.h"
#include <SFML/System.h>
#include <SFML/Window.h>
#include <SFML/Graphics.h>
#include <SFML/Audio.h>

extern sfRenderWindow* g_window;

static void _registerScriptFunc(duk_context* ctx, const char* name, duk_c_function fn);
static int  duk_CreateColor(duk_context* ctx);
static int  duk_FlipScreen(duk_context* ctx);
static int  duk_Rectangle(duk_context* ctx);

void
InitializeAPI(duk_context* ctx)
{
	_registerScriptFunc(ctx, "CreateColor", &duk_CreateColor);
	_registerScriptFunc(ctx, "FlipScreen", &duk_FlipScreen);
	_registerScriptFunc(ctx, "Rectangle", &duk_Rectangle);
}

void
_registerScriptFunc(duk_context* ctx, const char* name, duk_c_function fn)
{
	duk_push_global_object(ctx);
	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, name);
	duk_pop(ctx);
}

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

int
duk_CreateColor(duk_context* ctx)
{
	int nArgs = duk_get_top(ctx);
	int r = duk_to_int(ctx, 0);
	int g = duk_to_int(ctx, 1);
	int b = duk_to_int(ctx, 2);
	int a = 255;
	if (nArgs >= 4) {
		a = duk_to_int(ctx, 3);
	}
	duk_push_object(ctx);
	duk_push_int(ctx, r);
	duk_put_prop_string(ctx, -2, "r");
	duk_push_int(ctx, g);
	duk_put_prop_string(ctx, -2, "g");
	duk_push_int(ctx, b);
	duk_put_prop_string(ctx, -2, "b");
	duk_push_int(ctx, a);
	duk_put_prop_string(ctx, -2, "a");
	return 1;
}

int
duk_Rectangle(duk_context* ctx)
{
	int nArgs = duk_get_top(ctx);
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int cx = duk_to_int(ctx, 2);
	int cy = duk_to_int(ctx, 3);
	duk_get_prop_string(ctx, -1, "r");
	int r = duk_to_int(ctx, -1);
	duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "g");
	int g = duk_to_int(ctx, -1);
	duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "b");
	int b = duk_to_int(ctx, -1);
	duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "a");
	int a = duk_to_int(ctx, -1);
	duk_pop(ctx);
	// TODO: render rect to window
	return 0;
}
