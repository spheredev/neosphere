/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "cell.h"
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
	rts.num_tiles = (image_width(image) / tile_width) * (image_height(image) / tile_height);
	rts.has_obstructions = 0;
	rts.tile_width = (uint16_t)tile_width;
	rts.tile_height = (uint16_t)tile_height;
	rts.tile_bpp = 32;

	file = fopen(path_cstr(path), "wb");
	fwrite(&rts, sizeof(struct rts_header), 1, file);
	pitch = image_width(image);
	x = 0; y = 0;
	for (i = 0; i < rts.num_tiles; ++i) {
		for (i_y = 0; i_y < tile_height; ++i_y) {
			pixelbuf = image_bitmap(image, NULL) + x + (y + i_y) * pitch;
			fwrite(pixelbuf, tile_width * 4, 1, file);
		}
		if ((x += tile_width) + tile_width > image_width(image)) {
			x = 0;
			y += tile_height;
		}
	}
	memset(&tile, 0, sizeof(struct rts_tile_header));
	for (i = 0; i < rts.num_tiles; ++i)
		fwrite(&tile, sizeof(struct rts_tile_header), 1, file);
	fclose(file);
}
