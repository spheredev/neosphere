#ifndef MINISPHERE__ATLAS_H__INCLUDED
#define MINISPHERE__ATLAS_H__INCLUDED

#include "image.h"

typedef struct atlas atlas_t;

atlas_t* atlas_new    (int num_images, int max_width, int max_height);
void     atlas_free   (atlas_t* atlas);
image_t* atlas_image  (const atlas_t* atlas);
rect_t   atlas_xy     (const atlas_t* atlas, int image_index);
image_t* atlas_load   (atlas_t* atlas, sfs_file_t* file, int index, int width, int height);
void     atlas_lock   (atlas_t* atlas);
void     atlas_unlock (atlas_t* atlas);

#endif // MINISPHERE__ATLAS_H__INCLUDED
