#include "minisphere.h"
#include "spriteset.h"

#include "atlas.h"
#include "image.h"
#include "vector.h"

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

struct frame
{
	int image_idx;
	int delay;
};

struct pose
{
	lstring_t* name;
	vector_t*  frames;
};

struct spriteset
{
	unsigned int refcount;
	unsigned int id;
	rect_t       base;
	char*        filename;
	vector_t*    images;
	vector_t*    poses;
};

static struct pose* find_sprite_pose (const spriteset_t* spriteset, const char* pose_name);

static vector_t*    s_load_cache;
static unsigned int s_next_spriteset_id = 0;
static unsigned int s_num_cache_hits = 0;

void
spritesets_init(void)
{
	console_log(1, "initializing spriteset manager");
	s_load_cache = vector_new(sizeof(spriteset_t*));
}

void
spritesets_uninit(void)
{
	iter_t        iter;
	spriteset_t** p_spriteset;
	
	console_log(1, "shutting down spriteset manager");
	console_log(2, "    objects created: %u", s_next_spriteset_id);
	console_log(2, "    cache hits: %u", s_num_cache_hits);
	if (s_load_cache != NULL) {
		iter = vector_enum(s_load_cache);
		while (p_spriteset = vector_next(&iter))
			spriteset_free(*p_spriteset);
		vector_free(s_load_cache);
	}
}

spriteset_t*
spriteset_new(void)
{
	vector_t*    images;
	vector_t*    poses;
	spriteset_t* spriteset;

	console_log(2, "constructing new spriteset #%u", s_next_spriteset_id);
	
	images = vector_new(sizeof(image_t*));
	poses = vector_new(sizeof(struct pose));
	
	spriteset = calloc(1, sizeof(spriteset_t));
	spriteset->id = s_next_spriteset_id++;
	spriteset->images = images;
	spriteset->poses = poses;
	return spriteset_ref(spriteset);
}

