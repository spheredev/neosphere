#ifndef MINISPHERE__TILESET_H__INCLUDED
#define MINISPHERE__TILESET_H__INCLUDED

#include "atlas.h"
#include "image.h"
#include "obstruction.h"

typedef struct tileset tileset_t;

tileset_t*       tileset_new       (const char* filename);
tileset_t*       tileset_read      (file_t* file);
void             tileset_free      (tileset_t* tileset);
int              tileset_len       (const tileset_t* tileset);
const obsmap_t*  tileset_obsmap    (const tileset_t* tileset, int tile_index);
int              tileset_get_delay (const tileset_t* tileset, int tile_index);
image_t*         tileset_get_image (const tileset_t* tileset, int tile_index);
const lstring_t* tileset_get_name  (const tileset_t* tileset, int tile_index);
int              tileset_get_next  (const tileset_t* tileset, int tile_index);
void             tileset_get_size  (const tileset_t* tileset, int* out_w, int* out_h);
void             tileset_set_delay (tileset_t* tileset, int tile_index, int delay);
void             tileset_set_image (tileset_t* tileset, int tile_index, image_t* image);
void             tileset_set_next  (tileset_t* tileset, int tile_index, int next_index);
bool             tileset_set_name  (tileset_t* tileset, int tile_index, const lstring_t* name);
void             tileset_draw      (const tileset_t* tileset, color_t mask, float x, float y, int tile_index);
void             tileset_update    (tileset_t* tileset);

#endif // MINISPHERE__TILESET_H__INCLUDED
