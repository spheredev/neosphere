#include "image.h"

typedef struct tileset tileset_t;

tileset_t*       load_tileset    (const char* path);
tileset_t*       read_tileset    (ALLEGRO_FILE* file);
void             free_tileset    (tileset_t* tileset);
int              get_tile_count  (const tileset_t* tileset);
image_t*         get_tile_image  (const tileset_t* tileset, int tile_index);
const lstring_t* get_tile_name   (const tileset_t* tileset, int tile_index);
const obsmap_t*  get_tile_obsmap (const tileset_t* tileset, int tile_index);
void             get_tile_size   (const tileset_t* tileset, int* out_w, int* out_h);
void             set_tile_image  (tileset_t* tileset, int tile_index, image_t* image);
void             draw_tile       (const tileset_t* tileset, ALLEGRO_COLOR mask, float x, float y, int tile_index);
