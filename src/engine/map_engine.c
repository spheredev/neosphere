#include "minisphere.h"
#include "map_engine.h"

#include "api.h"
#include "audio.h"
#include "color.h"
#include "image.h"
#include "input.h"
#include "obsmap.h"
#include "persons.h"
#include "script.h"
#include "tileset.h"
#include "vanilla.h"
#include "vector.h"

#define MAX_PLAYERS 4

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

static struct map*         load_map               (const char* path);
static void                free_map               (struct map* map);
static bool                are_zones_at           (int x, int y, int layer, int* out_count);
static struct map_trigger* get_trigger_at         (int x, int y, int layer, int* out_index);
static struct map_zone*    get_zone_at            (int x, int y, int layer, int which, int* out_index);
static bool                change_map             (const char* filename, bool preserve_persons);
static int                 find_layer             (const char* name);
static void                map_screen_to_layer    (int layer, int camera_x, int camera_y, int* inout_x, int* inout_y);
static void                map_screen_to_map      (int camera_x, int camera_y, int* inout_x, int* inout_y);
static void                process_map_input      (void);
static void                render_map             (void);
static void                update_map_engine      (bool is_main_loop);

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
static duk_ret_t js_GetTriggerLayer         (duk_context* ctx);
static duk_ret_t js_GetTriggerX             (duk_context* ctx);
static duk_ret_t js_GetTriggerY             (duk_context* ctx);
static duk_ret_t js_GetZoneHeight           (duk_context* ctx);
static duk_ret_t js_GetZoneLayer            (duk_context* ctx);
static duk_ret_t js_GetZoneSteps            (duk_context* ctx);
static duk_ret_t js_GetZoneWidth            (duk_context* ctx);
static duk_ret_t js_GetZoneX                (duk_context* ctx);
static duk_ret_t js_GetZoneY                (duk_context* ctx);
static duk_ret_t js_SetCameraX              (duk_context* ctx);
static duk_ret_t js_SetCameraY              (duk_context* ctx);
static duk_ret_t js_SetColorMask            (duk_context* ctx);
static duk_ret_t js_SetDefaultMapScript     (duk_context* ctx);
static duk_ret_t js_SetLayerHeight          (duk_context* ctx);
static duk_ret_t js_SetLayerMask            (duk_context* ctx);
static duk_ret_t js_SetLayerReflective      (duk_context* ctx);
static duk_ret_t js_SetLayerRenderer        (duk_context* ctx);
static duk_ret_t js_SetLayerSize            (duk_context* ctx);
static duk_ret_t js_SetLayerVisible         (duk_context* ctx);
static duk_ret_t js_SetLayerWidth           (duk_context* ctx);
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
static duk_ret_t js_SetTriggerLayer         (duk_context* ctx);
static duk_ret_t js_SetTriggerScript        (duk_context* ctx);
static duk_ret_t js_SetTriggerXY            (duk_context* ctx);
static duk_ret_t js_SetUpdateScript         (duk_context* ctx);
static duk_ret_t js_SetZoneLayer            (duk_context* ctx);
static duk_ret_t js_SetZoneMetrics          (duk_context* ctx);
static duk_ret_t js_SetZoneScript           (duk_context* ctx);
static duk_ret_t js_SetZoneSteps            (duk_context* ctx);
static duk_ret_t js_AddTrigger              (duk_context* ctx);
static duk_ret_t js_AddZone                 (duk_context* ctx);
static duk_ret_t js_AttachCamera            (duk_context* ctx);
static duk_ret_t js_AttachInput             (duk_context* ctx);
static duk_ret_t js_AttachPlayerInput       (duk_context* ctx);
static duk_ret_t js_CallDefaultMapScript    (duk_context* ctx);
static duk_ret_t js_CallMapScript           (duk_context* ctx);
static duk_ret_t js_ChangeMap               (duk_context* ctx);
static duk_ret_t js_DetachCamera            (duk_context* ctx);
static duk_ret_t js_DetachInput             (duk_context* ctx);
static duk_ret_t js_DetachPlayerInput       (duk_context* ctx);
static duk_ret_t js_ExecuteTrigger          (duk_context* ctx);
static duk_ret_t js_ExecuteZones            (duk_context* ctx);
static duk_ret_t js_ExitMapEngine           (duk_context* ctx);
static duk_ret_t js_MapToScreenX            (duk_context* ctx);
static duk_ret_t js_MapToScreenY            (duk_context* ctx);
static duk_ret_t js_RemoveTrigger           (duk_context* ctx);
static duk_ret_t js_RemoveZone              (duk_context* ctx);
static duk_ret_t js_RenderMap               (duk_context* ctx);
static duk_ret_t js_ReplaceTilesOnLayer     (duk_context* ctx);
static duk_ret_t js_ScreenToMapX            (duk_context* ctx);
static duk_ret_t js_ScreenToMapY            (duk_context* ctx);
static duk_ret_t js_SetDelayScript          (duk_context* ctx);
static duk_ret_t js_UpdateMapEngine         (duk_context* ctx);

static mixer_t*            s_bgm_mixer = NULL;
static person_t*           s_camera_person = NULL;
static int                 s_cam_x = 0;
static int                 s_cam_y = 0;
static color_t             s_color_mask;
static color_t             s_fade_color_from;
static color_t             s_fade_color_to;
static int                 s_fade_frames;
static int                 s_fade_progress;
static int                 s_map_corner_x;
static int                 s_map_corner_y;
static int                 s_current_trigger = -1;
static int                 s_current_zone = -1;
static script_t*           s_def_scripts[MAP_SCRIPT_MAX];
static bool                s_exiting = false;
static int                 s_framerate = 0;
static unsigned int        s_frames = 0;
static bool                s_is_map_running = false;
static lstring_t*          s_last_bgm_file = NULL;
static struct map*         s_map = NULL;
static sound_t*            s_map_bgm_stream = NULL;
static char*               s_map_filename = NULL;
static struct map_trigger* s_on_trigger = NULL;
static struct player*      s_players;
static script_t*           s_render_script = NULL;
static int                 s_talk_button = 0;
static script_t*           s_update_script = NULL;
static int                 s_num_delay_scripts = 0;
static int                 s_max_delay_scripts = 0;
static struct delay_script *s_delay_scripts = NULL;

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
	lstring_t*         bgm_file;
	script_t*          scripts[MAP_SCRIPT_MAX];
	tileset_t*         tileset;
	vector_t*          triggers;
	vector_t*          zones;
	int                num_layers;
	int                num_persons;
	struct map_layer   *layers;
	struct map_person  *persons;
};

struct map_layer
{
	lstring_t*       name;
	bool             is_parallax;
	bool             is_reflective;
	bool             is_visible;
	float            autoscroll_x;
	float            autoscroll_y;
	color_t          color_mask;
	int              height;
	obsmap_t*        obsmap;
	float            parallax_x;
	float            parallax_y;
	script_t*        render_script;
	struct map_tile* tilemap;
	int              width;
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
	int       interval;
	int       steps_left;
	int       layer;
	script_t* script;
};

struct player
{
	bool      is_talk_allowed;
	person_t* person;
	int       talk_key;
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
	uint16_t interval;
	uint8_t  reserved[4];
};
#pragma pack(pop)

void
initialize_map_engine(void)
{
	int i;
	
	console_log(1, "initializing map engine");
	
	initialize_persons_manager();
	initialize_audio();
	s_bgm_mixer = mixer_new(44100, 16, 2);
	
	memset(s_def_scripts, 0, MAP_SCRIPT_MAX * sizeof(int));
	s_map = NULL; s_map_filename = NULL;
	s_camera_person = NULL;
	s_players = calloc(MAX_PLAYERS, sizeof(struct player));
	for (i = 0; i < MAX_PLAYERS; ++i)
		s_players[i].is_talk_allowed = true;
	s_current_trigger = -1;
	s_current_zone = -1;
	s_render_script = 0;
	s_update_script = 0;
	s_num_delay_scripts = s_max_delay_scripts = 0;
	s_delay_scripts = NULL;
	s_talk_button = 0;
	s_is_map_running = false;
	s_color_mask = color_new(0, 0, 0, 0);
	s_on_trigger = NULL;
}

void
shutdown_map_engine(void)
{
	int i;

	console_log(1, "shutting down map engine");
	
	for (i = 0; i < s_num_delay_scripts; ++i)
		free_script(s_delay_scripts[i].script);
	free(s_delay_scripts);
	for (i = 0; i < MAP_SCRIPT_MAX; ++i)
		free_script(s_def_scripts[i]);
	free_script(s_update_script);
	free_script(s_render_script);
	free_map(s_map);
	free(s_players);
	
	mixer_free(s_bgm_mixer);
	
	shutdown_persons_manager();
	shutdown_audio();
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
	
	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	bounds.x1 = 0; bounds.y1 = 0;
	bounds.x2 = s_map->width * tile_w;
	bounds.y2 = s_map->height * tile_h;
	return bounds;
}

const char*
get_map_name(void)
{
	return s_map ? s_map_filename : NULL;
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
get_trigger_xyz(int trigger_index, int* out_x, int* out_y, int* out_layer)
{
	struct map_trigger* trigger;

	trigger = vector_get(s_map->triggers, trigger_index);
	if (out_x) *out_x = trigger->x;
	if (out_y) *out_y = trigger->y;
	if (out_layer) *out_layer = trigger->z;
}

rect_t
get_zone_bounds(int zone_index)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	return zone->bounds;
}

int
get_zone_layer(int zone_index)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	return zone->layer;
}

int
get_zone_steps(int zone_index)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	return zone->interval;
}

void
set_trigger_layer(int trigger_index, int layer)
{
	struct map_trigger* trigger;

	trigger = vector_get(s_map->triggers, trigger_index);
	trigger->z = layer;
}

void
set_trigger_script(int trigger_index, script_t* script)
{
	script_t*           old_script;
	struct map_trigger* trigger;

	trigger = vector_get(s_map->triggers, trigger_index);
	old_script = trigger->script;
	trigger->script = ref_script(script);
	free_script(old_script);
}

