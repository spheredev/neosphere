#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "image.h"

#include "surface.h"

static void      _apply_blend_mode             (int blend_mode);
static void      _reset_blender                (void);
static duk_ret_t js_CreateSurface             (duk_context* ctx);
static duk_ret_t js_GrabSurface               (duk_context* ctx);
static duk_ret_t js_LoadSurface               (duk_context* ctx);
static duk_ret_t js_Surface_finalize          (duk_context* ctx);
static duk_ret_t js_Surface_setAlpha          (duk_context* ctx);
static duk_ret_t js_Surface_setBlendMode      (duk_context* ctx);
static duk_ret_t js_Surface_blit              (duk_context* ctx);
static duk_ret_t js_Surface_blitMaskSurface   (duk_context* ctx);
static duk_ret_t js_Surface_blitSurface       (duk_context* ctx);
static duk_ret_t js_Surface_clone             (duk_context* ctx);
static duk_ret_t js_Surface_cloneSection      (duk_context* ctx);
static duk_ret_t js_Surface_createImage       (duk_context* ctx);
static duk_ret_t js_Surface_drawText          (duk_context* ctx);
static duk_ret_t js_Surface_gradientRectangle (duk_context* ctx);
static duk_ret_t js_Surface_line              (duk_context* ctx);
static duk_ret_t js_Surface_outlinedRectangle (duk_context* ctx);
static duk_ret_t js_Surface_rotate            (duk_context* ctx);
static duk_ret_t js_Surface_rectangle         (duk_context* ctx);
static duk_ret_t js_Surface_save              (duk_context* ctx);

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
	register_api_func(g_duktape, NULL, "CreateSurface", &js_CreateSurface);
	register_api_func(g_duktape, NULL, "GrabSurface", &js_GrabSurface);
	register_api_func(g_duktape, NULL, "LoadSurface", &js_LoadSurface);
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
	duk_push_c_function(ctx, &js_Surface_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &js_Surface_setAlpha, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setAlpha");
	duk_push_c_function(ctx, &js_Surface_setBlendMode, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setBlendMode");
	duk_push_c_function(ctx, &js_Surface_blit, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blit");
	duk_push_c_function(ctx, &js_Surface_blitMaskSurface, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blitMaskSurface");
	duk_push_c_function(ctx, &js_Surface_blitSurface, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blitSurface");
	duk_push_c_function(ctx, &js_Surface_clone, DUK_VARARGS); duk_put_prop_string(ctx, -2, "clone");
	duk_push_c_function(ctx, &js_Surface_cloneSection, DUK_VARARGS); duk_put_prop_string(ctx, -2, "cloneSection");
	duk_push_c_function(ctx, &js_Surface_createImage, DUK_VARARGS); duk_put_prop_string(ctx, -2, "createImage");
	duk_push_c_function(ctx, &js_Surface_drawText, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawText");
	duk_push_c_function(ctx, &js_Surface_gradientRectangle, DUK_VARARGS); duk_put_prop_string(ctx, -2, "gradientRectangle");
	duk_push_c_function(ctx, &js_Surface_line, DUK_VARARGS); duk_put_prop_string(ctx, -2, "line");
	duk_push_c_function(ctx, &js_Surface_outlinedRectangle, DUK_VARARGS); duk_put_prop_string(ctx, -2, "outlinedRectangle");
	duk_push_c_function(ctx, &js_Surface_rotate, DUK_VARARGS); duk_put_prop_string(ctx, -2, "rotate");
	duk_push_c_function(ctx, &js_Surface_rectangle, DUK_VARARGS); duk_put_prop_string(ctx, -2, "rectangle");
	duk_push_c_function(ctx, &js_Surface_save, DUK_VARARGS); duk_put_prop_string(ctx, -2, "save");
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
js_CreateSurface(duk_context* ctx)
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
js_GrabSurface(duk_context* ctx)
{
	ALLEGRO_BITMAP* backbuffer;
	ALLEGRO_BITMAP* bitmap;
	int             x, y, w, h;

	backbuffer = al_get_backbuffer(g_display);
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	w = duk_to_int(ctx, 2);
	h = duk_to_int(ctx, 3);
	bitmap = al_create_bitmap(w, h);
	if (bitmap != NULL) {
		al_set_target_bitmap(bitmap);
		al_draw_bitmap_region(backbuffer, x, y, w, h, 0, 0, 0x0);
		al_set_target_backbuffer(g_display);
		_duk_push_sphere_Surface(ctx, bitmap);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "GrabSurface(): Unable to create surface bitmap");
	}
}

static duk_ret_t
js_LoadSurface(duk_context* ctx)
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
js_Surface_finalize(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	
	duk_get_prop_string(ctx, 0, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_destroy_bitmap(bitmap);
	return 0;
}

static duk_ret_t
js_Surface_blit(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	float           x, y;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	if (!g_skip_frame) al_draw_bitmap(bitmap, x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Surface_blitMaskSurface(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	int             blend_mode;
	ALLEGRO_COLOR   mask_color;
	int             n_args;
	ALLEGRO_BITMAP* src_bitmap;
	int             x, y;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	n_args = duk_get_top(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "bitmap_ptr"); src_bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);
	mask_color = duk_get_sphere_color(ctx, 3);
	_apply_blend_mode(blend_mode);
	al_set_target_bitmap(bitmap);
	al_draw_tinted_bitmap(src_bitmap, mask_color, x, y, 0x0);
	al_set_target_backbuffer(g_display);
	_reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_blitSurface(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	int             blend_mode;
	ALLEGRO_BITMAP* src_bitmap;
	int             x, y;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "bitmap_ptr"); src_bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);
	_apply_blend_mode(blend_mode);
	al_set_target_bitmap(bitmap);
	al_draw_bitmap(src_bitmap, x, y, 0x0);
	al_set_target_backbuffer(g_display);
	_reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_clone(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_BITMAP* new_bitmap;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	new_bitmap = al_clone_bitmap(bitmap);
	if (new_bitmap != NULL) {
		_duk_push_sphere_Surface(ctx, new_bitmap);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "Surface:clone() - Unable to create new bitmap");
	}
	return 1;
}

static duk_ret_t
js_Surface_cloneSection(duk_context* ctx)
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
js_Surface_createImage(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_BITMAP* new_bitmap;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	new_bitmap = al_clone_bitmap(bitmap);
	if (new_bitmap != NULL) {
		duk_push_sphere_Image(ctx, new_bitmap, true);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "Surface:createImage() - Unable to create new bitmap");
	}
}

static duk_ret_t
js_Surface_drawText(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	int             blend_mode;
	ALLEGRO_COLOR   color;
	ALLEGRO_FONT*   font;
	const char*     text;
	int             x, y;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "color_mask"); color = duk_get_sphere_color(ctx, -1); duk_pop(ctx);
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);
	text = duk_to_string(ctx, 3);
	_apply_blend_mode(blend_mode);
	al_set_target_bitmap(bitmap);
	al_draw_text(font, color, x, y, 0x0, text);
	al_set_target_backbuffer(g_display);
	_reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_getPixel(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_COLOR   color;
	int             x, y;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	color = al_get_pixel(bitmap, x, y);
	duk_push_sphere_color(ctx, color);
	return 1;
}

static duk_ret_t
js_Surface_gradientRectangle(duk_context* ctx)
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
	color_ul = duk_get_sphere_color(ctx, 4);
	color_ur = duk_get_sphere_color(ctx, 5);
	color_lr = duk_get_sphere_color(ctx, 6);
	color_ll = duk_get_sphere_color(ctx, 7);
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
js_Surface_line(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	int             blend_mode;
	ALLEGRO_COLOR   color;
	int             x1, y1, x2, y2;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	x1 = duk_to_int(ctx, 0);
	y1 = duk_to_int(ctx, 1);
	x2 = duk_to_int(ctx, 2);
	y2 = duk_to_int(ctx, 3);
	color = duk_get_sphere_color(ctx, 4);
	_apply_blend_mode(blend_mode);
	al_set_target_bitmap(bitmap);
	al_draw_line(x1, y1, x2, y2, color, 1);
	al_set_target_backbuffer(g_display);
	_reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_outlinedRectangle(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	int             blend_mode;
	ALLEGRO_COLOR   color;
	int             n_args;
	float           thickness;
	float           x1, y1, x2, y2;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	n_args = duk_get_top(ctx);
	x1 = duk_to_int(ctx, 0) + 0.5;
	y1 = duk_to_int(ctx, 1) + 0.5;
	x2 = x1 + duk_to_int(ctx, 2);
	y2 = y1 + duk_to_int(ctx, 3);
	color = duk_get_sphere_color(ctx, 4);
	thickness = n_args >= 6 ? duk_to_int(ctx, 5) : 1;
	_apply_blend_mode(blend_mode);
	al_set_target_bitmap(bitmap);
	al_draw_rectangle(x1, y1, x2, y2, color, thickness);
	al_set_target_backbuffer(g_display);
	_reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_rotate(duk_context* ctx)
{
	float           angle;
	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_BITMAP* new_bitmap;
	int             w, h;
	bool            want_resize;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	angle = duk_get_number(ctx, 0);
	want_resize = duk_require_boolean(ctx, 1);
	w = al_get_bitmap_width(bitmap);
	h = al_get_bitmap_height(bitmap);
	new_bitmap = al_create_bitmap(w, h);
	if (new_bitmap != NULL) {
		al_set_target_bitmap(new_bitmap);
		al_draw_rotated_bitmap(bitmap, (float)w / 2, (float)h / 2, (float)w / 2, (float)h / 2, angle, 0x0);
		al_set_target_backbuffer(g_display);
		al_destroy_bitmap(bitmap);
		duk_push_this(ctx);
		duk_push_pointer(ctx, new_bitmap); duk_put_prop_string(ctx, -2, "\xFF" "bitmap_ptr");
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "Surface:rotate() - Unable to create rotated bitmap");
	}
}

static duk_ret_t
js_Surface_rectangle(duk_context* ctx)
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
	color = duk_get_sphere_color(ctx, 4);
	_apply_blend_mode(blend_mode);
	al_set_target_bitmap(bitmap);
	al_draw_filled_rectangle(x, y, x + w, y + h, color);
	al_set_target_backbuffer(g_display);
	_reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_save(duk_context* ctx)
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
js_Surface_setAlpha(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "bitmap_ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Surface_setBlendMode(duk_context* ctx)
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
