#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "surface.h"

#include "image.h"

struct image
{
	int             c_refs;
	ALLEGRO_BITMAP* bitmap;
	int             width;
	int             height;
	image_t*        parent;
};

static duk_ret_t js_GetSystemArrow           (duk_context* ctx);
static duk_ret_t js_GetSystemDownArrow       (duk_context* ctx);
static duk_ret_t js_GetSystemUpArrow         (duk_context* ctx);
static duk_ret_t js_LoadImage                (duk_context* ctx);
static duk_ret_t js_GrabImage                (duk_context* ctx);
static duk_ret_t js_Image_finalize           (duk_context* ctx);
static duk_ret_t js_Image_blit               (duk_context* ctx);
static duk_ret_t js_Image_blitMask           (duk_context* ctx);
static duk_ret_t js_Image_createSurface      (duk_context* ctx);
static duk_ret_t js_Image_rotateBlit         (duk_context* ctx);
static duk_ret_t js_Image_rotateBlitMask     (duk_context* ctx);
static duk_ret_t js_Image_transformBlit      (duk_context* ctx);
static duk_ret_t js_Image_transformBlitMask  (duk_context* ctx);
static duk_ret_t js_Image_zoomBlit           (duk_context* ctx);
static duk_ret_t js_Image_zoomBlitMask       (duk_context* ctx);

static image_t* s_sys_arrow    = NULL;
static image_t* s_sys_dn_arrow = NULL;
static image_t* s_sys_up_arrow = NULL;

image_t*
create_image(int width, int height)
{
	image_t* image;

	if ((image = calloc(1, sizeof(image_t))) == NULL)
		goto on_error;
	if ((image->bitmap = al_create_bitmap(width, height)) == NULL)
		goto on_error;
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	return ref_image(image);

on_error:
	free(image);
	return NULL;
}

image_t*
create_subimage(image_t* parent, int x, int y, int width, int height)
{
	image_t* image;

	if ((image = calloc(1, sizeof(image_t))) == NULL) goto on_error;
	if ((image->bitmap = al_create_sub_bitmap(parent->bitmap, x, y, width, height)) == NULL)
		goto on_error;
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	image->parent = ref_image(parent);
	return ref_image(image);

on_error:
	free(image);
	return NULL;
}

image_t*
clone_image(const image_t* src_image)
{
	image_t* image;

	if ((image = calloc(1, sizeof(image_t))) == NULL)
		goto on_error;
	if ((image->bitmap = al_clone_bitmap(src_image->bitmap)) == NULL)
		goto on_error;
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	return ref_image(image);

on_error:
	free(image);
	return NULL;
}

image_t*
load_image(const char* path)
{
	image_t* image;

	if ((image = calloc(1, sizeof(image_t))) == NULL)
		goto on_error;
	if ((image->bitmap = al_load_bitmap(path)) == NULL)
		goto on_error;
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	return ref_image(image);

on_error:
	free(image);
	return NULL;
}

image_t*
read_image(ALLEGRO_FILE* file, int width, int height)
{
	image_t*               image;
	uint8_t*               line_ptr;
	size_t                 line_size;
	ALLEGRO_LOCKED_REGION* lock = NULL;
	int64_t                old_file_pos;

	int i_y;

	old_file_pos = al_ftell(file);
	if ((image = calloc(1, sizeof(image_t))) == NULL)
		goto on_error;
	if ((image->bitmap = al_create_bitmap(width, height)) == NULL)
		goto on_error;
	if ((lock = al_lock_bitmap(image->bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY)) == NULL)
		goto on_error;
	line_size = width * 4;
	for (i_y = 0; i_y < height; ++i_y) {
		line_ptr = (uint8_t*)lock->data + i_y * lock->pitch;
		if (al_fread(file, line_ptr, line_size) != line_size)
			goto on_error;
	}
	al_unlock_bitmap(image->bitmap);
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	return ref_image(image);

on_error:
	al_fseek(file, old_file_pos, ALLEGRO_SEEK_SET);
	if (lock != NULL) al_unlock_bitmap(image->bitmap);
	if (image != NULL) {
		if (image->bitmap != NULL) al_destroy_bitmap(image->bitmap);
		free(image);
	}
	return NULL;
}

