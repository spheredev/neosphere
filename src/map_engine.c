#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "image.h"
#include "input.h"
#include "obsmap.h"
#include "persons.h"
#include "script.h"
#include "surface.h"
#include "tileset.h"

#include "map_engine.h"

enum map_script_type
{
	MAP_SCRIPT_ON_ENTER,
	MAP_SCRIPT_ON_LEAVE,
	MAP_SCRIPT_ON_LEAVE_NORTH,
	MAP_SCRIPT_ON_LEAVE_EAST,
	MAP_SCRIPT_ON_LEAVE_SOUTH,
	MAP_SCRIPT_ON_LEAVE_WEST,
	MAP_SCRIPT_MAX
};

static struct map*         load_map            (const char* path);
static void                free_map            (struct map* map);
static bool                are_zones_at        (int x, int y, int layer, int* out_count);
static struct map_trigger* get_trigger_at      (int x, int y, int layer, int* out_index);
static struct map_zone*    get_zone_at         (int x, int y, int layer, int which, int* out_index);
static bool                change_map          (const char* filename, bool preserve_persons);
static int                 find_layer          (const char* name);
static void                map_screen_to_layer (int layer, int camera_x, int camera_y, int* inout_x, int* inout_y);
static void                process_map_input   (void);
static void                render_map          (void);
static void                update_map_engine   (bool is_main_loop);

static duk_ret_t js_MapEngine               (duk_context* ctx);
static duk_ret_t js_AreZonesAt              (duk_context* ctx);
static duk_ret_t js_IsCameraAttached        (duk_context* ctx);
static duk_ret_t js_IsInputAttached         (duk_context* ctx);
static duk_ret_t js_IsLayerReflective       (duk_context* ctx);
static duk_ret_t js_IsLayerVisible          (duk_context* ctx);
static duk_ret_t js_IsMapEngineRunning      (duk_context* ctx);
static duk_ret_t js_IsTriggerAt             (duk_context* ctx);
static duk_ret_t js_GetCameraPerson         (duk_context* ctx);
static duk_ret_t js_GetCameraX              (duk_context* ctx);
static duk_ret_t js_GetCameraY              (duk_context* ctx);
static duk_ret_t js_GetCurrentMap           (duk_context* ctx);
static duk_ret_t js_GetCurrentTrigger       (duk_context* ctx);
static duk_ret_t js_GetCurrentZone          (duk_context* ctx);
static duk_ret_t js_GetInputPerson          (duk_context* ctx);
static duk_ret_t js_GetLayerHeight          (duk_context* ctx);
static duk_ret_t js_GetLayerMask            (duk_context* ctx);
static duk_ret_t js_GetLayerWidth           (duk_context* ctx);
static duk_ret_t js_GetMapEngineFrameRate   (duk_context* ctx);
static duk_ret_t js_GetNextAnimatedTile     (duk_context* ctx);
static duk_ret_t js_GetNumLayers            (duk_context* ctx);
static duk_ret_t js_GetNumTiles             (duk_context* ctx);
static duk_ret_t js_GetNumTriggers          (duk_context* ctx);
static duk_ret_t js_GetNumZones             (duk_context* ctx);
static duk_ret_t js_GetTalkActivationButton (duk_context* ctx);
static duk_ret_t js_GetTalkActivationKey    (duk_context* ctx);
static duk_ret_t js_GetTile                 (duk_context* ctx);
static duk_ret_t js_GetTileDelay            (duk_context* ctx);
static duk_ret_t js_GetTileHeight           (duk_context* ctx);
static duk_ret_t js_GetTileImage            (duk_context* ctx);
static duk_ret_t js_GetTileName             (duk_context* ctx);
static duk_ret_t js_GetTileSurface          (duk_context* ctx);
static duk_ret_t js_GetTileWidth            (duk_context* ctx);
static duk_ret_t js_GetZoneHeight           (duk_context* ctx);
static duk_ret_t js_GetZoneLayer            (duk_context* ctx);
static duk_ret_t js_GetZoneWidth            (duk_context* ctx);
static duk_ret_t js_GetZoneX                (duk_context* ctx);
static duk_ret_t js_GetZoneY                (duk_context* ctx);
static duk_ret_t js_SetCameraX              (duk_context* ctx);
static duk_ret_t js_SetCameraY              (duk_context* ctx);
static duk_ret_t js_SetColorMask            (duk_context* ctx);
static duk_ret_t js_SetDefaultMapScript     (duk_context* ctx);
static duk_ret_t js_SetLayerMask            (duk_context* ctx);
static duk_ret_t js_SetLayerReflective      (duk_context* ctx);
static duk_ret_t js_SetLayerRenderer        (duk_context* ctx);
static duk_ret_t js_SetLayerVisible         (duk_context* ctx);
static duk_ret_t js_SetMapEngineFrameRate   (duk_context* ctx);
static duk_ret_t js_SetNextAnimatedTile     (duk_context* ctx);
static duk_ret_t js_SetRenderScript         (duk_context* ctx);
static duk_ret_t js_SetTalkActivationButton (duk_context* ctx);
static duk_ret_t js_SetTalkActivationKey    (duk_context* ctx);
static duk_ret_t js_SetTile                 (duk_context* ctx);
static duk_ret_t js_SetTileDelay            (duk_context* ctx);
static duk_ret_t js_SetTileImage            (duk_context* ctx);
static duk_ret_t js_SetTileName             (duk_context* ctx);
static duk_ret_t js_SetTileSurface          (duk_context* ctx);
static duk_ret_t js_SetUpdateScript         (duk_context* ctx);
static duk_ret_t js_SetZoneLayer            (duk_context* ctx);
static duk_ret_t js_AttachCamera            (duk_context* ctx);
static duk_ret_t js_AttachInput             (duk_context* ctx);
static duk_ret_t js_CallDefaultMapScript    (duk_context* ctx);
static duk_ret_t js_CallMapScript           (duk_context* ctx);
static duk_ret_t js_ChangeMap               (duk_context* ctx);
static duk_ret_t js_DetachCamera            (duk_context* ctx);
static duk_ret_t js_DetachInput             (duk_context* ctx);
static duk_ret_t js_ExecuteTrigger          (duk_context* ctx);
static duk_ret_t js_ExecuteZones            (duk_context* ctx);
static duk_ret_t js_ExitMapEngine           (duk_context* ctx);
static duk_ret_t js_MapToScreenX            (duk_context* ctx);
static duk_ret_t js_MapToScreenY            (duk_context* ctx);
static duk_ret_t js_RenderMap               (duk_context* ctx);
static duk_ret_t js_ReplaceTilesOnLayer     (duk_context* ctx);
static duk_ret_t js_ScreenToMapX            (duk_context* ctx);
static duk_ret_t js_ScreenToMapY            (duk_context* ctx);
static duk_ret_t js_SetDelayScript          (duk_context* ctx);
static duk_ret_t js_UpdateMapEngine         (duk_context* ctx);

static person_t*           s_camera_person     = NULL;
static int                 s_cam_x             = 0;
static int                 s_cam_y             = 0;
static color_t             s_color_mask;
static color_t             s_fade_color_from;
static color_t             s_fade_color_to;
static int                 s_fade_frames;
static int                 s_fade_progress;
static int                 s_map_corner_x;
static int                 s_map_corner_y;
static int                 s_current_trigger   = -1;
static int                 s_current_zone      = -1;
static script_t*           s_def_scripts[MAP_SCRIPT_MAX];
static bool                s_exiting           = false;
static int                 s_framerate         = 0;
static unsigned int        s_frames            = 0;
static person_t*           s_input_person      = NULL;
static bool                s_is_talk_allowed   = true;
static bool                s_is_map_running    = false;
static struct map*         s_map = NULL;
static char*               s_map_filename      = NULL;
static struct map_trigger* s_on_trigger        = NULL;
static script_t*           s_render_script     = 0;
static int                 s_talk_button       = 0;
static int                 s_talk_key          = ALLEGRO_KEY_SPACE;
static script_t*           s_update_script     = 0;
static int                 s_num_delay_scripts = 0;
static int                 s_max_delay_scripts = 0;
static struct delay_script *s_delay_scripts    = NULL;

struct delay_script
{
	script_t* script;
	int       frames_left;
};

struct map
{
	int                width, height;
	bool               is_repeating;
	point3_t           origin;
	script_t*          scripts[MAP_SCRIPT_MAX];
	tileset_t*         tileset;
	int                num_layers;
	int                num_persons;
	int                num_triggers;
	int                num_zones;
	struct map_layer   *layers;
	struct map_person  *persons;
	struct map_trigger *triggers;
	struct map_zone    *zones;
};

struct map_layer
{
	lstring_t*       name;
	bool             is_parallax;
	bool             is_reflective;
	bool             is_visible;
	int              width, height;
	float            autoscroll_x;
	float            autoscroll_y;
	float            parallax_x;
	float            parallax_y;
	struct map_tile* tilemap;
	obsmap_t*        obsmap;
	color_t          color_mask;
	script_t*        render_script;
};