spriteset_t*
spriteset_load(const char* filename)
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
	char                extra_pose_name[32];
	struct rss_frame_v2 frame_v2;
	struct rss_frame_v3 frame_v3;
	sfs_file_t*         file = NULL;
	image_t*            image;
	int                 image_index;
	int                 max_height = 0;
	int                 max_width = 0;
	lstring_t*          name;
	int                 num_images;
	const char*         pose_name;
	struct rss_header   rss;
	long                skip_size;
	spriteset_t*        spriteset = NULL;
	long                v2_data_offset;
	spriteset_t*        *p_spriteset;

	iter_t iter;
	int i, j;

	// check load cache to see if we loaded this file once already
	if (s_load_cache != NULL) {
		iter = vector_enum(s_load_cache);
		while (p_spriteset = vector_next(&iter)) {
			if (strcmp(filename, (*p_spriteset)->filename) == 0) {
				console_log(2, "using cached spriteset #%u for `%s`", (*p_spriteset)->id, filename);
				++s_num_cache_hits;
				return spriteset_clone(*p_spriteset);
			}
		}
	}
	else
		s_load_cache = vector_new(sizeof(spriteset_t*));

	// filename not in load cache, load the spriteset
	console_log(2, "loading spriteset #%u as `%s`", s_next_spriteset_id, filename);
	spriteset = spriteset_new();
	if (!(file = sfs_fopen(g_fs, filename, NULL, "rb")))
		goto on_error;
	if (sfs_fread(&rss, sizeof(struct rss_header), 1, file) != 1)
		goto on_error;
	if (memcmp(rss.signature, ".rss", 4) != 0)
		goto on_error;
	spriteset->filename = strdup(filename);
	spriteset->base.x1 = rss.base_x1;
	spriteset->base.y1 = rss.base_y1;
	spriteset->base.x2 = rss.base_x2;
	spriteset->base.y2 = rss.base_y2;
	normalize_rect(&spriteset->base);
	switch (rss.version) {
	case 1: // RSSv1: very simple, 
		if (!(atlas = atlas_new(rss.num_images, rss.frame_width, rss.frame_height)))
			goto on_error;
		atlas_lock(atlas);
		for (i = 0; i < rss.num_images; ++i) {
			image = atlas_load(atlas, file, i, rss.frame_width, rss.frame_height);
			spriteset_add_image(spriteset, image);
			image_free(image);
		}
		atlas_unlock(atlas);
		atlas_free(atlas);
		for (i = 0; i < 8; ++i) {
			spriteset_add_pose(spriteset, def_dir_names[i]);
			for (j = 0; j < 8; ++j)
				spriteset_add_frame(spriteset, def_dir_names[i], j + i * 8, 8);
		}
		break;
	case 2: // RSSv2, requires 2 passes
		// pass 1 - prepare structures, calculate number of images
		v2_data_offset = sfs_ftell(file);
		num_images = 0;
		for (i = 0; i < rss.num_directions; ++i) {
			if (sfs_fread(&dir_v2, sizeof(struct rss_dir_v2), 1, file) != 1)
				goto on_error;
			sprintf(extra_pose_name, "extra %d", i);
			pose_name = i < 8 ? def_dir_names[i] : extra_pose_name;
			spriteset_add_pose(spriteset, pose_name);
			for (j = 0; j < dir_v2.num_frames; ++j) {  // skip over frame and image data
				if (sfs_fread(&frame_v2, sizeof(struct rss_frame_v2), 1, file) != 1)
					goto on_error;
				max_width = fmax(rss.frame_width != 0 ? rss.frame_width : frame_v2.width, max_width);
				max_height = fmax(rss.frame_height != 0 ? rss.frame_height : frame_v2.height, max_height);
				skip_size = (rss.frame_width != 0 ? rss.frame_width : frame_v2.width)
					* (rss.frame_height != 0 ? rss.frame_height : frame_v2.height)
					* 4;
				sfs_fseek(file, skip_size, SFS_SEEK_CUR);
				++num_images;
			}
		}

		// pass 2 - read images and frame data
		if (!(atlas = atlas_new(num_images, max_width, max_height)))
			goto on_error;
		sfs_fseek(file, v2_data_offset, SFS_SEEK_SET);
		image_index = 0;
		atlas_lock(atlas);
		for (i = 0; i < rss.num_directions; ++i) {
			if (sfs_fread(&dir_v2, sizeof(struct rss_dir_v2), 1, file) != 1)
				goto on_error;
			pose_name = lstr_cstr(((struct pose*)vector_get(spriteset->poses, i))->name);
			for (j = 0; j < dir_v2.num_frames; ++j) {
				if (sfs_fread(&frame_v2, sizeof(struct rss_frame_v2), 1, file) != 1)
					goto on_error;
				image = atlas_load(atlas, file, image_index,
					rss.frame_width != 0 ? rss.frame_width : frame_v2.width,
					rss.frame_height != 0 ? rss.frame_height : frame_v2.height);
				spriteset_add_image(spriteset, image);
				spriteset_add_frame(spriteset, pose_name, image_index, frame_v2.delay);
				image_free(image);
				++image_index;
			}
		}
		atlas_unlock(atlas);
		atlas_free(atlas);
		break;
	case 3: // RSSv3, can be done in a single pass thankfully
		if (!(atlas = atlas_new(rss.num_images, rss.frame_width, rss.frame_height)))
			goto on_error;
		atlas_lock(atlas);
		for (i = 0; i < rss.num_images; ++i) {
			if (!(image = atlas_load(atlas, file, i, rss.frame_width, rss.frame_height)))
				goto on_error;
			spriteset_add_image(spriteset, image);
			image_free(image);
		}
		atlas_unlock(atlas);
		atlas_free(atlas);
		for (i = 0; i < rss.num_directions; ++i) {
			if (sfs_fread(&dir_v3, sizeof(struct rss_dir_v3), 1, file) != 1)
				goto on_error;
			if (!(name = read_lstring(file, true)))
				goto on_error;
			spriteset_add_pose(spriteset, lstr_cstr(name));
			for (j = 0; j < dir_v3.num_frames; ++j) {
				if (sfs_fread(&frame_v3, sizeof(struct rss_frame_v3), 1, file) != 1)
					goto on_error;
				spriteset_add_frame(spriteset, lstr_cstr(name), frame_v3.image_idx, frame_v3.delay);
			}
			lstr_free(name);
		}
		break;
	default: // invalid RSS version
		goto on_error;
	}
	sfs_fclose(file);

	if (s_load_cache != NULL) {
		while (vector_len(s_load_cache) >= 10) {
			p_spriteset = vector_get(s_load_cache, 0);
			spriteset_free(*p_spriteset);
			vector_remove(s_load_cache, 0);
		}
		spriteset_ref(spriteset);
		vector_push(s_load_cache, &spriteset);
	}
	return spriteset;

