#include "minisphere.h"
#include "atlas.h"
#include "image.h"
#include "obsmap.h"

#include "tileset.h"

struct tileset
{
	unsigned int id;
	atlas_t*     atlas;
	int          atlas_pitch;
	int          height;
	int          num_tiles;
	struct tile* tiles;
	int          width;
};

struct tile
{
	int        delay;
	int        frames_left;
	image_t*   image;
	int        image_index;
	lstring_t* name;
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

static unsigned int s_next_tileset_id = 0;

tileset_t*
load_tileset(const char* filename)
{
	sfs_file_t* file;
	tileset_t*  tileset;

	console_log(2, "loading tileset #%u as `%s`", s_next_tileset_id, filename);

	if ((file = sfs_fopen(g_fs, filename, NULL, "rb")) == NULL)
		goto on_error;
	tileset = read_tileset(file);
	sfs_fclose(file);
	return tileset;

on_error:
	console_log(2, "failed to load tileset #%u", s_next_tileset_id++);
	return NULL;
}

tileset_t*
read_tileset(sfs_file_t* file)
{
	atlas_t*               atlas = NULL;
	long                   file_pos;
	struct rts_header      rts;
	rect_t                 segment;
	struct rts_tile_header tilehdr;
	struct tile*           tiles = NULL;
	tileset_t*             tileset = NULL;

	int i, j;

	memset(&rts, 0, sizeof(struct rts_header));
	
	console_log(2, "reading tileset #%u from open file", s_next_tileset_id);

	if (file == NULL) goto on_error;
	file_pos = sfs_ftell(file);
	if ((tileset = calloc(1, sizeof(tileset_t))) == NULL) goto on_error;
	if (sfs_fread(&rts, sizeof(struct rts_header), 1, file) != 1)
		goto on_error;
	if (memcmp(rts.signature, ".rts", 4) != 0 || rts.version < 1 || rts.version > 1)
		goto on_error;
	if (rts.tile_bpp != 32) goto on_error;
	if (!(tiles = calloc(rts.num_tiles, sizeof(struct tile)))) goto on_error;
	
	// read in all the tile bitmaps (use atlasing)
	if (!(atlas = create_atlas(rts.num_tiles, rts.tile_width, rts.tile_height)))
		goto on_error;
	lock_atlas(atlas);
	for (i = 0; i < rts.num_tiles; ++i)
		if (!(tiles[i].image = read_atlas_image(atlas, file, i, rts.tile_width, rts.tile_height)))
			goto on_error;
	unlock_atlas(atlas);
	tileset->atlas = atlas;

	// read in tile headers and obstruction maps
	for (i = 0; i < rts.num_tiles; ++i) {
		if (sfs_fread(&tilehdr, sizeof(struct rts_tile_header), 1, file) != 1)
			goto on_error;
		tiles[i].name = read_lstring_raw(file, tilehdr.name_length, true);
		tiles[i].next_index = tilehdr.animated ? tilehdr.next_tile : i;
		tiles[i].delay = tilehdr.animated ? tilehdr.delay : 0;
		tiles[i].image_index = i;
		tiles[i].frames_left = tiles[i].delay;
		if (rts.has_obstructions) {
			switch (tilehdr.obsmap_type) {
			case 1:  // pixel-perfect obstruction (no longer supported)
				sfs_fseek(file, rts.tile_width * rts.tile_height, SFS_SEEK_CUR);
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
	tileset->id = s_next_tileset_id++;
	tileset->width = rts.tile_width;
	tileset->height = rts.tile_height;
	tileset->num_tiles = rts.num_tiles;
	tileset->tiles = tiles;
	return tileset;

on_error:  // oh no!
	console_log(2, "failed to read tileset #%u", s_next_tileset_id);
	if (file != NULL)
		sfs_fseek(file, file_pos, SFS_SEEK_SET);
	if (tiles != NULL) {
		for (i = 0; i < rts.num_tiles; ++i) {
			lstr_free(tiles[i].name);
			free_obsmap(tiles[i].obsmap);
			free_image(tiles[i].image);
		}
		free(tileset->tiles);
	}
	free_atlas(atlas);
	free(tileset);
	return NULL;
}

void
free_tileset(tileset_t* tileset)
{
	int i;

	console_log(3, "disposing tileset #%u as it is no longer in use", tileset->id);

	for (i = 0; i < tileset->num_tiles; ++i) {
		lstr_free(tileset->tiles[i].name);
		free_image(tileset->tiles[i].image);
		free_obsmap(tileset->tiles[i].obsmap);
	}
	free_atlas(tileset->atlas);
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

image_t*
get_tile_atlas(const tileset_t* tileset)
{
	return get_atlas_image(tileset->atlas);
}

float_rect_t
get_tile_atlas_uv(const tileset_t* tileset, int tile_index)
{
	return get_atlas_uv(tileset->atlas, tileset->tiles[tile_index].image_index);
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
	if (tile_index >= 0)
		return tileset->tiles[tile_index].obsmap;
	else
		return NULL;
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
	// CAUTION: the new tile image is assumed to be the same same size as the tile it
	//     replaces.  if it's not, the engine won't crash, but it may cause graphical
	//     glitches.
	
	int blend_mode_dest;
	int blend_mode_src;
	int blend_op;

	al_set_target_bitmap(get_image_bitmap(tileset->tiles[tile_index].image));
	al_get_blender(&blend_op, &blend_mode_src, &blend_mode_dest);
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
	al_draw_bitmap(get_image_bitmap(image), 0, 0, 0x0);
	al_set_blender(blend_op, blend_mode_src, blend_mode_dest);
	al_set_target_backbuffer(screen_display(g_screen));
}

bool
set_tile_name(tileset_t* tileset, int tile_index, const lstring_t* name)
{
	lstring_t* new_name;
	
	if (!(new_name = lstr_dup(name)))
		return false;
	lstr_free(tileset->tiles[tile_index].name);
	tileset->tiles[tile_index].name = new_name;
	return true;
}

bool
animate_tileset(tileset_t* tileset)
{
	bool         has_changed = false;
	struct tile* tile;
	
	int i;

	for (i = 0; i < tileset->num_tiles; ++i) {
		tile = &tileset->tiles[i];
		if (tile->frames_left > 0 && --tile->frames_left == 0) {
			tile->image_index = get_next_tile(tileset, tile->image_index);
			tile->frames_left = get_tile_delay(tileset, tile->image_index);
			has_changed = true;
		}
	}
	return has_changed;
}

void
draw_tile(const tileset_t* tileset, color_t mask, float x, float y, int tile_index)
{
	if (tile_index < 0)
		return;
	tile_index = tileset->tiles[tile_index].image_index;
	al_draw_tinted_bitmap(get_image_bitmap(tileset->tiles[tile_index].image),
		al_map_rgba(mask.r, mask.g, mask.b, mask.alpha), x, y, 0x0);
}