struct map_person
{
	lstring_t* name;
	lstring_t* spriteset;
	int        x, y, z;
	lstring_t* create_script;
	lstring_t* destroy_script;
	lstring_t* command_script;
	lstring_t* talk_script;
	lstring_t* touch_script;
};

struct map_tile
{
	int tile_index;
	int frames_left;
};

struct map_trigger
{
	script_t* script;
	int       x, y, z;
};

struct map_zone
{
	bool      is_active;
	rect_t    bounds;
	int       step_interval;
	int       steps_left;
	int       layer;
	script_t* script;
};

#pragma pack(push, 1)
struct rmp_header
{
	char    signature[4];
	int16_t version;
	uint8_t type;
	int8_t  num_layers;
	uint8_t reserved_1;
	int16_t num_entities;
	int16_t start_x;
	int16_t start_y;
	int8_t  start_layer;
	int8_t  start_direction;
	int16_t num_strings;
	int16_t num_zones;
	uint8_t repeat_map;
	uint8_t reserved[234];
};

struct rmp_entity_header
{
	uint16_t x;
	uint16_t y;
	uint16_t z;
	uint16_t type;
	uint8_t  reserved[8];
};

struct rmp_layer_header
{
	int16_t  width;
	int16_t  height;
	uint16_t flags;
	float    parallax_x;
	float    parallax_y;
	float    scrolling_x;
	float    scrolling_y;
	int32_t  num_segments;
	uint8_t  is_reflective;
	uint8_t  reserved[3];
};

struct rmp_zone_header
{
	uint16_t x1;
	uint16_t y1;
	uint16_t x2;
	uint16_t y2;
	uint16_t layer;
	uint16_t step_interval;
	uint8_t  reserved[4];
};
#pragma pack(pop)

void
initialize_map_engine(void)
{
	printf("Initializing map engine\n");
	
	initialize_persons_manager();
	memset(s_def_scripts, 0, MAP_SCRIPT_MAX * sizeof(int));
	s_map = NULL; s_map_filename = NULL;
	s_input_person = s_camera_person = NULL;
	s_current_trigger = -1;
	s_current_zone = -1;
	s_render_script = 0;
	s_update_script = 0;
	s_num_delay_scripts = s_max_delay_scripts = 0;
	s_delay_scripts = NULL;
	s_talk_key = ALLEGRO_KEY_SPACE;
	s_talk_button = 0;
	s_is_map_running = false;
	s_color_mask = rgba(0, 0, 0, 0);
	s_on_trigger = NULL;
}

void
shutdown_map_engine(void)
{
	int i;

	printf("Shutting down map engine\n");
	
	for (i = 0; i < s_num_delay_scripts; ++i)
		free_script(s_delay_scripts[i].script);
	free(s_delay_scripts);
	free_map(s_map);
	shutdown_persons_manager();
}

bool
is_map_engine_running(void)
{
	return s_is_map_running;
}

rect_t
get_map_bounds(void)
{
	rect_t bounds;
	int    tile_w, tile_h;
	
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	bounds.x1 = 0; bounds.y1 = 0;
	bounds.x2 = s_map->width * tile_w;
	bounds.y2 = s_map->height * tile_h;
	return bounds;
}

point3_t
get_map_origin(void)
{
	point3_t empty_point = { 0, 0, 0 };
	
	return s_map ? s_map->origin : empty_point;
}

int
get_map_tile(int x, int y, int layer)
{
	int layer_h = s_map->layers[layer].height;
	int layer_w = s_map->layers[layer].width;

	if (s_map->is_repeating || s_map->layers[layer].is_parallax) {
		x = (x % layer_w + layer_w) % layer_w;
		y = (y % layer_h + layer_h) % layer_h;
	}
	if (x < 0 || y < 0 || x >= layer_w || y >= layer_h)
		return -1;
	return s_map->layers[layer].tilemap[x + y * layer_w].tile_index;
}

const tileset_t*
get_map_tileset(void)
{
	return s_map->tileset;
}

const obsmap_t*
get_map_layer_obsmap(int layer)
{
	return s_map->layers[layer].obsmap;
}

void
detach_person(const person_t* person)
{
	if (s_camera_person == person)
		s_camera_person = NULL;
	if (s_input_person == person)
		s_input_person = NULL;
}

void
normalize_map_entity_xy(double* inout_x, double* inout_y, int layer)
{
	int tile_w, tile_h;
	int layer_w, layer_h;
	
	if (!s_map->is_repeating && !s_map->layers[layer].is_parallax)
		return;
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	layer_w = s_map->layers[layer].width * tile_w;
	layer_h = s_map->layers[layer].height * tile_h;
	if (inout_x) *inout_x = fmod(fmod(*inout_x, layer_w) + layer_w, layer_w);
	if (inout_y) *inout_y = fmod(fmod(*inout_y, layer_h) + layer_h, layer_h);
}