on_error:
	console_log(2, "couldn't load spriteset #%u", spriteset->id);
	spriteset_free(spriteset);
	if (file != NULL)
		sfs_fclose(file);
	if (atlas != NULL) {
		atlas_unlock(atlas);
		atlas_free(atlas);
	}
	return NULL;
}

spriteset_t*
spriteset_clone(const spriteset_t* spriteset)
{
	spriteset_t*  dolly;
	struct frame* frame;
	struct pose*  pose;
	
	iter_t iter, iter2;

	console_log(2, "cloning new spriteset from source spriteset #%u",
		s_next_spriteset_id, spriteset->id);
	
	dolly = spriteset_new();
	dolly->filename = strdup(spriteset->filename);
	dolly->base = spriteset->base;
	iter = vector_enum(spriteset->images);
	while (vector_next(&iter))
		spriteset_add_image(dolly, *(image_t**)iter.ptr);
	iter = vector_enum(spriteset->poses);
	while (vector_next(&iter)) {
		pose = iter.ptr;
		spriteset_add_pose(dolly, lstr_cstr(pose->name));
		iter2 = vector_enum(pose->frames);
		while (vector_next(&iter2)) {
			frame = iter2.ptr;
			spriteset_add_frame(dolly, lstr_cstr(pose->name), frame->image_idx, frame->delay);
		}
	}
	return dolly;
}

spriteset_t*
spriteset_ref(spriteset_t* it)
{
	++it->refcount;
	return it;
}

void
spriteset_free(spriteset_t* it)
{
	iter_t iter;
	struct pose* pose;

	if (it == NULL || --it->refcount > 0)
		return;
	
	console_log(3, "disposing spriteset #%u no longer in use", it->id);
	iter = vector_enum(it->images);
	while (vector_next(&iter))
		image_free(*(image_t**)iter.ptr);
	vector_free(it->images);
	iter = vector_enum(it->poses);
	while (vector_next(&iter)) {
		pose = iter.ptr;
		vector_free(pose->frames);
		lstr_free(pose->name);
	}
	vector_free(it->poses);
	free(it->filename);
	free(it);
}

int
spriteset_frame_delay(const spriteset_t* spriteset, const char* pose_name, int frame_index)
{
	const struct frame* frame;
	const struct pose*  pose;

	if ((pose = find_sprite_pose(spriteset, pose_name)) == NULL)
		return 0;
	frame_index %= vector_len(pose->frames);
	frame = vector_get(pose->frames, frame_index);
	return frame->delay;
}

int
spriteset_frame_image_index(const spriteset_t* spriteset, const char* pose_name, int frame_index)
{
	const struct frame* frame;
	const struct pose*  pose;

	if ((pose = find_sprite_pose(spriteset, pose_name)) == NULL)
		return 0;
	frame_index %= vector_len(pose->frames);
	frame = vector_get(pose->frames, frame_index);
	return frame->image_idx;
}

image_t*
spriteset_image(const spriteset_t* spriteset, int image_index)
{
	return *(image_t**)vector_get(spriteset->images, image_index);
}

int
spriteset_num_frames(const spriteset_t* it, const char* pose_name)
{
	struct pose* pose;

	pose = find_sprite_pose(it, pose_name);
	return vector_len(pose->frames);
}

int
spriteset_num_images(const spriteset_t* it)
{
	return vector_len(it->images);
}

