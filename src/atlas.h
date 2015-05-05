#ifndef MINISPHERE__ATLAS_H__INCLUDED
#define MINISPHERE__ATLAS_H__INCLUDED

#include "image.h"

typedef struct atlas atlas_t;

atlas_t* create_atlas     (int num_images, int max_width, int max_height);
void     free_atlas       (atlas_t* atlas);
void     lock_atlas       (atlas_t* atlas);
void     unlock_atlas     (atlas_t* atlas);
image_t* read_atlas_image (atlas_t* atlas, FILE* file, int index, int width, int height);

#endif