void
set_trigger_xy(int trigger_index, int x, int y)
{
	struct map_trigger* trigger;

	trigger = vector_get(s_map->triggers, trigger_index);
	trigger->x = x;
	trigger->y = y;
}

void
set_zone_bounds(int zone_index, rect_t bounds)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	normalize_rect(&bounds);
	zone->bounds = bounds;
}

void
set_zone_layer(int zone_index, int layer)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	zone->layer = layer;
}

void
set_zone_script(int zone_index, script_t* script)
{
	script_t*        old_script;
	struct map_zone* zone;
	
	zone = vector_get(s_map->zones, zone_index);
	old_script = zone->script;
	zone->script = ref_script(script);
	free_script(old_script);
}

void
set_zone_steps(int zone_index, int interval)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	zone->interval = interval;
	zone->steps_left = 0;
}

bool
add_trigger(int x, int y, int layer, script_t* script)
{
	struct map_trigger trigger;

	console_log(2, "creating trigger #%d on map `%s`", vector_len(s_map->triggers), s_map_filename);
	console_log(3, "    location: `%s` @ (%d,%d)", lstr_cstr(s_map->layers[layer].name), x, y);
	
	trigger.x = x; trigger.y = y;
	trigger.z = layer;
	trigger.script = ref_script(script);
	if (!vector_push(s_map->triggers, &trigger))
		return false;
	return true;
}

bool
add_zone(rect_t bounds, int layer, script_t* script, int steps)
{
	struct map_zone zone;

	console_log(2, "creating %u-step zone #%d on map `%s`", steps, vector_len(s_map->zones), s_map_filename);
	console_log(3, "    bounds: (%d,%d)-(%d,%d)", bounds.x1, bounds.y1, bounds.x2, bounds.y2);
	
	memset(&zone, 0, sizeof(struct map_zone));
	zone.bounds = bounds;
	zone.layer = layer;
	zone.script = ref_script(script);
	zone.interval = steps;
	zone.steps_left = 0;
	if (!vector_push(s_map->zones, &zone))
		return false;
	return true;
}

void
detach_person(const person_t* person)
{
	int i;
	
	if (s_camera_person == person)
		s_camera_person = NULL;
	for (i = 0; i < MAX_PLAYERS; ++i) if (s_players[i].person == person)
		s_players[i].person = NULL;
}

void
normalize_map_entity_xy(double* inout_x, double* inout_y, int layer)
{
	int tile_w, tile_h;
	int layer_w, layer_h;
	
	if (s_map == NULL)
		return;  // can't normalize if no map loaded
	if (!s_map->is_repeating && !s_map->layers[layer].is_parallax)
		return;
	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	layer_w = s_map->layers[layer].width * tile_w;
	layer_h = s_map->layers[layer].height * tile_h;
	if (inout_x)
		*inout_x = fmod(fmod(*inout_x, layer_w) + layer_w, layer_w);
	if (inout_y)
		*inout_y = fmod(fmod(*inout_y, layer_h) + layer_h, layer_h);
}

void
remove_trigger(int trigger_index)
{
	vector_remove(s_map->triggers, trigger_index);
}

void
remove_zone(int zone_index)
{
	vector_remove(s_map->zones, zone_index);
}

bool
resize_map_layer(int layer, int x_size, int y_size)
{
	int                 old_height;
	int                 old_width;
	struct map_tile*    tile;
	int                 tile_width;
	int                 tile_height;
	struct map_tile*    tilemap;
	struct map_trigger* trigger;
	struct map_zone*    zone;

	int x, y, i;

	old_width = s_map->layers[layer].width;
	old_height = s_map->layers[layer].height;

	// allocate a new tilemap and copy the old layer tiles into it.  we can't simply realloc
	// because the tilemap is a 2D array.
	if (!(tilemap = malloc(x_size * y_size * sizeof(struct map_tile))))
		return false;
	for (x = 0; x < x_size; ++x) {
		for (y = 0; y < y_size; ++y) {
			if (x < old_width && y < old_height)
				tilemap[x + y * x_size] = s_map->layers[layer].tilemap[x + y * old_width];
			else {
				tile = &tilemap[x + y * x_size];
				tile->frames_left = tileset_get_delay(s_map->tileset, 0);
				tile->tile_index = 0;
			}
		}
	}

	// free the old tilemap and substitute the new one
	free(s_map->layers[layer].tilemap);
	s_map->layers[layer].tilemap = tilemap;
	s_map->layers[layer].width = x_size;
	s_map->layers[layer].height = y_size;

	// if we resize the largest layer, the overall map size will change.
	// recalcuate it.
	tileset_get_size(s_map->tileset, &tile_width, &tile_height);
	s_map->width = 0;
	s_map->height = 0;
	for (i = 0; i < s_map->num_layers; ++i) {
		if (!s_map->layers[i].is_parallax) {
			s_map->width = fmax(s_map->width, s_map->layers[i].width * tile_width);
			s_map->height = fmax(s_map->height, s_map->layers[i].height * tile_height);
		}
	}

	// ensure zones and triggers remain in-bounds.  if any are completely
	// out-of-bounds, delete them.
	for (i = (int)vector_len(s_map->zones) - 1; i >= 0; --i) {
		zone = vector_get(s_map->zones, i);
		if (zone->bounds.x1 >= s_map->width || zone->bounds.y1 >= s_map->height)
			vector_remove(s_map->zones, i);
		else {
			if (zone->bounds.x2 > s_map->width)
				zone->bounds.x2 = s_map->width;
			if (zone->bounds.y2 > s_map->height)
				zone->bounds.y2 = s_map->height;
		}
	}
	for (i = (int)vector_len(s_map->triggers) - 1; i >= 0; --i) {
		trigger = vector_get(s_map->triggers, i);
		if (trigger->x >= s_map->width || trigger->y >= s_map->height)
			vector_remove(s_map->triggers, i);
	}

	return true;
}

static struct map*
load_map(const char* filename)
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
	sfs_file_t*              file = NULL;
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
	path_t*                  tileset_path;
	tileset_t*               tileset;
	struct map_trigger       trigger;
	struct map_zone          zone;
	struct rmp_zone_header   zone_hdr;
	lstring_t*               *strings = NULL;

	int i, j, x, y, z;

	console_log(2, "constructing new map from `%s`", filename);
	
	memset(&rmp, 0, sizeof(struct rmp_header));
	
	if (!(file = sfs_fopen(g_fs, filename, NULL, "rb")))
		goto on_error;
	map = calloc(1, sizeof(struct map));
	if (sfs_fread(&rmp, sizeof(struct rmp_header), 1, file) != 1)
		goto on_error;
	if (memcmp(rmp.signature, ".rmp", 4) != 0) goto on_error;
	if (rmp.num_strings != 3 && rmp.num_strings != 5 && rmp.num_strings < 9)
		goto on_error;
	if (rmp.start_layer < 0 || rmp.start_layer >= rmp.num_layers)
		rmp.start_layer = 0;  // being nice here, this really should fail outright
	switch (rmp.version) {
	case 1:
		// load strings (resource filenames, scripts, etc.)
		strings = calloc(rmp.num_strings, sizeof(lstring_t*));
		has_failed = false;
		for (i = 0; i < rmp.num_strings; ++i)
			has_failed = has_failed || ((strings[i] = read_lstring(file, true)) == NULL);
		if (has_failed) goto on_error;

		// pre-allocate map structures
		map->layers = calloc(rmp.num_layers, sizeof(struct map_layer));
		map->persons = calloc(rmp.num_entities, sizeof(struct map_person));
		map->triggers = vector_new(sizeof(struct map_trigger));
		map->zones = vector_new(sizeof(struct map_zone));

		// load layers
		for (i = 0; i < rmp.num_layers; ++i) {
			if (sfs_fread(&layer_hdr, sizeof(struct rmp_layer_header), 1, file) != 1)
				goto on_error;
			layer = &map->layers[i];
			layer->is_parallax = (layer_hdr.flags & 2) != 0x0;
			layer->is_reflective = layer_hdr.is_reflective;
			layer->is_visible = (layer_hdr.flags & 1) == 0x0;
			layer->color_mask = color_new(255, 255, 255, 255);
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
			layer->name = read_lstring(file, true);
			layer->obsmap = obsmap_new();
			num_tiles = layer_hdr.width * layer_hdr.height;
			if ((tile_data = malloc(num_tiles * 2)) == NULL) goto on_error;
			if (sfs_fread(tile_data, 2, num_tiles, file) != num_tiles)
				goto on_error;
			for (j = 0; j < num_tiles; ++j)
				layer->tilemap[j].tile_index = tile_data[j];
			for (j = 0; j < layer_hdr.num_segments; ++j) {
				if (!fread_rect_32(file, &segment)) goto on_error;
				obsmap_add_line(layer->obsmap, segment);
			}
			free(tile_data); tile_data = NULL;
		}

		// if either dimension is zero, the map has no non-parallax layers and is thus malformed
		if (map->width == 0 || map->height == 0)
			goto on_error;

		// load entities
		map->num_persons = 0;
		for (i = 0; i < rmp.num_entities; ++i) {
			if (sfs_fread(&entity_hdr, sizeof(struct rmp_entity_header), 1, file) != 1)
				goto on_error;
			if (entity_hdr.z < 0 || entity_hdr.z >= rmp.num_layers)
				entity_hdr.z = 0;
			switch (entity_hdr.type) {
			case 1:  // person
				++map->num_persons;
				person = &map->persons[map->num_persons - 1];
				memset(person, 0, sizeof(struct map_person));
				if (!(person->name = read_lstring(file, true))) goto on_error;
				if (!(person->spriteset = read_lstring(file, true))) goto on_error;
				person->x = entity_hdr.x; person->y = entity_hdr.y; person->z = entity_hdr.z;
				if (sfs_fread(&count, 2, 1, file) != 1 || count < 5) goto on_error;
				person->create_script = read_lstring(file, false);
				person->destroy_script = read_lstring(file, false);
				person->touch_script = read_lstring(file, false);
				person->talk_script = read_lstring(file, false);
				person->command_script = read_lstring(file, false);
				for (j = 5; j < count; ++j)
					lstr_free(read_lstring(file, true));
				sfs_fseek(file, 16, SFS_SEEK_CUR);
				break;
			case 2:  // trigger
				if ((script = read_lstring(file, false)) == NULL) goto on_error;
				memset(&trigger, 0, sizeof(struct map_trigger));
				trigger.x = entity_hdr.x;
				trigger.y = entity_hdr.y;
				trigger.z = entity_hdr.z;
				trigger.script = compile_script(script, "%s/trig%d", filename, vector_len(map->triggers));
				if (!vector_push(map->triggers, &trigger))
					return false;
				lstr_free(script);
				break;
			default:
				goto on_error;
			}
		}

		// load zones
		for (i = 0; i < rmp.num_zones; ++i) {
			if (sfs_fread(&zone_hdr, sizeof(struct rmp_zone_header), 1, file) != 1)
				goto on_error;
			if ((script = read_lstring(file, false)) == NULL) goto on_error;
			if (zone_hdr.layer < 0 || zone_hdr.layer >= rmp.num_layers)
				zone_hdr.layer = 0;
			zone.layer = zone_hdr.layer;
			zone.bounds = new_rect(zone_hdr.x1, zone_hdr.y1, zone_hdr.x2, zone_hdr.y2);
			zone.interval = zone_hdr.interval;
			zone.steps_left = 0;
			zone.script = compile_script(script, "%s/zone%d", filename, vector_len(map->zones));
			normalize_rect(&zone.bounds);
			if (!vector_push(map->zones, &zone))
				return false;
			lstr_free(script);
		}

		// load tileset
		if (strcmp(lstr_cstr(strings[0]), "") != 0) {
			tileset_path = path_strip(path_new(filename));
			path_append(tileset_path, lstr_cstr(strings[0]));
			tileset = tileset_new(path_cstr(tileset_path));
			path_free(tileset_path);
		}
		else {
			tileset = tileset_read(file);
		}
		if (tileset == NULL) goto on_error;

		// initialize tile animation
		for (z = 0; z < rmp.num_layers; ++z) {
			layer = &map->layers[z];
			for (x = 0; x < layer->width; ++x) for (y = 0; y < layer->height; ++y) {
				i = x + y * layer->width;
				map->layers[z].tilemap[i].frames_left =
					tileset_get_delay(tileset, map->layers[z].tilemap[i].tile_index);
			}
		}

		// wrap things up
		map->bgm_file = strcmp(lstr_cstr(strings[1]), "") != 0
			? lstr_dup(strings[1]) : NULL;
		map->num_layers = rmp.num_layers;
		map->is_repeating = rmp.repeat_map;
		map->origin.x = rmp.start_x;
		map->origin.y = rmp.start_y;
		map->origin.z = rmp.start_layer;
		map->tileset = tileset;
		if (rmp.num_strings >= 5) {
			map->scripts[MAP_SCRIPT_ON_ENTER] = compile_script(strings[3], "%s/onEnter", filename);
			map->scripts[MAP_SCRIPT_ON_LEAVE] = compile_script(strings[4], "%s/onLeave", filename);
		}
		if (rmp.num_strings >= 9) {
			map->scripts[MAP_SCRIPT_ON_LEAVE_NORTH] = compile_script(strings[5], "%s/onLeave", filename);
			map->scripts[MAP_SCRIPT_ON_LEAVE_EAST] = compile_script(strings[6], "%s/onLeaveEast", filename);
			map->scripts[MAP_SCRIPT_ON_LEAVE_SOUTH] = compile_script(strings[7], "%s/onLeaveSouth", filename);
			map->scripts[MAP_SCRIPT_ON_LEAVE_WEST] = compile_script(strings[8], "%s/onLeaveWest", filename);
		}
		for (i = 0; i < rmp.num_strings; ++i)
			lstr_free(strings[i]);
		free(strings);
		break;
	default:
		goto on_error;
	}
	sfs_fclose(file);
	return map;