static struct map*
load_map(const char* path)
{
	// strings: 0 - tileset filename
	//          1 - music filename
	//          2 - script filename (obsolete, not used)
	//          3 - entry script
	//          4 - exit script
	//          5 - exit north script
	//          6 - exit east script
	//          7 - exit south script
	//          8 - exit west script

	uint16_t                 count;
	struct rmp_entity_header entity_hdr;
	FILE*                    file;
	bool                     has_failed;
	struct map_layer*        layer;
	struct rmp_layer_header  layer_hdr;
	struct map*              map = NULL;
	int                      num_tiles;
	struct map_person*       person;
	struct rmp_header        rmp;
	lstring_t*               script;
	rect_t                   segment;
	int16_t*                 tile_data = NULL;
	ALLEGRO_PATH*            tileset_path;
	tileset_t*               tileset;
	struct map_trigger*      trigger;
	struct rmp_zone_header   zone_hdr;
	lstring_t*               *strings = NULL;

	int i, j, x, y, z;

	memset(&rmp, 0, sizeof(struct rmp_header));
	
	if (!(file = fopen(path, "rb"))) goto on_error;
	if (!(map = calloc(1, sizeof(struct map)))) goto on_error;
	if (fread(&rmp, sizeof(struct rmp_header), 1, file) != 1)
		goto on_error;
	if (memcmp(rmp.signature, ".rmp", 4) != 0) goto on_error;
	if (rmp.num_strings != 3 && rmp.num_strings != 5 && rmp.num_strings < 9)
		goto on_error;
	if (rmp.start_layer < 0 || rmp.start_layer >= rmp.num_layers)
		rmp.start_layer = 0;  // being nice here, this really should fail outright
	switch (rmp.version) {
	case 1:
		// load strings (resource filenames, scripts, etc.)
		if ((strings = calloc(rmp.num_strings, sizeof(lstring_t*))) == NULL)
			goto on_error;
		has_failed = false;
		for (i = 0; i < rmp.num_strings; ++i)
			has_failed = has_failed || ((strings[i] = read_lstring(file, true)) == NULL);
		if (has_failed) goto on_error;

		// pre-allocate map structures; if an allocation fails we won't waste time reading the rest of the file
		if ((map->layers = calloc(rmp.num_layers, sizeof(struct map_layer))) == NULL) goto on_error;
		if ((map->persons = calloc(rmp.num_entities, sizeof(struct map_person))) == NULL) goto on_error;
		if ((map->triggers = calloc(rmp.num_entities, sizeof(struct map_trigger))) == NULL) goto on_error;
		if ((map->zones = calloc(rmp.num_zones, sizeof(struct map_zone))) == NULL) goto on_error;

		// load layers
		for (i = 0; i < rmp.num_layers; ++i) {
			if (fread(&layer_hdr, sizeof(struct rmp_layer_header), 1, file) != 1)
				goto on_error;
			layer = &map->layers[i];
			layer->is_parallax = (layer_hdr.flags & 2) != 0x0;
			layer->is_reflective = layer_hdr.is_reflective;
			layer->is_visible = (layer_hdr.flags & 1) == 0x0;
			layer->color_mask = rgba(255, 255, 255, 255);
			layer->width = layer_hdr.width;
			layer->height = layer_hdr.height;
			layer->autoscroll_x = layer->is_parallax ? layer_hdr.scrolling_x : 0.0;
			layer->autoscroll_y = layer->is_parallax ? layer_hdr.scrolling_y : 0.0;
			layer->parallax_x = layer->is_parallax ? layer_hdr.parallax_x : 1.0;
			layer->parallax_y = layer->is_parallax ? layer_hdr.parallax_y : 1.0;
			if (!layer->is_parallax) {
				map->width = fmax(map->width, layer->width);
				map->height = fmax(map->height, layer->height);
			}
			if (!(layer->tilemap = malloc(layer_hdr.width * layer_hdr.height * sizeof(struct map_tile))))
				goto on_error;
			if ((layer->obsmap = new_obsmap()) == NULL) goto on_error;
			layer->name = read_lstring(file, true);
			num_tiles = layer_hdr.width * layer_hdr.height;
			if ((tile_data = malloc(num_tiles * 2)) == NULL) goto on_error;
			if (fread(tile_data, 2, num_tiles, file) != num_tiles) goto on_error;
			for (j = 0; j < num_tiles; ++j)
				layer->tilemap[j].tile_index = tile_data[j];
			for (j = 0; j < layer_hdr.num_segments; ++j) {
				if (!fread_rect_32(file, &segment)) goto on_error;
				add_obsmap_line(layer->obsmap, segment);
			}
			free(tile_data); tile_data = NULL;
		}

		// if either dimension is zero, the map has no non-parallax layers and is thus malformed
		if (map->width == 0 || map->height == 0) goto on_error;

		// load entities
		map->num_persons = 0;
		map->num_triggers = 0;
		for (i = 0; i < rmp.num_entities; ++i) {
			if (fread(&entity_hdr, sizeof(struct rmp_entity_header), 1, file) != 1)
				goto on_error;
			if (entity_hdr.z < 0 || entity_hdr.z >= rmp.num_layers)
				entity_hdr.z = 0;
			switch (entity_hdr.type) {
			case 1:  // person
				++map->num_persons;
				person = &map->persons[map->num_persons - 1];
				memset(person, 0, sizeof(struct map_person));
				if ((person->name = read_lstring(file, true)) == NULL) goto on_error;
				if ((person->spriteset = read_lstring(file, true)) == NULL) goto on_error;
				person->x = entity_hdr.x; person->y = entity_hdr.y; person->z = entity_hdr.z;
				if (fread(&count, 2, 1, file) != 1 || count < 5) goto on_error;
				person->create_script = read_lstring(file, false);
				person->destroy_script = read_lstring(file, false);
				person->touch_script = read_lstring(file, false);
				person->talk_script = read_lstring(file, false);
				person->command_script = read_lstring(file, false);
				for (j = 5; j < count; ++j) {
					free_lstring(read_lstring(file, true));
				}
				fseek(file, 16, SEEK_CUR);
				break;
			case 2:  // trigger
				if ((script = read_lstring(file, false)) == NULL) goto on_error;
				++map->num_triggers;
				trigger = &map->triggers[map->num_triggers - 1];
				memset(trigger, 0, sizeof(struct map_trigger));
				trigger->x = entity_hdr.x;
				trigger->y = entity_hdr.y;
				trigger->z = entity_hdr.z;
				trigger->script = compile_script(script, "[trigger script]");
				free_lstring(script);
				break;
			default:
				goto on_error;
			}
		}

		// load zones
		for (i = 0; i < rmp.num_zones; ++i) {
			if (fread(&zone_hdr, sizeof(struct rmp_zone_header), 1, file) != 1)
				goto on_error;
			if ((script = read_lstring(file, false)) == NULL) goto on_error;
			if (zone_hdr.layer < 0 || zone_hdr.layer >= rmp.num_layers)
				zone_hdr.layer = 0;
			map->zones[i].layer = zone_hdr.layer;
			map->zones[i].bounds = new_rect(zone_hdr.x1, zone_hdr.y1, zone_hdr.x2, zone_hdr.y2);
			map->zones[i].step_interval = zone_hdr.step_interval;
			map->zones[i].script = compile_script(script, "[zone script]");
			free_lstring(script);
		}

		// load tileset
		if (strcmp(lstring_cstr(strings[0]), "") != 0) {
			tileset_path = al_create_path(path);
			al_set_path_filename(tileset_path, lstring_cstr(strings[0]));
			tileset = load_tileset(al_path_cstr(tileset_path, ALLEGRO_NATIVE_PATH_SEP));
			al_destroy_path(tileset_path);
		}
		else {
			tileset = read_tileset(file);
		}
		if (tileset == NULL) goto on_error;

		// initialize tile animation
		for (z = 0; z < rmp.num_layers; ++z) {
			layer = &map->layers[z];
			for (x = 0; x < layer->width; ++x) for (y = 0; y < layer->height; ++y) {
				i = x + y * layer->width;
				map->layers[z].tilemap[i].frames_left =
					get_tile_delay(tileset, map->layers[z].tilemap[i].tile_index);
			}
		}

		// wrap things up
		map->num_layers = rmp.num_layers;
		map->num_zones = rmp.num_zones;
		map->is_repeating = rmp.repeat_map;
		map->origin.x = rmp.start_x;
		map->origin.y = rmp.start_y;
		map->origin.z = rmp.start_layer;
		map->tileset = tileset;
		if (rmp.num_strings >= 5) {
			map->scripts[MAP_SCRIPT_ON_ENTER] = compile_script(strings[3], "[enter map script]");
			map->scripts[MAP_SCRIPT_ON_LEAVE] = compile_script(strings[4], "[exit map script]");
		}
		if (rmp.num_strings >= 9) {
			map->scripts[MAP_SCRIPT_ON_LEAVE_NORTH] = compile_script(strings[5], "[leave map north script]");
			map->scripts[MAP_SCRIPT_ON_LEAVE_EAST] = compile_script(strings[6], "[leave map east script]");
			map->scripts[MAP_SCRIPT_ON_LEAVE_SOUTH] = compile_script(strings[7], "[leave map south script]");
			map->scripts[MAP_SCRIPT_ON_LEAVE_WEST] = compile_script(strings[8], "[leave map west script]");
		}
		for (i = 0; i < rmp.num_strings; ++i) free_lstring(strings[i]);
		free(strings);
		break;
	default:
		goto on_error;
	}
	fclose(file);
	return map;

on_error:
	if (file != NULL) fclose(file);
	free(tile_data);
	if (strings != NULL) {
		for (i = 0; i < rmp.num_strings; ++i) free_lstring(strings[i]);
		free(strings);
	}
	if (map != NULL) {
		if (map->layers != NULL) {
			for (i = 0; i < rmp.num_layers; ++i) {
				free_lstring(map->layers[i].name);
				free(map->layers[i].tilemap);
				free_obsmap(map->layers[i].obsmap);
			}
			free(map->layers);
		}
		if (map->persons != NULL) {
			for (i = 0; i < map->num_persons; ++i) {
				free_lstring(map->persons[i].name);
				free_lstring(map->persons[i].spriteset);
				free_lstring(map->persons[i].create_script);
				free_lstring(map->persons[i].destroy_script);
				free_lstring(map->persons[i].command_script);
				free_lstring(map->persons[i].talk_script);
				free_lstring(map->persons[i].touch_script);
			}
			free(map->persons);
		}
		free(map->triggers);
		free(map->zones);
		free(map);
	}
	return NULL;
}

static void
free_map(struct map* map)
{
	int i;

	if (map != NULL) {
		for (i = 0; i < MAP_SCRIPT_MAX; ++i)
			free_script(map->scripts[i]);
		for (i = 0; i < map->num_layers; ++i) {
			free_script(map->layers[i].render_script);
			free_lstring(map->layers[i].name);
			free(map->layers[i].tilemap);
			free_obsmap(map->layers[i].obsmap);
		}
		for (i = 0; i < map->num_persons; ++i) {
			free_lstring(map->persons[i].name);
			free_lstring(map->persons[i].spriteset);
			free_lstring(map->persons[i].create_script);
			free_lstring(map->persons[i].destroy_script);
			free_lstring(map->persons[i].command_script);
			free_lstring(map->persons[i].talk_script);
			free_lstring(map->persons[i].touch_script);
		}
		for (i = 0; i < map->num_triggers; ++i)
			free_script(map->triggers[i].script);
		for (i = 0; i < map->num_zones; ++i)
			free_script(map->zones[i].script);
		free_tileset(map->tileset);
		free(map->layers);
		free(map->persons);
		free(map->triggers);
		free(map);
	}
}

static bool
are_zones_at(int x, int y, int layer, int* out_count)
{
	int              count = 0;
	struct map_zone* zone;
	bool             zone_found;

	int i;

	zone_found = false;
	for (i = 0; i < s_map->num_zones; ++i) {
		zone = &s_map->zones[i];
		if (zone->layer != layer && false)  // layer ignored for compatibility
			continue;
		if (is_point_in_rect(x, y, zone->bounds)) {
			zone_found = true;
			++count;
		}
	}
	if (out_count) *out_count = count;
	return zone_found;
}

static struct map_trigger*
get_trigger_at(int x, int y, int layer, int* out_index)
{
	rect_t              bounds;
	struct map_trigger* found_item = NULL;
	int                 tile_w, tile_h;
	struct map_trigger* trigger;

	int i;

	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	for (i = 0; i < s_map->num_triggers; ++i) {
		trigger = &s_map->triggers[i];
		if (trigger->z != layer && false)  // layer ignored for compatibility reasons
			continue;
		bounds.x1 = trigger->x - tile_w / 2;
		bounds.y1 = trigger->y - tile_h / 2;
		bounds.x2 = bounds.x1 + tile_w;
		bounds.y2 = bounds.y1 + tile_h;
		if (is_point_in_rect(x, y, bounds)) {
			found_item = trigger;
			if (out_index) *out_index = i;
			break;
		}
	}
	return found_item;
}

static struct map_zone*
get_zone_at(int x, int y, int layer, int which, int* out_index)
{
	struct map_zone* found_item = NULL;
	struct map_zone* zone;
	
	int i;

	for (i = 0; i < s_map->num_zones; ++i) {
		zone = &s_map->zones[i];
		if (zone->layer != layer && false)  // layer ignored for compatibility
			continue;
		if (is_point_in_rect(x, y, zone->bounds) && which-- == 0) {
			found_item = zone;
			if (out_index) *out_index = i;
			break;
		}
	}
	return found_item;
}

