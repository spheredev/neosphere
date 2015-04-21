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
static duk_ret_t js_Surface_replaceColor      (duk_context* ctx);
static duk_ret_t js_Surface_rescale           (duk_context* ctx);
static duk_ret_t js_Surface_save              (duk_context* ctx);

void
init_surface_api(void)
{
	// register Surface API functions
	register_api_func(g_duktape, NULL, "GrabSurface", js_GrabSurface);
	
	// register Surface API constants	
	register_api_const(g_duktape, "BLEND", BLEND_BLEND);
	register_api_const(g_duktape, "REPLACE", BLEND_REPLACE);
	register_api_const(g_duktape, "RGB_ONLY", BLEND_RGB_ONLY);
	register_api_const(g_duktape, "ALPHA_ONLY", BLEND_ALPHA_ONLY);
	register_api_const(g_duktape, "ADD", BLEND_ADD);
	register_api_const(g_duktape, "SUBTRACT", BLEND_SUBTRACT);
	register_api_const(g_duktape, "MULTIPLY", BLEND_MULTIPLY);
	register_api_const(g_duktape, "AVERAGE", BLEND_AVERAGE);
	register_api_const(g_duktape, "INVERT", BLEND_INVERT);
	
	// register Surface methods and properties
	register_api_func(g_duktape, NULL, "CreateSurface", js_CreateSurface);
	register_api_func(g_duktape, NULL, "LoadSurface", js_LoadSurface);
	register_api_ctor(g_duktape, "Surface", js_new_Surface, js_Surface_finalize);
	register_api_prop(g_duktape, "Surface", "height", js_Surface_get_height, NULL);
	register_api_prop(g_duktape, "Surface", "width", js_Surface_get_width, NULL);
	register_api_func(g_duktape, "Surface", "toString", js_Surface_toString);
	register_api_func(g_duktape, "Surface", "getPixel", js_Surface_getPixel);
	register_api_func(g_duktape, "Surface", "setAlpha", js_Surface_setAlpha);
	register_api_func(g_duktape, "Surface", "setBlendMode", js_Surface_setBlendMode);
	register_api_func(g_duktape, "Surface", "setPixel", js_Surface_setPixel);
	register_api_func(g_duktape, "Surface", "applyLookup", js_Surface_applyLookup);
	register_api_func(g_duktape, "Surface", "blit", js_Surface_blit);
	register_api_func(g_duktape, "Surface", "blitMaskSurface", js_Surface_blitMaskSurface);
	register_api_func(g_duktape, "Surface", "blitSurface", js_Surface_blitSurface);
	register_api_func(g_duktape, "Surface", "clone", js_Surface_clone);
	register_api_func(g_duktape, "Surface", "cloneSection", js_Surface_cloneSection);
	register_api_func(g_duktape, "Surface", "createImage", js_Surface_createImage);
	register_api_func(g_duktape, "Surface", "drawText", js_Surface_drawText);
	register_api_func(g_duktape, "Surface", "flipHorizontally", js_Surface_flipHorizontally);
	register_api_func(g_duktape, "Surface", "flipVertically", js_Surface_flipVertically);
	register_api_func(g_duktape, "Surface", "gradientRectangle", js_Surface_gradientRectangle);
	register_api_func(g_duktape, "Surface", "line", js_Surface_line);
	register_api_func(g_duktape, "Surface", "outlinedRectangle", js_Surface_outlinedRectangle);
	register_api_func(g_duktape, "Surface", "pointSeries", js_Surface_pointSeries);
	register_api_func(g_duktape, "Surface", "rotate", js_Surface_rotate);
	register_api_func(g_duktape, "Surface", "rectangle", js_Surface_rectangle);
	register_api_func(g_duktape, "Surface", "replaceColor", js_Surface_replaceColor);
	register_api_func(g_duktape, "Surface", "rescale", js_Surface_rescale);
	register_api_func(g_duktape, "Surface", "save", js_Surface_save);
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
	char*       path;
	int         width, height;

	if (n_args >= 2) {
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		fill_color = n_args >= 3 ? duk_require_sphere_color(ctx, 2) : rgba(0, 0, 0, 255);
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
		path = get_asset_path(filename, "images", false);
		image = load_image(path);
		free(path);
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
	image = duk_require_sphere_surface(ctx, -1);
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
	image = duk_require_sphere_surface(ctx, -1);
	duk_pop(ctx);
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
	font_t* font = duk_require_sphere_font(ctx, 0);
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
	int x1 = duk_require_int(ctx, 0);
	int y1 = duk_require_int(ctx, 1);
	int x2 = duk_require_int(ctx, 2);
	int y2 = duk_require_int(ctx, 3);
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
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:pointSeries(): Failed to allocate vertex buffer");
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
	float x2 = x1 + duk_require_int(ctx, 2);
	float y2 = y1 + duk_require_int(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);
	int thickness = n_args >= 6 ? duk_to_int(ctx, 5) : 1;

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
	char*    path;

	duk_push_this(ctx);
	image = duk_require_sphere_surface(ctx, -1);
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
	image = duk_require_sphere_surface(ctx, -1);
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
