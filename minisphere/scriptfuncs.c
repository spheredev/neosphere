#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include "duktape.h"

extern ALLEGRO_DISPLAY*     g_display;
extern ALLEGRO_EVENT_QUEUE* g_events;

static int duk_CreateColor(duk_context* ctx);
static int duk_FlipScreen(duk_context* ctx);
static int duk_Rectangle(duk_context* ctx);

void
_registerScriptFunc(duk_context* ctx, const char* className, const char* name, duk_c_function fn)
{
	duk_push_global_object(ctx);
	if (className != NULL) {
		duk_get_prop_string(ctx, -1, className);
		duk_get_prop_string(ctx, -1, "prototype");
	}
	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, name);
	if (className != NULL) {
		duk_pop(ctx);
		duk_pop(ctx);
	}
	duk_pop(ctx);
}

void
InitializeAPI(duk_context* ctx)
{
	_registerScriptFunc(ctx, NULL, "CreateColor", &duk_CreateColor);
	_registerScriptFunc(ctx, NULL, "FlipScreen", &duk_FlipScreen);
	_registerScriptFunc(ctx, NULL, "Rectangle", &duk_Rectangle);
}

int
duk_Abort(duk_context* ctx)
{
	const char* message = duk_get_string(ctx, 0);
	duk_error(ctx, DUK_ERR_UNCAUGHT_ERROR, "%s", message);
	return 0;
}

int
duk_CreateColor(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int r = duk_get_int(ctx, 0);
	int g = duk_get_int(ctx, 1);
	int b = duk_get_int(ctx, 2);
	int a = n_args > 3 ? duk_get_int(ctx, 3) : 255;
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
duk_FlipScreen(duk_context* ctx)
{
	ALLEGRO_EVENT event;
	ALLEGRO_TIMEOUT timeout;
	al_init_timeout(&timeout, 0.05);
	bool got_event = al_wait_for_event_until(g_events, &event, &timeout);
	if (got_event && event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
		duk_error(ctx, DUK_ERR_NONE, "Display window closed by user");
	}
	al_flip_display();
	al_clear_to_color(al_map_rgb(0, 0, 0));
	return 0;
}

int
duk_Rectangle(duk_context* ctx)
{
	float x = (float)duk_get_number(ctx, 0);
	float y = (float)duk_get_number(ctx, 1);
	float width = (float)duk_get_number(ctx, 2);
	float height = (float)duk_get_number(ctx, 3);
	duk_get_prop_string(ctx, -1, "r");
	int r = duk_get_int(ctx, -1);
	duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "g");
	int g = duk_get_int(ctx, -1);
	duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "b");
	int b = duk_get_int(ctx, -1);
	duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "a");
	int a = duk_get_int(ctx, -1);
	duk_pop(ctx);
	al_draw_filled_rectangle(x, y, x + width, y + height, al_map_rgba(r, g, b, a));
	return 0;
}