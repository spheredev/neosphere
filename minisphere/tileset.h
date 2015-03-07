typedef struct tileset tileset_t;

tileset_t*      load_tileset    (const char* path);
tileset_t*      load_tileset_f  (ALLEGRO_FILE* file);
void            free_tileset    (tileset_t* tileset);
ALLEGRO_BITMAP* get_tile_bitmap (const tileset_t* tileset, int tile_index);
int             get_tile_count  (const tileset_t* tileset);
const obsmap_t* get_tile_obsmap (const tileset_t* tileset, int tile_index);
void            get_tile_size   (const tileset_t* tileset, int* out_w, int* out_h);
void            draw_tile       (const tileset_t* tileset, float x, float y, int tile_index);
