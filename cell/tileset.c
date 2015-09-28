#include "tileset.h"

#include "image.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

void
build_tileset(const path_t* path, const image_t* image, int tile_width, int tile_height)
{
	FILE*                  file;
	ptrdiff_t              pitch;
	const uint32_t*        pixelbuf;
	struct rts_header      rts;
	struct rts_tile_header tile;
	int                    x, y;

	int i_y;
	uint16_t i;

	memset(&rts, 0, sizeof(struct rts_header));
	memcpy(&rts.signature, ".rts", 4);
	rts.version = 1;
	rts.num_tiles = (image_get_width(image) / tile_width) * (image_get_height(image) / tile_height);
	rts.has_obstructions = 0;
	rts.tile_width = (uint16_t)tile_width;
	rts.tile_height = (uint16_t)tile_height;
	rts.tile_bpp = 32;

	file = fopen(path_cstr(path), "wb");
	fwrite(&rts, sizeof(struct rts_header), 1, file);
	pitch = image_get_pitch(image);
	x = 0; y = 0;
	for (i = 0; i < rts.num_tiles; ++i) {
		for (i_y = 0; i_y < tile_height; ++i_y) {
			pixelbuf = image_get_pixelbuf(image) + x + (y + i_y) * pitch;
			fwrite(pixelbuf, tile_width * 4, 1, file);
		}
		if ((x += tile_width) + tile_width > image_get_width(image)) {
			x = 0;
			y += tile_height;
		}
	}
	memset(&tile, 0, sizeof(struct rts_tile_header));
	for (i = 0; i < rts.num_tiles; ++i)
		fwrite(&tile, sizeof(struct rts_tile_header), 1, file);
	fclose(file);
}
