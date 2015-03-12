#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "image.h"
#include "input.h"
#include "obsmap.h"
#include "persons.h"
#include "surface.h"
#include "tileset.h"

#include "map_engine.h"

bool g_map_running = false;

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

static bool                are_zones_at      (int x, int y, int layer, int* out_count);
static struct map_trigger* get_trigger_at    (int x, int y, int layer);
static struct map_zone*    get_zone_at       (int x, int y, int layer, int index);
static bool                change_map        (const char* filename, bool preserve_persons);
static void                process_map_input (void);
static void                render_map_engine (void);
static void                update_map_engine (void);

static duk_ret_t js_MapEngine             (duk_context* ctx);
static duk_ret_t js_AreZonesAt            (duk_context* ctx);
static duk_ret_t js_IsCameraAttached      (duk_context* ctx);
static duk_ret_t js_IsInputAttached       (duk_context* ctx);
static duk_ret_t js_IsMapEngineRunning    (duk_context* ctx);
static duk_ret_t js_IsTriggerAt           (duk_context* ctx);
static duk_ret_t js_GetCameraPerson       (duk_context* ctx);
static duk_ret_t js_GetCameraX            (duk_context* ctx);
static duk_ret_t js_GetCameraY            (duk_context* ctx);
static duk_ret_t js_GetCurrentMap         (duk_context* ctx);
static duk_ret_t js_GetCurrentTrigger     (duk_context* ctx);
static duk_ret_t js_GetCurrentZone        (duk_context* ctx);
static duk_ret_t js_GetInputPerson        (duk_context* ctx);
static duk_ret_t js_GetLayerHeight        (duk_context* ctx);
static duk_ret_t js_GetLayerMask          (duk_context* ctx);
static duk_ret_t js_GetLayerWidth         (duk_context* ctx);
static duk_ret_t js_GetMapEngineFrameRate (duk_context* ctx);
static duk_ret_t js_GetNextAnimatedTile   (duk_context* ctx);
static duk_ret_t js_GetNumLayers          (duk_context* ctx);
static duk_ret_t js_GetNumTiles           (duk_context* ctx);
static duk_ret_t js_GetNumTriggers        (duk_context* ctx);
static duk_ret_t js_GetNumZones           (duk_context* ctx);
static duk_ret_t js_GetTalkActivationKey  (duk_context* ctx);
static duk_ret_t js_GetTile               (duk_context* ctx);
static duk_ret_t js_GetTileDelay          (duk_context* ctx);
static duk_ret_t js_GetTileHeight         (duk_context* ctx);
static duk_ret_t js_GetTileImage          (duk_context* ctx);
static duk_ret_t js_GetTileName           (duk_context* ctx);
static duk_ret_t js_GetTileSurface        (duk_context* ctx);
static duk_ret_t js_GetTileWidth          (duk_context* ctx);
static duk_ret_t js_SetCameraX            (duk_context* ctx);
static duk_ret_t js_SetCameraY            (duk_context* ctx);
static duk_ret_t js_SetDefaultMapScript   (duk_context* ctx);
static duk_ret_t js_SetLayerMask          (duk_context* ctx);
static duk_ret_t js_SetMapEngineFrameRate (duk_context* ctx);
static duk_ret_t js_SetNextAnimatedTile   (duk_context* ctx);
static duk_ret_t js_SetRenderScript       (duk_context* ctx);
static duk_ret_t js_SetTalkActivationKey  (duk_context* ctx);
static duk_ret_t js_SetTile               (duk_context* ctx);
static duk_ret_t js_SetTileDelay          (duk_context* ctx);
static duk_ret_t js_SetTileImage          (duk_context* ctx);
static duk_ret_t js_SetTileSurface        (duk_context* ctx);
static duk_ret_t js_SetUpdateScript       (duk_context* ctx);
static duk_ret_t js_AttachCamera          (duk_context* ctx);
static duk_ret_t js_AttachInput           (duk_context* ctx);
static duk_ret_t js_CallDefaultMapScript  (duk_context* ctx);
static duk_ret_t js_CallMapScript         (duk_context* ctx);
static duk_ret_t js_ChangeMap             (duk_context* ctx);
static duk_ret_t js_DetachCamera          (duk_context* ctx);
static duk_ret_t js_DetachInput           (duk_context* ctx);
static duk_ret_t js_ExecuteTrigger        (duk_context* ctx);
static duk_ret_t js_ExecuteZones          (duk_context* ctx);
static duk_ret_t js_ExitMapEngine         (duk_context* ctx);
static duk_ret_t js_MapToScreenX          (duk_context* ctx);
static duk_ret_t js_MapToScreenY          (duk_context* ctx);
static duk_ret_t js_RenderMap             (duk_context* ctx);
static duk_ret_t js_ScreenToMapX          (duk_context* ctx);
static duk_ret_t js_ScreenToMapY          (duk_context* ctx);
static duk_ret_t js_SetDelayScript        (duk_context* ctx);
static duk_ret_t js_UpdateMapEngine       (duk_context* ctx);

static person_t*           s_camera_person    = NULL;
static int                 s_cam_x            = 0;
static int                 s_cam_y            = 0;
static int                 s_map_corner_x;
static int                 s_map_corner_y;
static int                 s_current_trigger = -1;
static int                 s_current_zone     = -1;
static int                 s_def_scripts[MAP_SCRIPT_MAX];
static bool                s_exiting          = false;
static int                 s_framerate        = 0;
static unsigned int        s_frames           = 0;
static person_t*           s_input_person     = NULL;
static map_t*              s_map = NULL;
static char*               s_map_filename     = NULL;
static struct map_trigger* s_on_trigger       = NULL;
static int                 s_render_script    = 0;
static int                 s_talk_key         = ALLEGRO_KEY_SPACE;
static int                 s_update_script    = 0;
static int                 s_num_delay_scripts = 0;
static int                 s_max_delay_scripts = 0;
static struct delay_script *s_delay_scripts    = NULL;

