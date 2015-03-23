#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "image.h"

#include "surface.h"

static void duk_require_rgba_lut (duk_context* ctx, duk_idx_t index, uint8_t *out_lut);

static void apply_blend_mode (int blend_mode);
static void reset_blender    (void);

static duk_ret_t js_CreateSurface             (duk_context* ctx);
static duk_ret_t js_GrabSurface               (duk_context* ctx);
static duk_ret_t js_LoadSurface               (duk_context* ctx);
static duk_ret_t js_Surface_finalize          (duk_context* ctx);
static duk_ret_t js_Surface_toString          (duk_context* ctx);
static duk_ret_t js_Surface_getPixel          (duk_context* ctx);
static duk_ret_t js_Surface_setAlpha          (duk_context* ctx);
static duk_ret_t js_Surface_setBlendMode      (duk_context* ctx);
static duk_ret_t js_Surface_setPixel          (duk_context* ctx);
static duk_ret_t js_Surface_applyLookup       (duk_context* ctx);
static duk_ret_t js_Surface_blit              (duk_context* ctx);
static duk_ret_t js_Surface_blitMaskSurface   (duk_context* ctx);
static duk_ret_t js_Surface_blitSurface       (duk_context* ctx);
static duk_ret_t js_Surface_clone             (duk_context* ctx);
static duk_ret_t js_Surface_cloneSection      (duk_context* ctx);
static duk_ret_t js_Surface_createImage       (duk_context* ctx);
static duk_ret_t js_Surface_drawText          (duk_context* ctx);
static duk_ret_t js_Surface_flipHorizontally  (duk_context* ctx);
static duk_ret_t js_Surface_flipVertically    (duk_context* ctx);
static duk_ret_t js_Surface_gradientRectangle (duk_context* ctx);
static duk_ret_t js_Surface_line              (duk_context* ctx);
static duk_ret_t js_Surface_outlinedRectangle (duk_context* ctx);
static duk_ret_t js_Surface_pointSeries       (duk_context* ctx);
static duk_ret_t js_Surface_rotate            (duk_context* ctx);
static duk_ret_t js_Surface_rectangle         (duk_context* ctx);
static duk_ret_t js_Surface_rescale           (duk_context* ctx);
static duk_ret_t js_Surface_save              (duk_context* ctx);

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
	register_api_func(g_duktape, NULL, "CreateSurface", js_CreateSurface);
	register_api_func(g_duktape, NULL, "GrabSurface", js_GrabSurface);
	register_api_func(g_duktape, NULL, "LoadSurface", js_LoadSurface);
}

