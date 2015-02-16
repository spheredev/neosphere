#include "minisphere.h"
#include "api.h"
#include "surface.h"

static void      _apply_blend_mode             (int blend_mode);
static void      _reset_blender                (void);
static duk_ret_t _js_CreateSurface             (duk_context* ctx);
static duk_ret_t _js_LoadSurface               (duk_context* ctx);
static duk_ret_t _js_Surface_finalize          (duk_context* ctx);
static duk_ret_t _js_Surface_setBlendMode      (duk_context* ctx);
static duk_ret_t _js_Surface_blit              (duk_context* ctx);
static duk_ret_t _js_Surface_blitSurface       (duk_context* ctx);
static duk_ret_t _js_Surface_cloneSection      (duk_context* ctx);
static duk_ret_t _js_Surface_drawText          (duk_context* ctx);
static duk_ret_t _js_Surface_gradientRectangle (duk_context* ctx);
static duk_ret_t _js_Surface_rectangle         (duk_context* ctx);
static duk_ret_t _js_Surface_save              (duk_context* ctx);

static void _duk_push_sphere_Surface(duk_context* ctx, ALLEGRO_BITMAP* bitmap);

void
init_surface_api(void)
{
	register_api_const(g_duktape, "BLEND", BLEND_BLEND);
	register_api_const(g_duktape, "REPLACE", BLEND_REPLACE);
	register_api_const(g_duktape, "RGB_ONLY", BLEND_RGB_ONLY);
	register_api_const(g_duktape, "ALPHA_ONLY", BLEND_ALPHA_ONLY);
	register_api_const(g_duktape, "ADD", BLEND_ADD);
	register_api_const(g_duktape, "SUBTRACT", BLEND_SUBTRACT);
	register_api_const(g_duktape, "MULTIPLY", BLEND_MULTIPLY);
	register_api_const(g_duktape, "AVERAGE", BLEND_AVERAGE);
	register_api_const(g_duktape, "INVERT", BLEND_INVERT);
	register_api_func(g_duktape, NULL, "CreateSurface", &_js_CreateSurface);
	register_api_func(g_duktape, NULL, "LoadSurface", &_js_LoadSurface);
}

static void
_apply_blend_mode(int blend_mode)
{
	switch (blend_mode) {
	case BLEND_BLEND:
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
		break;
	case BLEND_REPLACE:
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
		break;
	case BLEND_ADD:
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
		break;
	case BLEND_SUBTRACT:
		al_set_blender(ALLEGRO_DEST_MINUS_SRC, ALLEGRO_ONE, ALLEGRO_ONE);
		break;
	}
}

static void
_reset_blender(void)
{
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
}

static void
_duk_push_sphere_Surface(duk_context* ctx, ALLEGRO_BITMAP* bitmap)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, bitmap); duk_put_prop_string(ctx, -2, "\xFF" "bitmap_ptr");
	duk_push_c_function(ctx, &_js_Surface_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &_js_Surface_setBlendMode, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setBlendMode");
	duk_push_c_function(ctx, &_js_Surface_blit, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blit");
	duk_push_c_function(ctx, &_js_Surface_blitSurface, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blitSurface");
	duk_push_c_function(ctx, &_js_Surface_cloneSection, DUK_VARARGS); duk_put_prop_string(ctx, -2, "cloneSection");
	duk_push_c_function(ctx, &_js_Surface_drawText, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawText");
	duk_push_c_function(ctx, &_js_Surface_gradientRectangle, DUK_VARARGS); duk_put_prop_string(ctx, -2, "gradientRectangle");
	duk_push_c_function(ctx, &_js_Surface_rectangle, DUK_VARARGS); duk_put_prop_string(ctx, -2, "rectangle");
	duk_push_c_function(ctx, &_js_Surface_save, DUK_VARARGS); duk_put_prop_string(ctx, -2, "save");
	duk_push_string(ctx, "width"); duk_push_int(ctx, al_get_bitmap_width(bitmap));
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0
		| DUK_DEFPROP_HAVE_WRITABLE | 0
		| DUK_DEFPROP_HAVE_VALUE);
	duk_push_string(ctx, "height"); duk_push_int(ctx, al_get_bitmap_height(bitmap));
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0
		| DUK_DEFPROP_HAVE_WRITABLE | 0
		| DUK_DEFPROP_HAVE_VALUE);
}

static duk_ret_t
_js_CreateSurface(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	int             w, h;
	
	w = duk_to_number(ctx, 0);
	h = duk_to_number(ctx, 1);
	bitmap = al_create_bitmap(w, h);
	_duk_push_sphere_Surface(ctx, bitmap);
	return 1;
}

static duk_ret_t
_js_LoadSurface(duk_context* ctx)
{
	const char* filename = duk_to_string(ctx, 0);
	char* path = get_asset_path(filename, "images", false);
	ALLEGRO_BITMAP* bitmap = al_load_bitmap(path);
	free(path);
	if (bitmap != NULL) {
		_duk_push_sphere_Surface(ctx, bitmap);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "LoadSurface(): Unable to load image file '%s'", filename);
	}
}

