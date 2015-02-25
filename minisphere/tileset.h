typedef struct tileset tileset_t;

tileset_t* load_tileset  (const char* path);
void       free_tileset  (tileset_t* tileset);
void       get_tile_size (const tileset_t* tileset, int* out_w, int* out_h);
void       draw_tile     (const tileset_t* tileset, float x, float y, int tile_index);
