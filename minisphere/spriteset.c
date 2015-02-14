#include "minisphere.h"
#include "api.h"
#include "image.h"
#include "spriteset.h"

struct rss_header {
	uint8_t signature[4];
	int16_t version;
	int16_t num_images;
	int16_t frame_width;
	int16_t frame_height;
	int16_t num_directions;
	int16_t base_x1;
	int16_t base_y1;
	int16_t base_x2;
	int16_t base_y2;
	uint8_t reserved[106];
};

struct v3_direction {
	int16_t num_frames;
	uint8_t reserved[6];
};

struct v3_frame {
	int16_t image_idx;
	int16_t delay;
	uint8_t reserved[4];
};

static ALLEGRO_BITMAP* _fread_sprite_image    (ALLEGRO_FILE* file, int width, int height);
static char*           _fread_string          (ALLEGRO_FILE* file);
static void            _duk_push_spriteset    (duk_context* ctx, spriteset_t* spriteset);
static duk_ret_t       _js_LoadSpriteset      (duk_context* ctx);
static duk_ret_t       _js_Spriteset_finalize (duk_context* ctx);

void
init_spriteset_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "LoadSpriteset", _js_LoadSpriteset);
}

spriteset_t*
load_spriteset(const char* path)
{
	struct v3_direction direction;
	struct v3_frame     frame;
	ALLEGRO_FILE*       file = NULL;
	struct rss_header   rss;
	spriteset_t*        spriteset = NULL;
	int                 i, j;

	if ((spriteset = al_calloc(1, sizeof(spriteset_t))) == NULL) goto on_error;
	if ((file = al_fopen(path, "rb")) == NULL) goto on_error;
	if (al_fread(file, &rss, sizeof(struct rss_header)) != sizeof(struct rss_header))
		goto on_error;
	if (memcmp(rss.signature, ".rss", 4) != 0) goto on_error;
	spriteset->base.x1 = rss.base_x1;
	spriteset->base.y1 = rss.base_y1;
	spriteset->base.x2 = rss.base_x2;
	spriteset->base.y2 = rss.base_y2;
	switch (rss.version) {
	case 1:
		spriteset->num_images = rss.num_images;
		spriteset->num_poses = 8;
		spriteset->poses = al_calloc(spriteset->num_poses, sizeof(spriteset_pose_t));
		spriteset->poses[0].name = strdup("north");
		spriteset->poses[1].name = strdup("northeast");
		spriteset->poses[2].name = strdup("east");
		spriteset->poses[3].name = strdup("southeast");
		spriteset->poses[4].name = strdup("south");
		spriteset->poses[5].name = strdup("southwest");
		spriteset->poses[6].name = strdup("west");
		spriteset->poses[7].name = strdup("northwest");
		if ((spriteset->bitmaps = al_calloc(spriteset->num_images, sizeof(ALLEGRO_BITMAP*))) == NULL)
			goto on_error;
		for (i = 0; i < spriteset->num_images; ++i) {
			spriteset->bitmaps[i] = _fread_sprite_image(file, rss.frame_width, rss.frame_height);
			if (spriteset->bitmaps[i] == NULL) goto on_error;
		}
		for (i = 0; i < spriteset->num_poses; ++i) {
			if ((spriteset->poses[i].frames = al_calloc(8, sizeof(spriteset_frame_t))) == NULL)
				goto on_error;
			for (j = 0; j < 8; ++j) {
				spriteset->poses[i].frames[j].image_idx = j + i * 8;
				spriteset->poses[i].frames[j].delay = 8;
			}
		}
		break;
	case 2:
		goto on_error;
		break;
	case 3:
		spriteset->num_images = rss.num_images;
		spriteset->num_poses = rss.num_directions;
		if ((spriteset->bitmaps = al_calloc(spriteset->num_images, sizeof(ALLEGRO_BITMAP*))) == NULL)
			goto on_error;
		if ((spriteset->poses = al_calloc(spriteset->num_poses, sizeof(spriteset_pose_t))) == NULL)
			goto on_error;
		for (i = 0; i < rss.num_images; ++i) {
			spriteset->bitmaps[i] = _fread_sprite_image(file, rss.frame_width, rss.frame_height);
			if (spriteset->bitmaps[i] == NULL) goto on_error;
		}
		for (i = 0; i < rss.num_directions; ++i) {
			if (al_fread(file, &direction, sizeof(struct v3_direction)) != sizeof(struct v3_direction))
				goto on_error;
			if ((spriteset->poses[i].name = _fread_string(file)) == NULL) goto on_error;
			spriteset->poses[i].num_frames = direction.num_frames;
			if ((spriteset->poses[i].frames = al_calloc(direction.num_frames, sizeof(spriteset_frame_t))) == NULL)
				goto on_error;
			for (j = 0; j < spriteset->poses[i].num_frames; ++j) {
				if (al_fread(file, &frame, sizeof(struct v3_frame)) != sizeof(struct v3_frame))
					goto on_error;
				spriteset->poses[i].frames[j].image_idx = frame.image_idx;
				spriteset->poses[i].frames[j].delay = frame.delay;
			}
		}
		break;
	default:  // invalid version number
		goto on_error;
	}
	al_fclose(file);
	return spriteset;

on_error:
	if (file != NULL) al_fclose(file);
	if (spriteset != NULL) {
		if (spriteset->poses != NULL) {
			for (i = 0; i < spriteset->num_poses; ++i) {
				al_free(spriteset->poses[i].frames);
				free(spriteset->poses[i].name);
			}
			al_free(spriteset->poses);
		}
		al_free(spriteset);
	}
	return NULL;
}