static bool
change_map(const char* filename, bool preserve_persons)
{
	// note: if an error is detected during a map change, change_map() will return false, but
	//       the map engine may be left in an inconsistent state. it is therefore probably wise
	//       to consider such a situation unrecoverable.
	
	struct map*        map;
	char*              path;
	person_t*          person;
	struct map_person* person_info;
	spriteset_t*       spriteset;

	int i;

	path = get_asset_path(filename, "maps", false);
	map = load_map(path);
	free(path);
	if (map == NULL) return false;
	if (s_map != NULL) {
		// run map exit scripts first, before loading new map
		run_script(s_def_scripts[MAP_SCRIPT_ON_LEAVE], false);
		run_script(s_map->scripts[MAP_SCRIPT_ON_LEAVE], false);
	}
	
	// close out old map and prep for new one
	free_map(s_map); free(s_map_filename);
	for (i = 0; i < s_num_delay_scripts; ++i)
		free_script(s_delay_scripts[i].script);
	s_num_delay_scripts = 0;
	s_map = map; s_map_filename = strdup(filename);
	reset_persons(preserve_persons);

	// populate persons
	for (i = 0; i < s_map->num_persons; ++i) {
		person_info = &s_map->persons[i];
		path = get_asset_path(lstring_cstr(person_info->spriteset), "spritesets", false);
		if (!(spriteset = load_spriteset(path)))
			goto on_error;
		free(path);
		if (!(person = create_person(person_info->name->cstr, spriteset, false, NULL)))
			goto on_error;
		set_person_xyz(person, person_info->x, person_info->y, person_info->z);
		compile_person_script(person, PERSON_SCRIPT_ON_CREATE, person_info->create_script);
		compile_person_script(person, PERSON_SCRIPT_ON_DESTROY, person_info->destroy_script);
		compile_person_script(person, PERSON_SCRIPT_ON_TOUCH, person_info->touch_script);
		compile_person_script(person, PERSON_SCRIPT_ON_TALK, person_info->talk_script);
		compile_person_script(person, PERSON_SCRIPT_GENERATOR, person_info->command_script);
		
		// normally this is handled by create_person(), but since in this case the
		// person-specific create script isn't compiled until after the person is created,
		// the map engine gets the responsibility.
		call_person_script(person, PERSON_SCRIPT_ON_CREATE, false);
	}

	// set camera over starting position
	s_cam_x = s_map->origin.x;
	s_cam_y = s_map->origin.y;

	// run map entry scripts
	run_script(s_def_scripts[MAP_SCRIPT_ON_ENTER], false);
	run_script(s_map->scripts[MAP_SCRIPT_ON_ENTER], false);

	s_frames = 0;
	return true;

on_error:
	free_map(s_map);
	return false;
}

static int
find_layer(const char* name)
{
	int i;

	for (i = 0; i < s_map->num_layers; ++i) {
		if (strcmp(name, s_map->layers[0].name->cstr) == 0)
			return i;
	}
	return -1;
}

static void
map_screen_to_layer(int layer, int camera_x, int camera_y, int* inout_x, int* inout_y)
{
	int   layer_w, layer_h;
	float plx_offset_x = 0.0, plx_offset_y = 0.0;
	int   tile_w, tile_h;
	int   x_offset, y_offset;
	
	// get layer dimensions
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	layer_w = s_map->layers[layer].width * tile_w;
	layer_h = s_map->layers[layer].height * tile_h;
	
	// remap screen coordinates to layer coordinates
	plx_offset_x = s_frames * s_map->layers[layer].autoscroll_x
		- camera_x * (s_map->layers[layer].parallax_x - 1.0);
	plx_offset_y = s_frames * s_map->layers[layer].autoscroll_y
		- camera_y * (s_map->layers[layer].parallax_y - 1.0);
	x_offset = camera_x - g_res_x / 2 - plx_offset_x;
	y_offset = camera_y - g_res_y / 2 - plx_offset_y;
	if (!s_map->is_repeating && !s_map->layers[layer].is_parallax) {
		// non-repeating map: clamp viewport to map bounds (windowbox if needed)
		x_offset = layer_w > g_res_x ? fmin(fmax(x_offset, 0), layer_w - g_res_x)
			: -(g_res_x - layer_w) / 2;
		y_offset = layer_h > g_res_y ? fmin(fmax(y_offset, 0), layer_h - g_res_y)
			: -(g_res_y - layer_h) / 2;
	}
	else {
		// repeating map or parallax layer: pre-wrap to layer dimensions
		// (simplifies rendering calculations)
		x_offset = (x_offset % layer_w + layer_w) % layer_w;
		y_offset = (y_offset % layer_h + layer_h) % layer_h;
	}
	if (inout_x) *inout_x += x_offset;
	if (inout_y) *inout_y += y_offset;
}

static void
process_map_input(void)
{
	ALLEGRO_KEYBOARD_STATE kb_state;
	int                    mv_x = 0, mv_y = 0;

	// clear out excess keys from key queue
	clear_key_queue();
	
	// check for player control of input person, if there is one
	if (s_input_person != NULL && !is_person_busy(s_input_person)) {
		al_get_keyboard_state(&kb_state);
		if (al_key_down(&kb_state, s_talk_key) || is_joy_button_down(0, s_talk_button)) {
			if (s_is_talk_allowed) talk_person(s_input_person);
			s_is_talk_allowed = false;
		}
		else // allow talking again only after key is released
			s_is_talk_allowed = true;
		if (al_key_down(&kb_state, ALLEGRO_KEY_UP)) mv_y = -1;
		if (al_key_down(&kb_state, ALLEGRO_KEY_RIGHT)) mv_x = 1;
		if (al_key_down(&kb_state, ALLEGRO_KEY_DOWN)) mv_y = 1;
		if (al_key_down(&kb_state, ALLEGRO_KEY_LEFT)) mv_x = -1;
		switch (mv_x + mv_y * 3) {
		case -3: // north
			queue_person_command(s_input_person, COMMAND_MOVE_NORTH, true);
			queue_person_command(s_input_person, COMMAND_FACE_NORTH, false);
			break;
		case -2: // northeast
			queue_person_command(s_input_person, COMMAND_MOVE_NORTH, true);
			queue_person_command(s_input_person, COMMAND_MOVE_EAST, true);
			queue_person_command(s_input_person, COMMAND_FACE_NORTHEAST, false);
			break;
		case 1: // east
			queue_person_command(s_input_person, COMMAND_MOVE_EAST, true);
			queue_person_command(s_input_person, COMMAND_FACE_EAST, false);
			break;
		case 4: // southeast
			queue_person_command(s_input_person, COMMAND_MOVE_SOUTH, true);
			queue_person_command(s_input_person, COMMAND_MOVE_EAST, true);
			queue_person_command(s_input_person, COMMAND_FACE_SOUTHEAST, false);
			break;
		case 3: // south
			queue_person_command(s_input_person, COMMAND_MOVE_SOUTH, true);
			queue_person_command(s_input_person, COMMAND_FACE_SOUTH, false);
			break;
		case 2: // southwest
			queue_person_command(s_input_person, COMMAND_MOVE_SOUTH, true);
			queue_person_command(s_input_person, COMMAND_MOVE_WEST, true);
			queue_person_command(s_input_person, COMMAND_FACE_SOUTHWEST, false);
			break;
		case -1: // west
			queue_person_command(s_input_person, COMMAND_MOVE_WEST, true);
			queue_person_command(s_input_person, COMMAND_FACE_WEST, false);
			break;
		case -4: // northwest
			queue_person_command(s_input_person, COMMAND_MOVE_NORTH, true);
			queue_person_command(s_input_person, COMMAND_MOVE_WEST, true);
			queue_person_command(s_input_person, COMMAND_FACE_NORTHWEST, false);
			break;
		}
	}
	
	update_bound_keys(true);
}

