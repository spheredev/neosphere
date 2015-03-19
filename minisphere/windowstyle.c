#include "minisphere.h"
#include "api.h"
#include "color.h"

#include "windowstyle.h"

struct windowstyle
{
	int             bg_style;
	ALLEGRO_BITMAP* bitmaps[9];
};

static void            _duk_push_winstyle          (duk_context* ctx, windowstyle_t* winstyle);
static duk_ret_t       js_GetSystemWindowStyle     (duk_context* ctx);
static duk_ret_t       js_LoadWindowStyle          (duk_context* ctx);
static duk_ret_t       js_WindowStyle_finalize     (duk_context* ctx);
static duk_ret_t       js_WindowStyle_toString     (duk_context* ctx);
static duk_ret_t       js_WindowStyle_drawWindow   (duk_context* ctx);
static duk_ret_t       js_WindowStyle_setColorMask (duk_context* ctx);

static windowstyle_t* s_sys_winstyle = NULL;

enum wstyle_bg_type
{
	WSTYLE_BG_TILE,
	WSTYLE_BG_STRETCH,
	WSTYLE_BG_GRADIENT,
	WSTYLE_BG_TILE_GRADIENT,
	WSTYLE_BG_STRETCH_GRADIENT
};

#pragma pack(push, 1)
struct rws_rgba
{
	uint8_t r, g, b, a;
};

struct rws_header
{
	uint8_t         signature[4];
	int16_t         version;
	uint8_t         edge_width;
	uint8_t         background_mode;
	struct rws_rgba corner_colors[4];
	uint8_t         edge_offsets[4];
	uint8_t         reserved[36];
};
#pragma pack(pop)

windowstyle_t*
load_windowstyle(const char* path)
{
	ALLEGRO_BITMAP*     bitmap;
	ALLEGRO_FILE*       file;
	struct rws_header   rws;
	int16_t             w, h;
	windowstyle_t*      winstyle = NULL;
	int                 i;

	if ((file = al_fopen(path, "rb")) == NULL) goto on_error;
	if ((winstyle = calloc(1, sizeof(windowstyle_t))) == NULL) goto on_error;
	if (al_fread(file, &rws, sizeof(struct rws_header)) != sizeof(struct rws_header))
		goto on_error;
	if (memcmp(rws.signature, ".rws", 4) != 0) goto on_error;
	switch (rws.version) {
	case 1:
		for (i = 0; i < 9; ++i) {
			if ((bitmap = al_fread_bitmap(file, rws.edge_width, rws.edge_width)) == NULL)
				goto on_error;
			winstyle->bitmaps[i] = bitmap;
		}
		break;
	case 2:
		for (i = 0; i < 9; ++i) {
			if (al_fread(file, &w, 2) != 2 || al_fread(file, &h, 2) != 2)
				goto on_error;
			if ((bitmap = al_fread_bitmap(file, w, h)) == NULL)
				goto on_error;
			winstyle->bitmaps[i] = bitmap;
		}
		break;
	default:  // invalid version number
		goto on_error;
	}
	al_fclose(file);
	winstyle->bg_style = rws.background_mode;
	return winstyle;

on_error:
	if (file != NULL) al_fclose(file);
	if (winstyle != NULL) {
		for (i = 0; i < 9; ++i) {
			if (winstyle->bitmaps[i] != NULL) al_destroy_bitmap(winstyle->bitmaps[i]);
		}
		free(winstyle);
	}
	return NULL;
}

void
free_windowstyle(windowstyle_t* winstyle)
{
	int i;

	for (i = 0; i < 9; ++i) {
		al_destroy_bitmap(winstyle->bitmaps[i]);
	}
	al_free(winstyle);
}