image_t*
read_subimage(ALLEGRO_FILE* file, image_t* parent, int x, int y, int width, int height)
{
	image_t*               image;
	uint8_t*               line_ptr;
	size_t                 line_size;
	ALLEGRO_LOCKED_REGION* lock = NULL;
	int64_t                old_file_pos;

	int i_y;

	old_file_pos = al_ftell(file);
	if (!(image = create_subimage(parent, x, y, width, height))) goto on_error;
	if ((lock = al_lock_bitmap(image->bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY)) == NULL)
		goto on_error;
	line_size = width * 4;
	for (i_y = 0; i_y < height; ++i_y) {
		line_ptr = (uint8_t*)lock->data + i_y * lock->pitch;
		if (al_fread(file, line_ptr, line_size) != line_size)
			goto on_error;
	}
	al_unlock_bitmap(image->bitmap);
	return image;

on_error:
	al_fseek(file, old_file_pos, ALLEGRO_SEEK_SET);
	if (lock != NULL) al_unlock_bitmap(image->bitmap);
	free_image(image);
	return NULL;
}

image_t*
ref_image(image_t* image)
{
	++image->c_refs;
	return image;
}

void
free_image(image_t* image)
{
	if (image == NULL || --image->c_refs > 0)
		return;
	al_destroy_bitmap(image->bitmap);
	free_image(image->parent);
	free(image);
}

ALLEGRO_BITMAP*
get_image_bitmap(const image_t* image)
{
	return image->bitmap;
}

int
get_image_height(const image_t* image)
{
	return image->height;
}

int
get_image_width(const image_t* image)
{
	return image->width;
}

bool
apply_image_lookup(image_t* image, int x, int y, int width, int height, uint8_t red_lu[256], uint8_t green_lu[256], uint8_t blue_lu[256], uint8_t alpha_lu[256])
{
	ALLEGRO_BITMAP*        bitmap = get_image_bitmap(image);
	uint8_t*               pixel;
	ALLEGRO_LOCKED_REGION* lock;

	int i_x, i_y;

	if ((lock = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_READWRITE)) == NULL)
		return false;
	for (i_x = x; i_x < x + width; ++i_x) for (i_y = y; i_y < y + height; ++i_y) {
		pixel = (uint8_t*)lock->data + i_x * 4 + i_y * lock->pitch;
		pixel[0] = red_lu[pixel[0]];
		pixel[1] = green_lu[pixel[1]];
		pixel[2] = blue_lu[pixel[2]];
		pixel[3] = alpha_lu[pixel[3]];
	}
	al_unlock_bitmap(bitmap);
	return true;
}

bool
flip_image(image_t* image, bool is_h_flip, bool is_v_flip)
{
	int             draw_flags = 0x0;
	ALLEGRO_BITMAP* new_bitmap;
	ALLEGRO_BITMAP* old_target;

	if (!is_h_flip && !is_v_flip)  // this really shouldn't happen...
		return true;
	if (!(new_bitmap = al_create_bitmap(image->width, image->height))) return false;
	old_target = al_get_target_bitmap();
	al_set_target_bitmap(new_bitmap);
	if (is_h_flip) draw_flags &= ALLEGRO_FLIP_HORIZONTAL;
	if (is_v_flip) draw_flags &= ALLEGRO_FLIP_VERTICAL;
	al_draw_bitmap(image->bitmap, 0, 0, draw_flags);
	al_set_target_bitmap(old_target);
	al_destroy_bitmap(image->bitmap);
	image->bitmap = new_bitmap;
	return true;
}

bool
rescale_image(image_t* image, int width, int height)
{
	ALLEGRO_BITMAP* new_bitmap;
	ALLEGRO_BITMAP* old_target;

	if (width == image->width && height == image->height)
		return true;
	if (!(new_bitmap = al_create_bitmap(width, height))) return false;
	old_target = al_get_target_bitmap();
	al_set_target_bitmap(new_bitmap);
	al_draw_scaled_bitmap(image->bitmap, 0, 0, image->width, image->height, 0, 0, width, height, 0x0);
	al_set_target_bitmap(old_target);
	al_destroy_bitmap(image->bitmap);
	image->bitmap = new_bitmap;
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	return true;
}

