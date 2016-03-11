#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "surface.h"

#include "image.h"

struct image
{
	unsigned int    refcount;
	unsigned int    id;
	ALLEGRO_BITMAP* bitmap;
	unsigned int    cache_hits;
	image_lock_t    lock;
	unsigned int    lock_count;
	color_t*        pixel_cache;
	int             width;
	int             height;
	image_t*        parent;
};

static duk_ret_t js_GetSystemArrow          (duk_context* ctx);
static duk_ret_t js_GetSystemDownArrow      (duk_context* ctx);
static duk_ret_t js_GetSystemUpArrow        (duk_context* ctx);
static duk_ret_t js_LoadImage               (duk_context* ctx);
static duk_ret_t js_GrabImage               (duk_context* ctx);
static duk_ret_t js_new_Image               (duk_context* ctx);
static duk_ret_t js_Image_finalize          (duk_context* ctx);
static duk_ret_t js_Image_toString          (duk_context* ctx);
static duk_ret_t js_Image_get_height        (duk_context* ctx);
static duk_ret_t js_Image_get_width         (duk_context* ctx);
static duk_ret_t js_Image_blit              (duk_context* ctx);
static duk_ret_t js_Image_blitMask          (duk_context* ctx);
static duk_ret_t js_Image_createSurface     (duk_context* ctx);
static duk_ret_t js_Image_rotateBlit        (duk_context* ctx);
static duk_ret_t js_Image_rotateBlitMask    (duk_context* ctx);
static duk_ret_t js_Image_transformBlit     (duk_context* ctx);
static duk_ret_t js_Image_transformBlitMask (duk_context* ctx);
static duk_ret_t js_Image_zoomBlit          (duk_context* ctx);
static duk_ret_t js_Image_zoomBlitMask      (duk_context* ctx);

static void cache_pixels   (image_t* image);
static void uncache_pixels (image_t* image);

static unsigned int s_next_image_id = 0;
static image_t*     s_sys_arrow = NULL;
static image_t*     s_sys_dn_arrow = NULL;
static image_t*     s_sys_up_arrow = NULL;

image_t*
create_image(int width, int height)
{
	image_t* image;

	console_log(3, "creating image #%u at %ix%i", s_next_image_id, width, height);
	image = calloc(1, sizeof(image_t));
	if ((image->bitmap = al_create_bitmap(width, height)) == NULL)
		goto on_error;
	image->id = s_next_image_id++;
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

	console_log(3, "creating image #%u as %ix%i subimage of image #%u", s_next_image_id, width, height, parent->id);
	image = calloc(1, sizeof(image_t));
	if (!(image->bitmap = al_create_sub_bitmap(parent->bitmap, x, y, width, height)))
		goto on_error;
	image->id = s_next_image_id++;
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

	console_log(3, "cloning image #%u from source image #%u",
		s_next_image_id, src_image->id);
	
	image = calloc(1, sizeof(image_t));
	if (!(image->bitmap = al_clone_bitmap(src_image->bitmap)))
		goto on_error;
	image->id = s_next_image_id++;
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	
	return ref_image(image);

on_error:
	free(image);
	return NULL;
}

image_t*
load_image(const char* filename)
{
	ALLEGRO_FILE* al_file = NULL;
	const char*   file_ext;
	size_t        file_size;
	uint8_t       first_16[16];
	image_t*      image;
	void*         slurp = NULL;

	console_log(2, "loading image #%u as '%s'", s_next_image_id, filename);
	
	image = calloc(1, sizeof(image_t));
	if (!(slurp = sfs_fslurp(g_fs, filename, NULL, &file_size)))
		goto on_error;
	al_file = al_open_memfile(slurp, file_size, "rb");

	// look at the first 16 bytes of the file to determine its actual type.
	// Allegro won't load it if the content doesn't match the file extension, so
	// we have to inspect the file ourselves.
	al_fread(al_file, first_16, 16);
	al_fseek(al_file, 0, ALLEGRO_SEEK_SET);
	file_ext = strrchr(filename, '.');
	if (memcmp(first_16, "BM", 2) == 0) file_ext = ".bmp";
	if (memcmp(first_16, "\211PNG\r\n\032\n", 8) == 0) file_ext = ".png";
	if (memcmp(first_16, "\0xFF\0xD8", 2) == 0) file_ext = ".jpg";

	if (!(image->bitmap = al_load_bitmap_f(al_file, file_ext)))
		goto on_error;
	al_fclose(al_file);
	free(slurp);
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	
	image->id = s_next_image_id++;
	return ref_image(image);

on_error:
	console_log(2, "    failed to load image #%u", s_next_image_id++);
	if (al_file != NULL)
		al_fclose(al_file);
	free(slurp);
	free(image);
	return NULL;
}

