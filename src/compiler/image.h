#ifndef CELL__IMAGE_H__INCLUDED
#define CELL__IMAGE_H__INCLUDED

#include "path.h"

#include <stdint.h>

typedef struct image image_t;

image_t*        image_open         (const path_t* path);
void            image_close        (image_t* image);
const uint32_t* image_get_pixelbuf (const image_t* image);
const ptrdiff_t image_get_pitch    (const image_t* image);
int             image_get_width    (const image_t* image);
int             image_get_height   (const image_t* image);

#endif // CELL__IMAGE_H__INCLUDED