void
draw_window(windowstyle_t* winstyle, ALLEGRO_COLOR mask, int x, int y, int width, int height)
{
	int w[9], h[9];
	int i;
	
	// 0 - upper left
	// 1 - top
	// 2 - upper right
	// 3 - right
	// 4 - lower right
	// 5 - bottom
	// 6 - lower left
	// 7 - left
	// 8 - background

	for (i = 0; i < 9; ++i) {
		w[i] = al_get_bitmap_width(winstyle->bitmaps[i]);
		h[i] = al_get_bitmap_height(winstyle->bitmaps[i]);
	}
	
	switch (winstyle->bg_style) {
	case WSTYLE_BG_TILE:
		al_draw_tinted_tiled_bitmap(winstyle->bitmaps[8], mask, x, y, width, height);
		break;
	case WSTYLE_BG_STRETCH:
		al_draw_tinted_scaled_bitmap(winstyle->bitmaps[8], mask, 0, 0, w[8], h[8], x, y, width, height, 0x0);
		break;
	}
	al_draw_tinted_bitmap(winstyle->bitmaps[0], mask, x - w[0], y - h[0], 0x0);
	al_draw_tinted_bitmap(winstyle->bitmaps[2], mask, x + width, y - h[2], 0x0);
	al_draw_tinted_bitmap(winstyle->bitmaps[4], mask, x + width, y + height, 0x0);
	al_draw_tinted_bitmap(winstyle->bitmaps[6], mask, x - w[6], y + height, 0x0);
	al_draw_tinted_tiled_bitmap(winstyle->bitmaps[1], mask, x, y - h[1], width, h[1]);
	al_draw_tinted_tiled_bitmap(winstyle->bitmaps[3], mask, x + width, y, w[3], height);
	al_draw_tinted_tiled_bitmap(winstyle->bitmaps[5], mask, x, y + height, width, h[5]);
	al_draw_tinted_tiled_bitmap(winstyle->bitmaps[7], mask, x - w[7], y, w[7], height);
}

void
init_windowstyle_api(void)
{
	const char*    filename;
	char*          path;
	
	// load system window style
	if (g_sys_conf != NULL) {
		filename = al_get_config_value(g_sys_conf, NULL, "WindowStyle");
		path = get_sys_asset_path(filename, "system");
		s_sys_winstyle = load_windowstyle(path);
		free(path);
	}

	// register windowstyle API functions
	register_api_func(g_duktape, NULL, "GetSystemWindowStyle", js_GetSystemWindowStyle);
	register_api_func(g_duktape, NULL, "LoadWindowStyle", js_LoadWindowStyle);
}

static void
_duk_push_winstyle(duk_context* ctx, windowstyle_t* winstyle)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, winstyle); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_sphere_color(ctx, al_map_rgba(255, 255, 255, 255)); duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
	duk_push_c_function(ctx, js_WindowStyle_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, js_WindowStyle_toString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "toString");
	duk_push_c_function(ctx, js_WindowStyle_drawWindow, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawWindow");
	duk_push_c_function(ctx, js_WindowStyle_setColorMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setColorMask");
}

static duk_ret_t
js_GetSystemWindowStyle(duk_context* ctx)
{
	if (s_sys_winstyle != NULL) {
		_duk_push_winstyle(ctx, s_sys_winstyle);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "GetSystemWindowStyle(): No system window style available");
	}
}

static duk_ret_t
js_LoadWindowStyle(duk_context* ctx)
{
	const char*    filename = NULL;
	char*          path = NULL;
	windowstyle_t* winstyle = NULL;

	filename = duk_require_string(ctx, 0);
	path = get_asset_path(filename, "windowstyles", false);
	if ((winstyle = load_windowstyle(path)) == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "LoadWindowStyle(): Unable to load windowstyle file '%s'", filename);
	free(path);
	_duk_push_winstyle(ctx, winstyle);
	return 1;
}

static duk_ret_t
js_WindowStyle_finalize(duk_context* ctx)
{
	windowstyle_t* winstyle;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); winstyle = duk_get_pointer(ctx, -1); duk_pop(ctx);
	//free_windowstyle(winstyle);
	return 0;
}

static duk_ret_t
js_WindowStyle_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object windowstyle]");
	return 1;
}

static duk_ret_t
js_WindowStyle_drawWindow(duk_context* ctx)
{
	ALLEGRO_COLOR  color_mask;
	windowstyle_t* winstyle;
	int            x, y, w, h;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	w = duk_to_int(ctx, 2);
	h = duk_to_int(ctx, 3);
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); winstyle = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); color_mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	draw_window(winstyle, color_mask, x, y, w, h);
	return 0;
}

static duk_ret_t
js_WindowStyle_setColorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
	duk_pop(ctx);
	return 0;
}
