/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "neosphere.h"
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

static struct pose* find_pose_by_name (const spriteset_t* spriteset, const char* pose_name);

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
	spriteset_t** spriteset_ptr;

	iter_t        iter;

	console_log(1, "shutting down spriteset manager");
	console_log(2, "    objects created: %u", s_next_spriteset_id);
	console_log(2, "    cache hits: %u", s_num_cache_hits);
	if (s_load_cache != NULL) {
		iter = vector_enum(s_load_cache);
		while ((spriteset_ptr = iter_next(&iter)))
			spriteset_unref(*spriteset_ptr);
		vector_free(s_load_cache);
	}
}

spriteset_t*
spriteset_new(void)
{
	vector_t*    images;
	vector_t*    poses;
	spriteset_t* spriteset;

	if (!(spriteset = calloc(1, sizeof(spriteset_t))))
		return NULL;

	console_log(2, "constructing new spriteset #%u", s_next_spriteset_id);

	images = vector_new(sizeof(image_t*));
	poses = vector_new(sizeof(struct pose));

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

	const char* const DEFAULT_POSE_NAMES[8] =
	{
		"north", "northeast", "east", "southeast",
		"south", "southwest", "west", "northwest"
	};

	atlas_t*            atlas = NULL;
	struct rss_dir_v2   dir_v2;
	struct rss_dir_v3   dir_v3;
	char                extra_pose_name[32];
	struct rss_frame_v2 frame_v2;
	struct rss_frame_v3 frame_v3;
	file_t*             file = NULL;
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
	spriteset_t**       spriteset_ptr;
	long                v2_data_offset;

	iter_t iter;
	int i, j;

	// check load cache to see if we loaded this file once already
	if (s_load_cache != NULL) {
		iter = vector_enum(s_load_cache);
		while ((spriteset_ptr = iter_next(&iter))) {
			if (strcmp(filename, (*spriteset_ptr)->filename) == 0) {
				console_log(2, "using cached spriteset #%u for '%s'", (*spriteset_ptr)->id, filename);
				++s_num_cache_hits;
				return spriteset_clone(*spriteset_ptr);
			}
		}
	}
	else
		s_load_cache = vector_new(sizeof(spriteset_t*));

	// filename not in load cache, load the spriteset
	console_log(2, "loading spriteset #%u from '%s'", s_next_spriteset_id, filename);
	spriteset = spriteset_new();
	if (!(file = file_open(g_game, filename, "rb")))
		goto on_error;
	if (file_read(file, &rss, 1, sizeof(struct rss_header)) != 1)
		goto on_error;
	if (memcmp(rss.signature, ".rss", 4) != 0)
		goto on_error;
	spriteset->filename = strdup(filename);
	spriteset->base.x1 = rss.base_x1;
	spriteset->base.y1 = rss.base_y1;
	spriteset->base.x2 = rss.base_x2;
	spriteset->base.y2 = rss.base_y2;
	rect_normalize(&spriteset->base);
	switch (rss.version) {
	case 1: // RSSv1: very simple, 8 directions of 8 frames each
		if (!(atlas = atlas_new(rss.num_images, rss.frame_width, rss.frame_height)))
			goto on_error;
		atlas_lock(atlas, false);
		for (i = 0; i < rss.num_images; ++i) {
			image = atlas_load(atlas, file, i, rss.frame_width, rss.frame_height);
			spriteset_add_image(spriteset, image);
			image_unref(image);
		}
		atlas_unlock(atlas);
		atlas_free(atlas);
		for (i = 0; i < 8; ++i) {
			spriteset_add_pose(spriteset, DEFAULT_POSE_NAMES[i]);
			for (j = 0; j < 8; ++j)
				spriteset_add_frame(spriteset, DEFAULT_POSE_NAMES[i], j + i * 8, 8);
		}
		break;
	case 2: // RSSv2: requires 2 passes
		// pass 1 - allocate poses, calculate number of images
		// note: RSSv2 is technically possible to load in a single pass, but we'd have
		//       to give up the sprite atlas.  the problem is that we don't know how
		//       many images there are without actually reading the direction headers,
		//       so we can't allocate the atlas yet.
		v2_data_offset = file_position(file);
		num_images = 0;
		for (i = 0; i < rss.num_directions; ++i) {
			if (file_read(file, &dir_v2, 1, sizeof(struct rss_dir_v2)) != 1)
				goto on_error;
			sprintf(extra_pose_name, "extra %d", i);
			pose_name = i < 8 ? DEFAULT_POSE_NAMES[i] : extra_pose_name;
			spriteset_add_pose(spriteset, pose_name);
			for (j = 0; j < dir_v2.num_frames; ++j) {  // skip over frame and image data
				if (file_read(file, &frame_v2, 1, sizeof(struct rss_frame_v2)) != 1)
					goto on_error;
				max_width = fmax(rss.frame_width != 0 ? rss.frame_width : frame_v2.width, max_width);
				max_height = fmax(rss.frame_height != 0 ? rss.frame_height : frame_v2.height, max_height);
				skip_size = (rss.frame_width != 0 ? rss.frame_width : frame_v2.width)
					* (rss.frame_height != 0 ? rss.frame_height : frame_v2.height)
					* 4;
				file_seek(file, skip_size, WHENCE_CUR);
				++num_images;
			}
		}

		// pass 2 - load images and frame data
		if (!(atlas = atlas_new(num_images, max_width, max_height)))
			goto on_error;
		file_seek(file, v2_data_offset, WHENCE_SET);
		image_index = 0;
		atlas_lock(atlas, false);
		for (i = 0; i < rss.num_directions; ++i) {
			if (file_read(file, &dir_v2, 1, sizeof(struct rss_dir_v2)) != 1)
				goto on_error;
			pose_name = lstr_cstr(((struct pose*)vector_get(spriteset->poses, i))->name);
			for (j = 0; j < dir_v2.num_frames; ++j) {
				if (file_read(file, &frame_v2, 1, sizeof(struct rss_frame_v2)) != 1)
					goto on_error;
				image = atlas_load(atlas, file, image_index,
					rss.frame_width != 0 ? rss.frame_width : frame_v2.width,
					rss.frame_height != 0 ? rss.frame_height : frame_v2.height);
				spriteset_add_image(spriteset, image);
				spriteset_add_frame(spriteset, pose_name, image_index, frame_v2.delay);
				image_unref(image);
				++image_index;
			}
		}
		atlas_unlock(atlas);
		atlas_free(atlas);
		break;
	case 3: // RSSv3: can be done in a single pass thankfully
		if (!(atlas = atlas_new(rss.num_images, rss.frame_width, rss.frame_height)))
			goto on_error;
		atlas_lock(atlas, false);
		for (i = 0; i < rss.num_images; ++i) {
			if (!(image = atlas_load(atlas, file, i, rss.frame_width, rss.frame_height)))
				goto on_error;
			spriteset_add_image(spriteset, image);
			image_unref(image);
		}
		atlas_unlock(atlas);
		atlas_free(atlas);
		for (i = 0; i < rss.num_directions; ++i) {
			if (file_read(file, &dir_v3, 1, sizeof(struct rss_dir_v3)) != 1)
				goto on_error;
			if (!(name = read_lstring(file, true)))
				goto on_error;
			spriteset_add_pose(spriteset, lstr_cstr(name));
			for (j = 0; j < dir_v3.num_frames; ++j) {
				if (file_read(file, &frame_v3, 1, sizeof(struct rss_frame_v3)) != 1)
					goto on_error;
				spriteset_add_frame(spriteset, lstr_cstr(name), frame_v3.image_idx, frame_v3.delay);
			}
			lstr_free(name);
		}
		break;
	default: // invalid RSS version
		goto on_error;
	}
	file_close(file);

	if (s_load_cache != NULL) {
		while (vector_len(s_load_cache) >= 10) {
			spriteset_ptr = vector_get(s_load_cache, 0);
			spriteset_unref(*spriteset_ptr);
			vector_remove(s_load_cache, 0);
		}
		spriteset_ref(spriteset);
		vector_push(s_load_cache, &spriteset);
	}
	return spriteset;

on_error:
	console_log(2, "couldn't load spriteset #%u", spriteset->id);
	spriteset_unref(spriteset);
	if (file != NULL)
		file_close(file);
	if (atlas != NULL) {
		atlas_unlock(atlas);
		atlas_free(atlas);
	}
	return NULL;
}