struct delay_script
{
	int script_id;
	int frames_left;
};

struct map
{
	bool               is_repeating;
	point3_t           origin;
	int                scripts[MAP_SCRIPT_MAX];
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
	int              width, height;
	struct map_tile* tilemap;
	obsmap_t*        obsmap;
	ALLEGRO_COLOR    color_mask;
	bool             is_reflective;
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
	int script_id;
	int x, y, z;
};

struct map_zone
{
	bool   is_active;
	rect_t bounds;
	int    step_interval;
	int    steps_left;
	int    layer;
	int    script_id;
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

map_t*
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
	ALLEGRO_FILE*            file;
	bool                     has_failed;
	struct map_layer*        layer;
	struct rmp_layer_header  layer_hdr;
	map_t*                   map = NULL;
	int                      num_tiles;
	struct map_person*       person;
	struct rmp_header        rmp;
	lstring_t*               script;
	rect_t                   segment;
	int16_t*                 tile_data = NULL;
	char*                    tile_path;
	tileset_t*               tileset;
	struct map_trigger*      trigger;
	struct rmp_zone_header   zone_hdr;
	lstring_t*               *strings = NULL;

	int i, j, x, y, z;
	
	if ((file = al_fopen(path, "rb")) == NULL) goto on_error;
	if ((map = calloc(1, sizeof(map_t))) == NULL) goto on_error;
	if (al_fread(file, &rmp, sizeof(struct rmp_header)) != sizeof(struct rmp_header))
		goto on_error;
	if (memcmp(rmp.signature, ".rmp", 4) != 0) goto on_error;
	if (rmp.num_strings != 3 && rmp.num_strings != 5 && rmp.num_strings < 9)
		goto on_error;
	switch (rmp.version) {
	case 1:
		// load strings (resource filenames, scripts, etc.)
		if ((strings = calloc(rmp.num_strings, sizeof(lstring_t*))) == NULL)
			goto on_error;
		has_failed = false;
		for (i = 0; i < rmp.num_strings; ++i)
			has_failed = has_failed || ((strings[i] = read_lstring(file, true)) == NULL);
		if (has_failed) goto on_error;
		
		// pre-allocate structures; this way if an allocation fails we don't waste time reading the rest of the file
		if ((map->layers = calloc(rmp.num_layers, sizeof(struct map_layer))) == NULL) goto on_error;
		if ((map->persons = calloc(rmp.num_entities, sizeof(struct map_person))) == NULL) goto on_error;
		if ((map->triggers = calloc(rmp.num_entities, sizeof(struct map_trigger))) == NULL) goto on_error;
		if ((map->zones = calloc(rmp.num_zones, sizeof(struct map_zone))) == NULL) goto on_error;
		
		// load layers
		for (i = 0; i < rmp.num_layers; ++i) {
			if (al_fread(file, &layer_hdr, sizeof(struct rmp_layer_header)) != sizeof(struct rmp_layer_header))
				goto on_error;
			map->layers[i].color_mask = al_map_rgba(255, 255, 255, 255);
			map->layers[i].is_reflective = layer_hdr.is_reflective;
			map->layers[i].width = layer_hdr.width;
			map->layers[i].height = layer_hdr.height;
			if ((map->layers[i].tilemap = malloc(layer_hdr.width * layer_hdr.height * sizeof(struct map_tile))) == NULL)
				goto on_error;
			if ((map->layers[i].obsmap = new_obsmap()) == NULL) goto on_error;
			map->layers[i].name = read_lstring(file, true);
			num_tiles = layer_hdr.width * layer_hdr.height;
			if ((tile_data = malloc(num_tiles * 2)) == NULL) goto on_error;
			if (al_fread(file, tile_data, num_tiles * 2) != num_tiles * 2) goto on_error;
			for (j = 0; j < num_tiles; ++j)
				map->layers[i].tilemap[j].tile_index = tile_data[j];
			for (j = 0; j < layer_hdr.num_segments; ++j) {
				if (!al_fread_rect_32(file, &segment)) goto on_error;
				add_obsmap_line(map->layers[i].obsmap, segment);
			}
			free(tile_data); tile_data = NULL;
		}
		
		// load entities
		map->num_persons = 0;
		map->num_triggers = 0;
		for (i = 0; i < rmp.num_entities; ++i) {
			al_fread(file, &entity_hdr, sizeof(struct rmp_entity_header));
			switch (entity_hdr.type) {
			case 1:  // person
				++map->num_persons;
				person = &map->persons[map->num_persons - 1];
				memset(person, 0, sizeof(struct map_person));
				if ((person->name = read_lstring(file, true)) == NULL) goto on_error;
				if ((person->spriteset = read_lstring(file, true)) == NULL) goto on_error;
				person->x = entity_hdr.x; person->y = entity_hdr.y; person->z = entity_hdr.z;
				if (al_fread(file, &count, 2) != 2 || count < 5) goto on_error;
				person->create_script = read_lstring(file, false);
				person->destroy_script = read_lstring(file, false);
				person->touch_script = read_lstring(file, false);
				person->talk_script = read_lstring(file, false);
				person->command_script = read_lstring(file, false);
				for (j = 5; j < count; ++j) {
					free_lstring(read_lstring(file, true));
				}
				al_fseek(file, 16, ALLEGRO_SEEK_CUR);
				break;
			case 2:  // trigger
				if ((script = read_lstring(file, false)) == NULL) goto on_error;
				++map->num_triggers;
				trigger = &map->triggers[map->num_triggers - 1];
				memset(trigger, 0, sizeof(struct map_trigger));
				trigger->x = entity_hdr.x;
				trigger->y = entity_hdr.y;
				trigger->z = entity_hdr.z;
				trigger->script_id = compile_script(script, "[trigger script]");
				free_lstring(script);
				break;
			default:
				goto on_error;
			}
		}

		// load zones
		for (i = 0; i < rmp.num_zones; ++i) {
			if (al_fread(file, &zone_hdr, sizeof(struct rmp_zone_header)) != sizeof(struct rmp_zone_header))
				goto on_error;
			if ((script = read_lstring(file, false)) == NULL) goto on_error;
			map->zones[i].layer = zone_hdr.layer;
			map->zones[i].bounds = new_rect(zone_hdr.x1, zone_hdr.y1, zone_hdr.x2, zone_hdr.y2);
			map->zones[i].step_interval = zone_hdr.step_interval;
			map->zones[i].script_id = compile_script(script, "[zone script]");
			free_lstring(script);
		}
		
		// load tileset
		tile_path = get_asset_path(strings[0]->cstr, "maps", false);
		tileset = strcmp(strings[0]->cstr, "") == 0 ? read_tileset(file) : load_tileset(tile_path);
		free(tile_path);
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
	al_fclose(file);
	return map;

on_error:
	if (file != NULL) al_fclose(file);
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

void
free_map(map_t* map)
{
	int i;
	
	if (map != NULL) {
		for (i = 0; i < MAP_SCRIPT_MAX; ++i)
			free_script(map->scripts[i]);
		for (i = 0; i < map->num_layers; ++i) {
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
			free_script(map->triggers[i].script_id);
		for (i = 0; i < map->num_zones; ++i)
			free_script(map->zones[i].script_id);
		free_tileset(map->tileset);
		free(map->layers);
		free(map->persons);
		free(map->triggers);
		free(map);
	}
}

void
init_map_engine_api(duk_context* ctx)
{
	memset(s_def_scripts, 0, MAP_SCRIPT_MAX * sizeof(int));
	
	register_api_func(ctx, NULL, "MapEngine", js_MapEngine);
	register_api_func(ctx, NULL, "AreZonesAt", js_AreZonesAt);
	register_api_func(ctx, NULL, "IsCameraAttached", js_IsCameraAttached);
	register_api_func(ctx, NULL, "IsInputAttached", js_IsInputAttached);
	register_api_func(ctx, NULL, "IsMapEngineRunning", js_IsMapEngineRunning);
	register_api_func(ctx, NULL, "IsTriggerAt", js_IsTriggerAt);
	register_api_func(ctx, NULL, "GetCameraPerson", js_GetCameraPerson);
	register_api_func(ctx, NULL, "GetCameraX", js_GetCameraX);
	register_api_func(ctx, NULL, "GetCameraY", js_GetCameraY);
	register_api_func(ctx, NULL, "GetCurrentMap", js_GetCurrentMap);
	register_api_func(ctx, NULL, "GetCurrentTrigger", js_GetCurrentTrigger);
	register_api_func(ctx, NULL, "GetCurrentZone", js_GetCurrentZone);
	register_api_func(ctx, NULL, "GetInputPerson", js_GetInputPerson);
	register_api_func(ctx, NULL, "GetLayerHeight", js_GetLayerHeight);
	register_api_func(ctx, NULL, "GetLayerMask", js_GetLayerMask);
	register_api_func(ctx, NULL, "GetLayerWidth", js_GetLayerWidth);
	register_api_func(ctx, NULL, "GetMapEngineFrameRate", js_GetMapEngineFrameRate);
	register_api_func(ctx, NULL, "GetNextAnimatedTile", js_GetNextAnimatedTile);
	register_api_func(ctx, NULL, "GetNumLayers", js_GetNumLayers);
	register_api_func(ctx, NULL, "GetNumTiles", js_GetNumTiles);
	register_api_func(ctx, NULL, "GetNumTriggers", js_GetNumTriggers);
	register_api_func(ctx, NULL, "GetNumZones", js_GetNumZones);
	register_api_func(ctx, NULL, "GetTalkActivationKey", js_GetTalkActivationKey);
	register_api_func(ctx, NULL, "GetTile", js_GetTile);
	register_api_func(ctx, NULL, "GetTileDelay", js_GetTileDelay);
	register_api_func(ctx, NULL, "GetTileImage", js_GetTileImage);
	register_api_func(ctx, NULL, "GetTileHeight", js_GetTileHeight);
	register_api_func(ctx, NULL, "GetTileName", js_GetTileName);
	register_api_func(ctx, NULL, "GetTileSurface", js_GetTileSurface);
	register_api_func(ctx, NULL, "GetTileWidth", js_GetTileWidth);
	register_api_func(ctx, NULL, "SetCameraX", js_SetCameraX);
	register_api_func(ctx, NULL, "SetCameraY", js_SetCameraY);
	register_api_func(ctx, NULL, "SetDefaultMapScript", js_SetDefaultMapScript);
	register_api_func(ctx, NULL, "SetLayerMask", js_SetLayerMask);
	register_api_func(ctx, NULL, "SetMapEngineFrameRate", js_SetMapEngineFrameRate);
	register_api_func(ctx, NULL, "SetNextAnimatedTile", js_SetNextAnimatedTile);
	register_api_func(ctx, NULL, "SetRenderScript", js_SetRenderScript);
	register_api_func(ctx, NULL, "SetTalkActivationKey", js_SetTalkActivationKey);
	register_api_func(ctx, NULL, "SetTile", js_SetTile);
	register_api_func(ctx, NULL, "SetTileDelay", js_SetTileDelay);
	register_api_func(ctx, NULL, "SetTileImage", js_SetTileImage);
	register_api_func(ctx, NULL, "SetTileSurface", js_SetTileSurface);
	register_api_func(ctx, NULL, "SetUpdateScript", js_SetUpdateScript);
	register_api_func(ctx, NULL, "AttachCamera", js_AttachCamera);
	register_api_func(ctx, NULL, "AttachInput", js_AttachInput);
	register_api_func(ctx, NULL, "CallDefaultMapScript", js_CallDefaultMapScript);
	register_api_func(ctx, NULL, "CallMapScript", js_CallMapScript);
	register_api_func(ctx, NULL, "ChangeMap", js_ChangeMap);
	register_api_func(ctx, NULL, "DetachCamera", js_DetachCamera);
	register_api_func(ctx, NULL, "DetachInput", js_DetachInput);
	register_api_func(ctx, NULL, "ExecuteTrigger", js_ExecuteTrigger);
	register_api_func(ctx, NULL, "ExecuteZones", js_ExecuteZones);
	register_api_func(ctx, NULL, "ExitMapEngine", js_ExitMapEngine);
	register_api_func(ctx, NULL, "MapToScreenX", js_MapToScreenX);
	register_api_func(ctx, NULL, "MapToScreenY", js_MapToScreenY);
	register_api_func(ctx, NULL, "RenderMap", js_RenderMap);
	register_api_func(ctx, NULL, "ScreenToMapX", js_ScreenToMapX);
	register_api_func(ctx, NULL, "ScreenToMapY", js_ScreenToMapY);
	register_api_func(ctx, NULL, "SetDelayScript", js_SetDelayScript);
	register_api_func(ctx, NULL, "UpdateMapEngine", js_UpdateMapEngine);

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

rect_t
get_map_bounds(void)
{
	rect_t bounds;
	int    tile_w, tile_h;
	
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	bounds.x1 = 0; bounds.y1 = 0;
	bounds.x2 = s_map->layers[0].width * tile_w;
	bounds.y2 = s_map->layers[0].height * tile_h;
	return bounds;
}

const obsmap_t*
get_map_layer_obsmap(int layer)
{
	return s_map->layers[layer].obsmap;
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

	x = (x % layer_w + layer_w) % layer_w;
	y = (y % layer_h + layer_h) % layer_h;
	return s_map->layers[layer].tilemap[x + y * layer_w].tile_index;
}

const tileset_t*
get_map_tileset(void)
{
	return s_map->tileset;
}

void
normalize_map_entity_xy(double* inout_x, double* inout_y, int layer)
{
	int tile_w, tile_h;
	int layer_w, layer_h;
	
	if (!s_map->is_repeating)
		return;
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	layer_w = s_map->layers[layer].width * tile_w;
	layer_h = s_map->layers[layer].height * tile_h;
	if (inout_x) *inout_x = fmod(fmod(*inout_x, layer_w) + layer_w, layer_w);
	if (inout_y) *inout_y = fmod(fmod(*inout_y, layer_h) + layer_h, layer_h);
}

static bool
change_map(const char* filename, bool preserve_persons)
{
	map_t*             map;
	char*              path;
	person_t*          person;
	struct map_person* person_info;

	int i;

	path = get_asset_path(filename, "maps", false);
	map = load_map(path);
	free(path);
	if (map != NULL) {
		if (s_map != NULL) {
			// run map exit scripts first, before loading new map
			run_script(s_def_scripts[MAP_SCRIPT_ON_LEAVE], false);
			run_script(s_map->scripts[MAP_SCRIPT_ON_LEAVE], false);
		}
		free_map(s_map); free(s_map_filename);
		s_map = map; s_map_filename = strdup(filename);
		reset_persons(s_map, preserve_persons);

		// populate persons
		for (i = 0; i < s_map->num_persons; ++i) {
			person_info = &s_map->persons[i];
			person = create_person(person_info->name->cstr, person_info->spriteset->cstr, false);
			set_person_xyz(person, person_info->x, person_info->y, person_info->z);
			set_person_script(person, PERSON_SCRIPT_ON_CREATE, person_info->create_script);
			set_person_script(person, PERSON_SCRIPT_ON_DESTROY, person_info->destroy_script);
			set_person_script(person, PERSON_SCRIPT_ON_TOUCH, person_info->touch_script);
			set_person_script(person, PERSON_SCRIPT_ON_TALK, person_info->talk_script);
			set_person_script(person, PERSON_SCRIPT_GENERATOR, person_info->command_script);
			call_person_script(person, PERSON_SCRIPT_ON_CREATE, true);
		}
		
		// run map entry scripts
		run_script(s_def_scripts[MAP_SCRIPT_ON_ENTER], false);
		run_script(s_map->scripts[MAP_SCRIPT_ON_ENTER], false);
		
		s_frames = 0;
		return true;
	}
	else {
		return false;
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
get_trigger_at(int x, int y, int layer)
{
	rect_t              bounds;
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
		if (is_point_in_rect(x, y, bounds))
			return trigger;
	}
	return NULL;
}

static struct map_zone*
get_zone_at(int x, int y, int layer, int index)
{
	struct map_zone* zone;
	
	int i;

	for (i = 0; i < s_map->num_zones; ++i) {
		zone = &s_map->zones[i];
		if (zone->layer != layer && false)  // layer ignored for compatibility
			continue;
		if (is_point_in_rect(x, y, zone->bounds) && index-- == 0)
			return zone;
	}
	return NULL;
}

static void
process_map_input(void)
{
	ALLEGRO_KEYBOARD_STATE kb_state;

	// clear out excess keys from key queue
	g_key_queue.num_keys = 0;
	
	// check for player control of input person, if there is one
	if (s_input_person != NULL && !is_person_busy(s_input_person)) {
		al_get_keyboard_state(&kb_state);
		if (al_key_down(&kb_state, s_talk_key)) {
			talk_person(s_input_person);
		}
		else if (al_key_down(&kb_state, ALLEGRO_KEY_UP)) {
			queue_person_command(s_input_person, COMMAND_FACE_NORTH, true);
			queue_person_command(s_input_person, COMMAND_MOVE_NORTH, true);
		}
		else if (al_key_down(&kb_state, ALLEGRO_KEY_RIGHT)) {
			queue_person_command(s_input_person, COMMAND_FACE_EAST, true);
			queue_person_command(s_input_person, COMMAND_MOVE_EAST, true);
		}
		else if (al_key_down(&kb_state, ALLEGRO_KEY_DOWN)) {
			queue_person_command(s_input_person, COMMAND_FACE_SOUTH, true);
			queue_person_command(s_input_person, COMMAND_MOVE_SOUTH, true);
		}
		else if (al_key_down(&kb_state, ALLEGRO_KEY_LEFT)) {
			queue_person_command(s_input_person, COMMAND_FACE_WEST, true);
			queue_person_command(s_input_person, COMMAND_MOVE_WEST, true);
		}
	}
}

static void
render_map_engine(void)
{
	int               cell_x, cell_y;
	int               first_cell_x, first_cell_y;
	struct map_layer* layer;
	int               map_w, map_h;
	int               tile_w, tile_h;
	int               off_x, off_y;
	int               tile_index;
	
	int x, y, z;
	
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	map_w = s_map->layers[0].width * tile_w;
	map_h = s_map->layers[0].height * tile_h;
	off_x = s_cam_x - g_res_x / 2;
	off_y = s_cam_y - g_res_y / 2;
	if (!s_map->is_repeating) {
		// non-repeating map - clamp viewport to map bounds
		off_x = fmin(fmax(off_x, 0), map_w - g_res_x);
		off_y = fmin(fmax(off_y, 0), map_h - g_res_y);
	}
	else {
		// repeating map - wrap offset to map bounds (simplifies calculations)
		off_x = (off_x % map_w + map_w) % map_w;
		off_y = (off_y % map_h + map_h) % map_h;
	}
	s_map_corner_x = off_x;
	s_map_corner_y = off_y;
	al_hold_bitmap_drawing(true);
	for (z = 0; z < s_map->num_layers; ++z) {
		layer = &s_map->layers[z];
		first_cell_x = off_x / tile_w;
		first_cell_y = off_y / tile_h;
		for (y = 0; y < g_res_y / tile_h + 2; ++y) for (x = 0; x < g_res_x / tile_w + 2; ++x) {
			cell_x = s_map->is_repeating ? (x + first_cell_x) % layer->width : x + first_cell_x;
			cell_y = s_map->is_repeating ? (y + first_cell_y) % layer->height : y + first_cell_y;
			if (cell_x < 0 || cell_x >= layer->width || cell_y < 0 || cell_y >= layer->height)
				continue;
			tile_index = layer->tilemap[cell_x + cell_y * layer->width].tile_index;
			draw_tile(s_map->tileset, layer->color_mask, x * tile_w - off_x % tile_w, y * tile_h - off_y % tile_h, tile_index);
		}
		if (s_map->is_repeating) {
			// for small repeating maps, persons need to be repeated as well
			for (y = 0; y < g_res_y / map_h + 2; ++y) for (x = 0; x < g_res_x / map_w + 2; ++x) {
				render_persons(z, off_x - x * map_w, off_y - y * map_h);
			}
		}
		else {
			render_persons(z, off_x, off_y);
		}
	}
	al_hold_bitmap_drawing(false);
	run_script(s_render_script, false);
}

static void
update_map_engine(void)
{
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
	map_w = s_map->layers[0].width * tile_w;
	map_h = s_map->layers[0].height * tile_h;
	
	update_persons();
	animate_tileset(s_map->tileset);

	// update camera
	if (s_camera_person != NULL) {
		get_person_xy(s_camera_person, &x, &y, true);
		s_cam_x = x; s_cam_y = y;
	}

	// run edge scripts if player walked off map (only for non-repeating map)
	if (!s_map->is_repeating && s_input_person != NULL) {
		get_person_xy(s_input_person, &x, &y, false);
		script_type = y < 0 ? MAP_SCRIPT_ON_LEAVE_NORTH
			: x >= map_w ? MAP_SCRIPT_ON_LEAVE_EAST
			: y >= map_h ? MAP_SCRIPT_ON_LEAVE_SOUTH
			: y < 0 ? MAP_SCRIPT_ON_LEAVE_WEST
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
		trigger = get_trigger_at(x, y, layer);
		if (trigger != s_on_trigger) {
			s_on_trigger = trigger;
			if (trigger) run_script(trigger->script_id, false);
		}

		// update any occupied zones
		i = 0;
		while (zone = get_zone_at(x, y, layer, i++)) {
			if (zone->steps_left-- <= 0) {
				zone->steps_left = zone->step_interval;
				run_script(zone->script_id, true);
			}
		}
	}
	
	run_script(s_update_script, false);
	
	// run delay scripts, if applicable
	for (i = 0; i < s_num_delay_scripts; ++i) {
		if (s_delay_scripts[i].frames_left-- <= 0) {
			run_script(s_delay_scripts[i].script_id, false);
			free_script(s_delay_scripts[i].script_id);
			for (j = i; j < s_num_delay_scripts - 1; ++j) s_delay_scripts[j] = s_delay_scripts[j + 1];
			--s_num_delay_scripts; --i;
		}
	}
}

static duk_ret_t
js_MapEngine(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	int framerate = duk_require_int(ctx, 1);
	
	g_map_running = true;
	s_exiting = false;
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	s_framerate = framerate;
	if (!change_map(filename, true)) duk_error(ctx, DUK_ERR_ERROR, "MapEngine(): Failed to load map file '%s' into map engine", filename);
	while (!s_exiting) {
		if (!begin_frame(s_framerate)) bail_out_game();
		if (!g_skip_frame) render_map_engine();
		update_map_engine();
		process_map_input();
	}
	g_map_running = false;
	return 0;
}

static duk_ret_t
js_AreZonesAt(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int z = duk_require_int(ctx, 2);

	if (z < 0 || z >= s_map->num_layers)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "AreZonesAt(): Invalid layer index (%i)", z);
	duk_push_boolean(ctx, are_zones_at(x, y, z, NULL));
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
js_IsMapEngineRunning(duk_context* ctx)
{
	duk_push_boolean(ctx, g_map_running);
	return 1;
}

static duk_ret_t
js_IsTriggerAt(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int z = duk_require_int(ctx, 2);
	
	if (z < 0 || z >= s_map->num_layers)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "IsTriggerAt(): Invalid layer index; valid range is 0-%i, caller passed %i", s_map->num_layers - 1, z);
	duk_push_boolean(ctx, get_trigger_at(x, y, z) != NULL);
	return 1;
}

static duk_ret_t
js_GetCameraPerson(duk_context* ctx)
{
	if (s_camera_person == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "GetCameraPerson(): Invalid operation, camera not attached");
	duk_push_string(ctx, get_person_name(s_camera_person));
	return 1;
}

static duk_ret_t
js_GetCameraX(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetCameraX(): Map engine must be running");
	duk_push_int(ctx, s_cam_x);
	return 1;
}

static duk_ret_t
js_GetCameraY(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetCameraX(): Map engine must be running");
	duk_push_int(ctx, s_cam_y);
	return 1;
}

static duk_ret_t
js_GetCurrentMap(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetCurrentMap(): Map engine not running");
	duk_push_string(ctx, s_map_filename);
	return 1;
}

static duk_ret_t
js_GetCurrentTrigger(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetCurrentTrigger(): Map engine not running");
	if (!s_current_trigger == -1)
		duk_error(ctx, DUK_ERR_ERROR, "GetCurrentTrigger(): Cannot be called outside of a trigger script");
	duk_push_int(ctx, s_current_trigger);
	return 1;
}

static duk_ret_t
js_GetCurrentZone(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetCurrentZone(): Map engine not running");
	if (!s_current_zone == -1)
		duk_error(ctx, DUK_ERR_ERROR, "GetCurrentZone(): Cannot be called outside of a zone script");
	duk_push_int(ctx, s_current_zone);
	return 1;
}

static duk_ret_t
js_GetInputPerson(duk_context* ctx)
{
	if (s_input_person == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "GetInputPerson(): Invalid operation, input not attached");
	duk_push_string(ctx, get_person_name(s_input_person));
	return 1;
}

static duk_ret_t
js_GetLayerHeight(duk_context* ctx)
{
	int z = duk_require_int(ctx, 0);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetLayerHeight(): Map engine must be running");
	if (z < 0 || z > s_map->num_layers)
		duk_error(ctx, DUK_ERR_ERROR, "GetLayerHeight(): Invalid layer index; valid range is 0-%i, called passed %i", s_map->num_layers, z);
	duk_push_int(ctx, s_map->layers[z].height);
	return 1;
}

static duk_ret_t
js_GetLayerMask(duk_context* ctx)
{
	int layer = duk_require_int(ctx, 0);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetLayerMask(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error(ctx, DUK_ERR_ERROR, "GetLayerMask(): Invalid layer index (%i)", layer);
	duk_push_sphere_color(ctx, s_map->layers[layer].color_mask);
	return 1;
}

static duk_ret_t
js_GetLayerWidth(duk_context* ctx)
{
	int z = duk_require_int(ctx, 0);
	
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetLayerWidth(): Map engine must be running");
	if (z < 0 || z > s_map->num_layers)
		duk_error(ctx, DUK_ERR_ERROR, "GetLayerWidth(): Invalid layer index; valid range is 0-%i, called passed %i", s_map->num_layers, z);
	duk_push_int(ctx, s_map->layers[z].width);
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

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetNextAnimatedTile(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "GetNextAnimatedTile(): Invalid tile index (%i)", tile_index);
	duk_push_int(ctx, get_next_tile(s_map->tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetNumLayers(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetNumLayers(): Map engine must be running");
	duk_push_int(ctx, s_map->num_layers);
	return 1;
}

static duk_ret_t
js_GetNumTiles(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetNumTiles(): Map engine must be running");
	duk_push_int(ctx, get_tile_count(s_map->tileset));
	return 1;
}

static duk_ret_t
js_GetNumTriggers(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetNumTriggers(): Map engine must be running");
	duk_push_int(ctx, s_map->num_triggers);
	return 1;
}

static duk_ret_t
js_GetNumZones(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetNumZones(): Map engine must be running");
	duk_push_int(ctx, s_map->num_zones);
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
	int layer = duk_require_int(ctx, 2);

	int layer_w, layer_h;

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetTile(): Map engine must be running");
	if (layer < 0 || layer >= s_map->num_layers)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "GetTile(): Invalid layer index (caller passed: %i)", layer);
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

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetTileDelay(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "GetTileDelay(): Invalid tile index (%i)", tile_index);
	duk_push_int(ctx, get_tile_delay(s_map->tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetTileHeight(duk_context* ctx)
{
	int w, h;

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetTileHeight(): Map engine must be running");
	get_tile_size(s_map->tileset, &w, &h);
	duk_push_int(ctx, h);
	return 1;
}

static duk_ret_t
js_GetTileImage(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetTileImage(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "GetTileImage(): Tile index out of range (%i)", tile_index);
	duk_push_sphere_image(ctx, get_tile_image(s_map->tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetTileName(duk_context* ctx)
{
	int        tile_index = duk_require_int(ctx, 0);
	
	const lstring_t* tile_name;

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetTileName(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "GetTileName(): Tile index out of range (%i)", tile_index);
	tile_name = get_tile_name(s_map->tileset, tile_index);
	duk_push_lstring(ctx, tile_name->cstr, tile_name->length);
	return 1;
}

static duk_ret_t
js_GetTileSurface(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);

	image_t* image;

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetTileSurface(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "GetTileSurface(): Tile index out of range (%i)", tile_index);
	if ((image = clone_image(get_tile_image(s_map->tileset, tile_index))) == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "GetTileSurface(): Failed to create new surface image");
	duk_push_sphere_surface(ctx, image);
	free_image(image);
	return 1;
}

static duk_ret_t
js_GetTileWidth(duk_context* ctx)
{
	int w, h;
	
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetTileWidth(): Map engine must be running");
	get_tile_size(s_map->tileset, &w, &h);
	duk_push_int(ctx, w);
	return 1;
}

static duk_ret_t
js_SetCameraX(duk_context* ctx)
{
	int new_x = duk_require_int(ctx, 0);
	
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "SetCameraX(): Map engine must be running");
	s_cam_x = new_x;
	return 0;
}

static duk_ret_t
js_SetCameraY(duk_context* ctx)
{
	int new_y = duk_require_int(ctx, 0);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "SetCameraY(): Map engine must be running");
	s_cam_y = new_y;
	return 0;
}

static duk_ret_t
js_SetDefaultMapScript(duk_context* ctx)
{
	int script_type = duk_require_int(ctx, 0);
	lstring_t* code = duk_require_lstring_t(ctx, 1);

	int         script_id;
	const char* script_name;

	if (script_type < 0 || script_type >= MAP_SCRIPT_MAX)
		duk_error(ctx, DUK_ERR_ERROR, "SetDefaultMapScript(): Invalid map script constant");
	script_name = (script_type == MAP_SCRIPT_ON_ENTER) ? "[default map enter script]"
		: (script_type == MAP_SCRIPT_ON_LEAVE) ? "[default map leave script]"
		: (script_type == MAP_SCRIPT_ON_LEAVE_NORTH) ? "[default map leave-north script]"
		: (script_type == MAP_SCRIPT_ON_LEAVE_EAST) ? "[default map leave-east script]"
		: (script_type == MAP_SCRIPT_ON_LEAVE_SOUTH) ? "[default map leave-south script]"
		: (script_type == MAP_SCRIPT_ON_LEAVE_WEST) ? "[default map leave-west script]"
		: NULL;
	script_id = compile_script(code, script_name);
	free_lstring(code);
	free_script(s_def_scripts[script_type]);
	s_def_scripts[script_type] = script_id;
	return 0;
}

static duk_ret_t
js_SetDelayScript(duk_context* ctx)
{
	int frames = duk_require_int(ctx, 0);
	lstring_t* script = duk_require_lstring_t(ctx, 1);

	struct delay_script* delay;
	char                 script_name[100];

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "SetDelayScript(): Map engine is not running");
	if (frames < 0)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "SetDelayScript(): Delay frames cannot be negative");
	++s_num_delay_scripts;
	if (s_num_delay_scripts > s_max_delay_scripts) {
		if (!(s_delay_scripts = realloc(s_delay_scripts, s_num_delay_scripts * sizeof(struct delay_script))))
			duk_error(ctx, DUK_ERR_ERROR, "SetDelayScript(): Failed to allocate new delay script (internal error)");
		s_max_delay_scripts = s_num_delay_scripts;
	}
	delay = &s_delay_scripts[s_num_delay_scripts - 1];
	sprintf(script_name, "[%i-frame delay script]", frames);
	delay->script_id = compile_script(script, script_name);
	delay->frames_left = frames;
	free_lstring(script);
	return 0;
}

static duk_ret_t
js_SetLayerMask(duk_context* ctx)
{
	int layer = duk_require_int(ctx, 0);
	ALLEGRO_COLOR mask = duk_get_sphere_color(ctx, 1);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "SetLayerMask(): Map engine must be running");
	if (layer < 0 || layer > s_map->num_layers)
		duk_error(ctx, DUK_ERR_ERROR, "SetLayerMask(): Invalid layer index (%i)", layer);
	s_map->layers[layer].color_mask = mask;
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

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "SetNextAnimatedTile(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "SetNextAnimatedTile(): Invalid tile index (%i)", tile_index);
	if (next_index < 0 || next_index >= get_tile_count(s_map->tileset))
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "SetNextAnimatedTile(): Invalid tile index for next tile (%i)", tile_index);
	set_next_tile(s_map->tileset, tile_index, next_index);
	return 0;
}

static duk_ret_t
js_SetRenderScript(duk_context* ctx)
{
	lstring_t* code = duk_require_lstring_t(ctx, 0);

	free_script(s_render_script);
	s_render_script = compile_script(code, "[render script]");
	free_lstring(code);
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
	int layer = duk_require_int(ctx, 2);
	int tile_index = duk_require_int(ctx, 3);

	int layer_w, layer_h;

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "SetTile(): Map engine must be running");
	if (layer < 0 || layer >= s_map->num_layers)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "SetTile(): Invalid layer index (caller passed: %i)", layer);
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

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "SetTileDelay(): Map engine must be running");
	if (tile_index < 0 || tile_index >= get_tile_count(s_map->tileset))
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "SetTileDelay(): Invalid tile index (%i)", tile_index);
	if (delay < 0)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "SetTileDelay(): Delay cannot be negative (%i)", delay);
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

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "SetTileImage(): Map engine must be running");
	c_tiles = get_tile_count(s_map->tileset);
	if (tile_index < 0 || tile_index >= c_tiles)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "SetTileImage(): Tile index out of range (caller passed %i)", tile_index);
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	image_w = get_image_width(image);
	image_h = get_image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		duk_error(ctx, DUK_ERR_TYPE_ERROR, "SetTileImage(): Image dimensions (%ix%i) don't match tile dimensions (%ix%i)", image_w, image_h, tile_w, tile_h);
	set_tile_image(s_map->tileset, tile_index, image);
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

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "SetTileSurface(): Map engine must be running");
	c_tiles = get_tile_count(s_map->tileset);
	if (tile_index < 0 || tile_index >= c_tiles)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "SetTileSurface(): Tile index out of range (caller passed %i)", tile_index);
	get_tile_size(s_map->tileset, &tile_w, &tile_h);
	image_w = get_image_width(image);
	image_h = get_image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		duk_error(ctx, DUK_ERR_TYPE_ERROR, "SetTileSurface(): Surface dimensions (%ix%i) don't match tile dimensions (%ix%i)", image_w, image_h, tile_w, tile_h);
	if ((new_image = clone_image(image)) == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "SetTileSurface(): Failed to create new tile image");
	set_tile_image(s_map->tileset, tile_index, new_image);
	free_image(new_image);
	return 0;
}