on_error:
	if (file != NULL) sfs_fclose(file);
	free(tile_data);
	if (strings != NULL) {
		for (i = 0; i < rmp.num_strings; ++i) lstr_free(strings[i]);
		free(strings);
	}
	if (map != NULL) {
		if (map->layers != NULL) {
			for (i = 0; i < rmp.num_layers; ++i) {
				lstr_free(map->layers[i].name);
				free(map->layers[i].tilemap);
				obsmap_free(map->layers[i].obsmap);
			}
			free(map->layers);
		}
		if (map->persons != NULL) {
			for (i = 0; i < map->num_persons; ++i) {
				lstr_free(map->persons[i].name);
				lstr_free(map->persons[i].spriteset);
				lstr_free(map->persons[i].create_script);
				lstr_free(map->persons[i].destroy_script);
				lstr_free(map->persons[i].command_script);
				lstr_free(map->persons[i].talk_script);
				lstr_free(map->persons[i].touch_script);
			}
			free(map->persons);
		}
		vector_free(map->triggers);
		vector_free(map->zones);
		free(map);
	}
	return NULL;
}

static void
free_map(struct map* map)
{
	struct map_trigger* trigger;
	struct map_zone*    zone;

	iter_t iter;
	int    i;

	if (map == NULL)
		return;
	for (i = 0; i < MAP_SCRIPT_MAX; ++i)
		free_script(map->scripts[i]);
	for (i = 0; i < map->num_layers; ++i) {
		free_script(map->layers[i].render_script);
		lstr_free(map->layers[i].name);
		free(map->layers[i].tilemap);
		obsmap_free(map->layers[i].obsmap);
	}
	for (i = 0; i < map->num_persons; ++i) {
		lstr_free(map->persons[i].name);
		lstr_free(map->persons[i].spriteset);
		lstr_free(map->persons[i].create_script);
		lstr_free(map->persons[i].destroy_script);
		lstr_free(map->persons[i].command_script);
		lstr_free(map->persons[i].talk_script);
		lstr_free(map->persons[i].touch_script);
	}
	iter = vector_enum(s_map->triggers);
	while (trigger = vector_next(&iter))
		free_script(trigger->script);
	iter = vector_enum(s_map->zones);
	while (zone = vector_next(&iter))
		free_script(zone->script);
	lstr_free(s_map->bgm_file);
	tileset_free(map->tileset);
	free(map->layers);
	free(map->persons);
	vector_free(map->triggers);
	vector_free(map->zones);
	free(map);
}

