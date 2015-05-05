#include "minisphere.h"
#include "image.h"

#include "atlas.h"

struct atlas
{
	image_t*      image;
	rect_t        size;
	int           pitch;
	int           max_width, max_height;
	image_lock_t* lock;
};

atlas_t*
create_atlas(int num_images, int max_width, int max_height)
{
	atlas_t* atlas;

	if (!(atlas = calloc(1, sizeof(atlas_t))))
		goto on_error;
	atlas->pitch = ceil(sqrt(num_images));
	atlas->max_width = max_width;
	atlas->max_height = max_height;
	atlas->size = new_rect(0, 0, atlas->pitch * atlas->max_width, atlas->pitch * atlas->max_height);
	if (!(atlas->image = create_image(atlas->size.x2, atlas->size.y2)))
		goto on_error;
	return atlas;

on_error:
	if (atlas != NULL) {
		free_image(atlas->image);
		free(atlas);
	}
	return NULL;
}

void
free_atlas(atlas_t* atlas)
{
	if (atlas->lock != NULL)
		unlock_image(atlas->image, atlas->lock);
	free_image(atlas->image);
	free(atlas);
}

void
lock_atlas(atlas_t* atlas)
{
	atlas->lock = lock_image(atlas->image);
}

void
unlock_atlas(atlas_t* atlas)
{
	unlock_image(atlas->image, atlas->lock);
	atlas->lock = NULL;
}

image_t*
read_atlas_image(atlas_t* atlas, FILE* file, int index, int width, int height)
{
	int off_x, off_y;
	
	if (width > atlas->max_width || height > atlas->max_height)
		return NULL;
	if (index >= atlas->pitch * atlas->pitch) return NULL;
	off_x = index % atlas->pitch * atlas->max_width;
	off_y = index / atlas->pitch * atlas->max_height;
	return read_subimage(file, atlas->image, off_x, off_y, width, height);
}