static void
render_map(void)
{
	int               cell_x, cell_y;
	int               first_cell_x, first_cell_y;
	bool              is_repeating;
	struct map_layer* layer;
	int               layer_w, layer_h;
	ALLEGRO_COLOR     overlay_color;
	int               tile_w, tile_h;
	int               off_x, off_y;
	int               tile_index;
	
	int x, y, z;
	
	if (is_skipped_frame())
		return;
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	for (z = 0; z < s_map->num_layers; ++z) {
		layer = &s_map->layers[z];
		if (!layer->is_visible)
			continue;
		is_repeating = s_map->is_repeating || layer->is_parallax;
		layer_w = layer->width * tile_w;
		layer_h = layer->height * tile_h;
		off_x = 0; off_y = 0;
		map_screen_to_layer(z, s_cam_x, s_cam_y, &off_x, &off_y);
		al_hold_bitmap_drawing(true);
		if (layer->is_reflective) {
			if (is_repeating) {
				// for small repeating maps, persons need to be repeated as well
				for (y = 0; y < g_res_y / layer_h + 2; ++y) for (x = 0; x < g_res_x / layer_w + 2; ++x)
					render_persons(z, true, off_x - x * layer_w, off_y - y * layer_h);
			}
			else {
				render_persons(z, true, off_x, off_y);
			}
		}
		first_cell_x = off_x / tile_w;
		first_cell_y = off_y / tile_h;
		for (y = 0; y < g_res_y / tile_h + 2; ++y) for (x = 0; x < g_res_x / tile_w + 2; ++x) {
			cell_x = is_repeating ? (x + first_cell_x) % layer->width : x + first_cell_x;
			cell_y = is_repeating ? (y + first_cell_y) % layer->height : y + first_cell_y;
			if (cell_x < 0 || cell_x >= layer->width || cell_y < 0 || cell_y >= layer->height)
				continue;
			tile_index = layer->tilemap[cell_x + cell_y * layer->width].tile_index;
			draw_tile(s_map->tileset, layer->color_mask, x * tile_w - off_x % tile_w, y * tile_h - off_y % tile_h, tile_index);
		}
		if (is_repeating) {
			// for small repeating maps, persons need to be repeated as well
			for (y = 0; y < g_res_y / layer_h + 2; ++y) for (x = 0; x < g_res_x / layer_w + 2; ++x)
				render_persons(z, false, off_x - x * layer_w, off_y - y * layer_h);
		}
		else {
			render_persons(z, false, off_x, off_y);
		}
		al_hold_bitmap_drawing(false);
		run_script(layer->render_script, false);
	}
	overlay_color = al_map_rgba(s_color_mask.r, s_color_mask.g, s_color_mask.b, s_color_mask.alpha);
	al_draw_filled_rectangle(0, 0, g_res_x, g_res_y, overlay_color);
	run_script(s_render_script, false);
}

static void
update_map_engine(bool is_main_loop)
{
	int                 index;
	int                 last_trigger;
	int                 last_zone;
	int                 layer;
	int                 map_w, map_h;
	int                 script_type;
	int                 tile_w, tile_h;
	struct map_trigger* trigger;
	double              x, y;
	struct map_zone*    zone;

	int i, j;
	
	++s_frames;
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	map_w = s_map->width * tile_w;
	map_h = s_map->height * tile_h;
	
	update_persons();
	animate_tileset(s_map->tileset);

	// update color mask fade level
	if (s_fade_progress < s_fade_frames) {
		++s_fade_progress;
		s_color_mask = blend_colors(s_fade_color_to, s_fade_color_from,
			s_fade_progress, s_fade_frames - s_fade_progress);
	}
	
	// update camera
	if (s_camera_person != NULL) {
		get_person_xy(s_camera_person, &x, &y, true);
		s_cam_x = x; s_cam_y = y;
	}

	// run edge script if the camera has moved past the edge of the map
	// note: only applies for non-repeating maps
	if (is_main_loop && !s_map->is_repeating) {
		script_type = s_cam_y < 0 ? MAP_SCRIPT_ON_LEAVE_NORTH
			: s_cam_x >= map_w ? MAP_SCRIPT_ON_LEAVE_EAST
			: s_cam_y >= map_h ? MAP_SCRIPT_ON_LEAVE_SOUTH
			: s_cam_x < 0 ? MAP_SCRIPT_ON_LEAVE_WEST
			: MAP_SCRIPT_MAX;
		if (script_type < MAP_SCRIPT_MAX) {
			run_script(s_def_scripts[script_type], false);
			run_script(s_map->scripts[script_type], false);
		}
	}

	// if the player moved the input person, process zones and triggers
	if (s_input_person != NULL && has_person_moved(s_input_person)) {
		// did we step on a trigger or move to a new one?
		get_person_xyz(s_input_person, &x, &y, &layer, true);
		trigger = get_trigger_at(x, y, layer, &index);
		if (trigger != s_on_trigger) {
			last_trigger = s_current_trigger;
			s_current_trigger = index;
			s_on_trigger = trigger;
			if (trigger) run_script(trigger->script, false);
			s_current_trigger = last_trigger;
		}

		// update any occupied zones
		i = 0;
		while (zone = get_zone_at(x, y, layer, i++, &index)) {
			if (zone->steps_left-- <= 0) {
				last_zone = s_current_zone;
				s_current_zone = index;
				zone->steps_left = zone->step_interval;
				run_script(zone->script, true);
				s_current_zone = last_zone;
			}
		}
	}
	
	run_script(s_update_script, false);
	
	// run delay scripts, if applicable
	for (i = 0; i < s_num_delay_scripts; ++i) {
		if (s_delay_scripts[i].frames_left-- <= 0) {
			run_script(s_delay_scripts[i].script, false);
			free_script(s_delay_scripts[i].script);
			for (j = i; j < s_num_delay_scripts - 1; ++j)
				s_delay_scripts[j] = s_delay_scripts[j + 1];
			--s_num_delay_scripts; --i;
		}
	}
}

void
init_map_engine_api(duk_context* ctx)
{
	register_api_function(ctx, NULL, "MapEngine", js_MapEngine);
	register_api_function(ctx, NULL, "AreZonesAt", js_AreZonesAt);
	register_api_function(ctx, NULL, "IsCameraAttached", js_IsCameraAttached);
	register_api_function(ctx, NULL, "IsInputAttached", js_IsInputAttached);
	register_api_function(ctx, NULL, "IsLayerReflective", js_IsLayerReflective);
	register_api_function(ctx, NULL, "IsLayerVisible", js_IsLayerVisible);
	register_api_function(ctx, NULL, "IsMapEngineRunning", js_IsMapEngineRunning);
	register_api_function(ctx, NULL, "IsTriggerAt", js_IsTriggerAt);
	register_api_function(ctx, NULL, "GetCameraPerson", js_GetCameraPerson);
	register_api_function(ctx, NULL, "GetCameraX", js_GetCameraX);
	register_api_function(ctx, NULL, "GetCameraY", js_GetCameraY);
	register_api_function(ctx, NULL, "GetCurrentMap", js_GetCurrentMap);
	register_api_function(ctx, NULL, "GetCurrentTrigger", js_GetCurrentTrigger);
	register_api_function(ctx, NULL, "GetCurrentZone", js_GetCurrentZone);
	register_api_function(ctx, NULL, "GetInputPerson", js_GetInputPerson);
	register_api_function(ctx, NULL, "GetLayerHeight", js_GetLayerHeight);
	register_api_function(ctx, NULL, "GetLayerMask", js_GetLayerMask);
	register_api_function(ctx, NULL, "GetLayerWidth", js_GetLayerWidth);
	register_api_function(ctx, NULL, "GetMapEngineFrameRate", js_GetMapEngineFrameRate);
	register_api_function(ctx, NULL, "GetNextAnimatedTile", js_GetNextAnimatedTile);
	register_api_function(ctx, NULL, "GetNumLayers", js_GetNumLayers);
	register_api_function(ctx, NULL, "GetNumTiles", js_GetNumTiles);
	register_api_function(ctx, NULL, "GetNumTriggers", js_GetNumTriggers);
	register_api_function(ctx, NULL, "GetNumZones", js_GetNumZones);
	register_api_function(ctx, NULL, "GetTalkActivationButton", js_GetTalkActivationButton);
	register_api_function(ctx, NULL, "GetTalkActivationKey", js_GetTalkActivationKey);
	register_api_function(ctx, NULL, "GetTile", js_GetTile);
	register_api_function(ctx, NULL, "GetTileDelay", js_GetTileDelay);
	register_api_function(ctx, NULL, "GetTileImage", js_GetTileImage);
	register_api_function(ctx, NULL, "GetTileHeight", js_GetTileHeight);
	register_api_function(ctx, NULL, "GetTileName", js_GetTileName);
	register_api_function(ctx, NULL, "GetTileSurface", js_GetTileSurface);
	register_api_function(ctx, NULL, "GetTileWidth", js_GetTileWidth);
	register_api_function(ctx, NULL, "GetZoneHeight", js_GetZoneHeight);
	register_api_function(ctx, NULL, "GetZoneLayer", js_GetZoneLayer);
	register_api_function(ctx, NULL, "GetZoneWidth", js_GetZoneWidth);
	register_api_function(ctx, NULL, "GetZoneX", js_GetZoneX);
	register_api_function(ctx, NULL, "GetZoneY", js_GetZoneY);
	register_api_function(ctx, NULL, "SetCameraX", js_SetCameraX);
	register_api_function(ctx, NULL, "SetCameraY", js_SetCameraY);
	register_api_function(ctx, NULL, "SetColorMask", js_SetColorMask);
	register_api_function(ctx, NULL, "SetDefaultMapScript", js_SetDefaultMapScript);
	register_api_function(ctx, NULL, "SetLayerMask", js_SetLayerMask);
	register_api_function(ctx, NULL, "SetLayerReflective", js_SetLayerReflective);
	register_api_function(ctx, NULL, "SetLayerRenderer", js_SetLayerRenderer);
	register_api_function(ctx, NULL, "SetLayerVisible", js_SetLayerVisible);
	register_api_function(ctx, NULL, "SetMapEngineFrameRate", js_SetMapEngineFrameRate);
	register_api_function(ctx, NULL, "SetNextAnimatedTile", js_SetNextAnimatedTile);
	register_api_function(ctx, NULL, "SetRenderScript", js_SetRenderScript);
	register_api_function(ctx, NULL, "SetTalkActivationButton", js_SetTalkActivationButton);
	register_api_function(ctx, NULL, "SetTalkActivationKey", js_SetTalkActivationKey);
	register_api_function(ctx, NULL, "SetTile", js_SetTile);
	register_api_function(ctx, NULL, "SetTileDelay", js_SetTileDelay);
	register_api_function(ctx, NULL, "SetTileImage", js_SetTileImage);
	register_api_function(ctx, NULL, "SetTileName", js_SetTileName);
	register_api_function(ctx, NULL, "SetTileSurface", js_SetTileSurface);
	register_api_function(ctx, NULL, "SetUpdateScript", js_SetUpdateScript);
	register_api_function(ctx, NULL, "SetZoneLayer", js_SetZoneLayer);
	register_api_function(ctx, NULL, "AttachCamera", js_AttachCamera);
	register_api_function(ctx, NULL, "AttachInput", js_AttachInput);
	register_api_function(ctx, NULL, "CallDefaultMapScript", js_CallDefaultMapScript);
	register_api_function(ctx, NULL, "CallMapScript", js_CallMapScript);
	register_api_function(ctx, NULL, "ChangeMap", js_ChangeMap);
	register_api_function(ctx, NULL, "DetachCamera", js_DetachCamera);
	register_api_function(ctx, NULL, "DetachInput", js_DetachInput);
	register_api_function(ctx, NULL, "ExecuteTrigger", js_ExecuteTrigger);
	register_api_function(ctx, NULL, "ExecuteZones", js_ExecuteZones);
	register_api_function(ctx, NULL, "ExitMapEngine", js_ExitMapEngine);
	register_api_function(ctx, NULL, "MapToScreenX", js_MapToScreenX);
	register_api_function(ctx, NULL, "MapToScreenY", js_MapToScreenY);
	register_api_function(ctx, NULL, "ReplaceTilesOnLayer", js_ReplaceTilesOnLayer);
	register_api_function(ctx, NULL, "RenderMap", js_RenderMap);
	register_api_function(ctx, NULL, "ScreenToMapX", js_ScreenToMapX);
	register_api_function(ctx, NULL, "ScreenToMapY", js_ScreenToMapY);
	register_api_function(ctx, NULL, "SetDelayScript", js_SetDelayScript);
	register_api_function(ctx, NULL, "UpdateMapEngine", js_UpdateMapEngine);

	// Map script types
	register_api_const(ctx, "SCRIPT_ON_ENTER_MAP", MAP_SCRIPT_ON_ENTER);
	register_api_const(ctx, "SCRIPT_ON_LEAVE_MAP", MAP_SCRIPT_ON_LEAVE);
	register_api_const(ctx, "SCRIPT_ON_LEAVE_MAP_NORTH", MAP_SCRIPT_ON_LEAVE_NORTH);
	register_api_const(ctx, "SCRIPT_ON_LEAVE_MAP_EAST", MAP_SCRIPT_ON_LEAVE_EAST);
	register_api_const(ctx, "SCRIPT_ON_LEAVE_MAP_SOUTH", MAP_SCRIPT_ON_LEAVE_SOUTH);
	register_api_const(ctx, "SCRIPT_ON_LEAVE_MAP_WEST", MAP_SCRIPT_ON_LEAVE_WEST);

	// initialize subcomponent APIs (persons, etc.)
	init_persons_api();
}