void
init_image_api(duk_context* ctx)
{
	const char* filename;
	char*       path;
	
	// load system-provided images
	if (g_sys_conf != NULL) {
		filename = al_get_config_value(g_sys_conf, NULL, "Arrow");
		path = get_sys_asset_path(filename, "system");
		s_sys_arrow = load_image(path);
		free(path);
		filename = al_get_config_value(g_sys_conf, NULL, "UpArrow");
		path = get_sys_asset_path(filename, "system");
		s_sys_up_arrow = load_image(path);
		free(path);
		filename = al_get_config_value(g_sys_conf, NULL, "DownArrow");
		path = get_sys_asset_path(filename, "system");
		s_sys_dn_arrow = load_image(path);
		free(path);
	}
	
	// register image API functions
	register_api_func(ctx, NULL, "GetSystemArrow", js_GetSystemArrow);
	register_api_func(ctx, NULL, "GetSystemDownArrow", js_GetSystemDownArrow);
	register_api_func(ctx, NULL, "GetSystemUpArrow", js_GetSystemUpArrow);
	register_api_func(ctx, NULL, "LoadImage", js_LoadImage);
	register_api_func(ctx, NULL, "GrabImage", js_GrabImage);
}

void
duk_push_sphere_image(duk_context* ctx, image_t* image)
{
	ref_image(image);
	duk_push_object(ctx);
	duk_push_string(ctx, "image"); duk_put_prop_string(ctx, -2, "\xFF" "sphere_type");
	duk_push_pointer(ctx, image); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_c_function(ctx, js_Image_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, js_Image_blit, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blit");
	duk_push_c_function(ctx, js_Image_blitMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blitMask");
	duk_push_c_function(ctx, js_Image_createSurface, DUK_VARARGS); duk_put_prop_string(ctx, -2, "createSurface");
	duk_push_c_function(ctx, js_Image_rotateBlit, DUK_VARARGS); duk_put_prop_string(ctx, -2, "rotateBlit");
	duk_push_c_function(ctx, js_Image_rotateBlitMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "rotateBlitMask");
	duk_push_c_function(ctx, js_Image_transformBlit, DUK_VARARGS); duk_put_prop_string(ctx, -2, "transformBlit");
	duk_push_c_function(ctx, js_Image_transformBlitMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "transformBlitMask");
	duk_push_c_function(ctx, js_Image_zoomBlit, DUK_VARARGS); duk_put_prop_string(ctx, -2, "zoomBlit");
	duk_push_c_function(ctx, js_Image_zoomBlitMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "zoomBlitMask");
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
duk_require_sphere_image(duk_context* ctx, duk_idx_t index)
{
	image_t*    image;
	const char* type;

	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	if (!duk_get_prop_string(ctx, index, "\xFF" "sphere_type"))
		goto on_error;
	type = duk_get_string(ctx, -1); duk_pop(ctx);
	if (strcmp(type, "image") != 0) goto on_error;
	duk_get_prop_string(ctx, index, "\xFF" "ptr");
	image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	return image;

on_error:
	duk_error(ctx, DUK_ERR_TYPE_ERROR, "Not a Sphere image");
}

static duk_ret_t
js_GetSystemArrow(duk_context* ctx)
{
	if (s_sys_arrow == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "GetSystemArrow(): No system arrow image available");
	duk_push_sphere_image(ctx, s_sys_arrow);
	return 1;
}

static duk_ret_t
js_GetSystemDownArrow(duk_context* ctx)
{
	if (s_sys_dn_arrow == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "GetSystemDownArrow(): No system down arrow image available");
	duk_push_sphere_image(ctx, s_sys_dn_arrow);
	return 1;
}

static duk_ret_t
js_GetSystemUpArrow(duk_context* ctx)
{
	if (s_sys_up_arrow != NULL)
		duk_error(ctx, DUK_ERR_ERROR, "GetSystemUpArrow(): No system up arrow image available");
	duk_push_sphere_image(ctx, s_sys_up_arrow);
	return 1;
}

static duk_ret_t
js_LoadImage(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	image_t* image;
	char*    path;
	
	path = get_asset_path(filename, "images", false);
	image = load_image(path);
	free(path);
	if (image == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "LoadImage(): Failed to load image file '%s'", filename);
	duk_push_sphere_image(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_GrabImage(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0) * g_scale_x;
	int y = duk_require_int(ctx, 1) * g_scale_y;
	int w = duk_require_int(ctx, 2) * g_scale_x;
	int h = duk_require_int(ctx, 3) * g_scale_y;

	ALLEGRO_BITMAP* backbuffer;
	image_t*        image;

	backbuffer = al_get_backbuffer(g_display);
	if ((image = create_image(w, h)) == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "GrabImage(): Failed to create image bitmap");
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_bitmap_region(backbuffer, x, y, w, h, 0, 0, 0x0);
	al_set_target_backbuffer(g_display);
	if (!rescale_image(image, g_res_x, g_res_y))
		duk_error(ctx, DUK_ERR_ERROR, "GrabImage(): Failed to rescale grabbed image (internal error)");
	duk_push_sphere_image(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_Image_finalize(duk_context* ctx)
{
	image_t* image;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	free_image(image);
	return 0;
}

static duk_ret_t
js_Image_blit(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	
	image_t* image;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame()) al_draw_bitmap(get_image_bitmap(image), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Image_blitMask(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	ALLEGRO_COLOR mask = duk_get_sphere_color(ctx, 2);

	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame()) al_draw_tinted_bitmap(get_image_bitmap(image), mask, x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Image_createSurface(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if ((new_image = clone_image(image)) == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "Image:createSurface(): Failed to create new surface image");
	duk_push_sphere_surface(ctx, new_image);
	free_image(new_image);
	return 1;
}

static duk_ret_t
js_Image_rotateBlit(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	float angle = duk_require_number(ctx, 2);

	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame())
		al_draw_rotated_bitmap(get_image_bitmap(image), image->width / 2, image->height / 2, x, y, angle, 0x0);
	return 0;
}

static duk_ret_t
js_Image_rotateBlitMask(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	float angle = duk_require_number(ctx, 2);
	ALLEGRO_COLOR mask = duk_get_sphere_color(ctx, 3);

	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame())
		al_draw_tinted_rotated_bitmap(get_image_bitmap(image), mask, image->width / 2, image->height / 2, x, y, angle, 0x0);
	return 0;
}

static duk_ret_t
js_Image_transformBlit(duk_context* ctx)
{
	int x1 = duk_require_int(ctx, 0);
	int y1 = duk_require_int(ctx, 1);
	int x2 = duk_require_int(ctx, 2);
	int y2 = duk_require_int(ctx, 3);
	int x3 = duk_require_int(ctx, 4);
	int y3 = duk_require_int(ctx, 5);
	int x4 = duk_require_int(ctx, 6);
	int y4 = duk_require_int(ctx, 7);

	image_t*      image;
	ALLEGRO_COLOR vertex_color;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	vertex_color = al_map_rgba(255, 255, 255, 255);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, vertex_color },
		{ x2, y2, 0, image->width, 0, vertex_color },
		{ x4, y4, 0, 0, image->height, vertex_color },
		{ x3, y3, 0, image->width, image->height, vertex_color }
	};
	if (!is_skipped_frame())
		al_draw_prim(v, NULL, get_image_bitmap(image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return 0;
}

static duk_ret_t
js_Image_transformBlitMask(duk_context* ctx)
{
	int x1 = duk_require_int(ctx, 0);
	int y1 = duk_require_int(ctx, 1);
	int x2 = duk_require_int(ctx, 2);
	int y2 = duk_require_int(ctx, 3);
	int x3 = duk_require_int(ctx, 4);
	int y3 = duk_require_int(ctx, 5);
	int x4 = duk_require_int(ctx, 6);
	int y4 = duk_require_int(ctx, 7);
	ALLEGRO_COLOR mask = duk_get_sphere_color(ctx, 8);

	image_t*      image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, mask },
		{ x2, y2, 0, image->width, 0, mask },
		{ x4, y4, 0, 0, image->height, mask },
		{ x3, y3, 0, image->width, image->height, mask }
	};
	if (!is_skipped_frame())
		al_draw_prim(v, NULL, get_image_bitmap(image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return 0;
}

static duk_ret_t
js_Image_zoomBlit(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	float scale = duk_require_number(ctx, 2);
	
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame())
		al_draw_scaled_bitmap(get_image_bitmap(image), 0, 0, image->width, image->height, x, y, image->width * scale, image->height * scale, 0x0);
	return 0;
}

static duk_ret_t
js_Image_zoomBlitMask(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	float scale = duk_require_number(ctx, 2);
	ALLEGRO_COLOR mask = duk_get_sphere_color(ctx, 3);

	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame())
		al_draw_tinted_scaled_bitmap(get_image_bitmap(image), mask, 0, 0, image->width, image->height, x, y, image->width * scale, image->height * scale, 0x0);
	return 0;
}