static duk_ret_t
js_SetUpdateScript(duk_context* ctx)
{
	lstring_t* code = duk_require_lstring_t(ctx, 0);

	free_script(s_update_script);
	s_update_script = compile_script(code, "[update script]");
	return 0;
}

static duk_ret_t
js_AttachCamera(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "AttachCamera(): Person '%s' doesn't exist", name);
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
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "AttachInput(): Person '%s' doesn't exist", name);
	s_input_person = person;
	return 0;
}

static duk_ret_t
js_CallDefaultMapScript(duk_context* ctx)
{
	int type = duk_require_int(ctx, 0);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "CallDefaultMapScript(): Map engine is not running");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "CallDefaultMapScript(): Invalid script type constant");
	run_script(s_def_scripts[type], false);
	return 0;
}

static duk_ret_t
js_CallMapScript(duk_context* ctx)
{
	int type = duk_require_int(ctx, 0);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "CallMapScript(): Map engine is not running");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "CallMapScript(): Invalid script type constant");
	run_script(s_map->scripts[type], false);
	return 0;
}

static duk_ret_t
js_ChangeMap(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "ChangeMap(): Map engine is not running");
	if (!change_map(filename, false))
		duk_error(ctx, DUK_ERR_ERROR, "ChangeMap(): Failed to load map file '%s' into map engine", filename);
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
	int layer = duk_require_int(ctx, 2);
	
	struct map_trigger* trigger;
	
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "ExecuteTrigger(): Map engine is not running");
	trigger = get_trigger_at(x, y, layer);
	if (trigger != NULL) run_script(trigger->script_id, true);
	return 0;
}

