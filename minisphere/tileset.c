#include "minisphere.h"
#include "image.h"
#include "obsmap.h"

#include "tileset.h"

struct tileset
{
	int         width, height;
	int         num_tiles;
	struct tile *tiles;
};

struct tile
{
	lstring_t* name;
	int        animate_index;
	int        frames_left;
	image_t*   image;
	int        delay;
	int        next_index;
	int        num_obs_lines;
	obsmap_t*  obsmap;
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

struct rts_tile_header
{
	uint8_t  unknown_1;
	uint8_t  animated;
	int16_t  next_tile;
	int16_t  delay;
	uint8_t  unknown_2;
	uint8_t  obsmap_type;
	uint16_t num_segments;
	uint16_t name_length;
	uint8_t  terraformed;
	uint8_t  reserved[19];
};
#pragma pack(pop)

tileset_t*
load_tileset(const char* path)
{
	FILE*      file;
	tileset_t* tileset;

	if ((file = fopen(path, "rb")) == NULL) return NULL;
	tileset = read_tileset(file);
	fclose(file);
	return tileset;
}

tileset_t*
read_tileset(FILE* file)
{
	image_t*               atlas = NULL;
	int                    atlas_w, atlas_h;
	long                   file_pos;
	int                    n_tiles_per_row;
	struct rts_header      rts;
	rect_t                 segment;
	struct rts_tile_header tilehdr;
	struct tile*           tiles = NULL;
	tileset_t*             tileset = NULL;

	int i, j;

	memset(&rts, 0, sizeof(struct rts_header));
	
	if (file == NULL) goto on_error;
	file_pos = ftell(file);
	if ((tileset = calloc(1, sizeof(tileset_t))) == NULL) goto on_error;
	if (fread(&rts, sizeof(struct rts_header), 1, file) != 1)
		goto on_error;
	if (memcmp(rts.signature, ".rts", 4) != 0 || rts.version < 1 || rts.version > 1)
		goto on_error;
	if (rts.tile_bpp != 32) goto on_error;
	if (!(tiles = calloc(rts.num_tiles, sizeof(struct tile)))) goto on_error;
	
	// prepare the tile atlas
	n_tiles_per_row = ceil(sqrt(rts.num_tiles));
	atlas_w = rts.tile_width * n_tiles_per_row;
	atlas_h = rts.tile_height * n_tiles_per_row;
	if (!(atlas = create_image(atlas_w, atlas_h))) goto on_error;

	// read in tile bitmaps
	for (i = 0; i < rts.num_tiles; ++i) {
		tiles[i].image = read_subimage(file, atlas,
			i % n_tiles_per_row * rts.tile_width, i / n_tiles_per_row * rts.tile_height,
			rts.tile_width, rts.tile_height);
		if (tiles[i].image == NULL) goto on_error;
	}

	// read in tile headers and obstruction maps
	for (i = 0; i < rts.num_tiles; ++i) {
		if (fread(&tilehdr, sizeof(struct rts_tile_header), 1, file) != 1)
			goto on_error;
		tiles[i].name = read_lstring_raw(file, tilehdr.name_length, true);
		tiles[i].next_index = tilehdr.animated ? tilehdr.next_tile : i;
		tiles[i].delay = tilehdr.animated ? tilehdr.delay : 0;
		tiles[i].animate_index = i;
		tiles[i].frames_left = tiles[i].delay;
		if (rts.has_obstructions) {
			switch (tilehdr.obsmap_type) {
			case 1:  // pixel-perfect obstruction (no longer supported)
				fseek(file, rts.tile_width * rts.tile_height, SEEK_CUR);
				break;
			case 2:  // line segment-based obstruction
				tiles[i].num_obs_lines = tilehdr.num_segments;
				if ((tiles[i].obsmap = new_obsmap()) == NULL) goto on_error;
				for (j = 0; j < tilehdr.num_segments; ++j) {
					if (!fread_rect_16(file, &segment))
						goto on_error;
					add_obsmap_line(tiles[i].obsmap, segment);
				}
				break;
			default:
				goto on_error;
			}
		}
	}

	// wrap things up
	free_image(atlas);
	tileset->width = rts.tile_width;
	tileset->height = rts.tile_height;
	tileset->num_tiles = rts.num_tiles;
	tileset->tiles = tiles;
	return tileset;

on_error:  // oh no!
	if (file != NULL) fseek(file, file_pos, SEEK_SET);
	if (tiles != NULL) {
		for (i = 0; i < rts.num_tiles; ++i) {
			free_lstring(tiles[i].name);
			free_obsmap(tiles[i].obsmap);
			free_image(tiles[i].image);
		}
		free(tileset->tiles);
	}
	free_image(atlas);
	free(tileset);
	return NULL;
}

void
free_tileset(tileset_t* tileset)
{
	int i;

	for (i = 0; i < tileset->num_tiles; ++i) {
		free_lstring(tileset->tiles[i].name);
		free_image(tileset->tiles[i].image);
		free_obsmap(tileset->tiles[i].obsmap);
	}
	free(tileset->tiles);
	free(tileset);
}

int
get_next_tile(const tileset_t* tileset, int tile_index)
{
	int next_index;
	
	next_index = tileset->tiles[tile_index].next_index;
	return next_index >= 0 && next_index < tileset->num_tiles
		? next_index : tile_index;
}

int
get_tile_count(const tileset_t* tileset)
{
	return tileset->num_tiles;
}

int
get_tile_delay(const tileset_t* tileset, int tile_index)
{
	return tileset->tiles[tile_index].delay;
}

image_t*
get_tile_image(const tileset_t* tileset, int tile_index)
{
	return tileset->tiles[tile_index].image;
}

const lstring_t*
get_tile_name(const tileset_t* tileset, int tile_index)
{
	return tileset->tiles[tile_index].name;
}

const obsmap_t*
get_tile_obsmap(const tileset_t* tileset, int tile_index)
{
	return tileset->tiles[tile_index].obsmap;
}

void
get_tile_size(const tileset_t* tileset, int* out_w, int* out_h)
{
	*out_w = tileset->width;
	*out_h = tileset->height;
}

void
set_next_tile(tileset_t* tileset, int tile_index, int next_index)
{
	tileset->tiles[tile_index].next_index = next_index;
}

void
set_tile_delay(tileset_t* tileset, int tile_index, int delay)
{
	tileset->tiles[tile_index].delay = delay;
}

void
set_tile_image(tileset_t* tileset, int tile_index, image_t* image)
{
	image_t* old_image;
	
	old_image = tileset->tiles[tile_index].image;
	tileset->tiles[tile_index].image = ref_image(image);
	free_image(old_image);
}

void
animate_tileset(tileset_t* tileset)
{
	struct tile* tile;
	
	int i;

	for (i = 0; i < tileset->num_tiles; ++i) {
		tile = &tileset->tiles[i];
		if (tile->frames_left > 0 && --tile->frames_left == 0) {
			tile->animate_index = get_next_tile(tileset, tile->animate_index);
			tile->frames_left = get_tile_delay(tileset, tile->animate_index);
		}
	}
}

void
draw_tile(const tileset_t* tileset, color_t mask, float x, float y, int tile_index)
{
	tile_index = tileset->tiles[tile_index].animate_index;
	al_draw_tinted_bitmap(get_image_bitmap(tileset->tiles[tile_index].image),
		al_map_rgba(mask.r, mask.g, mask.b, mask.alpha), x, y, 0x0);
}
