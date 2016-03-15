#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "image.h"

#include "windowstyle.h"

static duk_ret_t js_GetSystemWindowStyle      (duk_context* ctx);
static duk_ret_t js_LoadWindowStyle           (duk_context* ctx);
static duk_ret_t js_new_WindowStyle           (duk_context* ctx);
static duk_ret_t js_WindowStyle_finalize      (duk_context* ctx);
static duk_ret_t js_WindowStyle_get_colorMask (duk_context* ctx);
static duk_ret_t js_WindowStyle_set_colorMask (duk_context* ctx);
static duk_ret_t js_WindowStyle_toString      (duk_context* ctx);
static duk_ret_t js_WindowStyle_drawWindow    (duk_context* ctx);

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
	int       refcount;
	int       bg_style;
	color_t   gradient[4];
	image_t*  images[9];
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
load_windowstyle(const char* filename)
{
	sfs_file_t*       file;
	image_t*          image;
	struct rws_header rws;
	int16_t           w, h;
	windowstyle_t*    winstyle = NULL;
	int               i;

	if (!(file = sfs_fopen(g_fs, filename, NULL, "rb"))) goto on_error;
	if ((winstyle = calloc(1, sizeof(windowstyle_t))) == NULL) goto on_error;
	if (sfs_fread(&rws, sizeof(struct rws_header), 1, file) != 1)
		goto on_error;
	if (memcmp(rws.signature, ".rws", 4) != 0) goto on_error;
	switch (rws.version) {
	case 1:
		for (i = 0; i < 9; ++i) {
			if (!(image = read_image(file, rws.edge_w_h, rws.edge_w_h)))
				goto on_error;
			winstyle->images[i] = image;
		}
		break;
	case 2:
		for (i = 0; i < 9; ++i) {
			if (sfs_fread(&w, 2, 1, file) != 1 || sfs_fread(&h, 2, 1, file) != 1)
				goto on_error;
			if (!(image = read_image(file, w, h))) goto on_error;
			winstyle->images[i] = image;
		}
		break;
	default:  // invalid version number
		goto on_error;
	}
	sfs_fclose(file);
	winstyle->bg_style = rws.background_mode;
	for (i = 0; i < 4; ++i) {
		winstyle->gradient[i] = rgba(
			rws.corner_colors[i].r,
			rws.corner_colors[i].g,
			rws.corner_colors[i].b,
			rws.corner_colors[i].a);
	}
	return ref_windowstyle(winstyle);

on_error:
	if (file != NULL) sfs_fclose(file);
	if (winstyle != NULL) {
		for (i = 0; i < 9; ++i)
			free_image(winstyle->images[i]);
		free(winstyle);
	}
	return NULL;
}

windowstyle_t*
ref_windowstyle(windowstyle_t* winstyle)
{
	++winstyle->refcount;
	return winstyle;
}

void
free_windowstyle(windowstyle_t* winstyle)
{
	int i;

	if (winstyle == NULL || --winstyle->refcount > 0)
		return;
	for (i = 0; i < 9; ++i) {
		free_image(winstyle->images[i]);
	}
	free(winstyle);
}

void
draw_window(windowstyle_t* winstyle, color_t mask, int x, int y, int width, int height)
{
	color_t gradient[4];
	int     w[9], h[9];
	
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
		w[i] = get_image_width(winstyle->images[i]);
		h[i] = get_image_height(winstyle->images[i]);
	}
	for (i = 0; i < 4; ++i) {
		gradient[i].r = mask.r * winstyle->gradient[i].r / 255;
		gradient[i].g = mask.g * winstyle->gradient[i].g / 255;
		gradient[i].b = mask.b * winstyle->gradient[i].b / 255;
		gradient[i].alpha = mask.alpha * winstyle->gradient[i].alpha / 255;
	}
	ALLEGRO_VERTEX verts[] = {
		{ x, y, 0, 0, 0, nativecolor(gradient[0]) },
		{ x + width, y, 0, 0, 0, nativecolor(gradient[1]) },
		{ x, y + height, 0, 0, 0, nativecolor(gradient[2]) },
		{ x + width, y + height, 0, 0, 0, nativecolor(gradient[3]) },
	};
	
	switch (winstyle->bg_style) {
	case WSTYLE_BG_TILE:
		draw_image_tiled_masked(winstyle->images[8], mask, x, y, width, height);
		break;
	case WSTYLE_BG_STRETCH:
		draw_image_scaled_masked(winstyle->images[8], mask, x, y, width, height);
		break;
	case WSTYLE_BG_GRADIENT:
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
		break;
	case WSTYLE_BG_TILE_GRADIENT:
		draw_image_tiled_masked(winstyle->images[8], mask, x, y, width, height);
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
		break;
	case WSTYLE_BG_STRETCH_GRADIENT:
		draw_image_scaled_masked(winstyle->images[8], mask, x, y, width, height);
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
		break;
	}
	draw_image_masked(winstyle->images[0], mask, x - w[0], y - h[0]);
	draw_image_masked(winstyle->images[2], mask, x + width, y - h[2]);
	draw_image_masked(winstyle->images[4], mask, x + width, y + height);
	draw_image_masked(winstyle->images[6], mask, x - w[6], y + height);
	draw_image_tiled_masked(winstyle->images[1], mask, x, y - h[1], width, h[1]);
	draw_image_tiled_masked(winstyle->images[3], mask, x + width, y, w[3], height);
	draw_image_tiled_masked(winstyle->images[5], mask, x, y + height, width, h[5]);
	draw_image_tiled_masked(winstyle->images[7], mask, x - w[7], y, w[7], height);
}