int
duk_require_map_layer(duk_context* ctx, duk_idx_t index)
{
	int         layer;
	const char* name;

	duk_require_type_mask(ctx, index, DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER);
	if (duk_is_number(ctx, index))
		return duk_get_int(ctx, index);
	else {
		name = duk_get_string(ctx, index);
		if ((layer = find_layer(name)) == -1)
			duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "layer named '%s' doesn't exist", name);
		return layer;
	}
}

static duk_ret_t
js_MapEngine(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	int framerate = duk_require_int(ctx, 1);
	
	s_is_map_running = true;
	s_exiting = false;
	s_color_mask = rgba(0, 0, 0, 0);
	s_fade_color_to = s_fade_color_from = s_color_mask;
	s_fade_progress = s_fade_frames = 0;
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	s_framerate = framerate;
	if (!change_map(filename, true))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "MapEngine(): Failed to load map file '%s' into map engine", filename);
	while (!s_exiting) {
		update_map_engine(true);
		process_map_input();
		render_map();
		flip_screen(s_framerate);
	}
	reset_persons(false);
	s_is_map_running = false;
	return 0;
}

static duk_ret_t
js_AreZonesAt(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);

	if (layer < 0 || layer >= s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "AreZonesAt(): Invalid layer index (%i)", layer);
	duk_push_boolean(ctx, are_zones_at(x, y, layer, NULL));
	return 1;
}

static duk_ret_t
js_IsCameraAttached(duk_context* ctx)
{
	duk_push_boolean(ctx, s_camera_person != NULL);
	return 1;
}

static duk_ret_t
js_IsInputAttached(duk_context* ctx)
{
	duk_push_boolean(ctx, s_input_person != NULL);
	return 1;
}