int
spriteset_num_poses(const spriteset_t* it)
{
	return vector_len(it->poses);
}

const char*
spriteset_path(const spriteset_t* spriteset)
{
	return spriteset->filename;
}

const char*
spriteset_pose_name(const spriteset_t* it, int index)
{
	struct pose* pose;

	pose = vector_get(it->poses, index);
	return lstr_cstr(pose->name);
}

rect_t
spriteset_get_base(const spriteset_t* it)
{
	return it->base;
}

void
spriteset_set_base(spriteset_t* it, rect_t new_base)
{
	it->base = new_base;
}

void
spriteset_add_frame(spriteset_t* it, const char* pose_name, int image_idx, int delay)
{
	struct pose* pose;
	struct frame frame;

	frame.image_idx = image_idx;
	frame.delay = delay;
	pose = find_sprite_pose(it, pose_name);
	vector_push(pose->frames, &frame);
}

void
spriteset_add_image(spriteset_t* it, image_t* image)
{
	image_ref(image);
	vector_push(it->images, &image);
}

void
spriteset_add_pose(spriteset_t* it, const char* name)
{
	struct pose pose;

	pose.name = lstr_new(name);
	pose.frames = vector_new(sizeof(struct frame));
	vector_push(it->poses, &pose);
}

void
spriteset_draw(const spriteset_t* it, color_t mask, bool is_flipped, double theta, double scale_x, double scale_y, const char* pose_name, float x, float y, int frame_index)
{
	rect_t             base;
	struct frame*      frame;
	image_t*           image;
	int                image_index;
	int                image_w, image_h;
	const struct pose* pose;
	float              scale_w, scale_h;

	if ((pose = find_sprite_pose(it, pose_name)) == NULL)
		return;
	frame_index = frame_index % vector_len(pose->frames);
	frame = vector_get(pose->frames, frame_index);
	image_index = frame->image_idx;
	base = zoom_rect(it->base, scale_x, scale_y);
	x -= (base.x1 + base.x2) / 2;
	if (!is_flipped)
		y -= (base.y1 + base.y2) / 2;
	image = *(image_t**)vector_get(it->images, image_index);
	image_w = image_width(image);
	image_h = image_height(image);
	scale_w = image_w * scale_x;
	scale_h = image_h * scale_y;
	if (x + scale_w <= 0 || x >= g_res_x || y + scale_h <= 0 || y >= g_res_y)
		return;
	al_draw_tinted_scaled_rotated_bitmap(image_bitmap(image), al_map_rgba(mask.r, mask.g, mask.b, mask.a),
		(float)image_w / 2, (float)image_h / 2, x + scale_w / 2, y + scale_h / 2,
		scale_x, scale_y, theta, is_flipped ? ALLEGRO_FLIP_VERTICAL : 0x0);
}

void
get_sprite_size(const spriteset_t* spriteset, int* out_width, int* out_height)
{
	image_t* image;
	
	image = *(image_t**)vector_get(spriteset->images, 0);
	if (out_width)
		*out_width = image_width(image);
	if (out_height)
		*out_height = image_height(image);
}

static struct pose*
find_sprite_pose(const spriteset_t* spriteset, const char* pose_name)
{
	const char*        alt_name;
	const char*        name_to_find;
	const struct pose* pose = NULL;

	iter_t iter;

	alt_name = strcasecmp(pose_name, "northeast") == 0 ? "north"
		: strcasecmp(pose_name, "southeast") == 0 ? "south"
		: strcasecmp(pose_name, "southwest") == 0 ? "south"
		: strcasecmp(pose_name, "northwest") == 0 ? "north"
		: "";
	name_to_find = pose_name;
	while (pose == NULL) {
		iter = vector_enum(spriteset->poses);
		while (vector_next(&iter)) {
			pose = iter.ptr;
			if (strcasecmp(lstr_cstr(pose->name), name_to_find) == 0)
				goto search_over;
		}
		if (name_to_find != alt_name)
			name_to_find = alt_name;
		else
			goto search_over;
	}
	
search_over:
	return pose != NULL ? pose : vector_get(spriteset->poses, 0);
}
