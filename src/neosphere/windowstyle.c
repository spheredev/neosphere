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

#include "neosphere.h"
#include "windowstyle.h"

#include "color.h"
#include "image.h"

enum back_mode
{
	BG_TILE,
	BG_STRETCH,
	BG_GRADIENT,
	BG_TILE_GRADIENT,
	BG_STRETCH_GRADIENT
};

struct windowstyle
{
	int       refcount;
	int       bg_style;
	color_t   color_mask;
	color_t   gradient[4];
	image_t*  images[9];
};

#pragma pack(push, 1)
struct rws_header
{
	uint8_t signature[4];
	int16_t version;
	uint8_t edge_w_h;
	uint8_t background_mode;
	color_t corner_colors[4];
	uint8_t edge_offsets[4];
	uint8_t reserved[36];
};
#pragma pack(pop)

windowstyle_t*
winstyle_load(const char* filename)
{
	file_t*           file;
	image_t*          image;
	struct rws_header rws;
	int16_t           w, h;
	windowstyle_t*    winstyle = NULL;
	int               i;

	if (!(file = file_open(g_game, filename, "rb")))
		goto on_error;
	if ((winstyle = calloc(1, sizeof(windowstyle_t))) == NULL)
		goto on_error;
	if (file_read(file, &rws, 1, sizeof(struct rws_header)) != 1)
		goto on_error;
	if (memcmp(rws.signature, ".rws", 4) != 0)
		goto on_error;
	switch (rws.version) {
	case 1:
		for (i = 0; i < 9; ++i) {
			if (!(image = fread_image(file, rws.edge_w_h, rws.edge_w_h)))
				goto on_error;
			winstyle->images[i] = image;
		}
		break;
	case 2:
		for (i = 0; i < 9; ++i) {
			if (file_read(file, &w, 1, 2) != 1 || file_read(file, &h, 1, 2) != 1)
				goto on_error;
			if (!(image = fread_image(file, w, h)))
				goto on_error;
			winstyle->images[i] = image;
		}
		break;
	default:  // invalid version number
		goto on_error;
	}
	file_close(file);
	winstyle->bg_style = rws.background_mode;
	winstyle->color_mask = mk_color(255, 255, 255, 255);
	for (i = 0; i < 4; ++i)
		winstyle->gradient[i] = rws.corner_colors[i];
	return winstyle_ref(winstyle);

on_error:
	file_close(file);
	if (winstyle != NULL) {
		for (i = 0; i < 9; ++i)
			image_unref(winstyle->images[i]);
		free(winstyle);
	}
	return NULL;
}

windowstyle_t*
winstyle_ref(windowstyle_t* it)
{
	++it->refcount;
	return it;
}

void
winstyle_unref(windowstyle_t* it)
{
	int i;

	if (it == NULL || --it->refcount > 0)
		return;
	for (i = 0; i < 9; ++i) {
		image_unref(it->images[i]);
	}
	free(it);
}

color_t
winstyle_get_mask(const windowstyle_t* it)
{
	return it->color_mask;
}

void
winstyle_set_mask(windowstyle_t* it, color_t color)
{
	it->color_mask = color;
}

void
winstyle_draw(windowstyle_t* it, int x, int y, int width, int height)
{
	color_t gradient[4];
	color_t mask;
	int     w[9], h[9];

	int i;

	// 0 - upper left
	// 1 - top
	// 2 - upper right
	// 3 - right
	// 4 - lower right
	// 5 - bottom
	// 6 - lower left
	// 7 - left
	// 8 - background

	mask = it->color_mask;

	for (i = 0; i < 9; ++i) {
		w[i] = image_width(it->images[i]);
		h[i] = image_height(it->images[i]);
	}
	for (i = 0; i < 4; ++i) {
		gradient[i].r = mask.r * it->gradient[i].r / 255;
		gradient[i].g = mask.g * it->gradient[i].g / 255;
		gradient[i].b = mask.b * it->gradient[i].b / 255;
		gradient[i].a = mask.a * it->gradient[i].a / 255;
	}
	ALLEGRO_VERTEX verts[] = {
		{ x, y, 0, 0, 0, nativecolor(gradient[0]) },
		{ x + width, y, 0, 0, 0, nativecolor(gradient[1]) },
		{ x, y + height, 0, 0, 0, nativecolor(gradient[2]) },
		{ x + width, y + height, 0, 0, 0, nativecolor(gradient[3]) },
	};

	switch (it->bg_style) {
	case BG_TILE:
		image_draw_tiled_masked(it->images[8], mask, x, y, width, height);
		break;
	case BG_STRETCH:
		image_draw_scaled_masked(it->images[8], mask, x, y, width, height);
		break;
	case BG_GRADIENT:
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
		break;
	case BG_TILE_GRADIENT:
		image_draw_tiled_masked(it->images[8], mask, x, y, width, height);
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
		break;
	case BG_STRETCH_GRADIENT:
		image_draw_scaled_masked(it->images[8], mask, x, y, width, height);
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
		break;
	}
	image_draw_masked(it->images[0], mask, x - w[0], y - h[0]);
	image_draw_masked(it->images[2], mask, x + width, y - h[2]);
	image_draw_masked(it->images[4], mask, x + width, y + height);
	image_draw_masked(it->images[6], mask, x - w[6], y + height);
	image_draw_tiled_masked(it->images[1], mask, x, y - h[1], width, h[1]);
	image_draw_tiled_masked(it->images[3], mask, x + width, y, w[3], height);
	image_draw_tiled_masked(it->images[5], mask, x, y + height, width, h[5]);
	image_draw_tiled_masked(it->images[7], mask, x - w[7], y, w[7], height);
}