image_t*
read_image(sfs_file_t* file, int width, int height)
{
	long                   file_pos;
	image_t*               image;
	uint8_t*               line_ptr;
	size_t                 line_size;
	ALLEGRO_LOCKED_REGION* lock = NULL;

	int i_y;

	console_log(3, "reading %ix%i image #%u from open file", width, height, s_next_image_id);
	image = calloc(1, sizeof(image_t));
	file_pos = sfs_ftell(file);
	if (!(image->bitmap = al_create_bitmap(width, height))) goto on_error;
	if (!(lock = al_lock_bitmap(image->bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY)))
		goto on_error;
	line_size = width * 4;
	for (i_y = 0; i_y < height; ++i_y) {
		line_ptr = (uint8_t*)lock->data + i_y * lock->pitch;
		if (sfs_fread(line_ptr, line_size, 1, file) != 1)
			goto on_error;
	}
	al_unlock_bitmap(image->bitmap);
	image->id = s_next_image_id++;
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	return ref_image(image);

on_error:
	console_log(3, "    failed!");
	sfs_fseek(file, file_pos, SEEK_SET);
	if (lock != NULL) al_unlock_bitmap(image->bitmap);
	if (image != NULL) {
		if (image->bitmap != NULL) al_destroy_bitmap(image->bitmap);
		free(image);
	}
	return NULL;
}

image_t*
read_subimage(sfs_file_t* file, image_t* parent, int x, int y, int width, int height)
{
	long          file_pos;
	image_t*      image;
	image_lock_t* lock = NULL;
	color_t       *pline;

	int i_y;

	file_pos = sfs_ftell(file);
	if (!(image = create_subimage(parent, x, y, width, height))) goto on_error;
	if (!(lock = lock_image(parent))) goto on_error;
	for (i_y = 0; i_y < height; ++i_y) {
		pline = lock->pixels + x + (i_y + y) * lock->pitch;
		if (sfs_fread(pline, width * 4, 1, file) != 1)
			goto on_error;
	}
	unlock_image(parent, lock);
	return image;

on_error:
	sfs_fseek(file, file_pos, SEEK_SET);
	if (lock != NULL)
		unlock_image(parent, lock);
	free_image(image);
	return NULL;
}

image_t*
ref_image(image_t* image)
{
	
	if (image != NULL)
		++image->refcount;
	return image;
}

void
free_image(image_t* image)
{
	if (image == NULL || --image->refcount > 0)
		return;
	
	console_log(3, "disposing image #%u no longer in use",
		image->id);
	uncache_pixels(image);
	al_destroy_bitmap(image->bitmap);
	free_image(image->parent);
	free(image);
}

ALLEGRO_BITMAP*
get_image_bitmap(image_t* image)
{
	uncache_pixels(image);
	return image->bitmap;
}

int
get_image_height(const image_t* image)
{
	return image->height;
}

color_t
get_image_pixel(image_t* image, int x, int y)
{
	if (image->pixel_cache == NULL) {
		console_log(4, "get_image_pixel() cache miss for image #%u", image->id);
		cache_pixels(image);
	}
	else
		++image->cache_hits;
	return image->pixel_cache[x + y * image->width];
}

int
get_image_width(const image_t* image)
{
	return image->width;
}