static duk_ret_t
js_ExecuteZones(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int layer = duk_require_int(ctx, 2);

	struct map_zone* zone;

	int i;

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "ExecuteZones(): Map engine is not running");
	i = 0;
	while (zone = get_zone_at(x, y, layer, i++))
		run_script(zone->script_id, true);
	return 0;
}

static duk_ret_t
js_ExitMapEngine(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "ExitMapEngine(): Map engine is not running");
	s_exiting = true;
	return 0;
}

static duk_ret_t
js_MapToScreenX(duk_context* ctx)
{
	int layer = duk_require_int(ctx, 0);
	double x = duk_require_int(ctx, 1);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "MapToScreenX(): Map engine is not running");
	normalize_map_entity_xy(&x, NULL, layer);
	duk_push_int(ctx, x - s_map_corner_x);
	return 1;
}

static duk_ret_t
js_MapToScreenY(duk_context* ctx)
{
	int layer = duk_require_int(ctx, 0);
	double y = duk_require_int(ctx, 1);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "MapToScreenY(): Map engine is not running");
	normalize_map_entity_xy(NULL, &y, layer);
	duk_push_int(ctx, y - s_map_corner_y);
	return 1;
}

static duk_ret_t
js_RenderMap(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "RenderMap(): Operation requires the map engine to be running");
	render_map_engine();
	return 0;
}

static duk_ret_t
js_ScreenToMapX(duk_context* ctx)
{
	int x = duk_require_int(ctx, 1);
	
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "ScreenToMapX(): Map engine is not running");
	duk_push_int(ctx, x + s_map_corner_x);
	return 1;
}

static duk_ret_t
js_ScreenToMapY(duk_context* ctx)
{
	int y = duk_require_int(ctx, 1);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "ScreenToMapY(): Map engine is not running");
	duk_push_int(ctx, y + s_map_corner_y);
	return 1;
}

static duk_ret_t
js_UpdateMapEngine(duk_context* ctx)
{
	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "UpdateMapEngine(): Operation requires the map engine to be running");
	update_map_engine();
	return 0;
}
