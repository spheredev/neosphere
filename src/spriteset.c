#include "minisphere.h"
#include "api.h"
#include "atlas.h"
#include "image.h"
#include "vector.h"

#include "spriteset.h"

#pragma pack(push, 1)
struct rss_header
{
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

struct rss_dir_v2
{
	int16_t num_frames;
	uint8_t reserved[62];
};

struct rss_dir_v3
{
	int16_t num_frames;
	uint8_t reserved[6];
};

struct rss_frame_v2
{
	int16_t width;
	int16_t height;
	int16_t delay;
	uint8_t reserved[26];
};

struct rss_frame_v3
{
	int16_t image_idx;
	int16_t delay;
	uint8_t reserved[4];
};
#pragma pack(pop)

static duk_ret_t js_LoadSpriteset          (duk_context* ctx);
static duk_ret_t js_new_Spriteset          (duk_context* ctx);
static duk_ret_t js_Spriteset_finalize     (duk_context* ctx);
static duk_ret_t js_Spriteset_get_filename (duk_context* ctx);
static duk_ret_t js_Spriteset_toString     (duk_context* ctx);
static duk_ret_t js_Spriteset_clone        (duk_context* ctx);
static duk_ret_t js_Spriteset_get_image    (duk_context* ctx);
static duk_ret_t js_Spriteset_set_image    (duk_context* ctx);

static const spriteset_pose_t* find_sprite_pose    (const spriteset_t* spriteset, const char* pose_name);

static vector_t*    s_load_cache;
static unsigned int s_next_spriteset_id = 0;
static unsigned int s_num_cache_hits = 0;

void
initialize_spritesets(void)
{
	console_log(1, "Initializing spriteset manager");
	s_load_cache = new_vector(sizeof(spriteset_t*));
}

void
shutdown_spritesets(void)
{
	iter_t        iter;
	spriteset_t** p_spriteset;
	
	console_log(1, "Shutting down spriteset manager");
	console_log(2, "  Objects created: %u", s_next_spriteset_id);
	console_log(2, "  Cache hits: %u", s_num_cache_hits);
	if (s_load_cache != NULL) {
		iter = iterate_vector(s_load_cache);
		while (p_spriteset = next_vector_item(&iter))
			free_spriteset(*p_spriteset);
		free_vector(s_load_cache);
	}
}

spriteset_t*
clone_spriteset(const spriteset_t* spriteset)
{
	spriteset_t* clone = NULL;
	
	int i, j;

	console_log(2, "Cloning Spriteset %u from source Spriteset %u",
		s_next_spriteset_id, spriteset->id);
	
	clone = calloc(1, sizeof(spriteset_t));
	clone->base = spriteset->base;
	clone->num_images = spriteset->num_images;
	clone->num_poses = spriteset->num_poses;
	if (!(clone->images = calloc(clone->num_images, sizeof(image_t*))))
		goto on_error;
	for (i = 0; i < spriteset->num_images; ++i)
		clone->images[i] = ref_image(spriteset->images[i]);
	if (!(clone->poses = calloc(clone->num_poses, sizeof(spriteset_pose_t))))
		goto on_error;
	for (i = 0; i < spriteset->num_poses; ++i) {
		if ((clone->poses[i].name = lstr_dup(spriteset->poses[i].name)) == NULL)
			goto on_error;
		clone->poses[i].num_frames = spriteset->poses[i].num_frames;
		if ((clone->poses[i].frames = calloc(clone->poses[i].num_frames, sizeof(spriteset_frame_t))) == NULL)
			goto on_error;
		for (j = 0; j < spriteset->poses[i].num_frames; ++j)
			clone->poses[i].frames[j] = spriteset->poses[i].frames[j];
	}
	clone->id = s_next_spriteset_id++;
	
	return ref_spriteset(clone);

on_error:
	if (clone != NULL) {
		for (i = 0; i < clone->num_images; ++i) free_image(clone->images[i]);
		if (clone->poses != NULL)
			for (i = 0; i < clone->num_poses; ++i) {
				lstr_free(clone->poses[i].name);
				free(clone->poses[i].frames);
			}
		free(clone);
	}
	return NULL;
}

spriteset_t*
load_spriteset(const char* path)
{
	// HERE BE DRAGONS!
	// the Sphere .rss spriteset format is a nightmare. there are 3 different versions
	// and RSSv2 is the worst, requiring 2 passes to load properly. as a result this
	// function ended up being way more massive than it has any right to be.
	
	const char* const def_dir_names[8] = {
		"north", "northeast", "east", "southeast",
		"south", "southwest", "west", "northwest"
	};
	
	atlas_t*            atlas = NULL;
	struct rss_dir_v2   dir_v2;
	struct rss_dir_v3   dir_v3;
	char                extra_v2_dir_name[32];
	struct rss_frame_v2 frame_v2;
	struct rss_frame_v3 frame_v3;
	sfs_file_t*         file = NULL;
	int                 image_index;
	int                 max_width = 0, max_height = 0;
	struct rss_header   rss;
	long                skip_size;
	spriteset_t*        spriteset = NULL;
	long                v2_data_offset;
	spriteset_t*        *p_spriteset;
	
	iter_t iter;
	int i, j;

	// check load cache to see if we loaded this file once already
	if (s_load_cache != NULL) {
		iter = iterate_vector(s_load_cache);
		while (p_spriteset = next_vector_item(&iter)) {
			if (strcmp(path, (*p_spriteset)->filename) == 0) {
				console_log(2, "Using cached Spriteset %u for '%s'", (*p_spriteset)->id, path);
				++s_num_cache_hits;
				return clone_spriteset(*p_spriteset);
			}
		}
	}
	else
		s_load_cache = new_vector(sizeof(spriteset_t*));
	
	// filename not in load cache, load the spriteset
	console_log(2, "Loading Spriteset %u as '%s'", s_next_spriteset_id, path);
	spriteset = calloc(1, sizeof(spriteset_t));
	if (!(file = sfs_fopen(g_fs, path, "spritesets", "rb")))
		goto on_error;
	if (sfs_fread(&rss, sizeof(struct rss_header), 1, file) != 1)
		goto on_error;
	if (memcmp(rss.signature, ".rss", 4) != 0) goto on_error;
	if (!(spriteset->filename = strdup(path))) goto on_error;
	spriteset->base.x1 = rss.base_x1;
	spriteset->base.y1 = rss.base_y1;
	spriteset->base.x2 = rss.base_x2;
	spriteset->base.y2 = rss.base_y2;
	normalize_rect(&spriteset->base);
	switch (rss.version) {
	case 1: // RSSv1, very simple
		spriteset->num_images = rss.num_images;
		spriteset->num_poses = 8;
		spriteset->poses = calloc(spriteset->num_poses, sizeof(spriteset_pose_t));
		for (i = 0; i < spriteset->num_poses; ++i)
			spriteset->poses[i].name = lstr_newf("%s", def_dir_names[i]);
		if ((spriteset->images = calloc(spriteset->num_images, sizeof(image_t*))) == NULL)
			goto on_error;
		if (!(atlas = create_atlas(spriteset->num_images, rss.frame_width, rss.frame_height)))
			goto on_error;
		lock_atlas(atlas);
		for (i = 0; i < spriteset->num_images; ++i) {
			if (!(spriteset->images[i] = read_atlas_image(atlas, file, i, rss.frame_width, rss.frame_height)))
				goto on_error;
		}
		unlock_atlas(atlas);
		free_atlas(atlas);
		for (i = 0; i < spriteset->num_poses; ++i) {
			if ((spriteset->poses[i].frames = calloc(8, sizeof(spriteset_frame_t))) == NULL)
				goto on_error;
			for (j = 0; j < 8; ++j) {
				spriteset->poses[i].frames[j].image_idx = j + i * 8;
				spriteset->poses[i].frames[j].delay = 8;
			}
		}
		break;
	case 2: // RSSv2, requires 2 passes
		spriteset->num_poses = rss.num_directions;
		if (!(spriteset->poses = calloc(spriteset->num_poses, sizeof(spriteset_pose_t))))
			goto on_error;

		// pass 1 - prepare structures, calculate number of images
		v2_data_offset = sfs_ftell(file);
		spriteset->num_images = 0;
		for (i = 0; i < rss.num_directions; ++i) {
			if (sfs_fread(&dir_v2, sizeof(struct rss_dir_v2), 1, file) != 1)
				goto on_error;
			spriteset->num_images += dir_v2.num_frames;
			sprintf(extra_v2_dir_name, "extra %i", i);
			spriteset->poses[i].name = lstr_newf("%s", i < 8 ? def_dir_names[i] : extra_v2_dir_name);
			spriteset->poses[i].num_frames = dir_v2.num_frames;
			if (!(spriteset->poses[i].frames = calloc(dir_v2.num_frames, sizeof(spriteset_frame_t))))
				goto on_error;
			for (j = 0; j < dir_v2.num_frames; ++j) {  // skip over frame and image data
				if (sfs_fread(&frame_v2, sizeof(struct rss_frame_v2), 1, file) != 1)
					goto on_error;
				max_width = fmax(rss.frame_width != 0 ? rss.frame_width : frame_v2.width, max_width);
				max_height = fmax(rss.frame_height != 0 ? rss.frame_height : frame_v2.height, max_height);
				skip_size = (rss.frame_width != 0 ? rss.frame_width : frame_v2.width)
					* (rss.frame_height != 0 ? rss.frame_height : frame_v2.height)
					* 4;
				sfs_fseek(file, skip_size, SFS_SEEK_CUR);
			}
		}
		if (!(spriteset->images = calloc(spriteset->num_images, sizeof(image_t*))))
			goto on_error;

		// pass 2 - read images and frame data
		if (!(atlas = create_atlas(spriteset->num_images, max_width, max_height)))
			goto on_error;
		sfs_fseek(file, v2_data_offset, SFS_SEEK_SET);
		image_index = 0;
		lock_atlas(atlas);
		for (i = 0; i < rss.num_directions; ++i) {
			if (sfs_fread(&dir_v2, sizeof(struct rss_dir_v2), 1, file) != 1)
				goto on_error;
			for (j = 0; j < dir_v2.num_frames; ++j) {
				if (sfs_fread(&frame_v2, sizeof(struct rss_frame_v2), 1, file) != 1)
					goto on_error;
				spriteset->images[image_index] = read_atlas_image(atlas, file, image_index,
					rss.frame_width != 0 ? rss.frame_width : frame_v2.width,
					rss.frame_height != 0 ? rss.frame_height : frame_v2.height);
				spriteset->poses[i].frames[j].image_idx = image_index;
				spriteset->poses[i].frames[j].delay = frame_v2.delay;
				++image_index;
			}
		}
		unlock_atlas(atlas);
		free_atlas(atlas);
		break;
	case 3: // RSSv3, can be done in a single pass thankfully
		spriteset->num_images = rss.num_images;
		spriteset->num_poses = rss.num_directions;
		if ((spriteset->images = calloc(spriteset->num_images, sizeof(image_t*))) == NULL)
			goto on_error;
		if ((spriteset->poses = calloc(spriteset->num_poses, sizeof(spriteset_pose_t))) == NULL)
			goto on_error;
		if (!(atlas = create_atlas(spriteset->num_images, rss.frame_width, rss.frame_height)))
			goto on_error;
		lock_atlas(atlas);
		for (i = 0; i < rss.num_images; ++i) {
			if (!(spriteset->images[i] = read_atlas_image(atlas, file, i, rss.frame_width, rss.frame_height)))
				goto on_error;
		}
		unlock_atlas(atlas);
		free_atlas(atlas);
		for (i = 0; i < rss.num_directions; ++i) {
			if (sfs_fread(&dir_v3, sizeof(struct rss_dir_v3), 1, file) != 1)
				goto on_error;
			if ((spriteset->poses[i].name = read_lstring(file, true)) == NULL) goto on_error;
			spriteset->poses[i].num_frames = dir_v3.num_frames;
			if ((spriteset->poses[i].frames = calloc(dir_v3.num_frames, sizeof(spriteset_frame_t))) == NULL)
				goto on_error;
			for (j = 0; j < spriteset->poses[i].num_frames; ++j) {
				if (sfs_fread(&frame_v3, sizeof(struct rss_frame_v3), 1, file) != 1)
					goto on_error;
				spriteset->poses[i].frames[j].image_idx = frame_v3.image_idx;
				spriteset->poses[i].frames[j].delay = frame_v3.delay;
			}
		}
		break;
	default: // invalid RSS version
		goto on_error;
	}
	sfs_fclose(file);
	
	if (s_load_cache != NULL) {
		while (get_vector_size(s_load_cache) >= 10) {
			p_spriteset = get_vector_item(s_load_cache, 0);
			free_spriteset(*p_spriteset);
			remove_vector_item(s_load_cache, 0);
		}
		ref_spriteset(spriteset);
		push_back_vector(s_load_cache, &spriteset);
	}
	spriteset->id = s_next_spriteset_id++;
	return ref_spriteset(spriteset);

on_error:
	console_log(2, "Failed to load Spriteset %u", s_next_spriteset_id);
	if (file != NULL) sfs_fclose(file);
	if (spriteset != NULL) {
		if (spriteset->poses != NULL) {
			for (i = 0; i < spriteset->num_poses; ++i) {
				lstr_free(spriteset->poses[i].name);
				free(spriteset->poses[i].frames);
			}
			free(spriteset->poses);
		}
		free(spriteset);
	}
	if (atlas != NULL) {
		unlock_atlas(atlas);
		free_atlas(atlas);
	}
	return NULL;
}

spriteset_t*
ref_spriteset(spriteset_t* spriteset)
{
	console_log(4, "Incrementing Spriteset %u refcount, new: %u",
		spriteset->id, spriteset->refcount + 1);
	
	++spriteset->refcount;
	return spriteset;
}

void
free_spriteset(spriteset_t* spriteset)
{
	int i;

	if (spriteset == NULL) return;
	
	console_log(4, "Decrementing Spriteset %u refcount, new: %u",
		spriteset->id, spriteset->refcount - 1);
	if (--spriteset->refcount == 0) {
		console_log(3, "Disposing Spriteset %u as it is no longer in use", spriteset->id);
		for (i = 0; i < spriteset->num_images; ++i)
			free_image(spriteset->images[i]);
		free(spriteset->images);
		for (i = 0; i < spriteset->num_poses; ++i) {
			free(spriteset->poses[i].frames);
			lstr_free(spriteset->poses[i].name);
		}
		free(spriteset->poses);
		free(spriteset->filename);
		free(spriteset);
	}
}

rect_t
get_sprite_base(const spriteset_t* spriteset)
{
	return spriteset->base;
}

int
get_sprite_frame_delay(const spriteset_t* spriteset, const char* pose_name, int frame_index)
{
	const spriteset_pose_t* pose;
	
	if ((pose = find_sprite_pose(spriteset, pose_name)) == NULL)
		return 0;
	frame_index %= pose->num_frames;
	return pose->frames[frame_index].delay;
}

void
get_sprite_size(const spriteset_t* spriteset, int* out_width, int* out_height)
{
	image_t* image;
	
	image = spriteset->images[0];
	if (out_width) *out_width = get_image_width(image);
	if (out_height) *out_height = get_image_height(image);
}

void
get_spriteset_info(const spriteset_t* spriteset, int* out_num_images, int* out_num_poses)
{
	if (out_num_images) *out_num_images = spriteset->num_images;
	if (out_num_poses) *out_num_poses = spriteset->num_poses;
}

image_t*
get_spriteset_image(const spriteset_t* spriteset, int image_index)
{
	return spriteset->images[image_index];
}

bool
get_spriteset_pose_info(const spriteset_t* spriteset, const char* pose_name, int* out_num_frames)
{
	const spriteset_pose_t* pose;

	if ((pose = find_sprite_pose(spriteset, pose_name)) == NULL)
		return false;
	*out_num_frames = pose->num_frames;
	return true;
}

void
set_spriteset_image(const spriteset_t* spriteset, int image_index, image_t* image)
{
	image_t* old_image;
	
	old_image = spriteset->images[image_index];
	spriteset->images[image_index] = ref_image(image);
	free_image(old_image);
}

void
draw_sprite(const spriteset_t* spriteset, color_t mask, bool is_flipped, double theta, double scale_x, double scale_y, const char* pose_name, float x, float y, int frame_index)
{
	rect_t                   base;
	image_t*                 image;
	int                      image_index;
	int                      image_w, image_h;
	const spriteset_pose_t*  pose;
	float                    scale_w, scale_h;
	
	if ((pose = find_sprite_pose(spriteset, pose_name)) == NULL)
		return;
	frame_index = frame_index % pose->num_frames;
	image_index = pose->frames[frame_index].image_idx;
	base = zoom_rect(spriteset->base, scale_x, scale_y);
	x -= (base.x1 + base.x2) / 2;
	if (!is_flipped)
		y -= (base.y1 + base.y2) / 2;
	image = spriteset->images[image_index];
	image_w = get_image_width(image);
	image_h = get_image_height(image);
	scale_w = image_w * scale_x;
	scale_h = image_h * scale_y;
	al_draw_tinted_scaled_rotated_bitmap(get_image_bitmap(image), al_map_rgba(mask.r, mask.g, mask.b, mask.alpha),
		(float)image_w / 2, (float)image_h / 2, x + scale_w / 2, y + scale_h / 2,
		scale_x, scale_y, theta, is_flipped ? ALLEGRO_FLIP_VERTICAL : 0x0);
}

static const spriteset_pose_t*
find_sprite_pose(const spriteset_t* spriteset, const char* pose_name)
{
	const char*             alt_name;
	const char*             name_to_find;
	const spriteset_pose_t* pose = NULL;

	int i;

	alt_name = strcasecmp(pose_name, "northeast") == 0 ? "north"
		: strcasecmp(pose_name, "southeast") == 0 ? "south"
		: strcasecmp(pose_name, "southwest") == 0 ? "south"
		: strcasecmp(pose_name, "northwest") == 0 ? "north"
		: "";
	name_to_find = pose_name;
	while (pose == NULL) {
		for (i = 0; i < spriteset->num_poses; ++i) {
			if (strcasecmp(name_to_find, lstr_cstr(spriteset->poses[i].name)) == 0) {
				pose = &spriteset->poses[i];
				break;
			}
		}
		if (name_to_find != alt_name)
			name_to_find = alt_name;
		else
			break;
	}
	return pose != NULL ? pose : &spriteset->poses[0];
}

void
init_spriteset_api(duk_context* ctx)
{
	register_api_function(ctx, NULL, "LoadSpriteset", js_LoadSpriteset);
	register_api_ctor(ctx, "Spriteset", js_new_Spriteset, js_Spriteset_finalize);
	register_api_prop(ctx, "Spriteset", "filename", js_Spriteset_get_filename, NULL);
	register_api_function(ctx, "Spriteset", "toString", js_Spriteset_toString);
	register_api_function(ctx, "Spriteset", "clone", js_Spriteset_clone);
}

void
duk_push_sphere_spriteset(duk_context* ctx, spriteset_t* spriteset)
{
	char prop_name[20];
	
	int i, j;

	duk_push_sphere_obj(ctx, "Spriteset", ref_spriteset(spriteset));

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
		sprintf(prop_name, "%i", i);
		duk_push_string(ctx, prop_name);
		duk_push_c_function(ctx, js_Spriteset_get_image, DUK_VARARGS);
			duk_get_prop_string(ctx, -1, "bind"); duk_dup(ctx, -2);
			duk_dup(ctx, -6);
			duk_call_method(ctx, 1);
			duk_remove(ctx, -2);
		duk_push_c_function(ctx, js_Spriteset_set_image, DUK_VARARGS);
			duk_get_prop_string(ctx, -1, "bind"); duk_dup(ctx, -2);
			duk_dup(ctx, -7);
			duk_call_method(ctx, 1);
			duk_remove(ctx, -2);
		duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER
			| DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE);
	}
	duk_put_prop_string(ctx, -2, "images");

	// Spriteset:directions
	duk_push_array(ctx);
	for (i = 0; i < spriteset->num_poses; ++i) {
		duk_push_object(ctx);
		duk_push_lstring_t(ctx, spriteset->poses[i].name);
		duk_put_prop_string(ctx, -2, "name");
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
js_LoadSpriteset(duk_context* ctx)
{
	duk_require_string(ctx, 0);
	
	js_new_Spriteset(ctx);
	return 1;
}

static duk_ret_t
js_new_Spriteset(duk_context* ctx)
{
	const char*  filename = duk_require_string(ctx, 0);

	spriteset_t* spriteset;

	if ((spriteset = load_spriteset(filename)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Spriteset(): Failed to load spriteset file '%s'", filename);
	duk_push_sphere_spriteset(ctx, spriteset);
	free_spriteset(spriteset);
	return 1;
}

static duk_ret_t
js_Spriteset_finalize(duk_context* ctx)
{
	spriteset_t* spriteset;
	
	spriteset = duk_require_sphere_obj(ctx, 0, "Spriteset");
	free_spriteset(spriteset);
	return 0;
}

static duk_ret_t
js_Spriteset_get_filename(duk_context* ctx)
{
	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "Spriteset");
	duk_pop(ctx);
	duk_push_string(ctx, spriteset->filename);
	return 1;
}

static duk_ret_t
js_Spriteset_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object spriteset]");
	return 1;
}

static duk_ret_t
js_Spriteset_clone(duk_context* ctx)
{
	spriteset_t* new_spriteset;
	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "Spriteset");
	duk_pop(ctx);
	if ((new_spriteset = clone_spriteset(spriteset)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Spriteset:clone(): Failed to create new spriteset");
	duk_push_sphere_spriteset(ctx, new_spriteset);
	free_spriteset(new_spriteset);
	return 1;
}

static duk_ret_t
js_Spriteset_get_image(duk_context* ctx)
{
	duk_uarridx_t index = duk_to_int(ctx, 0);
	
	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "Spriteset");
	duk_pop(ctx);
	duk_push_sphere_image(ctx, get_spriteset_image(spriteset, index));
	return 1;
}

static duk_ret_t
js_Spriteset_set_image(duk_context* ctx)
{
	image_t* image = duk_require_sphere_image(ctx, 0);
	duk_uarridx_t index = duk_to_int(ctx, 1);

	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "Spriteset");
	duk_pop(ctx);
	set_spriteset_image(spriteset, index, image);
	return 0;
}
