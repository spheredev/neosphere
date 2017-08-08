#include "minisphere.h"
#include "windowstyle.h"

#include "color.h"
#include "image.h"

static windowstyle_t* s_sys_winstyle = NULL;

enum wstyle_bg_type
{
	WSTYLE_BG_TILE,
	WSTYLE_BG_STRETCH,
	WSTYLE_BG_GRADIENT,
	WSTYLE_BG_TILE_GRADIENT,
	WSTYLE_BG_STRETCH_GRADIENT
};

struct windowstyle
{
	int       refcount;
	int       bg_style;
	color_t   gradient[4];
	image_t*  images[9];
};

#pragma pack(push, 1)
struct rws_rgba
{
	uint8_t r, g, b, a;
};

struct rws_header
{
	uint8_t         signature[4];
	int16_t         version;
	uint8_t         edge_w_h;
	uint8_t         background_mode;
	struct rws_rgba corner_colors[4];
	uint8_t         edge_offsets[4];
	uint8_t         reserved[36];
};
#pragma pack(pop)

windowstyle_t*
load_windowstyle(const char* filename)
{
	file_t*           file;
	image_t*          image;
	struct rws_header rws;
	int16_t           w, h;
	windowstyle_t*    winstyle = NULL;
	int               i;

	if (!(file = file_open(g_game_fs, filename, NULL, "rb"))) goto on_error;
	if ((winstyle = calloc(1, sizeof(windowstyle_t))) == NULL) goto on_error;
	if (file_read(&rws, sizeof(struct rws_header), 1, file) != 1)
		goto on_error;
	if (memcmp(rws.signature, ".rws", 4) != 0) goto on_error;
	switch (rws.version) {
	case 1:
		for (i = 0; i < 9; ++i) {
			if (!(image = image_read(file, rws.edge_w_h, rws.edge_w_h)))
				goto on_error;
			winstyle->images[i] = image;
		}
		break;
	case 2:
		for (i = 0; i < 9; ++i) {
			if (file_read(&w, 2, 1, file) != 1 || file_read(&h, 2, 1, file) != 1)
				goto on_error;
			if (!(image = image_read(file, w, h))) goto on_error;
			winstyle->images[i] = image;
		}
		break;
	default:  // invalid version number
		goto on_error;
	}
	file_close(file);
	winstyle->bg_style = rws.background_mode;
	for (i = 0; i < 4; ++i) {
		winstyle->gradient[i] = color_new(
			rws.corner_colors[i].r,
			rws.corner_colors[i].g,
			rws.corner_colors[i].b,
			rws.corner_colors[i].a);
	}
	return ref_windowstyle(winstyle);

on_error:
	if (file != NULL) file_close(file);
	if (winstyle != NULL) {
		for (i = 0; i < 9; ++i)
			image_free(winstyle->images[i]);
		free(winstyle);
	}
	return NULL;
}

windowstyle_t*
ref_windowstyle(windowstyle_t* winstyle)
{
	++winstyle->refcount;
	return winstyle;
}

void
free_windowstyle(windowstyle_t* winstyle)
{
	int i;

	if (winstyle == NULL || --winstyle->refcount > 0)
		return;
	for (i = 0; i < 9; ++i) {
		image_free(winstyle->images[i]);
	}
	free(winstyle);
}

void
draw_window(windowstyle_t* winstyle, color_t mask, int x, int y, int width, int height)
{
	color_t gradient[4];
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

	for (i = 0; i < 9; ++i) {
		w[i] = image_width(winstyle->images[i]);
		h[i] = image_height(winstyle->images[i]);
	}
	for (i = 0; i < 4; ++i) {
		gradient[i].r = mask.r * winstyle->gradient[i].r / 255;
		gradient[i].g = mask.g * winstyle->gradient[i].g / 255;
		gradient[i].b = mask.b * winstyle->gradient[i].b / 255;
		gradient[i].a = mask.a * winstyle->gradient[i].a / 255;
	}
	ALLEGRO_VERTEX verts[] = {
		{ x, y, 0, 0, 0, nativecolor(gradient[0]) },
		{ x + width, y, 0, 0, 0, nativecolor(gradient[1]) },
		{ x, y + height, 0, 0, 0, nativecolor(gradient[2]) },
		{ x + width, y + height, 0, 0, 0, nativecolor(gradient[3]) },
	};
	
	switch (winstyle->bg_style) {
	case WSTYLE_BG_TILE:
		image_draw_tiled_masked(winstyle->images[8], mask, x, y, width, height);
		break;
	case WSTYLE_BG_STRETCH:
		image_draw_scaled_masked(winstyle->images[8], mask, x, y, width, height);
		break;
	case WSTYLE_BG_GRADIENT:
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
		break;
	case WSTYLE_BG_TILE_GRADIENT:
		image_draw_tiled_masked(winstyle->images[8], mask, x, y, width, height);
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
		break;
	case WSTYLE_BG_STRETCH_GRADIENT:
		image_draw_scaled_masked(winstyle->images[8], mask, x, y, width, height);
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
		break;
	}
	image_draw_masked(winstyle->images[0], mask, x - w[0], y - h[0]);
	image_draw_masked(winstyle->images[2], mask, x + width, y - h[2]);
	image_draw_masked(winstyle->images[4], mask, x + width, y + height);
	image_draw_masked(winstyle->images[6], mask, x - w[6], y + height);
	image_draw_tiled_masked(winstyle->images[1], mask, x, y - h[1], width, h[1]);
	image_draw_tiled_masked(winstyle->images[3], mask, x + width, y, w[3], height);
	image_draw_tiled_masked(winstyle->images[5], mask, x, y + height, width, h[5]);
	image_draw_tiled_masked(winstyle->images[7], mask, x - w[7], y, w[7], height);
}
