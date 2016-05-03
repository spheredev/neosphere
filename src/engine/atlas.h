#ifndef MINISPHERE__ATLAS_H__INCLUDED
#define MINISPHERE__ATLAS_H__INCLUDED

#include "image.h"

typedef struct atlas atlas_t;

atlas_t*     create_atlas     (int num_images, int max_width, int max_height);
void         free_atlas       (atlas_t* atlas);
image_t*     get_atlas_image  (const atlas_t* atlas);
float_rect_t get_atlas_uv     (const atlas_t* atlas, int image_index);
rect_t       get_atlas_xy     (const atlas_t* atlas, int image_index);
void         lock_atlas       (atlas_t* atlas);
void         unlock_atlas     (atlas_t* atlas);
image_t*     read_atlas_image (atlas_t* atlas, sfs_file_t* file, int index, int width, int height);

#endif // MINISPHERE__ATLAS_H__INCLUDED
