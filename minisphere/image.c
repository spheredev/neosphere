#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "surface.h"

#include "image.h"

struct image
{
	int             refcount;
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
static duk_ret_t js_new_Image                (duk_context* ctx);
static duk_ret_t js_Image_finalize           (duk_context* ctx);
static duk_ret_t js_Image_toString           (duk_context* ctx);
static duk_ret_t js_Image_get_height         (duk_context* ctx);
static duk_ret_t js_Image_get_width          (duk_context* ctx);
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
read_image(FILE* file, int width, int height)
{
	long                   file_pos;
	image_t*               image;
	uint8_t*               line_ptr;
	size_t                 line_size;
	ALLEGRO_LOCKED_REGION* lock = NULL;

	int i_y;

	file_pos = ftell(file);
	if ((image = calloc(1, sizeof(image_t))) == NULL) goto on_error;
	if ((image->bitmap = al_create_bitmap(width, height)) == NULL) goto on_error;
	if ((lock = al_lock_bitmap(image->bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY)) == NULL)
		goto on_error;
	line_size = width * 4;
	for (i_y = 0; i_y < height; ++i_y) {
		line_ptr = (uint8_t*)lock->data + i_y * lock->pitch;
		if (fread(line_ptr, line_size, 1, file) != 1)
			goto on_error;
	}
	al_unlock_bitmap(image->bitmap);
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	return ref_image(image);

on_error:
	fseek(file, file_pos, SEEK_SET);
	if (lock != NULL) al_unlock_bitmap(image->bitmap);
	if (image != NULL) {
		if (image->bitmap != NULL) al_destroy_bitmap(image->bitmap);
		free(image);
	}
	return NULL;
}

image_t*
read_subimage(FILE* file, image_t* parent, int x, int y, int width, int height)
{
	long                   file_pos;
	image_t*               image;
	uint8_t*               line_ptr;
	size_t                 line_size;
	ALLEGRO_LOCKED_REGION* lock = NULL;

	int i_y;

	file_pos = ftell(file);
	if (!(image = create_subimage(parent, x, y, width, height))) goto on_error;
	if ((lock = al_lock_bitmap(image->bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY)) == NULL)
		goto on_error;
	line_size = width * 4;
	for (i_y = 0; i_y < height; ++i_y) {
		line_ptr = (uint8_t*)lock->data + i_y * lock->pitch;
		if (fread(line_ptr, line_size, 1, file) != 1)
			goto on_error;
	}
	al_unlock_bitmap(image->bitmap);
	return image;

on_error:
	fseek(file, file_pos, SEEK_SET);
	if (lock != NULL) al_unlock_bitmap(image->bitmap);
	free_image(image);
	return NULL;
}

image_t*
ref_image(image_t* image)
{
	++image->refcount;
	return image;
}

void
free_image(image_t* image)
{
	if (image == NULL || --image->refcount > 0)
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

void
draw_image(image_t* image, int x, int y)
{
	al_draw_bitmap(image->bitmap, x, y, 0x0);
}

void
draw_image_masked(image_t* image, color_t mask, int x, int y)
{
	al_draw_tinted_bitmap(image->bitmap, al_map_rgba(mask.r, mask.g, mask.b, mask.alpha), x, y, 0x0);
}

void
draw_image_scaled(image_t* image, int x, int y, int width, int height)
{
	al_draw_scaled_bitmap(image->bitmap,
		0, 0, al_get_bitmap_width(image->bitmap), al_get_bitmap_height(image->bitmap),
		x, y, width, height, 0x0);
}

void
draw_image_scaled_masked(image_t* image, color_t mask, int x, int y, int width, int height)
{
	al_draw_tinted_scaled_bitmap(image->bitmap, nativecolor(mask),
		0, 0, al_get_bitmap_width(image->bitmap), al_get_bitmap_height(image->bitmap),
		x, y, width, height, 0x0);
}

void
draw_image_tiled(image_t* image, int x, int y, int width, int height)
{
	draw_image_tiled_masked(image, rgba(255, 255, 255, 255), x, y, width, height);
}

void
draw_image_tiled_masked(image_t* image, color_t mask, int x, int y, int width, int height)
{
	ALLEGRO_COLOR vtx_color = nativecolor(mask);

	ALLEGRO_VERTEX vbuf[] = {
		{ x, y, 0, 0, 0, vtx_color },
		{ x + width, y, 0, width, 0, vtx_color },
		{ x, y + height, 0, 0, height, vtx_color },
		{ x + width, y + height, 0, width, height, vtx_color }
	};
	al_draw_prim(vbuf, NULL, image->bitmap, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
}

void
fill_image(image_t* image, color_t color)
{
	int             clip_x, clip_y, clip_w, clip_h;
	ALLEGRO_BITMAP* last_target;

	al_get_clipping_rectangle(&clip_x, &clip_y, &clip_w, &clip_h);
	al_reset_clipping_rectangle();
	last_target = al_get_target_bitmap();
	al_set_target_bitmap(image->bitmap);
	al_clear_to_color(al_map_rgba(color.r, color.g, color.b, color.alpha));
	al_set_target_bitmap(last_target);
	al_set_clipping_rectangle(clip_x, clip_y, clip_w, clip_h);
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
replace_image_color(image_t* image, color_t color, color_t new_color)
{
	ALLEGRO_BITMAP*        bitmap = get_image_bitmap(image);
	uint8_t*               pixel;
	ALLEGRO_LOCKED_REGION* lock;
	int                    w, h;

	int i_x, i_y;

	if ((lock = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_READWRITE)) == NULL)
		return false;
	w = al_get_bitmap_width(bitmap);
	h = al_get_bitmap_height(bitmap);
	for (i_x = 0; i_x < w; ++i_x) for (i_y = 0; i_y < h; ++i_y) {
		pixel = (uint8_t*)lock->data + i_x * 4 + i_y * lock->pitch;
		if (pixel[0] == color.r &&
		    pixel[1] == color.g &&
		    pixel[2] == color.b &&
		    pixel[3] == color.alpha)
		{
			pixel[0] = new_color.r;
			pixel[1] = new_color.g;
			pixel[2] = new_color.b;
			pixel[3] = new_color.alpha;
		}
	}
	al_unlock_bitmap(bitmap);
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

	// register Image methods and properties
	register_api_func(ctx, NULL, "Image", js_new_Image);
	register_api_prop(ctx, "Image", "height", js_Image_get_height, NULL);
	register_api_prop(ctx, "Image", "width", js_Image_get_width, NULL);
	register_api_func(ctx, "Image", "toString", js_Image_toString);
	register_api_func(ctx, "Image", "blit", js_Image_blit);
	register_api_func(ctx, "Image", "blitMask", js_Image_blitMask);
	register_api_func(ctx, "Image", "createSurface", js_Image_createSurface);
	register_api_func(ctx, "Image", "rotateBlit", js_Image_rotateBlit);
	register_api_func(ctx, "Image", "rotateBlitMask", js_Image_rotateBlitMask);
	register_api_func(ctx, "Image", "transformBlit", js_Image_transformBlit);
	register_api_func(ctx, "Image", "transformBlitMask", js_Image_transformBlitMask);
	register_api_func(ctx, "Image", "zoomBlit", js_Image_zoomBlit);
	register_api_func(ctx, "Image", "zoomBlitMask", js_Image_zoomBlitMask);
}

void
duk_push_sphere_image(duk_context* ctx, image_t* image)
{
	ref_image(image);

	duk_push_sphere_object(ctx, "Image");
	duk_push_string(ctx, "image"); duk_put_prop_string(ctx, -2, "\xFF" "sphere_type");
	duk_push_pointer(ctx, image); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_c_function(ctx, js_Image_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
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
	duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "object is not a Sphere image");
}

static duk_ret_t
js_GetSystemArrow(duk_context* ctx)
{
	if (s_sys_arrow == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetSystemArrow(): no system arrow image available");
	duk_push_sphere_image(ctx, s_sys_arrow);
	return 1;
}

static duk_ret_t
js_GetSystemDownArrow(duk_context* ctx)
{
	if (s_sys_dn_arrow == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetSystemDownArrow(): no system down arrow image available");
	duk_push_sphere_image(ctx, s_sys_dn_arrow);
	return 1;
}

static duk_ret_t
js_GetSystemUpArrow(duk_context* ctx)
{
	if (s_sys_up_arrow == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetSystemUpArrow(): no system up arrow image available");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LoadImage(): failed to load image file '%s'", filename);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GrabImage(): failed to create image bitmap");
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_bitmap_region(backbuffer, x, y, w, h, 0, 0, 0x0);
	al_set_target_backbuffer(g_display);
	if (!rescale_image(image, g_res_x, g_res_y))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GrabImage(): failed to rescale grabbed image (internal error)");
	duk_push_sphere_image(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_new_Image(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	image_t* image;
	char*    path;

	path = get_asset_path(filename, "images", false);
	image = load_image(path);
	free(path);
	if (image == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image(): failed to load image file '%s'", filename);
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
js_Image_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, get_image_height(image));
	return 1;
}

static duk_ret_t
js_Image_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, get_image_width(image));
	return 1;
}

static duk_ret_t
js_Image_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object image]");
	return 1;
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
	color_t mask = duk_require_sphere_color(ctx, 2);

	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame()) al_draw_tinted_bitmap(get_image_bitmap(image), al_map_rgba(mask.r, mask.g, mask.b, mask.alpha), x, y, 0x0);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image:createSurface(): Failed to create new surface image");
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
	color_t mask = duk_require_sphere_color(ctx, 3);

	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame())
		al_draw_tinted_rotated_bitmap(get_image_bitmap(image), al_map_rgba(mask.r, mask.g, mask.b, mask.alpha),
			image->width / 2, image->height / 2, x, y, angle, 0x0);
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
	color_t mask = duk_require_sphere_color(ctx, 8);

	ALLEGRO_COLOR vtx_color;
	image_t*      image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	vtx_color = al_map_rgba(mask.r, mask.g, mask.b, mask.alpha);
	ALLEGRO_VERTEX v[] = {
		{ x1, y1, 0, 0, 0, vtx_color },
		{ x2, y2, 0, image->width, 0, vtx_color },
		{ x4, y4, 0, 0, image->height, vtx_color },
		{ x3, y3, 0, image->width, image->height, vtx_color }
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
	color_t mask = duk_require_sphere_color(ctx, 3);

	image_t* image;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); image = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame())
		al_draw_tinted_scaled_bitmap(get_image_bitmap(image), al_map_rgba(mask.r, mask.g, mask.b, mask.alpha),
			0, 0, image->width, image->height, x, y, image->width * scale, image->height * scale, 0x0);
	return 0;
}
