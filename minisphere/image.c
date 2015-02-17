#include "minisphere.h"
#include "api.h"
#include "image.h"

static duk_ret_t _js_LoadImage      (duk_context* ctx);
static duk_ret_t _js_GrabImage      (duk_context* ctx);
static duk_ret_t _js_Image_finalize (duk_context* ctx);
static duk_ret_t _js_Image_blit     (duk_context* ctx);
static duk_ret_t _js_Image_blitMask (duk_context* ctx);

void
init_image_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "LoadImage", &_js_LoadImage);
	register_api_func(ctx, NULL, "GrabImage", &_js_GrabImage);
}

void
duk_push_sphere_Image(duk_context* ctx, ALLEGRO_BITMAP* bitmap, bool allow_free)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, bitmap); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_boolean(ctx, allow_free); duk_put_prop_string(ctx, -2, "\xFF" "allow_free");
	duk_push_c_function(ctx, &_js_Image_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &_js_Image_blit, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blit");
	duk_push_c_function(ctx, &_js_Image_blitMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blitMask");
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
_js_LoadImage(duk_context* ctx)
{
	const char* filename = duk_get_string(ctx, 0);
	char* path = get_asset_path(filename, "images", false);
	ALLEGRO_BITMAP* bitmap = al_load_bitmap(path);
	free(path);
	if (bitmap != NULL) {
		duk_push_sphere_Image(ctx, bitmap, true);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "LoadImage(): Unable to load image file '%s'", filename);
	}
}

static duk_ret_t
_js_GrabImage(duk_context* ctx)
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
		duk_push_sphere_Image(ctx, bitmap, true);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "GrabImage(): Unable to create new bitmap");
	}
}

static duk_ret_t
_js_Image_finalize(duk_context* ctx)
{
	bool            allow_free;
	ALLEGRO_BITMAP* bitmap;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "allow_free"); allow_free = duk_get_boolean(ctx, -1); duk_pop(ctx);
	if (allow_free) {
		al_destroy_bitmap(bitmap);
	}
	return 0;
}

static duk_ret_t
_js_Image_blit(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	float           x, y;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	al_draw_bitmap(bitmap, x, y, 0x0);
	return 1;
}

static duk_ret_t
_js_Image_blitMask(duk_context* ctx)
{
	ALLEGRO_BITMAP* bitmap;
	float           x = (float)duk_get_number(ctx, 0);
	float           y = (float)duk_get_number(ctx, 1);
	int             r, g, b, a;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_get_prop_string(ctx, 2, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 2, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 2, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 2, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	al_draw_tinted_bitmap(bitmap, al_map_rgba(r, g, b, a), x, y, 0x0);
	return 1;
}