static bool
are_zones_at(int x, int y, int layer, int* out_count)
{
	int              count = 0;
	struct map_zone* zone;
	bool             zone_found;

	iter_t iter;

	zone_found = false;
	iter = vector_enum(s_map->zones);
	while (zone = vector_next(&iter)) {
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

	iter_t iter;

	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	iter = vector_enum(s_map->triggers);
	while (trigger = vector_next(&iter)) {
		if (trigger->z != layer && false)  // layer ignored for compatibility reasons
			continue;
		bounds.x1 = trigger->x - tile_w / 2;
		bounds.y1 = trigger->y - tile_h / 2;
		bounds.x2 = bounds.x1 + tile_w;
		bounds.y2 = bounds.y1 + tile_h;
		if (is_point_in_rect(x, y, bounds)) {
			found_item = trigger;
			if (out_index) *out_index = (int)iter.index;
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
	
	iter_t iter;
	int    i;

	iter = vector_enum(s_map->zones); i = -1;
	while (zone = vector_next(&iter)) {
		if (zone->layer != layer && false)  // layer ignored for compatibility
			continue;
		if (is_point_in_rect(x, y, zone->bounds) && which-- == 0) {
			found_item = zone;
			if (out_index) *out_index = (int)iter.index;
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
	person_t*          person;
	struct map_person* person_info;
	path_t*            path;
	spriteset_t*       spriteset = NULL;

	int i;

	console_log(2, "changing current map to `%s`", filename);
	
	map = load_map(filename);
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
		path = fs_make_path(lstr_cstr(person_info->spriteset), "spritesets", true);
		spriteset = load_spriteset(path_cstr(path));
		path_free(path);
		if (spriteset == NULL)
			goto on_error;
		if (!(person = create_person(lstr_cstr(person_info->name), spriteset, false, NULL)))
			goto on_error;
		free_spriteset(spriteset);
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

	// start up map BGM (if same as previous, leave alone)
	if (s_map->bgm_file == NULL && s_map_bgm_stream != NULL) {
		sound_free(s_map_bgm_stream);
		lstr_free(s_last_bgm_file);
		s_map_bgm_stream = NULL;
		s_last_bgm_file = NULL;
	}
	else if (s_map->bgm_file != NULL
		&& (s_last_bgm_file == NULL || lstr_cmp(s_map->bgm_file, s_last_bgm_file) != 0))
	{
		sound_free(s_map_bgm_stream);
		lstr_free(s_last_bgm_file);
		s_last_bgm_file = lstr_dup(s_map->bgm_file);
		path = fs_make_path(lstr_cstr(s_map->bgm_file), "sounds", true);
		if (s_map_bgm_stream = sound_new(path_cstr(path))) {
			sound_set_repeat(s_map_bgm_stream, true);
			sound_play(s_map_bgm_stream, s_bgm_mixer);
		}
		path_free(path);
	}

	// run map entry scripts
	run_script(s_def_scripts[MAP_SCRIPT_ON_ENTER], false);
	run_script(s_map->scripts[MAP_SCRIPT_ON_ENTER], false);

	s_frames = 0;
	return true;

on_error:
	free_spriteset(spriteset);
	free_map(s_map);
	return false;
}

static int
find_layer(const char* name)
{
	int i;

	for (i = 0; i < s_map->num_layers; ++i) {
		if (strcmp(name, lstr_cstr(s_map->layers[0].name)) == 0)
			return i;
	}
	return -1;
}

static void
map_screen_to_layer(int layer, int camera_x, int camera_y, int* inout_x, int* inout_y)
{
	rect_t bounds;
	int    center_x, center_y;
	int    layer_w, layer_h;
	float  plx_offset_x = 0.0, plx_offset_y = 0.0;
	int    tile_w, tile_h;
	int    x_offset, y_offset;
	
	// get layer and screen metrics
	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	layer_w = s_map->layers[layer].width * tile_w;
	layer_h = s_map->layers[layer].height * tile_h;
	center_x = g_res_x / 2;
	center_y = g_res_y / 2;

	// initial camera correction
	if (!s_map->is_repeating) {
		bounds = get_map_bounds();
		camera_x = fmin(fmax(camera_x, bounds.x1 + center_x), bounds.x2 - center_x);
		camera_y = fmin(fmax(camera_y, bounds.y1 + center_y), bounds.y2 - center_y);
	}

	// remap screen coordinates to layer coordinates
	plx_offset_x = s_frames * s_map->layers[layer].autoscroll_x
		- camera_x * (s_map->layers[layer].parallax_x - 1.0);
	plx_offset_y = s_frames * s_map->layers[layer].autoscroll_y
		- camera_y * (s_map->layers[layer].parallax_y - 1.0);
	x_offset = camera_x - center_x - plx_offset_x;
	y_offset = camera_y - center_y - plx_offset_y;
	if (!s_map->is_repeating && !s_map->layers[layer].is_parallax) {
		// if the map is smaller than the screen, windowbox it
		if (layer_w < g_res_x) x_offset = -(g_res_x - layer_w) / 2;
		if (layer_h < g_res_y) y_offset = -(g_res_y - layer_h) / 2;
	}
	if (inout_x) *inout_x += x_offset;
	if (inout_y) *inout_y += y_offset;

	// normalize coordinates. this simplifies rendering calculations.
	if (s_map->is_repeating || s_map->layers[layer].is_parallax) {
		if (inout_x) *inout_x = (*inout_x % layer_w + layer_w) % layer_w;
		if (inout_y) *inout_y = (*inout_y % layer_h + layer_h) % layer_h;
	}
}

static void
map_screen_to_map(int camera_x, int camera_y, int* inout_x, int* inout_y)
{
	rect_t bounds;
	int    center_x, center_y;
	int    map_w, map_h;
	int    tile_w, tile_h;
	int    x_offset, y_offset;

	// get layer and screen metrics
	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	map_w = s_map->width * tile_w;
	map_h = s_map->height * tile_h;
	center_x = g_res_x / 2;
	center_y = g_res_y / 2;

	// initial camera correction
	if (!s_map->is_repeating) {
		bounds = get_map_bounds();
		camera_x = fmin(fmax(camera_x, bounds.x1 + center_x), bounds.x2 - center_x);
		camera_y = fmin(fmax(camera_y, bounds.y1 + center_y), bounds.y2 - center_y);
	}

	// remap screen coordinates to map coordinates
	x_offset = camera_x - center_x;
	y_offset = camera_y - center_y;
	if (!s_map->is_repeating) {
		// if the map is smaller than the screen, windowbox it
		if (map_w < g_res_x) x_offset = -(g_res_x - map_w) / 2;
		if (map_h < g_res_y) y_offset = -(g_res_y - map_h) / 2;
	}
	if (inout_x) *inout_x += x_offset;
	if (inout_y) *inout_y += y_offset;
	
	// normalize coordinates
	if (s_map->is_repeating) {
		if (inout_x) *inout_x = (*inout_x % map_w + map_w) % map_w;
		if (inout_y) *inout_y = (*inout_y % map_h + map_h) % map_h;
	}
}

static void
process_map_input(void)
{
	int       mv_x, mv_y;
	person_t* person;

	int i;

	// clear out excess keys from key queue
	kb_clear_queue();
	
	// check for player control of input persons, if there are any
	for (i = 0; i < MAX_PLAYERS; ++i) {
		person = s_players[i].person;
		if (person != NULL) {
			if (kb_is_key_down(get_player_key(i, PLAYER_KEY_A))
				|| kb_is_key_down(s_players[i].talk_key)
				|| joy_is_button_down(i, s_talk_button))
			{
				if (s_players[i].is_talk_allowed)
					talk_person(person);
				s_players[i].is_talk_allowed = false;
			}
			else // allow talking again only after key is released
				s_players[i].is_talk_allowed = true;
			mv_x = 0; mv_y = 0;
			if (!is_person_busy(person)) {  // allow player control only if input person is idle
				if (kb_is_key_down(get_player_key(i, PLAYER_KEY_UP)) || joy_position(i, 1) <= -0.5) mv_y = -1;
				if (kb_is_key_down(get_player_key(i, PLAYER_KEY_RIGHT)) || joy_position(i, 0) >= 0.5) mv_x = 1;
				if (kb_is_key_down(get_player_key(i, PLAYER_KEY_DOWN)) || joy_position(i, 1) >= 0.5) mv_y = 1;
				if (kb_is_key_down(get_player_key(i, PLAYER_KEY_LEFT)) || joy_position(i, 0) <= -0.5) mv_x = -1;
			}
			switch (mv_x + mv_y * 3) {
			case -3: // north
				queue_person_command(person, COMMAND_MOVE_NORTH, true);
				queue_person_command(person, COMMAND_FACE_NORTH, true);
				queue_person_command(person, COMMAND_ANIMATE, false);
				break;
			case -2: // northeast
				queue_person_command(person, COMMAND_MOVE_NORTHEAST, true);
				queue_person_command(person, COMMAND_FACE_NORTHEAST, true);
				queue_person_command(person, COMMAND_ANIMATE, false);
				break;
			case 1: // east
				queue_person_command(person, COMMAND_MOVE_EAST, true);
				queue_person_command(person, COMMAND_FACE_EAST, true);
				queue_person_command(person, COMMAND_ANIMATE, false);
				break;
			case 4: // southeast
				queue_person_command(person, COMMAND_MOVE_SOUTHEAST, true);
				queue_person_command(person, COMMAND_FACE_SOUTHEAST, true);
				queue_person_command(person, COMMAND_ANIMATE, false);
				break;
			case 3: // south
				queue_person_command(person, COMMAND_MOVE_SOUTH, true);
				queue_person_command(person, COMMAND_FACE_SOUTH, true);
				queue_person_command(person, COMMAND_ANIMATE, false);
				break;
			case 2: // southwest
				queue_person_command(person, COMMAND_MOVE_SOUTHWEST, true);
				queue_person_command(person, COMMAND_FACE_SOUTHWEST, true);
				queue_person_command(person, COMMAND_ANIMATE, false);
				break;
			case -1: // west
				queue_person_command(person, COMMAND_MOVE_WEST, true);
				queue_person_command(person, COMMAND_FACE_WEST, true);
				queue_person_command(person, COMMAND_ANIMATE, false);
				break;
			case -4: // northwest
				queue_person_command(person, COMMAND_MOVE_NORTHWEST, true);
				queue_person_command(person, COMMAND_FACE_NORTHWEST, true);
				queue_person_command(person, COMMAND_ANIMATE, false);
				break;
			}
		}
	}

	update_bound_keys(true);
}

static void
render_map(void)
{
	bool              is_repeating;
	int               cell_x;
	int               cell_y;
	int               first_cell_x;
	int               first_cell_y;
	struct map_layer* layer;
	int               layer_height;
	int               layer_width;
	ALLEGRO_COLOR     overlay_color;
	int               tile_height;
	int               tile_index;
	int               tile_width;
	int               off_x, off_y;
	
	int x, y, z;
	
	if (screen_is_skipframe(g_screen))
		return;
	
	// render map layers from bottom to top (+Z = up)
	tileset_get_size(s_map->tileset, &tile_width, &tile_height);
	for (z = 0; z < s_map->num_layers; ++z) {
		layer = &s_map->layers[z];
		is_repeating = s_map->is_repeating || layer->is_parallax;
		layer_width = layer->width * tile_width;
		layer_height = layer->height * tile_height;
		off_x = 0; off_y = 0;
		map_screen_to_layer(z, s_cam_x, s_cam_y, &off_x, &off_y);

		// render person reflections if layer is reflective
		al_hold_bitmap_drawing(true);
		if (layer->is_reflective) {
			if (is_repeating) {  // for small repeating maps, persons need to be repeated as well
				for (y = 0; y < g_res_y / layer_height + 2; ++y) for (x = 0; x < g_res_x / layer_width + 2; ++x)
					render_persons(z, true, off_x - x * layer_width, off_y - y * layer_height);
			}
			else {
				render_persons(z, true, off_x, off_y);
			}
		}
		
		// render tiles, but only if the layer is visible
		if (layer->is_visible) {
			first_cell_x = off_x / tile_width;
			first_cell_y = off_y / tile_height;
			for (y = 0; y < g_res_y / tile_height + 2; ++y) for (x = 0; x < g_res_x / tile_width + 2; ++x) {
				cell_x = is_repeating ? (x + first_cell_x) % layer->width : x + first_cell_x;
				cell_y = is_repeating ? (y + first_cell_y) % layer->height : y + first_cell_y;
				if (cell_x < 0 || cell_x >= layer->width || cell_y < 0 || cell_y >= layer->height)
					continue;
				tile_index = layer->tilemap[cell_x + cell_y * layer->width].tile_index;
				tileset_draw(s_map->tileset, layer->color_mask, x * tile_width - off_x % tile_width, y * tile_height - off_y % tile_height, tile_index);
			}
		}

		// render persons
		if (is_repeating) {  // for small repeating maps, persons need to be repeated as well
			for (y = 0; y < g_res_y / layer_height + 2; ++y) for (x = 0; x < g_res_x / layer_width + 2; ++x)
				render_persons(z, false, off_x - x * layer_width, off_y - y * layer_height);
		}
		else {
			render_persons(z, false, off_x, off_y);
		}
		al_hold_bitmap_drawing(false);

		run_script(layer->render_script, false);
	}

	overlay_color = al_map_rgba(s_color_mask.r, s_color_mask.g, s_color_mask.b, s_color_mask.a);
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
	int                 num_zone_steps;
	script_t*           script_to_run;
	int                 script_type;
	double              start_x[MAX_PLAYERS];
	double              start_y[MAX_PLAYERS];
	int                 tile_w, tile_h;
	struct map_trigger* trigger;
	double              x, y, px, py;
	struct map_zone*    zone;

	int i, j, k;
	
	++s_frames;
	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	map_w = s_map->width * tile_w;
	map_h = s_map->height * tile_h;
	
	tileset_update(s_map->tileset);

	for (i = 0; i < MAX_PLAYERS; ++i) if (s_players[i].person != NULL)
		get_person_xy(s_players[i].person, &start_x[i], &start_y[i], false);
	update_persons();

	// update color mask fade level
	if (s_fade_progress < s_fade_frames) {
		++s_fade_progress;
		s_color_mask = color_mix(s_fade_color_to, s_fade_color_from,
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

	// if there are any input persons, check for trigger activation
	for (i = 0; i < MAX_PLAYERS; ++i) if (s_players[i].person != NULL) {
		// did we step on a trigger or move to a new one?
		get_person_xyz(s_players[i].person, &x, &y, &layer, true);
		trigger = get_trigger_at(x, y, layer, &index);
		if (trigger != s_on_trigger) {
			last_trigger = s_current_trigger;
			s_current_trigger = index;
			s_on_trigger = trigger;
			if (trigger)
				run_script(trigger->script, false);
			s_current_trigger = last_trigger;
		}
	}

	// update any zones occupied by the input person
	// note: a zone's step count is in reality a pixel count, so a zone
	//       may be updated multiple times in a single frame.
	for (k = 0; k < MAX_PLAYERS; ++k) if (s_players[k].person != NULL) {
		get_person_xy(s_players[k].person, &x, &y, false);
		px = abs(x - start_x[k]);
		py = abs(y - start_y[k]);
		num_zone_steps = px > py ? px : py;
		for (i = 0; i < num_zone_steps; ++i) {
			j = 0;
			while (zone = get_zone_at(x, y, layer, j++, &index)) {
				if (zone->steps_left-- <= 0) {
					last_zone = s_current_zone;
					s_current_zone = index;
					zone->steps_left = zone->interval;
					run_script(zone->script, true);
					s_current_zone = last_zone;
				}
			}
		}
	}
	
	// check if there are any delay scripts due to run this frame
	// and run the ones that are
	for (i = 0; i < s_num_delay_scripts; ++i) {
		if (s_delay_scripts[i].frames_left-- <= 0) {
			script_to_run = s_delay_scripts[i].script;
			for (j = i; j < s_num_delay_scripts - 1; ++j)
				s_delay_scripts[j] = s_delay_scripts[j + 1];
			--s_num_delay_scripts; --i;
			run_script(script_to_run, false);
			free_script(script_to_run);
		}
	}
	
	// now that everything else is in order, we can run the
	// update script!
	run_script(s_update_script, false);
}

void
init_map_engine_api(duk_context* ctx)
{
	api_register_method(ctx, NULL, "MapEngine", js_MapEngine);
	api_register_method(ctx, NULL, "AreZonesAt", js_AreZonesAt);
	api_register_method(ctx, NULL, "IsCameraAttached", js_IsCameraAttached);
	api_register_method(ctx, NULL, "IsInputAttached", js_IsInputAttached);
	api_register_method(ctx, NULL, "IsLayerReflective", js_IsLayerReflective);
	api_register_method(ctx, NULL, "IsLayerVisible", js_IsLayerVisible);
	api_register_method(ctx, NULL, "IsMapEngineRunning", js_IsMapEngineRunning);
	api_register_method(ctx, NULL, "IsTriggerAt", js_IsTriggerAt);
	api_register_method(ctx, NULL, "GetCameraPerson", js_GetCameraPerson);
	api_register_method(ctx, NULL, "GetCameraX", js_GetCameraX);
	api_register_method(ctx, NULL, "GetCameraY", js_GetCameraY);
	api_register_method(ctx, NULL, "GetCurrentMap", js_GetCurrentMap);
	api_register_method(ctx, NULL, "GetCurrentTrigger", js_GetCurrentTrigger);
	api_register_method(ctx, NULL, "GetCurrentZone", js_GetCurrentZone);
	api_register_method(ctx, NULL, "GetInputPerson", js_GetInputPerson);
	api_register_method(ctx, NULL, "GetLayerHeight", js_GetLayerHeight);
	api_register_method(ctx, NULL, "GetLayerMask", js_GetLayerMask);
	api_register_method(ctx, NULL, "GetLayerWidth", js_GetLayerWidth);
	api_register_method(ctx, NULL, "GetMapEngineFrameRate", js_GetMapEngineFrameRate);
	api_register_method(ctx, NULL, "GetNextAnimatedTile", js_GetNextAnimatedTile);
	api_register_method(ctx, NULL, "GetNumLayers", js_GetNumLayers);
	api_register_method(ctx, NULL, "GetNumTiles", js_GetNumTiles);
	api_register_method(ctx, NULL, "GetNumTriggers", js_GetNumTriggers);
	api_register_method(ctx, NULL, "GetNumZones", js_GetNumZones);
	api_register_method(ctx, NULL, "GetTalkActivationButton", js_GetTalkActivationButton);
	api_register_method(ctx, NULL, "GetTalkActivationKey", js_GetTalkActivationKey);
	api_register_method(ctx, NULL, "GetTile", js_GetTile);
	api_register_method(ctx, NULL, "GetTileDelay", js_GetTileDelay);
	api_register_method(ctx, NULL, "GetTileImage", js_GetTileImage);
	api_register_method(ctx, NULL, "GetTileHeight", js_GetTileHeight);
	api_register_method(ctx, NULL, "GetTileName", js_GetTileName);
	api_register_method(ctx, NULL, "GetTileSurface", js_GetTileSurface);
	api_register_method(ctx, NULL, "GetTileWidth", js_GetTileWidth);
	api_register_method(ctx, NULL, "GetTriggerLayer", js_GetTriggerLayer);
	api_register_method(ctx, NULL, "GetTriggerX", js_GetTriggerX);
	api_register_method(ctx, NULL, "GetTriggerY", js_GetTriggerY);
	api_register_method(ctx, NULL, "GetZoneHeight", js_GetZoneHeight);
	api_register_method(ctx, NULL, "GetZoneLayer", js_GetZoneLayer);
	api_register_method(ctx, NULL, "GetZoneSteps", js_GetZoneSteps);
	api_register_method(ctx, NULL, "GetZoneWidth", js_GetZoneWidth);
	api_register_method(ctx, NULL, "GetZoneX", js_GetZoneX);
	api_register_method(ctx, NULL, "GetZoneY", js_GetZoneY);
	api_register_method(ctx, NULL, "SetCameraX", js_SetCameraX);
	api_register_method(ctx, NULL, "SetCameraY", js_SetCameraY);
	api_register_method(ctx, NULL, "SetColorMask", js_SetColorMask);
	api_register_method(ctx, NULL, "SetDefaultMapScript", js_SetDefaultMapScript);
	api_register_method(ctx, NULL, "SetLayerHeight", js_SetLayerHeight);
	api_register_method(ctx, NULL, "SetLayerMask", js_SetLayerMask);
	api_register_method(ctx, NULL, "SetLayerReflective", js_SetLayerReflective);
	api_register_method(ctx, NULL, "SetLayerRenderer", js_SetLayerRenderer);
	api_register_method(ctx, NULL, "SetLayerSize", js_SetLayerSize);
	api_register_method(ctx, NULL, "SetLayerVisible", js_SetLayerVisible);
	api_register_method(ctx, NULL, "SetLayerWidth", js_SetLayerWidth);
	api_register_method(ctx, NULL, "SetMapEngineFrameRate", js_SetMapEngineFrameRate);
	api_register_method(ctx, NULL, "SetNextAnimatedTile", js_SetNextAnimatedTile);
	api_register_method(ctx, NULL, "SetRenderScript", js_SetRenderScript);
	api_register_method(ctx, NULL, "SetTalkActivationButton", js_SetTalkActivationButton);
	api_register_method(ctx, NULL, "SetTalkActivationKey", js_SetTalkActivationKey);
	api_register_method(ctx, NULL, "SetTile", js_SetTile);
	api_register_method(ctx, NULL, "SetTileDelay", js_SetTileDelay);
	api_register_method(ctx, NULL, "SetTileImage", js_SetTileImage);
	api_register_method(ctx, NULL, "SetTileName", js_SetTileName);
	api_register_method(ctx, NULL, "SetTileSurface", js_SetTileSurface);
	api_register_method(ctx, NULL, "SetTriggerLayer", js_SetTriggerLayer);
	api_register_method(ctx, NULL, "SetTriggerScript", js_SetTriggerScript);
	api_register_method(ctx, NULL, "SetTriggerXY", js_SetTriggerXY);
	api_register_method(ctx, NULL, "SetUpdateScript", js_SetUpdateScript);
	api_register_method(ctx, NULL, "SetZoneLayer", js_SetZoneLayer);
	api_register_method(ctx, NULL, "SetZoneMetrics", js_SetZoneMetrics);
	api_register_method(ctx, NULL, "SetZoneScript", js_SetZoneScript);
	api_register_method(ctx, NULL, "SetZoneSteps", js_SetZoneSteps);
	api_register_method(ctx, NULL, "AddTrigger", js_AddTrigger);
	api_register_method(ctx, NULL, "AddZone", js_AddZone);
	api_register_method(ctx, NULL, "AttachCamera", js_AttachCamera);
	api_register_method(ctx, NULL, "AttachInput", js_AttachInput);
	api_register_method(ctx, NULL, "AttachPlayerInput", js_AttachPlayerInput);
	api_register_method(ctx, NULL, "CallDefaultMapScript", js_CallDefaultMapScript);
	api_register_method(ctx, NULL, "CallMapScript", js_CallMapScript);
	api_register_method(ctx, NULL, "ChangeMap", js_ChangeMap);
	api_register_method(ctx, NULL, "DetachCamera", js_DetachCamera);
	api_register_method(ctx, NULL, "DetachInput", js_DetachInput);
	api_register_method(ctx, NULL, "DetachPlayerInput", js_DetachPlayerInput);
	api_register_method(ctx, NULL, "ExecuteTrigger", js_ExecuteTrigger);
	api_register_method(ctx, NULL, "ExecuteZones", js_ExecuteZones);
	api_register_method(ctx, NULL, "ExitMapEngine", js_ExitMapEngine);
	api_register_method(ctx, NULL, "MapToScreenX", js_MapToScreenX);
	api_register_method(ctx, NULL, "MapToScreenY", js_MapToScreenY);
	api_register_method(ctx, NULL, "RemoveTrigger", js_RemoveTrigger);
	api_register_method(ctx, NULL, "RemoveZone", js_RemoveZone);
	api_register_method(ctx, NULL, "RenderMap", js_RenderMap);
	api_register_method(ctx, NULL, "ReplaceTilesOnLayer", js_ReplaceTilesOnLayer);
	api_register_method(ctx, NULL, "ScreenToMapX", js_ScreenToMapX);
	api_register_method(ctx, NULL, "ScreenToMapY", js_ScreenToMapY);
	api_register_method(ctx, NULL, "SetDelayScript", js_SetDelayScript);
	api_register_method(ctx, NULL, "UpdateMapEngine", js_UpdateMapEngine);

	// Map script types
	api_register_const(ctx, NULL, "SCRIPT_ON_ENTER_MAP", MAP_SCRIPT_ON_ENTER);
	api_register_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP", MAP_SCRIPT_ON_LEAVE);
	api_register_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_NORTH", MAP_SCRIPT_ON_LEAVE_NORTH);
	api_register_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_EAST", MAP_SCRIPT_ON_LEAVE_EAST);
	api_register_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_SOUTH", MAP_SCRIPT_ON_LEAVE_SOUTH);
	api_register_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_WEST", MAP_SCRIPT_ON_LEAVE_WEST);

	// initialize subcomponent APIs (persons, etc.)
	init_persons_api();
}

int
duk_require_map_layer(duk_context* ctx, duk_idx_t index)
{
	int         layer;
	const char* name;

	duk_require_type_mask(ctx, index, DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER);
	if (duk_is_number(ctx, index)) {
		layer = duk_get_int(ctx, index);
		if (layer < 0 || layer >= s_map->num_layers)
			duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "layer index out of range (%d)", layer);
	}
	else {
		name = duk_get_string(ctx, index);
		if ((layer = find_layer(name)) == -1)
			duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no layer exists with name `%s`", name);
	}
	return layer;
}

static duk_ret_t
js_MapEngine(duk_context* ctx)
{
	const char* filename;
	int         framerate;
	int         num_args;
	
	num_args = duk_get_top(ctx);
	filename = duk_require_path(ctx, 0, "maps", true);
	framerate = num_args >= 2 ? duk_require_int(ctx, 1)
		: g_framerate;

	s_is_map_running = true;
	s_exiting = false;
	s_color_mask = color_new(0, 0, 0, 0);
	s_fade_color_to = s_fade_color_from = s_color_mask;
	s_fade_progress = s_fade_frames = 0;
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	s_framerate = framerate;
	if (!change_map(filename, true))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "MapEngine(): unable to load map file `%s` into map engine", filename);
	while (!s_exiting) {
		render_map();
		screen_flip(g_screen, s_framerate);
		update_map_engine(true);
		process_map_input();
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
	const char* name;
	person_t*   person;
	int         player;

	int i;

	if (duk_get_top(ctx) < 1)
		player = 0;
	else if (duk_is_string(ctx, 0)) {
		name = duk_get_string(ctx, 0);
		if (!(person = find_person(name)))
			duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "IsInputAttached(): person `%s` doesn't exist", name);
		player = -1;
		for (i = MAX_PLAYERS - 1; i >= 0; --i)  // ensures Sphere semantics
			player = s_players[i].person == person ? i : player;
		if (player == -1) return duk_push_false(ctx), 1;
	}
	else if (duk_is_number(ctx, 0))
		player = duk_get_int(ctx, 0);
	else
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "IsInputAttached(): player number or string expected");
	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "IsInputAttached(): player number out of range (%d)", player);
	duk_push_boolean(ctx, s_players[player].person != NULL);
	return 1;
}

static duk_ret_t
js_IsLayerReflective(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IsLayerReflective(): map engine not running");
	duk_push_boolean(ctx, s_map->layers[layer].is_reflective);
	return 1;
}

static duk_ret_t
js_IsLayerVisible(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IsLayerVisible(): map engine not running");
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
	
	duk_push_boolean(ctx, get_trigger_at(x, y, layer, NULL) != NULL);
	return 1;
}

static duk_ret_t
js_GetCameraPerson(duk_context* ctx)
{
	if (s_camera_person == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCameraPerson(): invalid operation, camera not attached");
	duk_push_string(ctx, get_person_name(s_camera_person));
	return 1;
}

static duk_ret_t
js_GetCameraX(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCameraX(): map engine not running");
	duk_push_int(ctx, s_cam_x);
	return 1;
}

static duk_ret_t
js_GetCameraY(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCameraX(): map engine not running");
	duk_push_int(ctx, s_cam_y);
	return 1;
}

static duk_ret_t
js_GetCurrentMap(duk_context* ctx)
{
	path_t* map_name;
	path_t* origin;
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentMap(): map engine not running");
	
	// GetCurrentMap() in Sphere 1.x returns the map path
	// relative to the 'maps' directory.
	map_name = path_new(s_map_filename);
	if (!path_is_rooted(map_name)) {
		origin = path_new("maps/");
		path_relativize(map_name, origin);
		path_free(origin);
	}
	duk_push_string(ctx, path_cstr(map_name));
	path_free(map_name);
	return 1;
}

static duk_ret_t
js_GetCurrentTrigger(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentTrigger(): map engine not running");
	if (s_current_trigger == -1)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentTrigger(): cannot call outside of a trigger script");
	duk_push_int(ctx, s_current_trigger);
	return 1;
}

static duk_ret_t
js_GetCurrentZone(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentZone(): map engine not running");
	if (s_current_zone == -1)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentZone(): cannot call outside of a zone script");
	duk_push_int(ctx, s_current_zone);
	return 1;
}

static duk_ret_t
js_GetInputPerson(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int player = n_args >= 1 ? duk_require_int(ctx, 0) : 0;
	
	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetInputPerson(): player number out of range (%d)", player);
	if (s_players[player].person == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetInputPerson(): input not attached for player %d", player + 1);
	duk_push_string(ctx, get_person_name(s_players[player].person));
	return 1;
}

static duk_ret_t
js_GetLayerHeight(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerHeight(): map engine not running");
	duk_push_int(ctx, s_map->layers[layer].height);
	return 1;
}

static duk_ret_t
js_GetLayerMask(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerMask(): map engine not running");
	duk_push_sphere_color(ctx, s_map->layers[layer].color_mask);
	return 1;
}

static duk_ret_t
js_GetLayerName(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerName(): map engine not running");
	duk_push_lstring_t(ctx, s_map->layers[layer].name);
	return 1;
}

static duk_ret_t
js_GetLayerWidth(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetLayerWidth(): map engine not running");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetNextAnimatedTile(): map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetNextAnimatedTile(): invalid tile index (%d)", tile_index);
	duk_push_int(ctx, tileset_get_next(s_map->tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetNumLayers(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetNumLayers(): map engine not running");
	duk_push_int(ctx, s_map->num_layers);
	return 1;
}

static duk_ret_t
js_GetNumTiles(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetNumTiles(): map engine not running");
	duk_push_int(ctx, tileset_len(s_map->tileset));
	return 1;
}

static duk_ret_t
js_GetNumTriggers(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetNumTriggers(): map engine not running");
	duk_push_number(ctx, vector_len(s_map->triggers));
	return 1;
}

static duk_ret_t
js_GetNumZones(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetNumZones(): map engine not running");
	duk_push_number(ctx, vector_len(s_map->zones));
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
	int n_args = duk_get_top(ctx);
	int player = n_args >= 1 ? duk_require_int(ctx, 0) : 0;

	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTalkActivationKey(): player number out of range (%d)", player);
	duk_push_int(ctx, s_players[player].talk_key);
	return 1;
}

static duk_ret_t
js_GetTile(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);

	int              layer_w, layer_h;
	struct map_tile* tilemap;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTile(): map engine not running");
	layer_w = s_map->layers[layer].width;
	layer_h = s_map->layers[layer].height;
	tilemap = s_map->layers[layer].tilemap;
	duk_push_int(ctx, tilemap[x + y * layer_w].tile_index);
	return 1;
}

static duk_ret_t
js_GetTileDelay(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileDelay(): map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTileDelay(): invalid tile index (%d)", tile_index);
	duk_push_int(ctx, tileset_get_delay(s_map->tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetTileHeight(duk_context* ctx)
{
	int w, h;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileHeight(): map engine not running");
	tileset_get_size(s_map->tileset, &w, &h);
	duk_push_int(ctx, h);
	return 1;
}

static duk_ret_t
js_GetTileImage(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileImage(): map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTileImage(): invalid tile index (%d)", tile_index);
	duk_push_sphere_obj(ctx, "ssImage", image_ref(tileset_get_image(s_map->tileset, tile_index)));
	return 1;
}

static duk_ret_t
js_GetTileName(duk_context* ctx)
{
	int        tile_index = duk_require_int(ctx, 0);
	
	const lstring_t* tile_name;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileName(): map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTileName(): invalid tile index (%d)", tile_index);
	tile_name = tileset_get_name(s_map->tileset, tile_index);
	duk_push_lstring_t(ctx, tile_name);
	return 1;
}

static duk_ret_t
js_GetTileSurface(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);

	image_t* image;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileSurface(): map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTileSurface(): invalid tile index (%d)", tile_index);
	if ((image = image_clone(tileset_get_image(s_map->tileset, tile_index))) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileSurface(): unable to create new surface image");
	duk_push_sphere_obj(ctx, "ssSurface", image);
	return 1;
}

static duk_ret_t
js_GetTileWidth(duk_context* ctx)
{
	int w, h;
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTileWidth(): map engine not running");
	tileset_get_size(s_map->tileset, &w, &h);
	duk_push_int(ctx, w);
	return 1;
}

static duk_ret_t
js_GetTriggerLayer(duk_context* ctx)
{
	int trigger_index = duk_require_int(ctx, 0);

	int layer;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTriggerLayer(): map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTriggerLayer(): invalid trigger index (%d)", trigger_index);
	get_trigger_xyz(trigger_index, NULL, NULL, &layer);
	duk_push_int(ctx, layer);
	return 1;
}

static duk_ret_t
js_GetTriggerX(duk_context* ctx)
{
	int trigger_index = duk_require_int(ctx, 0);

	int x;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTriggerX(): map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTriggerX(): invalid trigger index (%d)", trigger_index);
	get_trigger_xyz(trigger_index, &x, NULL, NULL);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_GetTriggerY(duk_context* ctx)
{
	int trigger_index = duk_require_int(ctx, 0);

	int y;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetTriggerX(): map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTriggerX(): invalid trigger index (%d)", trigger_index);
	get_trigger_xyz(trigger_index, NULL, &y, NULL);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_GetZoneHeight(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	rect_t bounds;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneHeight(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneHeight(): invalid zone index (%d)", zone_index);
	bounds = get_zone_bounds(zone_index);
	duk_push_int(ctx, bounds.y2 - bounds.y1);
	return 1;
}

static duk_ret_t
js_GetZoneLayer(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneLayer(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneLayer(): invalid zone index (%d)", zone_index);
	duk_push_int(ctx, get_zone_layer(zone_index));
	return 1;
}

static duk_ret_t
js_GetZoneSteps(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneLayer(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneLayer(): invalid zone index (%d)", zone_index);
	duk_push_int(ctx, get_zone_steps(zone_index));
	return 1;
}

static duk_ret_t
js_GetZoneWidth(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	rect_t bounds;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneWidth(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneWidth(): invalid zone index (%d)", zone_index);
	bounds = get_zone_bounds(zone_index);
	duk_push_int(ctx, bounds.x2 - bounds.x1);
	return 1;
}

static duk_ret_t
js_GetZoneX(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	rect_t bounds;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneX(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneX(): invalid zone index (%d)", zone_index);
	bounds = get_zone_bounds(zone_index);
	duk_push_int(ctx, bounds.x1);
	return 1;
}

static duk_ret_t
js_GetZoneY(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	rect_t bounds;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetZoneY(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetZoneY(): invalid zone index (%d)", zone_index);
	bounds = get_zone_bounds(zone_index);
	duk_push_int(ctx, bounds.y1);
	return 1;
}

static duk_ret_t
js_SetCameraX(duk_context* ctx)
{
	int new_x = duk_require_int(ctx, 0);
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetCameraX(): map engine not running");
	s_cam_x = new_x;
	return 0;
}

static duk_ret_t
js_SetCameraY(duk_context* ctx)
{
	int new_y = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetCameraY(): map engine not running");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetColorMask(): map engine not running");
	if (frames < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetColorMask(): frames must be positive (got: %d)", frames);
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
	const char* script_name = (script_type == MAP_SCRIPT_ON_ENTER) ? "synth:onLoadMap.js"
		: (script_type == MAP_SCRIPT_ON_LEAVE) ? "synth:onUnloadMap.js"
		: (script_type == MAP_SCRIPT_ON_LEAVE_NORTH) ? "synth:onLeaveMapNorth.js"
		: (script_type == MAP_SCRIPT_ON_LEAVE_EAST) ? "synth:onLeaveMapEast.js"
		: (script_type == MAP_SCRIPT_ON_LEAVE_SOUTH) ? "synth:onLeaveMapSouth.js"
		: (script_type == MAP_SCRIPT_ON_LEAVE_WEST) ? "synth:onLeaveMapWest.js"
		: NULL;
	script_t* script = duk_require_sphere_script(ctx, 1, script_name);

	if (script_type < 0 || script_type >= MAP_SCRIPT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetDefaultMapScript(): invalid map script constant");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetDelayScript(): map engine not running");
	if (frames < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetDelayScript(): frames must be positive");
	if (++s_num_delay_scripts > s_max_delay_scripts) {
		s_max_delay_scripts = s_num_delay_scripts * 2;
		if (!(s_delay_scripts = realloc(s_delay_scripts, s_max_delay_scripts * sizeof(struct delay_script))))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetDelayScript(): unable to enlarge delay script queue");
	}
	delay = &s_delay_scripts[s_num_delay_scripts - 1];
	delay->script = script;
	delay->frames_left = frames;
	return 0;
}

static duk_ret_t
js_SetLayerHeight(duk_context* ctx)
{
	int layer;
	int new_height;

	layer = duk_require_map_layer(ctx, 0);
	new_height = duk_require_int(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerHeight(): map engine not running");
	if (new_height <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerHeight(): height must be positive and nonzero (got: %d)", new_height);
	if (!resize_map_layer(layer, s_map->layers[layer].width, new_height))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerHeight(): unable to resize layer %d", layer);
	return 0;
}

static duk_ret_t
js_SetLayerMask(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	color_t mask = duk_require_sphere_color(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerMask(): map engine not running");
	s_map->layers[layer].color_mask = mask;
	return 0;
}

static duk_ret_t
js_SetLayerReflective(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	bool is_reflective = duk_require_boolean(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerReflective(): map engine not running");
	s_map->layers[layer].is_reflective = is_reflective;
	return 0;
}

static duk_ret_t
js_SetLayerRenderer(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	
	char      script_name[50];
	script_t* script;
	
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerRenderer(): map engine not running");
	sprintf(script_name, "[layer %d render script]", layer);
	script = duk_require_sphere_script(ctx, 1, script_name);
	free_script(s_map->layers[layer].render_script);
	s_map->layers[layer].render_script = script;
	return 0;
}

static duk_ret_t
js_SetLayerSize(duk_context* ctx)
{
	int layer;
	int new_height;
	int new_width;

	layer = duk_require_map_layer(ctx, 0);
	new_width = duk_require_int(ctx, 1);
	new_height = duk_require_int(ctx, 2);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerHeight(): map engine not running");
	if (new_width <= 0 || new_height <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerHeight(): dimensions must be positive and nonzero (got W: %d, H: %d)", new_width, new_height);
	if (!resize_map_layer(layer, new_width, new_height))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerHeight(): unable to resize layer %d", layer);
	return 0;
}

static duk_ret_t
js_SetLayerVisible(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	bool is_visible = duk_require_boolean(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerVisible(): map engine not running");
	s_map->layers[layer].is_visible = is_visible;
	return 0;
}

static duk_ret_t
js_SetLayerWidth(duk_context* ctx)
{
	int layer;
	int new_width;

	layer = duk_require_map_layer(ctx, 0);
	new_width = duk_require_int(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerWidth(): map engine not running");
	if (new_width <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerWidth(): width must be positive and nonzero (got: %d)", new_width);
	if (!resize_map_layer(layer, new_width, s_map->layers[layer].height))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetLayerWidth(): unable to resize layer %d", layer);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetNextAnimatedTile(): map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetNextAnimatedTile(): invalid tile index (%d)", tile_index);
	if (next_index < 0 || next_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetNextAnimatedTile(): invalid tile index for next tile (%d)", tile_index);
	tileset_set_next(s_map->tileset, tile_index, next_index);
	return 0;
}

static duk_ret_t
js_SetRenderScript(duk_context* ctx)
{
	script_t* script = duk_require_sphere_script(ctx, 0, "default/renderScript");

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
	int n_args = duk_get_top(ctx);
	int key = duk_require_int(ctx, 0);
	int player = n_args >= 2 ? duk_require_int(ctx, 1) : 0;
	
	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "GetTalkActivationKey(): player number out of range (%d)", player);
	s_players[player].talk_key = key;
	return 0;
}

static duk_ret_t
js_SetTile(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);
	int tile_index = duk_require_int(ctx, 3);

	int              layer_w, layer_h;
	struct map_tile* tilemap;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTile(): map engine not running");
	layer_w = s_map->layers[layer].width;
	layer_h = s_map->layers[layer].height;
	tilemap = s_map->layers[layer].tilemap;
	tilemap[x + y * layer_w].tile_index = tile_index;
	tilemap[x + y * layer_w].frames_left = tileset_get_delay(s_map->tileset, tile_index);
	return 0;
}

static duk_ret_t
js_SetTileDelay(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);
	int delay = duk_require_int(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTileDelay(): map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTileDelay(): invalid tile index (%d)", tile_index);
	if (delay < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTileDelay(): delay must be positive (got: %d)", delay);
	tileset_set_delay(s_map->tileset, tile_index, delay);
	return 0;
}

static duk_ret_t
js_SetTileImage(duk_context* ctx)
{
	image_t* image;
	int      image_h;
	int      image_w;
	int      tile_index;
	int      tile_h;
	int      tile_w;

	tile_index = duk_require_int(ctx, 0);
	image = duk_require_sphere_obj(ctx, 1, "ssImage");

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "map engine is not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	image_w = image_width(image);
	image_h = image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "image dimensions don't match tile dimensions");
	tileset_set_image(s_map->tileset, tile_index, image);
	return 0;
}

static duk_ret_t
js_SetTileName(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);
	lstring_t* name = duk_require_lstring_t(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTileName(): map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTileName(): invalid tile index (%d)", tile_index);
	if (!tileset_set_name(s_map->tileset, tile_index, name))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTileName(): unable to set tile name");
	lstr_free(name);
	return 0;
}

static duk_ret_t
js_SetTileSurface(duk_context* ctx)
{
	int tile_index = duk_require_int(ctx, 0);
	image_t* image = duk_require_sphere_obj(ctx, 1, "ssSurface");

	int image_w, image_h;
	int num_tiles;
	int tile_w, tile_h;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTileSurface(): map engine not running");
	num_tiles = tileset_len(s_map->tileset);
	if (tile_index < 0 || tile_index >= num_tiles)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTileSurface(): invalid tile index (%d)", tile_index);
	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	image_w = image_width(image);
	image_h = image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "SetTileSurface(): surface dimensions (%dx%d) don't match tile dimensions (%dx%d)", image_w, image_h, tile_w, tile_h);
	tileset_set_image(s_map->tileset, tile_index, image);
	return 0;
}

static duk_ret_t
js_SetTriggerLayer(duk_context* ctx)
{
	int trigger_index = duk_require_int(ctx, 0);
	int layer = duk_require_int(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTriggerLayer(): map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTriggerLayer(): invalid trigger index (%d)", trigger_index);
	set_trigger_layer(trigger_index, layer);
	return 0;
}

static duk_ret_t
js_SetTriggerScript(duk_context* ctx)
{
	int trigger_index = duk_require_int(ctx, 0);
	script_t* script;

	lstring_t* script_name;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTriggerScript(): map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTriggerScript(): invalid trigger index (%d)", trigger_index);
	if (!(script_name = lstr_newf("%s/trigger~%d/onStep", get_map_name(), trigger_index)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTriggerScript(): unable to compile trigger script");
	script = duk_require_sphere_script(ctx, 1, lstr_cstr(script_name));
	lstr_free(script_name);
	set_trigger_script(trigger_index, script);
	free_script(script);
	return 0;
}

static duk_ret_t
js_SetTriggerXY(duk_context* ctx)
{
	int trigger_index = duk_require_int(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetTriggerXY(): map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTriggerXY(): invalid trigger index (%d)", trigger_index);
	set_trigger_xy(trigger_index, x, y);
	return 0;
}

static duk_ret_t
js_SetUpdateScript(duk_context* ctx)
{
	script_t* script = duk_require_sphere_script(ctx, 0, "default/updateScript");

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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetZoneLayer(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetZoneLayer(): invalid zone index (%d)", zone_index);
	set_zone_layer(zone_index, layer);
	return 0;
}

static duk_ret_t
js_SetZoneMetrics(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int zone_index = duk_require_int(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);
	int width = duk_require_int(ctx, 3);
	int height = duk_require_int(ctx, 4);
	int layer = n_args >= 6 ? duk_require_map_layer(ctx, 5) : -1;

	rect_t map_bounds;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetZoneDimensions(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetZoneDimensions(): invalid zone index (%d)", zone_index);
	map_bounds = get_map_bounds();
	if (width <= 0 || height <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetZoneDimensions(): width and height must be greater than zero");
	if (x < map_bounds.x1 || y < map_bounds.y1 || x + width > map_bounds.x2 || y + height > map_bounds.y2)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetZoneDimensions(): zone cannot extend outside map (%d,%d,%d,%d)", x, y, width, height);
	set_zone_bounds(zone_index, new_rect(x, y, x + width, y + height));
	if (layer >= 0)
		set_zone_layer(zone_index, layer);
	return 0;
}

static duk_ret_t
js_SetZoneScript(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);
	script_t* script;
	
	lstring_t* script_name;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetZoneScript(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetZoneScript(): invalid zone index `%d`", zone_index);
	if (!(script_name = lstr_newf("%s/zone%d", get_map_name(), zone_index)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetZoneScript(): error compiling zone script");
	script = duk_require_sphere_script(ctx, 1, lstr_cstr(script_name));
	lstr_free(script_name);
	set_zone_script(zone_index, script);
	free_script(script);
	return 0;
}

static duk_ret_t
js_SetZoneSteps(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);
	int steps = duk_require_int(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetZoneLayer(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetZoneLayer(): invalid zone index `%d`", zone_index);
	if (steps <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetZoneSteps(): steps must be positive (got: %d)", steps);
	set_zone_steps(zone_index, steps);
	return 0;
}

static duk_ret_t
js_AddTrigger(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);
	lstring_t* script_name;

	rect_t     bounds;
	script_t*  script;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "AddTrigger(): map engine not running");
	bounds = get_map_bounds();
	if (x < bounds.x1 || y < bounds.y1 || x >= bounds.x2 || y >= bounds.y2)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "AddTrigger(): trigger must be inside map (got X: %d, Y: %d)", x, y);
	if (!(script_name = lstr_newf("%s/trig%d", get_map_name(), vector_len(s_map->triggers))))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "AddTrigger(): unable to compile trigger script");
	script = duk_require_sphere_script(ctx, 3, lstr_cstr(script_name));
	lstr_free(script_name);
	if (!add_trigger(x, y, layer, script))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "AddTrigger(): unable to add trigger to map");
	duk_push_number(ctx, vector_len(s_map->triggers) - 1);
	return 1;
}

static duk_ret_t
js_AddZone(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int width = duk_require_int(ctx, 2);
	int height = duk_require_int(ctx, 3);
	int layer = duk_require_map_layer(ctx, 4);
	lstring_t* script_name;

	rect_t     bounds;
	script_t*  script;

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "AddZone(): map engine not running");
	bounds = get_map_bounds();
	if (width <= 0 || height <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "AddZone(): width and height must be greater than zero");
	if (x < bounds.x1 || y < bounds.y1 || x + width > bounds.x2 || y + height > bounds.y2)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "AddZone(): zone must be inside map (got X: %d, Y: %d, W: %d,H: %d)", x, y, width, height);
	if (!(script_name = lstr_newf("%s/zone%d", get_map_name(), vector_len(s_map->zones))))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "AddZone(): unable to compile zone script");
	script = duk_require_sphere_script(ctx, 5, lstr_cstr(script_name));
	lstr_free(script_name);
	if (!add_zone(new_rect(x, y, width, height), layer, script, 8))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "AddZone(): unable to add zone to map");
	duk_push_number(ctx, vector_len(s_map->zones) - 1);
	return 1;
}

static duk_ret_t
js_AttachCamera(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "AttachCamera(): person `%s` doesn't exist", name);
	s_camera_person = person;
	return 0;
}

static duk_ret_t
js_AttachInput(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	
	person_t*   person;

	int i;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "AttachInput(): person `%s` doesn't exist", name);
	for (i = 0; i < MAX_PLAYERS; ++i)  // detach person from other players
		if (s_players[i].person == person) s_players[i].person = NULL;
	s_players[0].person = person;
	return 0;
}

static duk_ret_t
js_AttachPlayerInput(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int player = duk_require_int(ctx, 1);

	person_t*   person;

	int i;

	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "AttachPlayerInput(): player number out of range (%d)", player);
	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "AttachInput(): person `%s` doesn't exist", name);
	for (i = 0; i < MAX_PLAYERS; ++i)  // detach person from other players
		if (s_players[i].person == person) s_players[i].person = NULL;
	s_players[player].person = person;
	return 0;
}

static duk_ret_t
js_CallDefaultMapScript(duk_context* ctx)
{
	int type = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CallDefaultMapScript(): map engine not running");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "CallDefaultMapScript(): invalid script type constant");
	run_script(s_def_scripts[type], false);
	return 0;
}

static duk_ret_t
js_CallMapScript(duk_context* ctx)
{
	int type = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CallMapScript(): map engine not running");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "CallMapScript(): invalid script type constant");
	run_script(s_map->scripts[type], false);
	return 0;
}

static duk_ret_t
js_ChangeMap(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, "maps", true);
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ChangeMap(): map engine not running");
	if (!change_map(filename, false))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ChangeMap(): unable to load map file `%s`", filename);
	return 0;
}

static duk_ret_t
js_RemoveTrigger(duk_context* ctx)
{
	int trigger_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RemoveTrigger(): map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RemoveTrigger(): invalid trigger index (%d)", trigger_index);
	remove_trigger(trigger_index);
	return 0;
}

static duk_ret_t
js_RemoveZone(duk_context* ctx)
{
	int zone_index = duk_require_int(ctx, 0);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RemoveZone(): map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RemoveZone(): invalid zone index (%d)", zone_index);
	remove_zone(zone_index);
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
	s_players[0].person = NULL;
	return 0;
}

static duk_ret_t
js_DetachPlayerInput(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	int         player;

	int i;

	if (duk_get_top(ctx) < 1)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "DetachPlayerInput(): player number or string expected as argument");
	else if (duk_is_string(ctx, 0)) {
		name = duk_get_string(ctx, 0);
		if (!(person = find_person(name)))
			duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "DetachPlayerInput(): person `%s` doesn't exist", name);
		player = -1;
		for (i = MAX_PLAYERS - 1; i >= 0; --i)  // ensures Sphere semantics
			player = s_players[i].person == person ? i : player;
		if (player == -1)
			return 0;
	}
	else if (duk_is_number(ctx, 0))
		player = duk_get_int(ctx, 0);
	else
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "DetachPlayerInput(): player number or string expected as argument");
	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "DetachPlayerInput(): player number out of range (%d)", player);
	s_players[player].person = NULL;
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ExecuteTrigger(): map engine not running");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ExecuteZones(): map engine not running");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ExitMapEngine(): map engine not running");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "MapToScreenX(): map engine not running");
	offset_x = 0;
	map_screen_to_map(s_cam_x, s_cam_y, &offset_x, NULL);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "MapToScreenY(): map engine not running");
	offset_y = 0;
	map_screen_to_map(s_cam_x, s_cam_y, NULL, &offset_y);
	duk_push_int(ctx, y - offset_y);
	return 1;
}

static duk_ret_t
js_RenderMap(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RenderMap(): map engine not running");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ReplaceTilesOnLayer(): map engine not running");
	if (old_index < 0 || old_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ReplaceTilesOnLayer(): old invalid tile index (%d)", old_index);
	if (new_index < 0 || new_index >= tileset_len(s_map->tileset))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ReplaceTilesOnLayer(): new invalid tile index (%d)", new_index);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ScreenToMapX(): map engine not running");
	map_screen_to_map(s_cam_x, s_cam_y, &x, NULL);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_ScreenToMapY(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	int y = duk_require_int(ctx, 1);

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ScreenToMapY(): map engine not running");
	map_screen_to_map(s_cam_x, s_cam_y, NULL, &y);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_UpdateMapEngine(duk_context* ctx)
{
	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "UpdateMapEngine(): map engine not running");
	update_map_engine(false);
	return 0;
}
