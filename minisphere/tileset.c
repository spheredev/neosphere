#include "minisphere.h"

#include "tileset.h"

struct tileset
{
	int         width, height;
	int         num_tiles;
	struct tile *tiles;
};

struct tile
{
	ALLEGRO_BITMAP* bitmap;
	int             delay;
	struct tile*    next_tile;
};

#pragma pack(push, 1)
struct rts_header
{
	uint8_t  signature[4];
	uint16_t version;
	uint16_t num_tiles;
	uint16_t tile_width;
	uint16_t tile_height;
	uint16_t tile_bpp;
	uint8_t  compression;
	uint8_t  has_obstructions;
	uint8_t  reserved[240];
};

struct rts_tile_info
{
	uint8_t  unknown_1;
	uint8_t  animated;
	int16_t  next_tile;
	int16_t  delay;
	uint8_t  unknown_2;
	uint8_t  blocked;
	uint16_t num_segments;
	uint16_t name_length;
	uint8_t terraformed;
	uint8_t reserved[19];
};
#pragma pack(pop)

tileset_t*
load_tileset(const char* path)
{
	ALLEGRO_FILE* file;
	tileset_t*    tileset;

	if ((file = al_fopen(path, "rb")) == NULL) return NULL;
	tileset = load_tileset_f(file);
	al_fclose(file);
	return tileset;
}

tileset_t*
load_tileset_f(ALLEGRO_FILE* file)
{
	int64_t              file_pos;
	struct rts_header    rts;
	struct rts_tile_info tile_info;
	struct tile*         tiles = NULL;
	tileset_t*           tileset;

	int i;

	if (file == NULL) goto on_error;
	file_pos = al_ftell(file);
	if ((tileset = calloc(1, sizeof(tileset_t))) == NULL) goto on_error;
	if (al_fread(file, &rts, sizeof(struct rts_header)) != sizeof(struct rts_header))
		goto on_error;
	if (memcmp(rts.signature, ".rts", 4) != 0 || rts.version < 1 || rts.version > 1)
		goto on_error;
	if (rts.tile_bpp != 32) goto on_error;
	if ((tiles = calloc(rts.num_tiles, sizeof(struct tile))) == NULL) goto on_error;
	for (i = 0; i < rts.num_tiles; ++i) {
		if ((tiles[i].bitmap = al_fread_bitmap(file, rts.tile_width, rts.tile_height)) == NULL)
			goto on_error;
	}
	for (i = 0; i < rts.num_tiles; ++i) {
		if (al_fread(file, &tile_info, sizeof(struct rts_tile_info)) != sizeof(struct rts_tile_info))
			goto on_error;
		tiles[i].delay = tile_info.delay;
		tiles[i].next_tile = tile_info.animated ? &tiles[tile_info.next_tile] : NULL;
	}
	tileset->width = rts.tile_width;
	tileset->height = rts.tile_height;
	tileset->num_tiles = rts.num_tiles;
	tileset->tiles = tiles;
	return tileset;

on_error:
	if (file != NULL) al_fseek(file, file_pos, ALLEGRO_SEEK_SET);
	if (tiles != NULL) {
		for (i = 0; i < rts.num_tiles; ++i) al_destroy_bitmap(tiles[i].bitmap);
		free(tileset->tiles);
	}
	free(tileset);
	return NULL;
}

void
free_tileset(tileset_t* tileset)
{
	int i;

	for (i = 0; i < tileset->num_tiles; ++i) {
		al_destroy_bitmap(tileset->tiles[i].bitmap);
	}
	free(tileset->tiles);
	free(tileset);
}

int
get_tile_count(const tileset_t* tileset)
{
	return tileset->num_tiles;
}

void
get_tile_size(const tileset_t* tileset, int* out_w, int* out_h)
{
	*out_w = tileset->width;
	*out_h = tileset->height;
}

void
draw_tile(const tileset_t* tileset, float x, float y, int tile_index)
{
	al_draw_bitmap(tileset->tiles[tile_index].bitmap, x, y, 0x0);
}
