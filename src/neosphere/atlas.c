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
#include "atlas.h"

#include "image.h"

struct atlas
{
	unsigned int  id;
	image_t*      image;
	rect_t        size;
	int           pitch;
	int           max_width, max_height;
	image_lock_t* lock;
};

static unsigned int s_next_atlas_id = 0;

atlas_t*
atlas_new(int num_images, int max_width, int max_height)
{
	atlas_t* atlas;

	console_log(4, "creating atlas #%u at %dx%d per image", s_next_atlas_id,
		max_width, max_height);

	if (!(atlas = calloc(1, sizeof(atlas_t))))
		goto on_error;
	atlas->pitch = ceil(sqrt(num_images));
	atlas->max_width = max_width;
	atlas->max_height = max_height;
	atlas->size = mk_rect(0, 0, atlas->pitch * atlas->max_width, atlas->pitch * atlas->max_height);
	if (!(atlas->image = image_new(atlas->size.x2, atlas->size.y2, NULL)))
		goto on_error;

	atlas->id = s_next_atlas_id++;
	return atlas;

on_error:
	console_log(4, "failed to create atlas #%u", s_next_atlas_id++);
	if (atlas != NULL) {
		image_unref(atlas->image);
		free(atlas);
	}
	return NULL;
}

void
atlas_free(atlas_t* atlas)
{
	console_log(4, "disposing atlas #%u no longer in use", atlas->id);

	if (atlas->lock != NULL)
		image_unlock(atlas->image, atlas->lock);
	image_unref(atlas->image);
	free(atlas);
}

image_t*
atlas_image(const atlas_t* atlas)
{
	return atlas->image;
}

rectf_t
atlas_uv(const atlas_t* atlas, int image_index)
{
	float   atlas_height;
	float   atlas_width;
	rectf_t uv;

	atlas_width = image_width(atlas->image);
	atlas_height = image_height(atlas->image);
	uv.x1 = (image_index % atlas->pitch) * atlas->max_width;
	uv.y1 = (image_index / atlas->pitch) * atlas->max_height;
	uv.x2 = uv.x1 + atlas->max_width;
	uv.y2 = uv.y1 + atlas->max_height;
	return uv;
}

rect_t
atlas_xy(const atlas_t* atlas, int image_index)
{
	float  atlas_height;
	float  atlas_width;
	rect_t xy;

	atlas_width = image_width(atlas->image);
	atlas_height = image_height(atlas->image);
	xy.x1 = (image_index % atlas->pitch) * atlas->max_width;
	xy.y1 = (image_index / atlas->pitch) * atlas->max_height;
	xy.x2 = xy.x1 + atlas->max_width;
	xy.y2 = xy.y1 + atlas->max_height;
	return xy;
}

void
atlas_lock(atlas_t* atlas, bool keep_contents)
{
	console_log(4, "locking atlas #%u for direct access", atlas->id);
	atlas->lock = image_lock(atlas->image, true, keep_contents);
}

void
atlas_unlock(atlas_t* atlas)
{
	console_log(4, "unlocking atlas #%u", atlas->id);
	image_unlock(atlas->image, atlas->lock);
	atlas->lock = NULL;
}

image_t*
atlas_load(atlas_t* atlas, file_t* file, int index, int width, int height)
{
	int off_x, off_y;

	if (width > atlas->max_width || height > atlas->max_height)
		return NULL;
	if (index >= atlas->pitch * atlas->pitch) return NULL;
	off_x = index % atlas->pitch * atlas->max_width;
	off_y = index / atlas->pitch * atlas->max_height;
	return fread_image_slice(file, atlas->image, off_x, off_y, width, height);
}