void
set_image_pixel(image_t* image, int x, int y, color_t color)
{
	ALLEGRO_BITMAP* old_target;

	uncache_pixels(image);
	old_target = al_get_target_bitmap();
	al_set_target_bitmap(image->bitmap);
	al_draw_pixel(x + 0.5, y + 0.5, nativecolor(color));
	al_set_target_bitmap(old_target);
}

bool
apply_color_matrix(image_t* image, colormatrix_t matrix, int x, int y, int width, int height)
{
	image_lock_t* lock;
	color_t*      pixel;

	int i_x, i_y;

	if (!(lock = lock_image(image)))
		return false;
	uncache_pixels(image);
	for (i_x = x; i_x < x + width; ++i_x) for (i_y = y; i_y < y + height; ++i_y) {
		pixel = &lock->pixels[i_x + i_y * lock->pitch];
		*pixel = transform_pixel(*pixel, matrix);
	}
	unlock_image(image, lock);
	return true;
}

bool
apply_color_matrix_4(image_t* image, colormatrix_t ul_mat, colormatrix_t ur_mat, colormatrix_t ll_mat, colormatrix_t lr_mat, int x, int y, int w, int h)
{
	// this function might be difficult to understand at first. the implementation
	// is, however, much easier to follow than the one in Sphere. basically what it
	// boils down to is bilinear interpolation, but with matrices. it's much more
	// straightforward than it sounds.
	
	int           i1, i2;
	image_lock_t* lock;
	colormatrix_t mat_1, mat_2, mat_3;
	color_t*      pixel;

	int i_x, i_y;

	if (!(lock = lock_image(image)))
		return false;
	uncache_pixels(image);
	for (i_y = y; i_y < y + h; ++i_y) {
		// thankfully, we don't have to do a full bilinear interpolation every frame.
		// two thirds of the work is done in the outer loop, giving us two color matrices
		// which we then use in the inner loop to calculate the transforms for individual
		// pixels.
		i1 = y + h - 1 - i_y;
		i2 = i_y - y;
		mat_1 = blend_color_matrices(ul_mat, ll_mat, i1, i2);
		mat_2 = blend_color_matrices(ur_mat, lr_mat, i1, i2);
		for (i_x = x; i_x < x + w; ++i_x) {
			// calculate the final matrix for this pixel and transform it
			i1 = x + w - 1 - i_x;
			i2 = i_x - x;
			mat_3 = blend_color_matrices(mat_1, mat_2, i1, i2);
			pixel = &lock->pixels[i_x + i_y * lock->pitch];
			*pixel = transform_pixel(*pixel, mat_3);

		}
	}
	unlock_image(image, lock);
	return true;
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
	uncache_pixels(image);
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
	ALLEGRO_COLOR native_mask = nativecolor(mask);
	int           img_w, img_h;
	bool          is_drawing_held;
	int           tile_w, tile_h;

	int i_x, i_y;

	img_w = image->width; img_h = image->height;
	if (img_w >= 16 && img_h >= 16) {
		// tile in hardware whenever possible
		ALLEGRO_VERTEX vbuf[] = {
			{ x, y, 0, 0, 0, native_mask },
			{ x + width, y, 0, width, 0, native_mask },
			{ x, y + height, 0, 0, height, native_mask },
			{ x + width, y + height, 0, width, height, native_mask }
		};
		al_draw_prim(vbuf, NULL, image->bitmap, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	}
	else {
		// texture smaller than 16x16, tile it in software (Allegro pads it)
		is_drawing_held = al_is_bitmap_drawing_held();
		al_hold_bitmap_drawing(true);
		for (i_x = width / img_w; i_x >= 0; --i_x) for (i_y = height / img_h; i_y >= 0; --i_y) {
			tile_w = i_x == width / img_w ? width % img_w : img_w;
			tile_h = i_y == height / img_h ? height % img_h : img_h;
			al_draw_tinted_bitmap_region(image->bitmap, native_mask,
				0, 0, tile_w, tile_h,
				x + i_x * img_w, y + i_y * img_h, 0x0);
		}
		al_hold_bitmap_drawing(is_drawing_held);
	}
}

void
fill_image(image_t* image, color_t color)
{
	int             clip_x, clip_y, clip_w, clip_h;
	ALLEGRO_BITMAP* last_target;

	uncache_pixels(image);
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
	uncache_pixels(image);
	if (!(new_bitmap = al_create_bitmap(image->width, image->height))) return false;
	old_target = al_get_target_bitmap();
	al_set_target_bitmap(new_bitmap);
	if (is_h_flip) draw_flags |= ALLEGRO_FLIP_HORIZONTAL;
	if (is_v_flip) draw_flags |= ALLEGRO_FLIP_VERTICAL;
	al_draw_bitmap(image->bitmap, 0, 0, draw_flags);
	al_set_target_bitmap(old_target);
	al_destroy_bitmap(image->bitmap);
	image->bitmap = new_bitmap;
	return true;
}

image_lock_t*
lock_image(image_t* image)
{
	ALLEGRO_LOCKED_REGION* ll_lock;

	if (image->lock_count == 0) {
		if (!(ll_lock = al_lock_bitmap(image->bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_READWRITE)))
			return NULL;
		ref_image(image);
		image->lock.pixels = ll_lock->data;
		image->lock.pitch = ll_lock->pitch / 4;
	}
	++image->lock_count;
	return &image->lock;
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
	uncache_pixels(image);
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
	if (!(new_bitmap = al_create_bitmap(width, height)))
		return false;
	uncache_pixels(image);
	old_target = al_get_target_bitmap();
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
	al_set_target_bitmap(new_bitmap);
	al_draw_scaled_bitmap(image->bitmap, 0, 0, image->width, image->height, 0, 0, width, height, 0x0);
	al_set_target_bitmap(old_target);
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	al_destroy_bitmap(image->bitmap);
	image->bitmap = new_bitmap;
	image->width = al_get_bitmap_width(image->bitmap);
	image->height = al_get_bitmap_height(image->bitmap);
	return true;
}

bool
save_image(image_t* image, const char* filename)
{
	void*         buffer = NULL;
	size_t        file_size;
	bool          is_eof;
	ALLEGRO_FILE* memfile;
	size_t        next_buf_size;
	bool          result;

	next_buf_size = 65536;
	do {
		buffer = realloc(buffer, next_buf_size);
		memfile = al_open_memfile(buffer, next_buf_size, "wb");
		next_buf_size *= 2;
		al_save_bitmap_f(memfile, strrchr(filename, '.'), image->bitmap);
		file_size = al_ftell(memfile);
		is_eof = al_feof(memfile);
		al_fclose(memfile);
	} while (is_eof);
	result = sfs_fspew(g_fs, filename, NULL, buffer, file_size);
	free(buffer);
	return result;
}

void
unlock_image(image_t* image, image_lock_t* lock)
{
	// if the caller provides the wrong lock pointer, the image
	// won't be unlocked. this prevents accidental unlocking.
	if (lock != &image->lock) return;

	if (image->lock_count == 0 || --image->lock_count > 0)
		return;
	al_unlock_bitmap(image->bitmap);
	free_image(image);
}

static void
cache_pixels(image_t* image)
{
	color_t*      cache;
	image_lock_t* lock;
	void          *psrc, *pdest;

	int i;

	free(image->pixel_cache); image->pixel_cache = NULL;
	if (!(lock = lock_image(image)))
		goto on_error;
	if (!(cache = malloc(image->width * image->height * 4)))
		goto on_error;
	console_log(4, "creating new pixel cache for image #%u", image->id);
	for (i = 0; i < image->height; ++i) {
		psrc = lock->pixels + i * lock->pitch;
		pdest = cache + i * image->width;
		memcpy(pdest, psrc, image->width * 4);
	}
	unlock_image(image, lock);
	image->pixel_cache = cache;
	image->cache_hits = 0;
	return;
	
on_error:
	if (lock != NULL)
		al_unlock_bitmap(image->bitmap);
}

static void
uncache_pixels(image_t* image)
{
	if (image->pixel_cache == NULL)
		return;
	console_log(4, "pixel cache invalidated for image #%u, hits: %u", image->id, image->cache_hits);
	free(image->pixel_cache);
	image->pixel_cache = NULL;
}

void
init_image_api(duk_context* ctx)
{
	const char* filename;
	
	// load system-provided images
	if (g_sys_conf != NULL) {
		filename = read_string_rec(g_sys_conf, "Arrow", "pointer.png");
		s_sys_arrow = load_image(systempath(filename));
		filename = read_string_rec(g_sys_conf, "UpArrow", "up_arrow.png");
		s_sys_up_arrow = load_image(systempath(filename));
		filename = read_string_rec(g_sys_conf, "DownArrow", "down_arrow.png");
		s_sys_dn_arrow = load_image(systempath(filename));
	}
	
	// register image API functions
	register_api_method(ctx, NULL, "GetSystemArrow", js_GetSystemArrow);
	register_api_method(ctx, NULL, "GetSystemDownArrow", js_GetSystemDownArrow);
	register_api_method(ctx, NULL, "GetSystemUpArrow", js_GetSystemUpArrow);
	register_api_method(ctx, NULL, "LoadImage", js_LoadImage);
	register_api_method(ctx, NULL, "GrabImage", js_GrabImage);

	// register Image properties and methods
	register_api_ctor(ctx, "Image", js_new_Image, js_Image_finalize);
	register_api_method(ctx, "Image", "toString", js_Image_toString);
	register_api_prop(ctx, "Image", "height", js_Image_get_height, NULL);
	register_api_prop(ctx, "Image", "width", js_Image_get_width, NULL);
	register_api_method(ctx, "Image", "blit", js_Image_blit);
	register_api_method(ctx, "Image", "blitMask", js_Image_blitMask);
	register_api_method(ctx, "Image", "createSurface", js_Image_createSurface);
	register_api_method(ctx, "Image", "rotateBlit", js_Image_rotateBlit);
	register_api_method(ctx, "Image", "rotateBlitMask", js_Image_rotateBlitMask);
	register_api_method(ctx, "Image", "transformBlit", js_Image_transformBlit);
	register_api_method(ctx, "Image", "transformBlitMask", js_Image_transformBlitMask);
	register_api_method(ctx, "Image", "zoomBlit", js_Image_zoomBlit);
	register_api_method(ctx, "Image", "zoomBlitMask", js_Image_zoomBlitMask);
}

void
duk_push_sphere_image(duk_context* ctx, image_t* image)
{
	duk_push_sphere_obj(ctx, "Image", ref_image(image));
}

image_t*
duk_require_sphere_image(duk_context* ctx, duk_idx_t index)
{
	return duk_require_sphere_obj(ctx, index, "Image");
}

static duk_ret_t
js_GetSystemArrow(duk_context* ctx)
{
	if (s_sys_arrow == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetSystemArrow(): No system arrow image available");
	duk_push_sphere_image(ctx, s_sys_arrow);
	return 1;
}

static duk_ret_t
js_GetSystemDownArrow(duk_context* ctx)
{
	if (s_sys_dn_arrow == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetSystemDownArrow(): No system down arrow image available");
	duk_push_sphere_image(ctx, s_sys_dn_arrow);
	return 1;
}

static duk_ret_t
js_GetSystemUpArrow(duk_context* ctx)
{
	if (s_sys_up_arrow == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetSystemUpArrow(): No system up arrow image available");
	duk_push_sphere_image(ctx, s_sys_up_arrow);
	return 1;
}

static duk_ret_t
js_LoadImage(duk_context* ctx)
{
	// LoadImage(filename); (legacy)
	// Constructs an Image object from an image file.
	// Arguments:
	//     filename: The name of the image file, relative to ~sgm/images.

	const char* filename;
	image_t*    image;

	filename = duk_require_path(ctx, 0, "images", false);
	if (!(image = load_image(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image(): Failed to load image file '%s'", filename);
	duk_push_sphere_image(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_GrabImage(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);

	image_t* image;

	if (!(image = screen_grab(g_screen, x, y, w, h)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GrabImage(): Failed to grab backbuffer image");
	duk_push_sphere_image(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_new_Image(duk_context* ctx)
{
	int         n_args;
	const char* filename;
	color_t     fill_color;
	image_t*    image;
	image_t*    src_image;
	int         width, height;

	n_args = duk_get_top(ctx);
	if (n_args >= 3) {
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		fill_color = duk_require_sphere_color(ctx, 2);
		if (!(image = create_image(width, height)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image(): Failed to create new image");
		fill_image(image, fill_color);
	}
	else if (duk_is_sphere_obj(ctx, 0, "Surface")) {
		src_image = duk_require_sphere_surface(ctx, 0);
		if (!(image = clone_image(src_image)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image(): Failed to create image from surface");
	}
	else {
		filename = duk_require_path(ctx, 0, NULL, false);
		image = load_image(filename);
		if (image == NULL)
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image(): Failed to load image file '%s'", filename);
	}
	duk_push_sphere_image(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_Image_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_sphere_image(ctx, 0);
	free_image(image);
	return 0;
}

static duk_ret_t
js_Image_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_image(ctx, -1);
	duk_pop(ctx);
	duk_push_int(ctx, get_image_height(image));
	return 1;
}

static duk_ret_t
js_Image_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_image(ctx, -1);
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
	image = duk_require_sphere_image(ctx, -1);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen)) al_draw_bitmap(get_image_bitmap(image), x, y, 0x0);
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
	image = duk_require_sphere_image(ctx, -1);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen)) al_draw_tinted_bitmap(get_image_bitmap(image), al_map_rgba(mask.r, mask.g, mask.b, mask.alpha), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Image_createSurface(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	image = duk_require_sphere_image(ctx, -1);
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
	image = duk_require_sphere_image(ctx, -1);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen))
		al_draw_rotated_bitmap(get_image_bitmap(image),
			image->width / 2, image->height / 2, x + image->width / 2, y + image->height / 2,
			angle, 0x0);
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
	image = duk_require_sphere_image(ctx, -1);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen))
		al_draw_tinted_rotated_bitmap(get_image_bitmap(image), al_map_rgba(mask.r, mask.g, mask.b, mask.alpha),
			image->width / 2, image->height / 2, x + image->width / 2, y + image->height / 2,
			angle, 0x0);
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
	image = duk_require_sphere_image(ctx, -1);
	duk_pop(ctx);
	vertex_color = al_map_rgba(255, 255, 255, 255);
	ALLEGRO_VERTEX v[] = {
		{ x1 + 0.5, y1 + 0.5, 0, 0, 0, vertex_color },
		{ x2 + 0.5, y2 + 0.5, 0, image->width, 0, vertex_color },
		{ x4 + 0.5, y4 + 0.5, 0, 0, image->height, vertex_color },
		{ x3 + 0.5, y3 + 0.5, 0, image->width, image->height, vertex_color }
	};
	if (!screen_is_skipframe(g_screen))
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
	image = duk_require_sphere_image(ctx, -1);
	duk_pop(ctx);
	vtx_color = al_map_rgba(mask.r, mask.g, mask.b, mask.alpha);
	ALLEGRO_VERTEX v[] = {
		{ x1 + 0.5, y1 + 0.5, 0, 0, 0, vtx_color },
		{ x2 + 0.5, y2 + 0.5, 0, image->width, 0, vtx_color },
		{ x4 + 0.5, y4 + 0.5, 0, 0, image->height, vtx_color },
		{ x3 + 0.5, y3 + 0.5, 0, image->width, image->height, vtx_color }
	};
	if (!screen_is_skipframe(g_screen))
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
	image = duk_require_sphere_image(ctx, -1);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen))
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
	image = duk_require_sphere_image(ctx, -1);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen))
		al_draw_tinted_scaled_bitmap(get_image_bitmap(image), al_map_rgba(mask.r, mask.g, mask.b, mask.alpha),
			0, 0, image->width, image->height, x, y, image->width * scale, image->height * scale, 0x0);
	return 0;
}
