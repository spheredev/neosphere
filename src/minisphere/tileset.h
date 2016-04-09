#ifndef MINISPHERE__TILESET_H__INCLUDED
#define MINISPHERE__TILESET_H__INCLUDED

#include "atlas.h"
#include "image.h"

typedef struct tileset tileset_t;

tileset_t*       load_tileset      (const char* filename);
tileset_t*       read_tileset      (sfs_file_t* file);
void             free_tileset      (tileset_t* tileset);
int              get_next_tile     (const tileset_t* tileset, int tile_index);
image_t*         get_tile_atlas    (const tileset_t* tileset);
float_rect_t     get_tile_atlas_uv (const tileset_t* tileset, int tile_index);
int              get_tile_count    (const tileset_t* tileset);
int              get_tile_delay    (const tileset_t* tileset, int tile_index);
image_t*         get_tile_image    (const tileset_t* tileset, int tile_index);
const lstring_t* get_tile_name     (const tileset_t* tileset, int tile_index);
const obsmap_t*  get_tile_obsmap   (const tileset_t* tileset, int tile_index);
void             get_tile_size     (const tileset_t* tileset, int* out_w, int* out_h);
void             set_next_tile     (tileset_t* tileset, int tile_index, int next_index);
void             set_tile_delay    (tileset_t* tileset, int tile_index, int delay);
void             set_tile_image    (tileset_t* tileset, int tile_index, image_t* image);
bool             set_tile_name     (tileset_t* tileset, int tile_index, const lstring_t* name);
void             animate_tileset   (tileset_t* tileset);
void             draw_tile         (const tileset_t* tileset, color_t mask, float x, float y, int tile_index);

#endif // MINISPHERE__TILESET_H__INCLUDED