static duk_ret_t
_js_Surface_finalize(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	
	duk_get_prop_string(ctx, 0, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_destroy_bitmap(bitmap);
	return 0;
}

static duk_ret_t
_js_Surface_blit(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	float           x, y;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	x = (float)duk_to_number(ctx, 0);
	y = (float)duk_to_number(ctx, 1);
	al_draw_bitmap(bitmap, x, y, 0x0);
	return 0;
}

static duk_ret_t
_js_Surface_blitSurface(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	int             blend_mode;
	ALLEGRO_BITMAP* src_bitmap;
	float           x, y;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "bitmap_ptr"); src_bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	x = (float)duk_to_number(ctx, 1);
	y = (float)duk_to_number(ctx, 2);
	_apply_blend_mode(blend_mode);
	al_set_target_bitmap(bitmap);
	al_draw_bitmap(src_bitmap, x, y, 0x0);
	al_set_target_backbuffer(g_display);
	_reset_blender();
	return 0;
}

static duk_ret_t
_js_Surface_cloneSection(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_BITMAP* new_bitmap;
	float           x, y, w, h;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	x = duk_to_number(ctx, 0);
	y = duk_to_number(ctx, 1);
	w = duk_to_number(ctx, 2);
	h = duk_to_number(ctx, 3);
	new_bitmap = al_create_bitmap(w, h);
	al_set_target_bitmap(new_bitmap);
	al_draw_bitmap_region(bitmap, x, y, w, h, 0, 0, 0x0);
	al_set_target_backbuffer(g_display);
	_duk_push_sphere_Surface(ctx, new_bitmap);
	return 1;
}

static duk_ret_t
_js_Surface_drawText(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	int             blend_mode;
	ALLEGRO_COLOR   color;
	ALLEGRO_FONT*   font;
	const char*     text;
	float           x, y;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "color_mask");
	duk_get_prop_string(ctx, -1, "red"); color.r = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "green"); color.g = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "blue"); color.b = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "alpha"); color.a = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_pop(ctx);
	x = (float)duk_to_number(ctx, 1);
	y = (float)duk_to_number(ctx, 2);
	text = duk_to_string(ctx, 3);
	_apply_blend_mode(blend_mode);
	al_set_target_bitmap(bitmap);
	al_draw_text(font, color, x, y, 0x0, text);
	al_set_target_backbuffer(g_display);
	_reset_blender();
	return 0;
}

static duk_ret_t
_js_Surface_gradientRectangle(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	int             blend_mode;
	ALLEGRO_COLOR   color_ul, color_ur, color_lr, color_ll;
	float           x1, y1, x2, y2;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	x1 = (float)duk_to_number(ctx, 0);
	y1 = (float)duk_to_number(ctx, 1);
	x2 = x1 + (float)duk_to_number(ctx, 2);
	y2 = y1 + (float)duk_to_number(ctx, 3);
	duk_get_prop_string(ctx, 4, "red"); color_ul .r = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "green"); color_ul.g = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "blue"); color_ul.b = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "alpha"); color_ul.a = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 5, "red"); color_ur.r = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 5, "green"); color_ur.g = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 5, "blue"); color_ur.b = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 5, "alpha"); color_ur.a = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 6, "red"); color_lr.r = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 6, "green"); color_lr.g = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 6, "blue"); color_lr.b = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 6, "alpha"); color_lr.a = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 7, "red"); color_ll.r = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 7, "green"); color_ll.g = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 7, "blue"); color_ll.b = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 7, "alpha"); color_ll.a = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	_apply_blend_mode(blend_mode);
	al_set_target_bitmap(bitmap);
	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, color_ul },
		{ x2, y1, 0, 0, 0, color_ur },
		{ x1, y2, 0, 0, 0, color_ll },
		{ x2, y2, 0, 0, 0, color_lr }
	};
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	al_set_target_backbuffer(g_display);
	_reset_blender();
	return 0;
}

static duk_ret_t
_js_Surface_rectangle(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	int             blend_mode;
	ALLEGRO_COLOR   color;
	float           x, y, w, h;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	x = (float)duk_to_number(ctx, 0);
	y = (float)duk_to_number(ctx, 1);
	w = (float)duk_to_number(ctx, 2);
	h = (float)duk_to_number(ctx, 3);
	duk_get_prop_string(ctx, 4, "red"); color.r = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "green"); color.g = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "blue"); color.b = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "alpha"); color.a = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	_apply_blend_mode(blend_mode);
	al_set_target_bitmap(bitmap);
	al_draw_filled_rectangle(x, y, x + w, y + h, color);
	al_set_target_backbuffer(g_display);
	_reset_blender();
	return 0;
}

static duk_ret_t
_js_Surface_save(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	const char*     filename;
	const char*     path;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	filename = duk_to_string(ctx, 0);
	path = get_asset_path(filename, "images", true);
	al_save_bitmap(path, bitmap);
	return 1;
}

static duk_ret_t
_js_Surface_setBlendMode(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_this(ctx);
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "blend_mode");
	duk_pop(ctx);
	return 0;
}
