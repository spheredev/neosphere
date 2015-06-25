#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "image.h"

#include "surface.h"

static void duk_require_rgba_lut (duk_context* ctx, duk_idx_t index, uint8_t *out_lut);

static void apply_blend_mode (int blend_mode);
static void reset_blender    (void);

static duk_ret_t js_GrabSurface               (duk_context* ctx);
static duk_ret_t js_CreateSurface             (duk_context* ctx);
static duk_ret_t js_LoadSurface               (duk_context* ctx);
static duk_ret_t js_new_Surface               (duk_context* ctx);
static duk_ret_t js_Surface_finalize          (duk_context* ctx);
static duk_ret_t js_Surface_get_height        (duk_context* ctx);
static duk_ret_t js_Surface_get_width         (duk_context* ctx);
static duk_ret_t js_Surface_toString          (duk_context* ctx);
static duk_ret_t js_Surface_getPixel          (duk_context* ctx);
static duk_ret_t js_Surface_setAlpha          (duk_context* ctx);
static duk_ret_t js_Surface_setBlendMode      (duk_context* ctx);
static duk_ret_t js_Surface_setPixel          (duk_context* ctx);
static duk_ret_t js_Surface_applyColorFX      (duk_context* ctx);
static duk_ret_t js_Surface_applyColorFX4     (duk_context* ctx);
static duk_ret_t js_Surface_applyLookup       (duk_context* ctx);
static duk_ret_t js_Surface_blit              (duk_context* ctx);
static duk_ret_t js_Surface_blitMaskSurface   (duk_context* ctx);
static duk_ret_t js_Surface_blitSurface       (duk_context* ctx);
static duk_ret_t js_Surface_clone             (duk_context* ctx);
static duk_ret_t js_Surface_cloneSection      (duk_context* ctx);
static duk_ret_t js_Surface_createImage       (duk_context* ctx);
static duk_ret_t js_Surface_drawText          (duk_context* ctx);
static duk_ret_t js_Surface_filledCircle      (duk_context* ctx);
static duk_ret_t js_Surface_flipHorizontally  (duk_context* ctx);
static duk_ret_t js_Surface_flipVertically    (duk_context* ctx);
static duk_ret_t js_Surface_gradientCircle    (duk_context* ctx);
static duk_ret_t js_Surface_gradientRectangle (duk_context* ctx);
static duk_ret_t js_Surface_line              (duk_context* ctx);
static duk_ret_t js_Surface_outlinedCircle    (duk_context* ctx);
static duk_ret_t js_Surface_outlinedRectangle (duk_context* ctx);
static duk_ret_t js_Surface_pointSeries       (duk_context* ctx);
static duk_ret_t js_Surface_rotate            (duk_context* ctx);
static duk_ret_t js_Surface_rectangle         (duk_context* ctx);
static duk_ret_t js_Surface_replaceColor      (duk_context* ctx);
static duk_ret_t js_Surface_rescale           (duk_context* ctx);
static duk_ret_t js_Surface_save              (duk_context* ctx);