void
duk_push_sphere_surface(duk_context* ctx, image_t* image)
{
	ref_image(image);

	duk_push_object(ctx);
	duk_push_string(ctx, "surface"); duk_put_prop_string(ctx, -2, "\xFF" "sphere_type");
	duk_push_pointer(ctx, image); duk_put_prop_string(ctx, -2, "\xFF" "image_ptr");
	duk_push_c_function(ctx, js_Surface_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, js_Surface_toString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "toString");
	duk_push_c_function(ctx, js_Surface_getPixel, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getPixel");
	duk_push_c_function(ctx, js_Surface_setAlpha, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setAlpha");
	duk_push_c_function(ctx, js_Surface_setBlendMode, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setBlendMode");
	duk_push_c_function(ctx, js_Surface_setPixel, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setPixel");
	duk_push_c_function(ctx, js_Surface_applyLookup, DUK_VARARGS); duk_put_prop_string(ctx, -2, "applyLookup");
	duk_push_c_function(ctx, js_Surface_blit, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blit");
	duk_push_c_function(ctx, js_Surface_blitMaskSurface, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blitMaskSurface");
	duk_push_c_function(ctx, js_Surface_blitSurface, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blitSurface");
	duk_push_c_function(ctx, js_Surface_clone, DUK_VARARGS); duk_put_prop_string(ctx, -2, "clone");
	duk_push_c_function(ctx, js_Surface_cloneSection, DUK_VARARGS); duk_put_prop_string(ctx, -2, "cloneSection");
	duk_push_c_function(ctx, js_Surface_createImage, DUK_VARARGS); duk_put_prop_string(ctx, -2, "createImage");
	duk_push_c_function(ctx, js_Surface_drawText, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawText");
	duk_push_c_function(ctx, js_Surface_flipHorizontally, DUK_VARARGS); duk_put_prop_string(ctx, -2, "flipHorizontally");
	duk_push_c_function(ctx, js_Surface_flipVertically, DUK_VARARGS); duk_put_prop_string(ctx, -2, "flipVertically");
	duk_push_c_function(ctx, js_Surface_gradientRectangle, DUK_VARARGS); duk_put_prop_string(ctx, -2, "gradientRectangle");
	duk_push_c_function(ctx, js_Surface_line, DUK_VARARGS); duk_put_prop_string(ctx, -2, "line");
	duk_push_c_function(ctx, js_Surface_outlinedRectangle, DUK_VARARGS); duk_put_prop_string(ctx, -2, "outlinedRectangle");
	duk_push_c_function(ctx, js_Surface_pointSeries, DUK_VARARGS); duk_put_prop_string(ctx, -2, "pointSeries");
	duk_push_c_function(ctx, js_Surface_rotate, DUK_VARARGS); duk_put_prop_string(ctx, -2, "rotate");
	duk_push_c_function(ctx, js_Surface_rectangle, DUK_VARARGS); duk_put_prop_string(ctx, -2, "rectangle");
	duk_push_c_function(ctx, js_Surface_rescale, DUK_VARARGS); duk_put_prop_string(ctx, -2, "rescale");
	duk_push_c_function(ctx, js_Surface_save, DUK_VARARGS); duk_put_prop_string(ctx, -2, "save");
	duk_push_string(ctx, "width"); duk_push_int(ctx, get_image_width(image));
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0
		| DUK_DEFPROP_HAVE_WRITABLE | 0
		| DUK_DEFPROP_HAVE_VALUE);
	duk_push_string(ctx, "height"); duk_push_int(ctx, get_image_height(image));
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0
		| DUK_DEFPROP_HAVE_WRITABLE | 0
		| DUK_DEFPROP_HAVE_VALUE);
}

image_t*
duk_require_sphere_surface(duk_context* ctx, duk_idx_t index)
{
	image_t*    image;
	const char* type;

	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	if (!duk_get_prop_string(ctx, index, "\xFF" "sphere_type"))
		goto on_error;
	type = duk_get_string(ctx, -1); duk_pop(ctx);
	if (strcmp(type, "surface") != 0) goto on_error;
	duk_get_prop_string(ctx, index, "\xFF" "image_ptr");
	image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	return image;

on_error:
	js_error(JS_TYPE_ERROR, -1, "Object is not a Sphere surface");
}

static void
duk_require_rgba_lut(duk_context* ctx, duk_idx_t index, uint8_t *out_lut)
{
	int length;

	int i;
	
	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	length = fmin(duk_get_length(ctx, index), 256);
	for (i = 0; i < 256; ++i) out_lut[i] = i;
	for (i = 0; i < length; ++i) {
		duk_get_prop_index(ctx, index, i);
		out_lut[i] = min(max(duk_require_int(ctx, -1), 0), 255);
		duk_pop(ctx);
	}
}

static void
apply_blend_mode(int blend_mode)
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
reset_blender(void)
{
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
}

static duk_ret_t
js_CreateSurface(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int w = duk_require_int(ctx, 0);
	int h = duk_require_int(ctx, 1);
	color_t color = n_args >= 3 ? duk_require_sphere_color(ctx, 2) : rgba(0, 0, 0, 0);
	
	image_t* image;
	
	if (!(image = create_image(w, h)))
		js_error(JS_ERROR, -1, "CreateSurface(): Failed to create image for surface (internal error)");
	fill_image(image, color);
	duk_push_sphere_surface(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_GrabSurface(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0) * g_scale_x;
	int y = duk_require_int(ctx, 1) * g_scale_y;
	int w = duk_require_int(ctx, 2) * g_scale_x;
	int h = duk_require_int(ctx, 3) * g_scale_y;

	ALLEGRO_BITMAP* backbuffer;
	image_t*        image;

	backbuffer = al_get_backbuffer(g_display);
	if ((image = create_image(w, h)) == NULL)
		js_error(JS_ERROR, -1, "GrabSurface(): Failed to create surface bitmap");
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_bitmap_region(backbuffer, x, y, w, h, 0, 0, 0x0);
	al_set_target_backbuffer(g_display);
	if (!rescale_image(image, g_res_x, g_res_y))
		js_error(JS_ERROR, -1, "GrabSurface(): Failed to rescale grabbed image (internal error)");
	duk_push_sphere_surface(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_LoadSurface(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	
	image_t* image;

	char* path = get_asset_path(filename, "images", false);
	image = load_image(path);
	free(path);
	if (image == NULL)
		js_error(JS_ERROR, -1, "LoadSurface(): Failed to load image file '%s'", filename);
	duk_push_sphere_surface(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_Surface_finalize(duk_context* ctx)
{
	image_t* image;
	
	duk_get_prop_string(ctx, 0, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	free_image(image);
	return 0;
}

static duk_ret_t
js_Surface_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object surface]");
	return 1;
}

static duk_ret_t
js_Surface_setPixel(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	color_t color = duk_require_sphere_color(ctx, 2);
	
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	al_set_target_bitmap(get_image_bitmap(image));
	al_put_pixel(x, y, nativecolor(color));
	al_set_target_backbuffer(g_display);
	return 0;
}

static duk_ret_t
js_Surface_getPixel(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);

	image_t*      image;
	ALLEGRO_COLOR pixel;
	uint8_t       r, g, b, alpha;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	pixel = al_get_pixel(get_image_bitmap(image), x, y);
	al_unmap_rgba(pixel, &r, &g, &b, &alpha);
	duk_push_sphere_color(ctx, rgba(r, g, b, alpha));
	return 1;
}

static duk_ret_t
js_Surface_applyLookup(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	uint8_t red_lu[256]; duk_require_rgba_lut(ctx, 4, red_lu);
	uint8_t green_lu[256]; duk_require_rgba_lut(ctx, 5, green_lu);
	uint8_t blue_lu[256]; duk_require_rgba_lut(ctx, 6, blue_lu);
	uint8_t alpha_lu[256]; duk_require_rgba_lut(ctx, 7, alpha_lu);

	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!apply_image_lookup(image, x, y, w, h, red_lu, green_lu, blue_lu, alpha_lu))
		js_error(JS_ERROR, -1, "Surface:applyLookup(): Failed to apply lookup transformation (internal error)");
	return 0;
}

static duk_ret_t
js_Surface_blit(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	
	image_t* image;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame()) al_draw_bitmap(get_image_bitmap(image), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Surface_blitMaskSurface(duk_context* ctx)
{
	int c_args = duk_get_top(ctx);
	image_t* src_image;
		duk_get_prop_string(ctx, 0, "\xFF" "image_ptr"); src_image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);
	color_t mask = duk_require_sphere_color(ctx, 3);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_tinted_bitmap(get_image_bitmap(src_image), nativecolor(mask), x, y, 0x0);
	al_set_target_backbuffer(g_display);
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_blitSurface(duk_context* ctx)
{
	int c_args = duk_get_top(ctx);
	image_t* src_image;
	duk_get_prop_string(ctx, 0, "\xFF" "image_ptr"); src_image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_bitmap(get_image_bitmap(src_image), x, y, 0x0);
	al_set_target_backbuffer(g_display);
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_clone(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if ((new_image = clone_image(image)) == NULL)
		js_error(JS_ERROR, -1, "Surface:clone() - Unable to create new surface image");
	duk_push_sphere_surface(ctx, new_image);
	free_image(new_image);
	return 1;
}

static duk_ret_t
js_Surface_cloneSection(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);

	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if ((new_image = create_image(w, h)) == NULL)
		js_error(JS_ERROR, -1, "Surface:cloneSection() - Unable to create new surface image");
	al_set_target_bitmap(get_image_bitmap(new_image));
	al_draw_bitmap_region(get_image_bitmap(image), x, y, w, h, 0, 0, 0x0);
	al_set_target_backbuffer(g_display);
	duk_push_sphere_surface(ctx, new_image);
	free_image(new_image);
	return 1;
}

static duk_ret_t
js_Surface_createImage(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if ((new_image = clone_image(image)) == NULL)
		js_error(JS_ERROR, -1, "Surface:createImage() - Failed to create new image bitmap");
	duk_push_sphere_image(ctx, new_image);
	free_image(new_image);
	return 1;
}

static duk_ret_t
js_Surface_drawText(duk_context* ctx)
{
	font_t* font = duk_require_sphere_font(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);
	const char* text = duk_to_string(ctx, 3);
	
	int      blend_mode;
	color_t  color;
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "color_mask"); color = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	draw_text(font, color, x, y, TEXT_ALIGN_LEFT, text);
	al_set_target_backbuffer(g_display);
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_flipHorizontally(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	flip_image(image, true, false);
	return 0;
}

static duk_ret_t
js_Surface_flipVertically(duk_context* ctx)
{
	image_t* image;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	flip_image(image, false, true);
	return 0;
}

static duk_ret_t
js_Surface_gradientRectangle(duk_context* ctx)
{
	int x1 = duk_require_int(ctx, 0);
	int y1 = duk_require_int(ctx, 1);
	int x2 = x1 + duk_require_int(ctx, 2);
	int y2 = y1 + duk_require_int(ctx, 3);
	color_t color_ul = duk_require_sphere_color(ctx, 4);
	color_t color_ur = duk_require_sphere_color(ctx, 5);
	color_t color_lr = duk_require_sphere_color(ctx, 6);
	color_t color_ll = duk_require_sphere_color(ctx, 7);
	
	int           blend_mode;
	image_t*      image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	
	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, nativecolor(color_ul) },
		{ x2, y1, 0, 0, 0, nativecolor(color_ur) },
		{ x1, y2, 0, 0, 0, nativecolor(color_ll) },
		{ x2, y2, 0, 0, 0, nativecolor(color_lr) }
	};
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	al_set_target_backbuffer(g_display);
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_line(duk_context* ctx)
{
	int x1 = duk_require_int(ctx, 0);
	int y1 = duk_require_int(ctx, 1);
	int x2 = duk_require_int(ctx, 2);
	int y2 = duk_require_int(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);
	
	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_line(x1, y1, x2, y2, nativecolor(color), 1);
	al_set_target_backbuffer(g_display);
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_pointSeries(duk_context* ctx)
{
	duk_require_object_coercible(ctx, 0);
	color_t color = duk_require_sphere_color(ctx, 1);
	
	int             blend_mode;
	image_t*        image;
	size_t          num_points;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	unsigned int i;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!duk_is_array(ctx, 0))
		js_error(JS_ERROR, -1, "Surface:pointSeries(): First argument must be an array");
	duk_get_prop_string(ctx, 0, "length"); num_points = duk_get_uint(ctx, 0); duk_pop(ctx);
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		js_error(JS_ERROR, -1, "Surface:pointSeries(): Failed to allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, i);
		duk_get_prop_string(ctx, 0, "x"); x = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, 0, "y"); y = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x; vertices[i].y = y;
		vertices[i].color = vtx_color;
	}
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_prim(vertices, NULL, NULL, 0, num_points, ALLEGRO_PRIM_POINT_LIST);
	al_set_target_backbuffer(g_display);
	reset_blender();
	free(vertices);
	return 0;
}

static duk_ret_t
js_Surface_outlinedRectangle(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float x1 = duk_require_int(ctx, 0) + 0.5;
	float y1 = duk_require_int(ctx, 1) + 0.5;
	float x2 = x1 + duk_require_int(ctx, 2);
	float y2 = y1 + duk_require_int(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);
	int thickness = n_args >= 6 ? duk_to_int(ctx, 5) : 1;

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_rectangle(x1, y1, x2, y2, nativecolor(color), thickness);
	al_set_target_backbuffer(g_display);
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_rescale(duk_context* ctx)
{
	int width = duk_require_int(ctx, 0);
	int height = duk_require_int(ctx, 1);
	
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!rescale_image(image, width, height))
		js_error(JS_ERROR, -1, "Surface:rescale() - Failed to rescale image (internal error)");
	return 0;
}

static duk_ret_t
js_Surface_rotate(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float angle = duk_require_number(ctx, 0);
	bool want_resize = n_args >= 2 ? duk_require_boolean(ctx, 1) : true;
	
	image_t* image;
	image_t* new_image;
	int      new_w, new_h;
	int      w, h;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	w = new_w = get_image_width(image);
	h = new_h = get_image_height(image);
	if (want_resize) {
		// TODO: implement in-place resizing for Surface:rotate()
	}
	if ((new_image = create_image(new_w, new_h)) == NULL)
		js_error(JS_ERROR, -1, "Surface:rotate() - Unable to create new surface bitmap");
	al_set_target_bitmap(get_image_bitmap(new_image));
	al_draw_rotated_bitmap(get_image_bitmap(image), (float)w / 2, (float)h / 2, (float)new_w / 2, (float)new_h / 2, angle, 0x0);
	al_set_target_backbuffer(g_display);
	duk_push_this(ctx);
	duk_push_pointer(ctx, new_image); duk_put_prop_string(ctx, -2, "\xFF" "image_ptr");
	return 0;
}

static duk_ret_t
js_Surface_rectangle(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);
	
	image_t* image;
	int      blend_mode;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_filled_rectangle(x, y, x + w, y + h, nativecolor(color));
	al_set_target_backbuffer(g_display);
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_save(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	
	image_t* image;
	char*    path;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	path = get_asset_path(filename, "images", true);
	al_save_bitmap(path, get_image_bitmap(image));
	free(path);
	return 1;
}

static duk_ret_t
js_Surface_setAlpha(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "image_ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Surface_setBlendMode(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "blend_mode");
	duk_pop(ctx);
	return 0;
}