spriteset_t*
spriteset_clone(const spriteset_t* it)
{
	spriteset_t*  dolly;
	struct frame* frame;
	struct pose*  pose;

	iter_t iter, iter2;

	console_log(2, "cloning new spriteset from source spriteset #%u",
		s_next_spriteset_id, it->id);

	dolly = spriteset_new();
	dolly->filename = strdup(it->filename);
	dolly->base = it->base;
	iter = vector_enum(it->images);
	while (iter_next(&iter))
		spriteset_add_image(dolly, *(image_t**)iter.ptr);
	iter = vector_enum(it->poses);
	while (iter_next(&iter)) {
		pose = iter.ptr;
		spriteset_add_pose(dolly, lstr_cstr(pose->name));
		iter2 = vector_enum(pose->frames);
		while (iter_next(&iter2)) {
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
spriteset_unref(spriteset_t* it)
{
	iter_t iter;
	struct pose* pose;

	if (it == NULL || --it->refcount > 0)
		return;

	console_log(3, "disposing spriteset #%u no longer in use", it->id);
	iter = vector_enum(it->images);
	while (iter_next(&iter))
		image_unref(*(image_t**)iter.ptr);
	vector_free(it->images);
	iter = vector_enum(it->poses);
	while (iter_next(&iter)) {
		pose = iter.ptr;
		vector_free(pose->frames);
		lstr_free(pose->name);
	}
	vector_free(it->poses);
	free(it->filename);
	free(it);
}

int
spriteset_frame_delay(const spriteset_t* it, const char* pose_name, int frame_index)
{
	const struct frame* frame;
	const struct pose*  pose;

	if ((pose = find_pose_by_name(it, pose_name)) == NULL)
		return 0;
	frame_index %= vector_len(pose->frames);
	frame = vector_get(pose->frames, frame_index);
	return frame->delay;
}

int
spriteset_frame_image_index(const spriteset_t* it, const char* pose_name, int frame_index)
{
	const struct frame* frame;
	const struct pose*  pose;

	if ((pose = find_pose_by_name(it, pose_name)) == NULL)
		return 0;
	frame_index %= vector_len(pose->frames);
	frame = vector_get(pose->frames, frame_index);
	return frame->image_idx;
}

int
spriteset_height(const spriteset_t* it)
{
	image_t* image;

	image = *(image_t**)vector_get(it->images, 0);
	return image_height(image);
}

image_t*
spriteset_image(const spriteset_t* it, int image_index)
{
	return *(image_t**)vector_get(it->images, image_index);
}

int
spriteset_num_frames(const spriteset_t* it, const char* pose_name)
{
	struct pose* pose;

	pose = find_pose_by_name(it, pose_name);
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
spriteset_pathname(const spriteset_t* it)
{
	return it->filename;
}

const char*
spriteset_pose_name(const spriteset_t* it, int index)
{
	struct pose* pose;

	pose = vector_get(it->poses, index);
	return lstr_cstr(pose->name);
}

int
spriteset_width(const spriteset_t* it)
{
	image_t* image;

	image = *(image_t**)vector_get(it->images, 0);
	return image_width(image);
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
	pose = find_pose_by_name(it, pose_name);
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

	if ((pose = find_pose_by_name(it, pose_name)) == NULL)
		return;
	frame_index = frame_index % vector_len(pose->frames);
	frame = vector_get(pose->frames, frame_index);
	image_index = frame->image_idx;
	base = rect_zoom(it->base, scale_x, scale_y);
	x -= (base.x1 + base.x2) / 2;
	if (!is_flipped)
		y -= (base.y1 + base.y2) / 2;
	image = *(image_t**)vector_get(it->images, image_index);
	image_w = image_width(image);
	image_h = image_height(image);
	scale_w = image_w * scale_x;
	scale_h = image_h * scale_y;
	al_draw_tinted_scaled_rotated_bitmap(image_bitmap(image), nativecolor(mask),
		(float)image_w / 2, (float)image_h / 2, x + scale_w / 2, y + scale_h / 2,
		scale_x, scale_y, theta, is_flipped ? ALLEGRO_FLIP_VERTICAL : 0x0);
}

bool
spriteset_save(const spriteset_t* it, const char* filename)
{
	file_t*             file;
	struct frame*       frame;
	struct rss_frame_v3 frame_data;
	image_t*            image;
	struct rss_header   header;
	struct pose*        pose;
	struct rss_dir_v3   pose_data;

	int i, j;

	file = file_open(g_game, filename, "wb");

	// write out the RSS header	first
	image = spriteset_image(it, 0);
	memcpy(header.signature, ".rss", 4);
	header.version = 3;
	header.frame_width = image_width(image);
	header.frame_height = image_height(image);
	header.base_x1 = it->base.x1;
	header.base_y1 = it->base.y1;
	header.base_x2 = it->base.x2;
	header.base_y2 = it->base.y2;
	header.num_images = vector_len(it->images);
	header.num_directions = vector_len(it->poses);
	if (file_write(file, &header, 1, sizeof(struct rss_header)) != 1)
		goto on_error;

	for (i = 0; i < vector_len(it->images); ++i) {
		image = *(image_t**)vector_get(it->images, i);
		fwrite_image(file, image);
	}

	for (i = 0; i < vector_len(it->poses); ++i) {
		pose = vector_get(it->poses, i);
		pose_data.num_frames = vector_len(pose->frames);
		if (file_write(file, &pose_data, 1, sizeof(struct rss_dir_v3)) != 1)
			goto on_error;
		write_lstring(file, pose->name, true);
		for (j = 0; j < vector_len(pose->frames); ++j) {
			frame = vector_get(pose->frames, j);
			frame_data.image_idx = frame->image_idx;
			frame_data.delay = frame->delay;
			if (file_write(file, &frame_data, 1, sizeof(struct rss_frame_v3)) != 1)
				goto on_error;
		}
	}

	file_close(file);
	return true;

on_error:
	file_close(file);
	return false;
}

static struct pose*
find_pose_by_name(const spriteset_t* spriteset, const char* pose_name)
{
	const char*  alt_name;
	const char*  name_to_find;
	struct pose* pose = NULL;

	iter_t iter;

	alt_name = strcasecmp(pose_name, "northeast") == 0 ? "north"
		: strcasecmp(pose_name, "southeast") == 0 ? "south"
		: strcasecmp(pose_name, "southwest") == 0 ? "south"
		: strcasecmp(pose_name, "northwest") == 0 ? "north"
		: "";
	name_to_find = pose_name;
	while (pose == NULL) {
		iter = vector_enum(spriteset->poses);
		while (iter_next(&iter)) {
			pose = iter.ptr;
			if (strcasecmp(lstr_cstr(pose->name), name_to_find) == 0)
				goto search_over;
			else
				pose = NULL;
		}
		if (name_to_find != alt_name)
			name_to_find = alt_name;
		else
			goto search_over;
	}

search_over:
	return pose != NULL ? pose : vector_get(spriteset->poses, 0);
}