void
init_surface_api(void)
{
	// register Surface API functions
	register_api_function(g_duk, NULL, "GrabSurface", js_GrabSurface);
	
	// register Surface API constants	
	register_api_const(g_duk, "BLEND", BLEND_BLEND);
	register_api_const(g_duk, "REPLACE", BLEND_REPLACE);
	register_api_const(g_duk, "RGB_ONLY", BLEND_RGB_ONLY);
	register_api_const(g_duk, "ALPHA_ONLY", BLEND_ALPHA_ONLY);
	register_api_const(g_duk, "ADD", BLEND_ADD);
	register_api_const(g_duk, "SUBTRACT", BLEND_SUBTRACT);
	register_api_const(g_duk, "MULTIPLY", BLEND_MULTIPLY);
	register_api_const(g_duk, "AVERAGE", BLEND_AVERAGE);
	register_api_const(g_duk, "INVERT", BLEND_INVERT);
	
	// register Surface methods and properties
	register_api_function(g_duk, NULL, "CreateSurface", js_CreateSurface);
	register_api_function(g_duk, NULL, "LoadSurface", js_LoadSurface);
	register_api_ctor(g_duk, "Surface", js_new_Surface, js_Surface_finalize);
	register_api_function(g_duk, "Surface", "toString", js_Surface_toString);
	register_api_prop(g_duk, "Surface", "height", js_Surface_get_height, NULL);
	register_api_prop(g_duk, "Surface", "width", js_Surface_get_width, NULL);
	register_api_function(g_duk, "Surface", "getPixel", js_Surface_getPixel);
	register_api_function(g_duk, "Surface", "setAlpha", js_Surface_setAlpha);
	register_api_function(g_duk, "Surface", "setBlendMode", js_Surface_setBlendMode);
	register_api_function(g_duk, "Surface", "setPixel", js_Surface_setPixel);
	register_api_function(g_duk, "Surface", "applyColorFX", js_Surface_applyColorFX);
	register_api_function(g_duk, "Surface", "applyColorFX4", js_Surface_applyColorFX4);
	register_api_function(g_duk, "Surface", "applyLookup", js_Surface_applyLookup);
	register_api_function(g_duk, "Surface", "blit", js_Surface_blit);
	register_api_function(g_duk, "Surface", "blitMaskSurface", js_Surface_blitMaskSurface);
	register_api_function(g_duk, "Surface", "blitSurface", js_Surface_blitSurface);
	register_api_function(g_duk, "Surface", "clone", js_Surface_clone);
	register_api_function(g_duk, "Surface", "cloneSection", js_Surface_cloneSection);
	register_api_function(g_duk, "Surface", "createImage", js_Surface_createImage);
	register_api_function(g_duk, "Surface", "drawText", js_Surface_drawText);
	register_api_function(g_duk, "Surface", "filledCircle", js_Surface_filledCircle);
	register_api_function(g_duk, "Surface", "flipHorizontally", js_Surface_flipHorizontally);
	register_api_function(g_duk, "Surface", "flipVertically", js_Surface_flipVertically);
	register_api_function(g_duk, "Surface", "gradientCircle", js_Surface_gradientCircle);
	register_api_function(g_duk, "Surface", "gradientRectangle", js_Surface_gradientRectangle);
	register_api_function(g_duk, "Surface", "line", js_Surface_line);
	register_api_function(g_duk, "Surface", "outlinedCircle", js_Surface_outlinedCircle);
	register_api_function(g_duk, "Surface", "outlinedRectangle", js_Surface_outlinedRectangle);
	register_api_function(g_duk, "Surface", "pointSeries", js_Surface_pointSeries);
	register_api_function(g_duk, "Surface", "rotate", js_Surface_rotate);
	register_api_function(g_duk, "Surface", "rectangle", js_Surface_rectangle);
	register_api_function(g_duk, "Surface", "replaceColor", js_Surface_replaceColor);
	register_api_function(g_duk, "Surface", "rescale", js_Surface_rescale);
	register_api_function(g_duk, "Surface", "save", js_Surface_save);
}

void
duk_push_sphere_surface(duk_context* ctx, image_t* image)
{
	duk_push_sphere_obj(ctx, "Surface", ref_image(image));
}

image_t*
duk_require_sphere_surface(duk_context* ctx, duk_idx_t index)
{
	return duk_require_sphere_obj(ctx, index, "Surface");
}

static void
duk_require_rgba_lut(duk_context* ctx, duk_idx_t index, uint8_t *out_lut)
{
	int length;

	int i;
	
	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	length = fmin(duk_get_length(ctx, index), 256);
	for (i = length; i < 256; ++i) out_lut[i] = i;
	for (i = 0; i < length; ++i) {
		duk_get_prop_index(ctx, index, i);
		out_lut[i] = fmin(fmax(duk_require_int(ctx, -1), 0), 255);
		duk_pop(ctx);
	}
}