static duk_ret_t
js_IsLayerReflective(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IsLayerReflective(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IsLayerReflective(): Invalid layer index (%i)", layer);
	duk_push_boolean(ctx, s_map->layers[layer].is_reflective);
	return 1;
}

static duk_ret_t
js_IsLayerVisible(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IsLayerVisible(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IsLayerVisible(): Invalid layer index (%i)", layer);
	duk_push_boolean(ctx, s_map->layers[layer].is_visible);
	return 1;
}

static duk_ret_t
js_IsMapEngineRunning(duk_context* ctx)
{
	duk_push_boolean(ctx, s_is_map_running);
	return 1;
}

static duk_ret_t
js_IsTriggerAt(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);
	
	if (layer < 0 || layer >= s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "IsTriggerAt(): Invalid layer index (%i)", layer);
	duk_push_boolean(ctx, get_trigger_at(x, y, layer, NULL) != NULL);
	return 1;
}

static duk_ret_t
js_GetCameraPerson(duk_context* ctx)
{
	if (s_camera_person == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCameraPerson(): Invalid operation, camera not attached");
	duk_push_string(ctx, get_person_name(s_camera_person));
	return 1;
}

static duk_ret_t
js_GetCameraX(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCameraX(): Map engine must be running");
	duk_push_int(ctx, s_cam_x);
	return 1;
}

static duk_ret_t
js_GetCameraY(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCameraX(): Map engine must be running");
	duk_push_int(ctx, s_cam_y);
	return 1;
}

static duk_ret_t
js_GetCurrentMap(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentMap(): Map engine not running");
	duk_push_string(ctx, s_map_filename);
	return 1;
}

static duk_ret_t
js_GetCurrentTrigger(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentTrigger(): Map engine not running");
	if (s_current_trigger == -1)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentTrigger(): Cannot be called outside of a trigger script");
	duk_push_int(ctx, s_current_trigger);
	return 1;
}

static duk_ret_t
js_GetCurrentZone(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentZone(): Map engine not running");
	if (s_current_zone == -1)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentZone(): Cannot be called outside of a zone script");
	duk_push_int(ctx, s_current_zone);
	return 1;
}

static duk_ret_t
js_GetInputPerson(duk_context* ctx)
{
	if (s_input_person == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetInputPerson(): Invalid operation, input not attached");
	duk_push_string(ctx, get_person_name(s_input_person));
	return 1;
}

static duk_ret_t
js_GetLayerHeight(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerHeight(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerHeight(): Invalid layer index (%i)", layer);
	duk_push_int(ctx, s_map->layers[layer].height);
	return 1;
}

static duk_ret_t
js_GetLayerMask(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerMask(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerMask(): Invalid layer index (%i)", layer);
	duk_push_sphere_color(ctx, s_map->layers[layer].color_mask);
	return 1;
}

static duk_ret_t
js_GetLayerName(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerName(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerName(): Invalid layer index (%i)", layer);
	duk_push_lstring(ctx, s_map->layers[layer].name->cstr, s_map->layers[layer].name->length);
	return 1;
}

static duk_ret_t
js_GetLayerWidth(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerWidth(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerWidth(): Invalid layer index (%i)", layer);
	duk_push_int(ctx, s_map->layers[layer].width);
	return 1;
}

static duk_ret_t
js_GetMapEngineFrameRate(duk_context* ctx)
{
	duk_push_int(ctx, s_framerate);
	return 1;
}

static duk_ret_t
js_GetNextAnimatedTile(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetNextAnimatedTile(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetNextAnimatedTile(): Invalid tile index (%i)", tile_index);
	duk_push_int(ctx, get_next_tile(s_map->tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetNumLayers(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetNumLayers(): Map engine must be running");
	duk_push_int(ctx, s_map->num_layers);
	return 1;
}

static duk_ret_t
js_GetNumTiles(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetNumTiles(): Map engine must be running");
	duk_push_int(ctx, get_tile_count(s_map->tileset));
	return 1;
}

static duk_ret_t
js_GetNumTriggers(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetNumTriggers(): Map engine must be running");
	duk_push_int(ctx, s_map->num_triggers);
	return 1;
}

static duk_ret_t
js_GetNumZones(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetNumZones(): Map engine must be running");
	duk_push_int(ctx, s_map->num_zones);
	return 1;
}

static duk_ret_t
js_GetTalkActivationButton(duk_context* ctx)
{
	duk_push_int(ctx, s_talk_button);
	return 1;
}

static duk_ret_t
js_GetTalkActivationKey(duk_context* ctx)
{
	duk_push_int(ctx, s_talk_key);
	return 1;
}

static duk_ret_t
js_GetTile(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);

	int layer_w, layer_h;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTile(): Map engine must be running");
	if (layer < 0 || layer >= s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTile(): Invalid layer index (caller passed: %i)", layer);
	layer_w = s_map->layers[layer].width;
	layer_h = s_map->layers[layer].height;
	struct map_tile* tilemap = s_map->layers[layer].tilemap;
	duk_push_int(ctx, tilemap[x + y * layer_w].tile_index);
	return 1;
}

static duk_ret_t
js_GetTileDelay(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileDelay(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTileDelay(): Invalid tile index (%i)", tile_index);
	duk_push_int(ctx, get_tile_delay(s_map->tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetTileHeight(duk_context* ctx)
{
	int w, h;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileHeight(): Map engine must be running");
	get_tile_size(s_map->tileset, &w, &h);
	duk_push_int(ctx, h);
	return 1;
}

static duk_ret_t
js_GetTileImage(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileImage(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTileImage(): Tile index out of range (%i)", tile_index);
	duk_push_sphere_image(ctx, get_tile_image(s_map->tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetTileName(duk_context* ctx)
{
	int        tile_index = duk_require_int(ctx, 0);
	
	const lstring_t* tile_name;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileName(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTileName(): Tile index out of range (%i)", tile_index);
	tile_name = get_tile_name(s_map->tileset, tile_index);
	duk_push_lstring(ctx, tile_name->cstr, tile_name->length);
	return 1;
}

static duk_ret_t
js_GetTileSurface(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);

	image_t* image;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileSurface(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTileSurface(): Tile index out of range (%i)", tile_index);
	if ((image = clone_image(get_tile_image(s_map->tileset, tile_index))) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileSurface(): Failed to create new surface image");
	duk_push_sphere_surface(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_GetTileWidth(duk_context* ctx)
{
	int w, h;
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileWidth(): Map engine must be running");
	get_tile_size(s_map->tileset, &w, &h);
	duk_push_int(ctx, w);
	return 1;
}

static duk_ret_t
js_GetZoneHeight(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneHeight(): Map engine must be running");
	if (zone_index < 0 || zone_index >= s_map->num_zones)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneHeight(): Invalid zone index (%i)", zone_index);
	duk_push_int(ctx, s_map->zones[zone_index].bounds.y2 - s_map->zones[zone_index].bounds.y1);
	return 1;
}

static duk_ret_t
js_GetZoneLayer(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneLayer(): Map engine must be running");
	if (zone_index < 0 || zone_index >= s_map->num_zones)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneLayer(): Invalid zone index (%i)", zone_index);
	duk_push_int(ctx, s_map->zones[zone_index].layer);
	return 1;
}

static duk_ret_t
js_GetZoneWidth(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneWidth(): Map engine must be running");
	if (zone_index < 0 || zone_index >= s_map->num_zones)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneWidth(): Invalid zone index (%i)", zone_index);
	duk_push_int(ctx, s_map->zones[zone_index].bounds.x2 - s_map->zones[zone_index].bounds.x1);
	return 1;
}

static duk_ret_t
js_GetZoneX(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneX(): Map engine must be running");
	if (zone_index < 0 || zone_index >= s_map->num_zones)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneX(): Invalid zone index (%i)", zone_index);
	duk_push_int(ctx, s_map->zones[zone_index].bounds.x1);
	return 1;
}

static duk_ret_t
js_GetZoneY(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneY(): Map engine must be running");
	if (zone_index < 0 || zone_index >= s_map->num_zones)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneY(): Invalid zone index (%i)", zone_index);
	duk_push_int(ctx, s_map->zones[zone_index].bounds.y1);
	return 1;
}

static duk_ret_t
js_SetCameraX(duk_context* ctx)
{
	int new_x = duk_require_int(ctx, 0);
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetCameraX(): Map engine must be running");
	s_cam_x = new_x;
	return 0;
}

static duk_ret_t
js_SetCameraY(duk_context* ctx)
{
	int new_y = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetCameraY(): Map engine must be running");
	s_cam_y = new_y;
	return 0;
}

static duk_ret_t
js_SetColorMask(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	color_t new_mask = duk_require_sphere_color(ctx, 0);
	int frames = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetColorMask(): Map engine must be running");
	if (frames < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetColorMask(): Frame count cannot be negative");
	if (frames > 0) {
		s_fade_color_to = new_mask;
		s_fade_color_from = s_color_mask;
		s_fade_frames = frames;
		s_fade_progress = 0;
	}
	else {
		s_color_mask = new_mask;
		s_fade_color_to = s_fade_color_from = new_mask;
		s_fade_progress = s_fade_frames = 0;
	}
	return 0;
}

static duk_ret_t
js_SetDefaultMapScript(duk_context* ctx)
{
	int script_type = duk_require_int(ctx, 0);
	const char* script_name = (script_type == MAP_SCRIPT_ON_ENTER) ? "[default map enter script]"
		: (script_type == MAP_SCRIPT_ON_LEAVE) ? "[default map leave script]"
		: (script_type == MAP_SCRIPT_ON_LEAVE_NORTH) ? "[default map leave-north script]"
		: (script_type == MAP_SCRIPT_ON_LEAVE_EAST) ? "[default map leave-east script]"
		: (script_type == MAP_SCRIPT_ON_LEAVE_SOUTH) ? "[default map leave-south script]"
		: (script_type == MAP_SCRIPT_ON_LEAVE_WEST) ? "[default map leave-west script]"
		: NULL;
	script_t* script = duk_require_sphere_script(ctx, 1, script_name);

	if (script_type < 0 || script_type >= MAP_SCRIPT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetDefaultMapScript(): Invalid map script constant");
	free_script(s_def_scripts[script_type]);
	s_def_scripts[script_type] = script;
	return 0;
}

static duk_ret_t
js_SetDelayScript(duk_context* ctx)
{
	int frames = duk_require_int(ctx, 0);
	script_t* script = duk_require_sphere_script(ctx, 1, "[delay script]");

	struct delay_script* delay;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetDelayScript(): Map engine is not running");
	if (frames < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetDelayScript(): Delay frames cannot be negative");
	if (++s_num_delay_scripts > s_max_delay_scripts) {
		s_max_delay_scripts = s_num_delay_scripts * 2;
		if (!(s_delay_scripts = realloc(s_delay_scripts, s_max_delay_scripts * sizeof(struct delay_script))))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetDelayScript(): Failed to enlarge delay script queue");
	}
	delay = &s_delay_scripts[s_num_delay_scripts - 1];
	delay->script = script;
	delay->frames_left = frames;
	return 0;
}

static duk_ret_t
js_SetLayerMask(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	color_t mask = duk_require_sphere_color(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerMask(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerMask(): Invalid layer index (%i)", layer);
	s_map->layers[layer].color_mask = mask;
	return 0;
}

static duk_ret_t
js_SetLayerReflective(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	bool is_reflective = duk_require_boolean(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerReflective(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerReflective(): Invalid layer index (%i)", layer);
	s_map->layers[layer].is_reflective = is_reflective;
	return 0;
}

static duk_ret_t
js_SetLayerRenderer(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	char script_name[50]; sprintf(script_name, "[layer %i render script]", layer);
	script_t* script = duk_require_sphere_script(ctx, 1, script_name);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerRenderer(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerRenderer(): Invalid layer index (%i)", layer);
	free_script(s_map->layers[layer].render_script);
	s_map->layers[layer].render_script = script;
	return 0;
}

static duk_ret_t
js_SetLayerVisible(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	bool is_visible = duk_require_boolean(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerVisible(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerVisible(): Invalid layer index (%i)", layer);
	s_map->layers[layer].is_visible = is_visible;
	return 0;
}

static duk_ret_t
js_SetMapEngineFrameRate(duk_context* ctx)
{
	s_framerate = duk_to_int(ctx, 0);
	return 0;
}

static duk_ret_t
js_SetNextAnimatedTile(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);
	int next_index = duk_require_int(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetNextAnimatedTile(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetNextAnimatedTile(): Invalid tile index (%i)", tile_index);
	if (next_index < 0 || next_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetNextAnimatedTile(): Invalid tile index for next tile (%i)", tile_index);
	set_next_tile(s_map->tileset, tile_index, next_index);
	return 0;
}

static duk_ret_t
js_SetRenderScript(duk_context* ctx)
{
	script_t* script = duk_require_sphere_script(ctx, 0, "[render script]");

	free_script(s_render_script);
	s_render_script = script;
	return 0;
}

static duk_ret_t
js_SetTalkActivationButton(duk_context* ctx)
{
	int button = duk_require_int(ctx, 0);

	s_talk_button = button;
	return 0;
}

static duk_ret_t
js_SetTalkActivationKey(duk_context* ctx)
{
	int key = duk_require_int(ctx, 0);
	
	s_talk_key = key;
	return 0;
}

static duk_ret_t
js_SetTile(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);
	int tile_index = duk_require_int(ctx, 3);

	int layer_w, layer_h;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTile(): Map engine must be running");
	if (layer < 0 || layer >= s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTile(): Invalid layer index (caller passed: %i)", layer);
	layer_w = s_map->layers[layer].width;
	layer_h = s_map->layers[layer].height;
	struct map_tile* tilemap = s_map->layers[layer].tilemap;
	tilemap[x + y * layer_w].tile_index = tile_index;
	tilemap[x + y * layer_w].frames_left = get_tile_delay(s_map->tileset, tile_index);
	return 0;
}

static duk_ret_t
js_SetTileDelay(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);
	int delay = duk_require_int(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTileDelay(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTileDelay(): Invalid tile index (%i)", tile_index);
	if (delay < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTileDelay(): Delay cannot be negative (%i)", delay);
	set_tile_delay(s_map->tileset, tile_index, delay);
	return 0;
}

static duk_ret_t
js_SetTileImage(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);
	image_t* image = duk_require_sphere_image(ctx, 1);

	int c_tiles;
	int image_w, image_h;
	int tile_w, tile_h;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTileImage(): Map engine must be running");
	c_tiles = get_tile_count(s_map->tileset);
	if (tile_index < 0 || tile_index >= c_tiles)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTileImage(): Tile index out of range (%i)", tile_index);
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	image_w = get_image_width(image);
	image_h = get_image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "SetTileImage(): Image dimensions (%ix%i) don't match tile dimensions (%ix%i)", image_w, image_h, tile_w, tile_h);
	set_tile_image(s_map->tileset, tile_index, image);
	return 0;
}

static duk_ret_t
js_SetTileName(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);
	lstring_t* name = duk_require_lstring_t(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTileName(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTileName(): Tile index out of range (%i)", tile_index);
	if (!set_tile_name(s_map->tileset, tile_index, name))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTileName(): Failed to set tile name");
	free_lstring(name);
	return 0;
}

static duk_ret_t
js_SetTileSurface(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);
	image_t* image = duk_require_sphere_surface(ctx, 1);

	int      c_tiles;
	int      image_w, image_h;
	image_t* new_image;
	int      tile_w, tile_h;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTileSurface(): Map engine must be running");
	c_tiles = get_tile_count(s_map->tileset);
	if (tile_index < 0 || tile_index >= c_tiles)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTileSurface(): Tile index out of range (%i)", tile_index);
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	image_w = get_image_width(image);
	image_h = get_image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "SetTileSurface(): Surface dimensions (%ix%i) don't match tile dimensions (%ix%i)", image_w, image_h, tile_w, tile_h);
	if ((new_image = clone_image(image)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTileSurface(): Failed to create new tile image");
	set_tile_image(s_map->tileset, tile_index, new_image);
	free_image(new_image);
	return 0;
}

static duk_ret_t
js_SetUpdateScript(duk_context* ctx)
{
	script_t* script = duk_require_sphere_script(ctx, 0, "[render script]");

	free_script(s_update_script);
	s_update_script = script;
	return 0;
}

static duk_ret_t
js_SetZoneLayer(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);
	int layer = duk_require_map_layer(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetZoneLayer(): Map engine must be running");
	if (zone_index < 0 || zone_index >= s_map->num_zones)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetZoneLayer(): Invalid zone index (%i)", zone_index);
	if (layer < 0 || layer >= s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetZoneLayer(): Invalid layer index (%i)", layer);
	s_map->zones[zone_index].layer = layer;
	return 0;
}

static duk_ret_t
js_AttachCamera(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "AttachCamera(): Person '%s' doesn't exist", name);
	s_camera_person = person;
	return 0;
}

static duk_ret_t
js_AttachInput(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "AttachInput(): Person '%s' doesn't exist", name);
	s_input_person = person;
	return 0;
}

static duk_ret_t
js_CallDefaultMapScript(duk_context* ctx)
{
	int type = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CallDefaultMapScript(): Map engine is not running");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "CallDefaultMapScript(): Invalid script type constant");
	run_script(s_def_scripts[type], false);
	return 0;
}

static duk_ret_t
js_CallMapScript(duk_context* ctx)
{
	int type = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CallMapScript(): Map engine is not running");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "CallMapScript(): Invalid script type constant");
	run_script(s_map->scripts[type], false);
	return 0;
}

static duk_ret_t
js_ChangeMap(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ChangeMap(): Map engine is not running");
	if (!change_map(filename, false))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ChangeMap(): Failed to load map file '%s' into map engine", filename);
	return 0;
}

static duk_ret_t
js_DetachCamera(duk_context* ctx)
{
	s_camera_person = NULL;
	return 0;
}

static duk_ret_t
js_DetachInput(duk_context* ctx)
{
	s_input_person = NULL;
	return 0;
}

static duk_ret_t
js_ExecuteTrigger(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);
	
	int                 index;
	int                 last_trigger;
	struct map_trigger* trigger;
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ExecuteTrigger(): Map engine is not running");
	if (trigger = get_trigger_at(x, y, layer, &index)) {
		last_trigger = s_current_trigger;
		s_current_trigger = index;
		run_script(trigger->script, true);
		s_current_trigger = last_trigger;
	}
	return 0;
}

static duk_ret_t
js_ExecuteZones(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);

	int              index;
	int              last_zone;
	struct map_zone* zone;

	int i;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ExecuteZones(): Map engine is not running");
	i = 0;
	while (zone = get_zone_at(x, y, layer, i++, &index)) {
		last_zone = s_current_zone;
		s_current_zone = index;
		run_script(zone->script, true);
		s_current_zone = last_zone;
	}
	return 0;
}

static duk_ret_t
js_ExitMapEngine(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ExitMapEngine(): Map engine is not running");
	s_exiting = true;
	return 0;
}

static duk_ret_t
js_MapToScreenX(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	double x = duk_require_int(ctx, 1);

	int offset_x;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "MapToScreenX(): Map engine is not running");
	if (layer < 0 || layer >= s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "MapToScreenX(): Layer index out of range (%i)", layer);
	normalize_map_entity_xy(&x, NULL, layer);
	offset_x = 0;
	map_screen_to_layer(layer, s_cam_x, s_cam_y, &offset_x, NULL);
	duk_push_int(ctx, x - offset_x);
	return 1;
}

static duk_ret_t
js_MapToScreenY(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	double y = duk_require_int(ctx, 1);

	int offset_y;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "MapToScreenY(): Map engine is not running");
	if (layer < 0 || layer >= s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "MapToScreenY(): Layer index out of range (%i)", layer);
	normalize_map_entity_xy(NULL, &y, layer);
	offset_y = 0;
	map_screen_to_layer(layer, s_cam_x, s_cam_y, NULL, &offset_y);
	duk_push_int(ctx, y - offset_y);
	return 1;
}

static duk_ret_t
js_RenderMap(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RenderMap(): Map engine is not running");
	render_map();
	return 0;
}

static duk_ret_t
js_ReplaceTilesOnLayer(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	int old_index = duk_require_int(ctx, 1);
	int new_index = duk_require_int(ctx, 2);

	int              layer_w, layer_h;
	struct map_tile* p_tile;

	int i_x, i_y;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ReplaceTilesOnLayer(): Map engine is not running");
	if (old_index < 0 || old_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ReplaceTilesOnLayer(): Old tile index out of range (%i)", old_index);
	if (new_index < 0 || new_index >= get_tile_count(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ReplaceTilesOnLayer(): New tile index out of range (%i)", new_index);
	layer_w = s_map->layers[layer].width;
	layer_h = s_map->layers[layer].height;
	for (i_x = 0; i_x < layer_w; ++i_x) for (i_y = 0; i_y < layer_h; ++i_y) {
		p_tile = &s_map->layers[layer].tilemap[i_x + i_y * layer_w];
		if (p_tile->tile_index == old_index) p_tile->tile_index = new_index;
	}
	return 0;
}

static duk_ret_t
js_ScreenToMapX(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	int x = duk_require_int(ctx, 1);
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ScreenToMapX(): Map engine is not running");
	if (layer < 0 || layer >= s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ScreenToMapX(): Layer index out of range (%i)", layer);
	map_screen_to_layer(layer, s_cam_x, s_cam_y, &x, NULL);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_ScreenToMapY(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	int y = duk_require_int(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ScreenToMapY(): Map engine is not running");
	if (layer < 0 || layer >= s_map->num_layers)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ScreenToMapY(): Layer index out of range (%i)", layer);
	map_screen_to_layer(layer, s_cam_x, s_cam_y, NULL, &y);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_UpdateMapEngine(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "UpdateMapEngine(): Operation requires the map engine to be running");
	update_map_engine(false);
	return 0;
}