void
init_windowstyle_api(void)
{
	const char* filename;
	
	// load system window style
	if (g_sys_conf != NULL) {
		filename = read_string_rec(g_sys_conf, "WindowStyle", "system.rws");
		s_sys_winstyle = load_windowstyle(systempath(filename));
	}

	// WindowStyle API functions
	register_api_method(g_duk, NULL, "GetSystemWindowStyle", js_GetSystemWindowStyle);
	
	// WindowStyle object
	register_api_method(g_duk, NULL, "LoadWindowStyle", js_LoadWindowStyle);
	register_api_ctor(g_duk, "WindowStyle", js_new_WindowStyle, js_WindowStyle_finalize);
	register_api_prop(g_duk, "WindowStyle", "colorMask", js_WindowStyle_get_colorMask, js_WindowStyle_set_colorMask);
	register_api_method(g_duk, "WindowStyle", "toString", js_WindowStyle_toString);
	register_api_method(g_duk, "WindowStyle", "getColorMask", js_WindowStyle_get_colorMask);
	register_api_method(g_duk, "WindowStyle", "setColorMask", js_WindowStyle_set_colorMask);
	register_api_method(g_duk, "WindowStyle", "drawWindow", js_WindowStyle_drawWindow);
}

void
duk_push_sphere_windowstyle(duk_context* ctx, windowstyle_t* winstyle)
{
	duk_push_sphere_obj(ctx, "WindowStyle", ref_windowstyle(winstyle));
	duk_push_sphere_color(ctx, rgba(255, 255, 255, 255)); duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
}

static duk_ret_t
js_GetSystemWindowStyle(duk_context* ctx)
{
	if (s_sys_winstyle == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetSystemWindowStyle(): missing system window style");
	duk_push_sphere_windowstyle(ctx, s_sys_winstyle);
	return 1;
}

static duk_ret_t
js_LoadWindowStyle(duk_context* ctx)
{
	const char*    filename;
	windowstyle_t* winstyle;

	filename = duk_require_path(ctx, 0, "windowstyles", false);
	if (!(winstyle = load_windowstyle(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LoadWindowStyle(): unable to load windowstyle file `%s`", filename);
	duk_push_sphere_windowstyle(ctx, winstyle);
	free_windowstyle(winstyle);
	return 1;
}

static duk_ret_t
js_new_WindowStyle(duk_context* ctx)
{
	const char*    filename;
	windowstyle_t* winstyle;

	filename = duk_require_path(ctx, 0, NULL, false);
	if (!(winstyle = load_windowstyle(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "WindowStyle(): unable to load windowstyle file `%s`", filename);
	duk_push_sphere_windowstyle(ctx, winstyle);
	free_windowstyle(winstyle);
	return 1;
}

static duk_ret_t
js_WindowStyle_finalize(duk_context* ctx)
{
	windowstyle_t* winstyle;

	winstyle = duk_require_sphere_obj(ctx, 0, "WindowStyle");
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
js_WindowStyle_get_colorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_require_sphere_obj(ctx, -1, "WindowStyle");
	duk_get_prop_string(ctx, -2, "\xFF" "color_mask");
	duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_WindowStyle_set_colorMask(duk_context* ctx)
{
	color_t mask = duk_require_sphere_color(ctx, 0);
	
	duk_push_this(ctx);
	duk_require_sphere_obj(ctx, -1, "WindowStyle");
	duk_push_sphere_color(ctx, mask);
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
	duk_pop(ctx);
	return 0;
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
	winstyle = duk_require_sphere_obj(ctx, -1, "WindowStyle");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	draw_window(winstyle, mask, x, y, w, h);
	return 0;
}