static void
apply_blend_mode(int blend_mode)
{
	switch (blend_mode) {
	case BLEND_BLEND:
		al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
			ALLEGRO_ADD, ALLEGRO_INVERSE_DEST_COLOR, ALLEGRO_ONE);
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
	case BLEND_MULTIPLY:
		al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_DEST_COLOR, ALLEGRO_ZERO,
			ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_ONE);
		break;
	case BLEND_INVERT:
		al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_INVERSE_SRC_COLOR,
			ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_ONE);
		break;
	}
}

static void
reset_blender(void)
{
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GrabSurface(): Failed to create surface bitmap");
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_bitmap_region(backbuffer, x, y, w, h, 0, 0, 0x0);
	al_set_target_backbuffer(g_display);
	if (!rescale_image(image, g_res_x, g_res_y))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GrabSurface(): Failed to rescale grabbed image");
	duk_push_sphere_surface(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_CreateSurface(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int w = duk_require_int(ctx, 0);
	int h = duk_require_int(ctx, 1);
	if (n_args >= 3) duk_require_sphere_color(ctx, 2);

	js_new_Surface(ctx);
	return 1;
}

static duk_ret_t
js_LoadSurface(duk_context* ctx)
{
	duk_require_string(ctx, 0);
	
	js_new_Surface(ctx);
	return 1;
}

static duk_ret_t
js_new_Surface(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	
	const char* filename;
	color_t     fill_color;
	image_t*    image;
	image_t*    src_image;
	int         width, height;

	if (n_args >= 2) {
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		fill_color = n_args >= 3 ? duk_require_sphere_color(ctx, 2) : rgba(0, 0, 0, 0);
		if (!(image = create_image(width, height)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface(): Failed to create new surface");
		fill_image(image, fill_color);
	}
	else if (duk_is_sphere_obj(ctx, 0, "Image")) {
		src_image = duk_require_sphere_image(ctx, 0);
		if (!(image = clone_image(src_image)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface(): Failed to create surface from image");
	}
	else {
		filename = duk_require_string(ctx, 0);
		image = load_image(filename);
		if (image == NULL)
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface(): Failed to load image file '%s'", filename);
	}
	duk_push_sphere_surface(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_Surface_finalize(duk_context* ctx)
{
	image_t* image;
	
	image = duk_require_sphere_obj(ctx, 0, "Surface");
	free_image(image);
	return 0;
}

static duk_ret_t
js_Surface_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_pop(ctx);
	duk_push_int(ctx, get_image_height(image));
	return 1;
}

static duk_ret_t
js_Surface_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_pop(ctx);
	duk_push_int(ctx, get_image_width(image));
	return 1;
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
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	set_image_pixel(image, x, y, color);
	return 0;
}

static duk_ret_t
js_Surface_getPixel(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);

	image_t* image;
	color_t  pixel;
	int      w, h;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	w = get_image_width(image);
	h = get_image_height(image);
	if (x < 0 || x >= w || y < 0 || y >= h)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Surface:getPixel(): X/Y out of range (%i,%i) for %ix%i surface", x, y, w, h);
	pixel = get_image_pixel(image, x, y);
	duk_push_sphere_color(ctx, pixel);
	return 1;
}

static duk_ret_t
js_Surface_applyColorFX(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	colormatrix_t matrix = duk_require_sphere_colormatrix(ctx, 4);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	if (x < 0 || y < 0 || x + w > get_image_width(image) || y + h > get_image_height(image))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Surface:applyColorFX(): Box extends outside image (%i,%i,%i,%i)", x, y, w, h);
	if (!apply_color_matrix(image, matrix, x, y, w, h))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:applyColorFX(): Failed to apply transformation");
	return 0;
}

static duk_ret_t
js_Surface_applyColorFX4(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	colormatrix_t ul_mat = duk_require_sphere_colormatrix(ctx, 4);
	colormatrix_t ur_mat = duk_require_sphere_colormatrix(ctx, 5);
	colormatrix_t ll_mat = duk_require_sphere_colormatrix(ctx, 6);
	colormatrix_t lr_mat = duk_require_sphere_colormatrix(ctx, 7);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	if (x < 0 || y < 0 || x + w > get_image_width(image) || y + h > get_image_height(image))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Surface:applyColorFX4(): Box extends outside image (%i,%i,%i,%i)", x, y, w, h);
	if (!apply_color_matrix_4(image, ul_mat, ur_mat, ll_mat, lr_mat, x, y, w, h))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:applyColorFX4(): Failed to apply transformation");
	return 0;
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
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	if (x < 0 || y < 0 || x + w > get_image_width(image) || y + h > get_image_height(image))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Surface:applyColorFX(): Specified area extends outside image (%i,%i,%i,%i)", x, y, w, h);
	if (!apply_image_lookup(image, x, y, w, h, red_lu, green_lu, blue_lu, alpha_lu))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:applyLookup(): Failed to apply lookup transformation");
	return 0;
}

static duk_ret_t
js_Surface_blit(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	
	image_t* image;
	
	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	if (!is_skipped_frame()) al_draw_bitmap(get_image_bitmap(image), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Surface_blitMaskSurface(duk_context* ctx)
{
	image_t* src_image = duk_require_sphere_surface(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);
	color_t mask = duk_require_sphere_color(ctx, 3);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
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
	image_t* src_image = duk_require_sphere_surface(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
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
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	if ((new_image = clone_image(image)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:clone() - Unable to create new surface image");
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
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	if ((new_image = create_image(w, h)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:cloneSection() - Failed to create new surface");
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
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	if ((new_image = clone_image(image)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:createImage() - Failed to create new image");
	duk_push_sphere_image(ctx, new_image);
	free_image(new_image);
	return 1;
}

static duk_ret_t
js_Surface_drawText(duk_context* ctx)
{
	font_t* font = duk_require_sphere_obj(ctx, 0, "Font");
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);
	const char* text = duk_to_string(ctx, 3);
	
	int      blend_mode;
	color_t  color;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
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
js_Surface_filledCircle(duk_context* ctx)
{	
	int n_args = duk_get_top(ctx);
	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);
	int radius = duk_require_number(ctx, 2);
	color_t color = duk_require_sphere_color(ctx, 3);
	bool use_aa = n_args >= 5 ? duk_require_boolean(ctx, 4) : false;

	int      blend_mode;
	image_t* image;
	
	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_filled_circle(x, y, radius, nativecolor(color));
	al_set_target_backbuffer(g_display);
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_flipHorizontally(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	flip_image(image, true, false);
	return 0;
}

static duk_ret_t
js_Surface_flipVertically(duk_context* ctx)
{
	image_t* image;
	
	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	flip_image(image, false, true);
	return 0;
}

static duk_ret_t
js_Surface_gradientCircle(duk_context* ctx)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);
	int radius = duk_require_number(ctx, 2);
	color_t in_color = duk_require_sphere_color(ctx, 3);
	color_t out_color = duk_require_sphere_color(ctx, 4);

	int      blend_mode;
	image_t* image;
	double   phi;
	int      vcount;

	int i;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	vcount = fmin(radius, 126);
	s_vbuf[0].x = x; s_vbuf[0].y = y; s_vbuf[0].z = 0;
	s_vbuf[0].color = nativecolor(in_color);
	for (i = 0; i < vcount; ++i) {
		phi = 2 * M_PI * i / vcount;
		s_vbuf[i + 1].x = x + cos(phi) * radius;
		s_vbuf[i + 1].y = y - sin(phi) * radius;
		s_vbuf[i + 1].z = 0;
		s_vbuf[i + 1].color = nativecolor(out_color);
	}
	s_vbuf[i + 1].x = x + cos(0) * radius;
	s_vbuf[i + 1].y = y - sin(0) * radius;
	s_vbuf[i + 1].z = 0;
	s_vbuf[i + 1].color = nativecolor(out_color);
	al_draw_prim(s_vbuf, NULL, NULL, 0, vcount + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	al_set_target_backbuffer(g_display);
	reset_blender();
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
	image = duk_require_sphere_surface(ctx, -1);
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
	float x1 = duk_require_int(ctx, 0) + 0.5;
	float y1 = duk_require_int(ctx, 1) + 0.5;
	float x2 = duk_require_int(ctx, 2) + 0.5;
	float y2 = duk_require_int(ctx, 3) + 0.5;
	color_t color = duk_require_sphere_color(ctx, 4);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
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
js_Surface_outlinedCircle(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);
	int radius = duk_require_number(ctx, 2);
	color_t color = duk_require_sphere_color(ctx, 3);
	bool use_aa = n_args >= 5 ? duk_require_boolean(ctx, 4) : false;

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_circle(x, y, radius, nativecolor(color), 1);
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
	image = duk_require_sphere_surface(ctx, -1);
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode"); blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:pointSeries(): First argument must be an array");
	duk_get_prop_string(ctx, 0, "length"); num_points = duk_get_uint(ctx, 0); duk_pop(ctx);
	if (num_points > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Surface:pointSeries(): Too many vertices (%u)", num_points);
	vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX));
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, i);
		duk_get_prop_string(ctx, 0, "x"); x = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, 0, "y"); y = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points, ALLEGRO_PRIM_POINT_LIST);
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
	float x2 = x1 + duk_require_int(ctx, 2) - 1;
	float y2 = y1 + duk_require_int(ctx, 3) - 1;
	color_t color = duk_require_sphere_color(ctx, 4);
	int thickness = n_args >= 6 ? duk_require_int(ctx, 5) : 1;

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
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
js_Surface_replaceColor(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 0);
	color_t new_color = duk_require_sphere_color(ctx, 1);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	if (!replace_image_color(image, color, new_color))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:replaceColor() - Failed to perform replacement");
	return 0;
}

static duk_ret_t
js_Surface_rescale(duk_context* ctx)
{
	int width = duk_require_int(ctx, 0);
	int height = duk_require_int(ctx, 1);
	
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	if (!rescale_image(image, width, height))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:rescale() - Failed to rescale image");
	duk_push_this(ctx);
	return 1;
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
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	w = new_w = get_image_width(image);
	h = new_h = get_image_height(image);
	if (want_resize) {
		// TODO: implement in-place resizing for Surface:rotate()
	}
	if ((new_image = create_image(new_w, new_h)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:rotate() - Failed to create new surface bitmap");
	al_set_target_bitmap(get_image_bitmap(new_image));
	al_draw_rotated_bitmap(get_image_bitmap(image), (float)w / 2, (float)h / 2, (float)new_w / 2, (float)new_h / 2, angle, 0x0);
	al_set_target_backbuffer(g_display);
	
	// free old image and replace internal image pointer
	// at one time this was an acceptable thing to do; now it's just a hack
	free_image(image);
	duk_push_this(ctx);
	duk_push_pointer(ctx, new_image); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	return 1;
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
	image = duk_require_sphere_surface(ctx, -1);
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

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	save_image(image, filename);
	return 1;
}

static duk_ret_t
js_Surface_setAlpha(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int alpha = duk_require_int(ctx, 0);
	bool want_all = n_args >= 2 ? duk_require_boolean(ctx, 1) : true;
	
	image_t*      image;
	image_lock_t* lock;
	color_t*      pixel;
	int           w, h;

	int i_x, i_y;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
	if (!(lock = lock_image(image)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:setAlpha(): Failed to lock surface");
	w = get_image_width(image);
	h = get_image_height(image);
	alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;
	for (i_y = h - 1; i_y >= 0; --i_y) for (i_x = w - 1; i_x >= 0; --i_x) {
		pixel = &lock->pixels[i_x + i_y * lock->pitch];
		pixel->alpha = want_all || pixel->alpha != 0
			? alpha : pixel->alpha;
	}
	unlock_image(image, lock);
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
