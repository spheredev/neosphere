#include "minisphere.h"
#include "image.h"

#include "atlas.h"

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
create_atlas(int num_images, int max_width, int max_height)
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
free_atlas(atlas_t* atlas)
{
	console_log(4, "disposing atlas #%u no longer in use", atlas->id);
	
	if (atlas->lock != NULL)
		unlock_image(atlas->image, atlas->lock);
	free_image(atlas->image);
	free(atlas);
}

void
lock_atlas(atlas_t* atlas)
{
	console_log(4, "locking atlas #%u for direct access", atlas->id);
	atlas->lock = lock_image(atlas->image);
}

void
unlock_atlas(atlas_t* atlas)
{
	console_log(4, "unlocking atlas #%u", atlas->id);
	unlock_image(atlas->image, atlas->lock);
	atlas->lock = NULL;
}

image_t*
read_atlas_image(atlas_t* atlas, sfs_file_t* file, int index, int width, int height)
{
	int off_x, off_y;
	
	if (width > atlas->max_width || height > atlas->max_height)
		return NULL;
	if (index >= atlas->pitch * atlas->pitch) return NULL;
	off_x = index % atlas->pitch * atlas->max_width;
	off_y = index / atlas->pitch * atlas->max_height;
	return read_subimage(file, atlas->image, off_x, off_y, width, height);
}
