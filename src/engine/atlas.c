#include "minisphere.h"
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

	console_log(4, "creating atlas #%u at %ix%i per image", s_next_atlas_id,
		max_width, max_height);
	
	atlas = calloc(1, sizeof(atlas_t));
	atlas->pitch = ceil(sqrt(num_images));
	atlas->max_width = max_width;
	atlas->max_height = max_height;
	atlas->size = new_rect(0, 0, atlas->pitch * atlas->max_width, atlas->pitch * atlas->max_height);
	if (!(atlas->image = create_image(atlas->size.x2, atlas->size.y2)))
		goto on_error;
	
	atlas->id = s_next_atlas_id++;
	return atlas;

on_error:
	console_log(4, "failed to create atlas #%u", s_next_atlas_id++);
	if (atlas != NULL) {
		free_image(atlas->image);
		free(atlas);
	}
	return NULL;
}

void
atlas_free(atlas_t* atlas)
{
	console_log(4, "disposing atlas #%u no longer in use", atlas->id);
	
	if (atlas->lock != NULL)
		unlock_image(atlas->image, atlas->lock);
	free_image(atlas->image);
	free(atlas);
}

image_t*
atlas_image(const atlas_t* atlas)
{
	return atlas->image;
}

float_rect_t
atlas_uv(const atlas_t* atlas, int image_index)
{
	float        atlas_height;
	float        atlas_width;
	float_rect_t uv;
	
	atlas_width = get_image_width(atlas->image);
	atlas_height = get_image_height(atlas->image);
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

	atlas_width = get_image_width(atlas->image);
	atlas_height = get_image_height(atlas->image);
	xy.x1 = (image_index % atlas->pitch) * atlas->max_width;
	xy.y1 = (image_index / atlas->pitch) * atlas->max_height;
	xy.x2 = xy.x1 + atlas->max_width;
	xy.y2 = xy.y1 + atlas->max_height;
	return xy;
}

void
atlas_lock(atlas_t* atlas)
{
	console_log(4, "locking atlas #%u for direct access", atlas->id);
	atlas->lock = lock_image(atlas->image);
}

void
atlas_unlock(atlas_t* atlas)
{
	console_log(4, "unlocking atlas #%u", atlas->id);
	unlock_image(atlas->image, atlas->lock);
	atlas->lock = NULL;
}

image_t*
atlas_load(atlas_t* atlas, sfs_file_t* file, int index, int width, int height)
{
	int off_x, off_y;
	
	if (width > atlas->max_width || height > atlas->max_height)
		return NULL;
	if (index >= atlas->pitch * atlas->pitch) return NULL;
	off_x = index % atlas->pitch * atlas->max_width;
	off_y = index / atlas->pitch * atlas->max_height;
	return read_subimage(file, atlas->image, off_x, off_y, width, height);
}
