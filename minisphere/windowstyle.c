#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "image.h"

#include "windowstyle.h"

static duk_ret_t js_GetSystemWindowStyle(duk_context* ctx);
static duk_ret_t js_LoadWindowStyle          (duk_context* ctx);
static duk_ret_t js_WindowStyle_finalize     (duk_context* ctx);
static duk_ret_t js_WindowStyle_toString     (duk_context* ctx);
static duk_ret_t js_WindowStyle_drawWindow   (duk_context* ctx);
static duk_ret_t js_WindowStyle_setColorMask (duk_context* ctx);

static windowstyle_t* s_sys_winstyle = NULL;

enum wstyle_bg_type
{
	WSTYLE_BG_TILE,
	WSTYLE_BG_STRETCH,
	WSTYLE_BG_GRADIENT,
	WSTYLE_BG_TILE_GRADIENT,
	WSTYLE_BG_STRETCH_GRADIENT
};

struct windowstyle
{
	int      bg_style;
	image_t* images[9];
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
	uint8_t         edge_w_h;
	uint8_t         background_mode;
	struct rws_rgba corner_colors[4];
	uint8_t         edge_offsets[4];
	uint8_t         reserved[36];
};
#pragma pack(pop)

windowstyle_t*
load_windowstyle(const char* path)
{
	image_t*          atlas = NULL;
	ALLEGRO_FILE*     file;
	image_t*          image;
	int16_t           max_w = 0, max_h = 0;
	struct rws_header rws;
	int16_t           w, h;
	windowstyle_t*    winstyle = NULL;
	int               i;

	if ((file = al_fopen(path, "rb")) == NULL) goto on_error;
	if ((winstyle = calloc(1, sizeof(windowstyle_t))) == NULL) goto on_error;
	if (al_fread(file, &rws, sizeof(struct rws_header)) != sizeof(struct rws_header))
		goto on_error;
	if (memcmp(rws.signature, ".rws", 4) != 0) goto on_error;
	switch (rws.version) {
	case 1:
		max_w = rws.edge_w_h; max_h = rws.edge_w_h;
		if (!(atlas = create_image(rws.edge_w_h * 3, rws.edge_w_h * 3)))
			goto on_error;
		for (i = 0; i < 9; ++i) {
			if (!(image = read_image(file, rws.edge_w_h, rws.edge_w_h)))
				goto on_error;
			winstyle->images[i] = image;
		}
		break;
	case 2:
		for (i = 0; i < 9; ++i) {
			if (al_fread(file, &w, 2) != 2 || al_fread(file, &h, 2) != 2)
				goto on_error;
			if ((image = read_image(file, w, h)) == NULL)
				goto on_error;
			winstyle->images[i] = image;
		}
		break;
	default:  // invalid version number
		goto on_error;
	}
	al_fclose(file);
	winstyle->bg_style = rws.background_mode;
	free_image(atlas);
	return winstyle;

on_error:
	if (file != NULL) al_fclose(file);
	if (winstyle != NULL) {
		for (i = 0; i < 9; ++i)
			free_image(winstyle->images[i]);
		free(winstyle);
	}
	free_image(atlas);
	return NULL;
}

void
free_windowstyle(windowstyle_t* winstyle)
{
	int i;

	for (i = 0; i < 9; ++i) {
		free_image(winstyle->images[i]);
	}
	free(winstyle);
}

void
draw_window(windowstyle_t* winstyle, color_t mask, int x, int y, int width, int height)
{
	ALLEGRO_BITMAP* bitmaps[9];
	ALLEGRO_COLOR   native_mask = to_native_color(mask);
	int             w[9], h[9];
	
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
		bitmaps[i] = get_image_bitmap(winstyle->images[i]);
		w[i] = get_image_width(winstyle->images[i]);
		h[i] = get_image_height(winstyle->images[i]);
	}
	
	switch (winstyle->bg_style) {
	case WSTYLE_BG_TILE:
		al_draw_tinted_tiled_bitmap(bitmaps[8], native_mask, x, y, width, height);
		break;
	case WSTYLE_BG_STRETCH:
		al_draw_tinted_scaled_bitmap(bitmaps[8], native_mask, 0, 0, w[8], h[8], x, y, width, height, 0x0);
		break;
	}
	al_draw_tinted_bitmap(bitmaps[0], native_mask, x - w[0], y - h[0], 0x0);
	al_draw_tinted_bitmap(bitmaps[2], native_mask, x + width, y - h[2], 0x0);
	al_draw_tinted_bitmap(bitmaps[4], native_mask, x + width, y + height, 0x0);
	al_draw_tinted_bitmap(bitmaps[6], native_mask, x - w[6], y + height, 0x0);
	al_draw_tinted_tiled_bitmap(bitmaps[1], native_mask, x, y - h[1], width, h[1]);
	al_draw_tinted_tiled_bitmap(bitmaps[3], native_mask, x + width, y, w[3], height);
	al_draw_tinted_tiled_bitmap(bitmaps[5], native_mask, x, y + height, width, h[5]);
	al_draw_tinted_tiled_bitmap(bitmaps[7], native_mask, x - w[7], y, w[7], height);
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
duk_push_sphere_windowstyle(duk_context* ctx, windowstyle_t* winstyle)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, winstyle); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_sphere_color(ctx, rgba(255, 255, 255, 255)); duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
	duk_push_c_function(ctx, js_WindowStyle_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, js_WindowStyle_toString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "toString");
	duk_push_c_function(ctx, js_WindowStyle_drawWindow, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawWindow");
	duk_push_c_function(ctx, js_WindowStyle_setColorMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setColorMask");
}

static duk_ret_t
js_GetSystemWindowStyle(duk_context* ctx)
{
	if (s_sys_winstyle == NULL)
		js_error(JS_REF_ERROR, -1, "GetSystemWindowStyle(): No system window style available");
	duk_push_sphere_windowstyle(ctx, s_sys_winstyle);
	return 1;
}

static duk_ret_t
js_LoadWindowStyle(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	char*          path = NULL;
	windowstyle_t* winstyle = NULL;

	path = get_asset_path(filename, "windowstyles", false);
	if (!(winstyle = load_windowstyle(path)))
		js_error(JS_ERROR, -1, "LoadWindowStyle(): Failed to load windowstyle file '%s'", filename);
	free(path);
	duk_push_sphere_windowstyle(ctx, winstyle);
	return 1;
}

static duk_ret_t
js_WindowStyle_finalize(duk_context* ctx)
{
	windowstyle_t* winstyle;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); winstyle = duk_get_pointer(ctx, -1); duk_pop(ctx);
	free_windowstyle(winstyle);
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
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	
	color_t        mask;
	windowstyle_t* winstyle;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); winstyle = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	draw_window(winstyle, mask, x, y, w, h);
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