void
free_spriteset(spriteset_t* spriteset)
{
	int i;
	
	for (i = 0; i < spriteset->num_images; ++i) {
		al_destroy_bitmap(spriteset->bitmaps[i]);
	}
	al_free(spriteset->bitmaps);
	for (i = 0; i < spriteset->num_poses; ++i) {
		al_free(spriteset->poses[i].frames);
		free(spriteset->poses[i].name);
	}
	al_free(spriteset->poses);
	al_free(spriteset);
}

static ALLEGRO_BITMAP*
_fread_sprite_image(ALLEGRO_FILE* file, int width, int height)
{
	ALLEGRO_BITMAP*        bitmap = NULL;
	ALLEGRO_LOCKED_REGION* lock   = NULL;
	
	if ((bitmap = al_create_bitmap(width, height)) == NULL)
		goto on_error;
	if ((lock = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY)) == NULL)
		goto on_error;
	size_t data_size = width * height * 4;
	if (al_fread(file, lock->data, data_size) != data_size)
		goto on_error;
	al_unlock_bitmap(bitmap);
	return bitmap;

on_error:
	if (lock != NULL) al_unlock_bitmap(bitmap);
	if (bitmap != NULL) al_destroy_bitmap(bitmap);
	return NULL;
}

static char*
_fread_string(ALLEGRO_FILE* file)
{
	uint16_t length;
	char*    string = NULL;

	if (al_fread(file, &length, 2) != 2) goto on_error;
	if ((string = calloc(length, sizeof(char))) == NULL) goto on_error;
	if (al_fread(file, string, length) != length) goto on_error;
	return string;

on_error:
	free(string);
	return NULL;
}

static void
_duk_push_spriteset(duk_context* ctx, spriteset_t* spriteset)
{
	int i, j;
	
	duk_push_object(ctx);
	duk_push_pointer(ctx, spriteset); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_c_function(ctx, &_js_Spriteset_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	
	// Spriteset:base
	duk_push_object(ctx);
	duk_push_int(ctx, spriteset->base.x1); duk_put_prop_string(ctx, -2, "x1");
	duk_push_int(ctx, spriteset->base.y1); duk_put_prop_string(ctx, -2, "y1");
	duk_push_int(ctx, spriteset->base.x2); duk_put_prop_string(ctx, -2, "x2");
	duk_push_int(ctx, spriteset->base.y2); duk_put_prop_string(ctx, -2, "y2");
	duk_put_prop_string(ctx, -2, "base");

	// Spriteset:images
	duk_push_array(ctx);
	for (i = 0; i < spriteset->num_images; ++i) {
		duk_push_sphere_Image(ctx, spriteset->bitmaps[i], false);
		duk_put_prop_index(ctx, -2, i);
	}
	duk_put_prop_string(ctx, -2, "images");

	// Spriteset:directions
	duk_push_array(ctx);
	for (i = 0; i < spriteset->num_poses; ++i) {
		duk_push_object(ctx);
		duk_push_string(ctx, spriteset->poses[i].name); duk_put_prop_string(ctx, -2, "name");
		duk_push_array(ctx);
		for (j = 0; j < spriteset->poses[i].num_frames; ++j) {
			duk_push_object(ctx);
			duk_push_int(ctx, spriteset->poses[i].frames[j].image_idx); duk_put_prop_string(ctx, -2, "index");
			duk_push_int(ctx, spriteset->poses[i].frames[j].delay); duk_put_prop_string(ctx, -2, "delay");
			duk_put_prop_index(ctx, -2, j);
		}
		duk_put_prop_string(ctx, -2, "frames");
		duk_put_prop_index(ctx, -2, i);
	}
	duk_put_prop_string(ctx, -2, "directions");
}

static duk_ret_t
_js_LoadSpriteset(duk_context* ctx)
{
	const char*  filename  = NULL;
	char*        path      = NULL;
	spriteset_t* spriteset = NULL;

	filename = duk_require_string(ctx, 0);
	path = get_asset_path(filename, "spritesets", false);
	if ((spriteset = load_spriteset(path)) == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "LoadSpriteset(): Unable to load spriteset file '%s'", filename);
	free(path);
	_duk_push_spriteset(ctx, spriteset);
	return 1;
}

static duk_ret_t
_js_Spriteset_finalize(duk_context* ctx)
{
	spriteset_t* spriteset;
	
	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); spriteset = duk_get_pointer(ctx, -1); duk_pop(ctx);
	free_spriteset(spriteset);
	return 0;
}
