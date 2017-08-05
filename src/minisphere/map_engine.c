#include "minisphere.h"
#include "map_engine.h"

#include "api.h"
#include "audio.h"
#include "color.h"
#include "image.h"
#include "input.h"
#include "obstruction.h"
#include "script.h"
#include "spriteset.h"
#include "tileset.h"
#include "vanilla.h"
#include "vector.h"

#define MAX_PLAYERS 4

static duk_ret_t js_AddTrigger                   (duk_context* ctx);
static duk_ret_t js_AddZone                      (duk_context* ctx);
static duk_ret_t js_AreZonesAt                   (duk_context* ctx);
static duk_ret_t js_AttachCamera                 (duk_context* ctx);
static duk_ret_t js_AttachInput                  (duk_context* ctx);
static duk_ret_t js_AttachPlayerInput            (duk_context* ctx);
static duk_ret_t js_CallDefaultMapScript         (duk_context* ctx);
static duk_ret_t js_CallDefaultPersonScript      (duk_context* ctx);
static duk_ret_t js_CallMapScript                (duk_context* ctx);
static duk_ret_t js_CallPersonScript             (duk_context* ctx);
static duk_ret_t js_ChangeMap                    (duk_context* ctx);
static duk_ret_t js_ClearPersonCommands          (duk_context* ctx);
static duk_ret_t js_CreatePerson                 (duk_context* ctx);
static duk_ret_t js_DestroyPerson                (duk_context* ctx);
static duk_ret_t js_DetachCamera                 (duk_context* ctx);
static duk_ret_t js_DetachInput                  (duk_context* ctx);
static duk_ret_t js_DetachPlayerInput            (duk_context* ctx);
static duk_ret_t js_DoesPersonExist              (duk_context* ctx);
static duk_ret_t js_ExecuteTrigger               (duk_context* ctx);
static duk_ret_t js_ExecuteZoneScript            (duk_context* ctx);
static duk_ret_t js_ExecuteZones                 (duk_context* ctx);
static duk_ret_t js_ExitMapEngine                (duk_context* ctx);
static duk_ret_t js_FollowPerson                 (duk_context* ctx);
static duk_ret_t js_GetActingPerson              (duk_context* ctx);
static duk_ret_t js_GetCameraPerson              (duk_context* ctx);
static duk_ret_t js_GetCameraX                   (duk_context* ctx);
static duk_ret_t js_GetCameraY                   (duk_context* ctx);
static duk_ret_t js_GetCurrentMap                (duk_context* ctx);
static duk_ret_t js_GetCurrentPerson             (duk_context* ctx);
static duk_ret_t js_GetCurrentTrigger            (duk_context* ctx);
static duk_ret_t js_GetCurrentZone               (duk_context* ctx);
static duk_ret_t js_GetInputPerson               (duk_context* ctx);
static duk_ret_t js_GetLayerHeight               (duk_context* ctx);
static duk_ret_t js_GetLayerMask                 (duk_context* ctx);
static duk_ret_t js_GetLayerName                 (duk_context* ctx);
static duk_ret_t js_GetLayerWidth                (duk_context* ctx);
static duk_ret_t js_GetMapEngineFrameRate        (duk_context* ctx);
static duk_ret_t js_GetNextAnimatedTile          (duk_context* ctx);
static duk_ret_t js_GetNumLayers                 (duk_context* ctx);
static duk_ret_t js_GetNumTiles                  (duk_context* ctx);
static duk_ret_t js_GetNumTriggers               (duk_context* ctx);
static duk_ret_t js_GetNumZones                  (duk_context* ctx);
static duk_ret_t js_GetObstructingPerson         (duk_context* ctx);
static duk_ret_t js_GetObstructingTile           (duk_context* ctx);
static duk_ret_t js_GetPersonAngle               (duk_context* ctx);
static duk_ret_t js_GetPersonBase                (duk_context* ctx);
static duk_ret_t js_GetPersonData                (duk_context* ctx);
static duk_ret_t js_GetPersonDirection           (duk_context* ctx);
static duk_ret_t js_GetPersonFollowDistance      (duk_context* ctx);
static duk_ret_t js_GetPersonFollowers           (duk_context* ctx);
static duk_ret_t js_GetPersonFrame               (duk_context* ctx);
static duk_ret_t js_GetPersonFrameNext           (duk_context* ctx);
static duk_ret_t js_GetPersonFrameRevert         (duk_context* ctx);
static duk_ret_t js_GetPersonIgnoreList          (duk_context* ctx);
static duk_ret_t js_GetPersonLayer               (duk_context* ctx);
static duk_ret_t js_GetPersonLeader              (duk_context* ctx);
static duk_ret_t js_GetPersonList                (duk_context* ctx);
static duk_ret_t js_GetPersonMask                (duk_context* ctx);
static duk_ret_t js_GetPersonOffsetX             (duk_context* ctx);
static duk_ret_t js_GetPersonOffsetY             (duk_context* ctx);
static duk_ret_t js_GetPersonSpeedX              (duk_context* ctx);
static duk_ret_t js_GetPersonSpeedY              (duk_context* ctx);
static duk_ret_t js_GetPersonSpriteset           (duk_context* ctx);
static duk_ret_t js_GetPersonValue               (duk_context* ctx);
static duk_ret_t js_GetPersonX                   (duk_context* ctx);
static duk_ret_t js_GetPersonY                   (duk_context* ctx);
static duk_ret_t js_GetPersonXFloat              (duk_context* ctx);
static duk_ret_t js_GetPersonYFloat              (duk_context* ctx);
static duk_ret_t js_GetTalkActivationButton      (duk_context* ctx);
static duk_ret_t js_GetTalkActivationKey         (duk_context* ctx);
static duk_ret_t js_GetTalkDistance              (duk_context* ctx);
static duk_ret_t js_GetTile                      (duk_context* ctx);
static duk_ret_t js_GetTileDelay                 (duk_context* ctx);
static duk_ret_t js_GetTileHeight                (duk_context* ctx);
static duk_ret_t js_GetTileImage                 (duk_context* ctx);
static duk_ret_t js_GetTileName                  (duk_context* ctx);
static duk_ret_t js_GetTileSurface               (duk_context* ctx);
static duk_ret_t js_GetTileWidth                 (duk_context* ctx);
static duk_ret_t js_GetTriggerLayer              (duk_context* ctx);
static duk_ret_t js_GetTriggerX                  (duk_context* ctx);
static duk_ret_t js_GetTriggerY                  (duk_context* ctx);
static duk_ret_t js_GetZoneHeight                (duk_context* ctx);
static duk_ret_t js_GetZoneLayer                 (duk_context* ctx);
static duk_ret_t js_GetZoneSteps                 (duk_context* ctx);
static duk_ret_t js_GetZoneWidth                 (duk_context* ctx);
static duk_ret_t js_GetZoneX                     (duk_context* ctx);
static duk_ret_t js_GetZoneY                     (duk_context* ctx);
static duk_ret_t js_IgnorePersonObstructions     (duk_context* ctx);
static duk_ret_t js_IgnoreTileObstructions       (duk_context* ctx);
static duk_ret_t js_IsCameraAttached             (duk_context* ctx);
static duk_ret_t js_IsCommandQueueEmpty          (duk_context* ctx);
static duk_ret_t js_IsIgnoringPersonObstructions (duk_context* ctx);
static duk_ret_t js_IsIgnoringTileObstructions   (duk_context* ctx);
static duk_ret_t js_IsInputAttached              (duk_context* ctx);
static duk_ret_t js_IsLayerReflective            (duk_context* ctx);
static duk_ret_t js_IsLayerVisible               (duk_context* ctx);
static duk_ret_t js_IsMapEngineRunning           (duk_context* ctx);
static duk_ret_t js_IsPersonObstructed           (duk_context* ctx);
static duk_ret_t js_IsPersonVisible              (duk_context* ctx);
static duk_ret_t js_IsTriggerAt                  (duk_context* ctx);
static duk_ret_t js_MapEngine                    (duk_context* ctx);
static duk_ret_t js_MapToScreenX                 (duk_context* ctx);
static duk_ret_t js_MapToScreenY                 (duk_context* ctx);
static duk_ret_t js_QueuePersonCommand           (duk_context* ctx);
static duk_ret_t js_QueuePersonScript            (duk_context* ctx);
static duk_ret_t js_RemoveTrigger                (duk_context* ctx);
static duk_ret_t js_RemoveZone                   (duk_context* ctx);
static duk_ret_t js_RenderMap                    (duk_context* ctx);
static duk_ret_t js_ReplaceTilesOnLayer          (duk_context* ctx);
static duk_ret_t js_ScreenToMapX                 (duk_context* ctx);
static duk_ret_t js_ScreenToMapY                 (duk_context* ctx);
static duk_ret_t js_SetCameraX                   (duk_context* ctx);
static duk_ret_t js_SetCameraY                   (duk_context* ctx);
static duk_ret_t js_SetColorMask                 (duk_context* ctx);
static duk_ret_t js_SetDefaultMapScript          (duk_context* ctx);
static duk_ret_t js_SetDefaultPersonScript       (duk_context* ctx);
static duk_ret_t js_SetDelayScript               (duk_context* ctx);
static duk_ret_t js_SetLayerHeight               (duk_context* ctx);
static duk_ret_t js_SetLayerMask                 (duk_context* ctx);
static duk_ret_t js_SetLayerReflective           (duk_context* ctx);
static duk_ret_t js_SetLayerRenderer             (duk_context* ctx);
static duk_ret_t js_SetLayerSize                 (duk_context* ctx);
static duk_ret_t js_SetLayerVisible              (duk_context* ctx);
static duk_ret_t js_SetLayerWidth                (duk_context* ctx);
static duk_ret_t js_SetMapEngineFrameRate        (duk_context* ctx);
static duk_ret_t js_SetNextAnimatedTile          (duk_context* ctx);
static duk_ret_t js_SetPersonAngle               (duk_context* ctx);
static duk_ret_t js_SetPersonData                (duk_context* ctx);
static duk_ret_t js_SetPersonDirection           (duk_context* ctx);
static duk_ret_t js_SetPersonFollowDistance      (duk_context* ctx);
static duk_ret_t js_SetPersonFrame               (duk_context* ctx);
static duk_ret_t js_SetPersonFrameNext           (duk_context* ctx);
static duk_ret_t js_SetPersonFrameRevert         (duk_context* ctx);
static duk_ret_t js_SetPersonIgnoreList          (duk_context* ctx);
static duk_ret_t js_SetPersonLayer               (duk_context* ctx);
static duk_ret_t js_SetPersonMask                (duk_context* ctx);
static duk_ret_t js_SetPersonOffsetX             (duk_context* ctx);
static duk_ret_t js_SetPersonOffsetY             (duk_context* ctx);
static duk_ret_t js_SetPersonScaleAbsolute       (duk_context* ctx);
static duk_ret_t js_SetPersonScaleFactor         (duk_context* ctx);
static duk_ret_t js_SetPersonScript              (duk_context* ctx);
static duk_ret_t js_SetPersonSpeed               (duk_context* ctx);
static duk_ret_t js_SetPersonSpeedXY             (duk_context* ctx);
static duk_ret_t js_SetPersonSpriteset           (duk_context* ctx);
static duk_ret_t js_SetPersonValue               (duk_context* ctx);
static duk_ret_t js_SetPersonVisible             (duk_context* ctx);
static duk_ret_t js_SetPersonX                   (duk_context* ctx);
static duk_ret_t js_SetPersonXYFloat             (duk_context* ctx);
static duk_ret_t js_SetPersonY                   (duk_context* ctx);
static duk_ret_t js_SetRenderScript              (duk_context* ctx);
static duk_ret_t js_SetTalkActivationButton      (duk_context* ctx);
static duk_ret_t js_SetTalkActivationKey         (duk_context* ctx);
static duk_ret_t js_SetTalkDistance              (duk_context* ctx);
static duk_ret_t js_SetTile                      (duk_context* ctx);
static duk_ret_t js_SetTileDelay                 (duk_context* ctx);
static duk_ret_t js_SetTileImage                 (duk_context* ctx);
static duk_ret_t js_SetTileName                  (duk_context* ctx);
static duk_ret_t js_SetTileSurface               (duk_context* ctx);
static duk_ret_t js_SetTriggerLayer              (duk_context* ctx);
static duk_ret_t js_SetTriggerScript             (duk_context* ctx);
static duk_ret_t js_SetTriggerXY                 (duk_context* ctx);
static duk_ret_t js_SetUpdateScript              (duk_context* ctx);
static duk_ret_t js_SetZoneLayer                 (duk_context* ctx);
static duk_ret_t js_SetZoneMetrics               (duk_context* ctx);
static duk_ret_t js_SetZoneScript                (duk_context* ctx);
static duk_ret_t js_SetZoneSteps                 (duk_context* ctx);
static duk_ret_t js_UpdateMapEngine              (duk_context* ctx);

static const person_t*     s_acting_person;
static mixer_t*            s_bgm_mixer = NULL;
static person_t*           s_camera_person = NULL;
static int                 s_camera_x = 0;
static int                 s_camera_y = 0;
static color_t             s_color_mask;
static const person_t*     s_current_person = NULL;
static int                 s_current_trigger = -1;
static int                 s_current_zone = -1;
static script_t*           s_def_map_scripts[MAP_SCRIPT_MAX];
static script_t*           s_def_person_scripts[PERSON_SCRIPT_MAX];
static bool                s_exiting = false;
static color_t             s_fade_color_from;
static color_t             s_fade_color_to;
static int                 s_fade_frames;
static int                 s_fade_progress;
static int                 s_framerate = 0;
static unsigned int        s_frames = 0;
static bool                s_is_map_running = false;
static lstring_t*          s_last_bgm_file = NULL;
static struct map*         s_map = NULL;
static sound_t*            s_map_bgm_stream = NULL;
static char*               s_map_filename = NULL;
static int                 s_max_delay_scripts = 0;
static int                 s_max_persons = 0;
static unsigned int        s_next_person_id = 0;
static int                 s_num_delay_scripts = 0;
static int                 s_num_persons = 0;
static struct map_trigger* s_on_trigger = NULL;
static unsigned int        s_queued_id = 0;
static struct player*      s_players;
static script_t*           s_render_script = NULL;
static int                 s_talk_button = 0;
static int                 s_talk_distance = 8;
static script_t*           s_update_script = NULL;
static struct delay_script *s_delay_scripts = NULL;
static person_t*           *s_persons = NULL;

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

struct person
{
	unsigned int    id;
	char*           name;
	int             anim_frames;
	char*           direction;
	int             follow_distance;
	int             frame;
	bool            ignore_all_persons;
	bool            ignore_all_tiles;
	bool            is_persistent;
	bool            is_visible;
	int             layer;
	person_t*       leader;
	color_t         mask;
	int             mv_x, mv_y;
	int             revert_delay;
	int             revert_frames;
	double          scale_x;
	double          scale_y;
	script_t*       scripts[PERSON_SCRIPT_MAX];
	double          speed_x, speed_y;
	spriteset_t*    sprite;
	double          theta;
	double          x, y;
	int             x_offset, y_offset;
	int             max_commands;
	int             max_history;
	int             num_commands;
	int             num_ignores;
	struct command  *commands;
	char*           *ignores;
	struct step     *steps;
};

struct step
{
	double x, y;
};

struct command
{
	int       type;
	bool      is_immediate;
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

static bool                change_map           (const char* filename, bool preserve_persons);
static void                command_person       (person_t* person, int command);
static int                 compare_persons      (const void* a, const void* b);
static bool                does_person_exist    (const person_t* person);
static void                draw_persons         (int layer, bool is_flipped, int cam_x, int cam_y);
static bool                enlarge_step_history (person_t* person, int new_size);
static int                 find_layer           (const char* name);
static void                free_map             (struct map* map);
static void                free_person          (person_t* person);
static struct map_trigger* get_trigger_at       (int x, int y, int layer, int* out_index);
static struct map_zone*    get_zone_at          (int x, int y, int layer, int which, int* out_index);
static struct map*         load_map             (const char* path);
static void                map_screen_to_layer  (int layer, int camera_x, int camera_y, int* inout_x, int* inout_y);
static void                map_screen_to_map    (int camera_x, int camera_y, int* inout_x, int* inout_y);
static void                process_map_input    (void);
static void                record_step          (person_t* person);
static void                render_map           (void);
static void                reset_persons        (bool keep_existing);
static void                set_person_direction (person_t* person, const char* direction);
static void                set_person_name      (person_t* person, const char* name);
static void                sort_persons         (void);
static void                update_map_engine    (bool is_main_loop);
static void                update_person        (person_t* person, bool* out_has_moved);

static int                 duk_require_map_layer (duk_context* ctx, duk_idx_t index);

void
map_engine_init(void)
{
	int i;
	
	console_log(1, "initializing map engine subsystem");
	
	audio_init();
	s_bgm_mixer = mixer_new(44100, 16, 2);
	
	memset(s_def_map_scripts, 0, MAP_SCRIPT_MAX * sizeof(int));
	memset(s_def_person_scripts, 0, PERSON_SCRIPT_MAX * sizeof(int));
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

	s_num_persons = s_max_persons = 0;
	s_persons = NULL;
	s_talk_distance = 8;
	s_acting_person = NULL;
	s_current_person = NULL;
}

void
map_engine_uninit(void)
{
	int i;

	console_log(1, "shutting down map engine subsystem");
	
	for (i = 0; i < s_num_delay_scripts; ++i)
		script_free(s_delay_scripts[i].script);
	free(s_delay_scripts);
	for (i = 0; i < MAP_SCRIPT_MAX; ++i)
		script_free(s_def_map_scripts[i]);
	script_free(s_update_script);
	script_free(s_render_script);
	free_map(s_map);
	free(s_players);
	
	for (i = 0; i < s_num_persons; ++i)
		free_person(s_persons[i]);
	for (i = 0; i < PERSON_SCRIPT_MAX; ++i)
		script_free(s_def_person_scripts[i]);
	free(s_persons);
	
	mixer_free(s_bgm_mixer);
	
	audio_uninit();
}

bool
map_engine_running(void)
{
	return s_is_map_running;
}

void
map_engine_exit(void)
{
	s_exiting = true;
}

rect_t
map_bounds(void)
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
map_name(void)
{
	return s_map ? s_map_filename : NULL;
}

int
map_num_layers(void)
{
	return s_map->num_layers;
}

int
map_num_persons(void)
{
	return s_num_persons;
}

int
map_num_triggers(void)
{
	return vector_len(s_map->triggers);
}

int
map_num_zones(void)
{
	return vector_len(s_map->zones);
}

const obsmap_t*
map_obsmap(int layer)
{
	return s_map->layers[layer].obsmap;
}

point3_t
map_origin(void)
{
	point3_t empty_point = { 0, 0, 0 };
	
	return s_map ? s_map->origin : empty_point;
}

int
map_tile_index(int x, int y, int layer)
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

tileset_t*
map_tileset(void)
{
	return s_map->tileset;
}

int
map_trigger_at(int x, int y, int layer)
{
	rect_t              bounds;
	int                 tile_w, tile_h;
	struct map_trigger* trigger;

	iter_t iter;

	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	iter = vector_enum(s_map->triggers);
	while (trigger = vector_next(&iter)) {
		if (trigger->z != layer && false)  // layer ignored for compatibility
			continue;
		bounds.x1 = trigger->x - tile_w / 2;
		bounds.y1 = trigger->y - tile_h / 2;
		bounds.x2 = bounds.x1 + tile_w;
		bounds.y2 = bounds.y1 + tile_h;
		if (is_point_in_rect(x, y, bounds))
			return iter.index;
	}
	return -1;
}

bool
map_zones_at(int x, int y, int layer, int* out_count)
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
	if (out_count != NULL)
		*out_count = count;
	return zone_found;
}

bool
map_add_trigger(int x, int y, int layer, script_t* script)
{
	struct map_trigger trigger;

	console_log(2, "creating trigger #%d on map `%s`", vector_len(s_map->triggers), s_map_filename);
	console_log(3, "    location: `%s` @ (%d,%d)", lstr_cstr(s_map->layers[layer].name), x, y);
	
	trigger.x = x; trigger.y = y;
	trigger.z = layer;
	trigger.script = script_ref(script);
	if (!vector_push(s_map->triggers, &trigger))
		return false;
	return true;
}

bool
map_add_zone(rect_t bounds, int layer, script_t* script, int steps)
{
	struct map_zone zone;

	console_log(2, "creating %u-step zone #%d on map `%s`", steps, vector_len(s_map->zones), s_map_filename);
	console_log(3, "    bounds: (%d,%d)-(%d,%d)", bounds.x1, bounds.y1, bounds.x2, bounds.y2);
	
	memset(&zone, 0, sizeof(struct map_zone));
	zone.bounds = bounds;
	zone.layer = layer;
	zone.script = script_ref(script);
	zone.interval = steps;
	zone.steps_left = 0;
	if (!vector_push(s_map->zones, &zone))
		return false;
	return true;
}

bool
map_call_script(map_op_t op, bool use_default)
{
	if (use_default)
		script_run(s_def_map_scripts[op], false);
	script_run(s_map->scripts[op], false);
	return true;
}

void
map_detach_person(const person_t* person)
{
	int i;
	
	if (s_camera_person == person)
		s_camera_person = NULL;
	for (i = 0; i < MAX_PLAYERS; ++i) if (s_players[i].person == person)
		s_players[i].person = NULL;
}

void
map_normalize_xy(double* inout_x, double* inout_y, int layer)
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
map_remove_trigger(int trigger_index)
{
	vector_remove(s_map->triggers, trigger_index);
}

void
map_remove_zone(int zone_index)
{
	vector_remove(s_map->zones, zone_index);
}

bool
map_resize(int layer, int x_size, int y_size)
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

person_t*
person_new(const char* name, spriteset_t* spriteset, bool is_persistent, script_t* create_script)
{
	point3_t  origin = map_origin();
	person_t* person;

	if (++s_num_persons > s_max_persons) {
		s_max_persons = s_num_persons * 2;
		s_persons = realloc(s_persons, s_max_persons * sizeof(person_t*));
	}
	person = s_persons[s_num_persons - 1] = calloc(1, sizeof(person_t));
	person->id = s_next_person_id++;
	person->sprite = spriteset_ref(spriteset);
	set_person_name(person, name);
	set_person_direction(person, spriteset_pose_name(spriteset, 0));
	person->is_persistent = is_persistent;
	person->is_visible = true;
	person->x = origin.x;
	person->y = origin.y;
	person->layer = origin.z;
	person->speed_x = 1.0;
	person->speed_y = 1.0;
	person->anim_frames = spriteset_frame_delay(person->sprite, person->direction, 0);
	person->mask = color_new(255, 255, 255, 255);
	person->scale_x = person->scale_y = 1.0;
	person->scripts[PERSON_SCRIPT_ON_CREATE] = create_script;
	person_call_script(person, PERSON_SCRIPT_ON_CREATE, true);
	sort_persons();
	return person;
}

void
person_free(person_t* person)
{
	int i, j;

	// call the person's destroy script *before* renouncing leadership.
	// the destroy script may want to reassign followers (they will be orphaned otherwise), so
	// we want to give it a chance to do so.
	person_call_script(person, PERSON_SCRIPT_ON_DESTROY, true);
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->leader == person)
			s_persons[i]->leader = NULL;
	}

	// remove the person from the engine
	map_detach_person(person);
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i] == person) {
			for (j = i; j < s_num_persons - 1; ++j)
				s_persons[j] = s_persons[j + 1];
			--s_num_persons;
			--i;
		}
	}

	free_person(person);
	sort_persons();
}

bool
person_busy(const person_t* person)
{
	return person->num_commands > 0 || person->leader != NULL;
}

bool
person_following(const person_t* person, const person_t* leader)
{
	const person_t* node;

	node = person;
	while (node = node->leader)
		if (node == leader) return true;
	return false;
}

bool
person_has_moved(const person_t* person)
{
	return person->mv_x != 0 || person->mv_y != 0;
}

bool
person_ignoring(const person_t* person, const person_t* by_person)
{
	// note: commutative; if either person ignores the other, the function will return true

	int i;

	if (by_person->ignore_all_persons || person->ignore_all_persons)
		return true;
	for (i = 0; i < by_person->num_ignores; ++i)
		if (strcmp(by_person->ignores[i], person->name) == 0) return true;
	for (i = 0; i < person->num_ignores; ++i)
		if (strcmp(person->ignores[i], by_person->name) == 0) return true;
	return false;
}

const char*
person_name(const person_t* person)
{
	return person->name;
}

bool
person_obstructed_at(const person_t* person, double x, double y, person_t** out_obstructing_person, int* out_tile_index)
{
	rect_t           area;
	rect_t           base, my_base;
	double           cur_x, cur_y;
	bool             is_obstructed = false;
	int              layer;
	const obsmap_t*  obsmap;
	int              tile_w, tile_h;
	const tileset_t* tileset;

	int i, i_x, i_y;

	map_normalize_xy(&x, &y, person->layer);
	person_get_xyz(person, &cur_x, &cur_y, &layer, true);
	my_base = translate_rect(person_get_base(person), x - cur_x, y - cur_y);
	if (out_obstructing_person)
		*out_obstructing_person = NULL;
	if (out_tile_index)
		*out_tile_index = -1;

	// check for obstructing persons
	if (!person->ignore_all_persons) {
		for (i = 0; i < s_num_persons; ++i) {
			if (s_persons[i] == person)  // these persons aren't going to obstruct themselves!
				continue;
			if (s_persons[i]->layer != layer)
				continue;  // ignore persons not on the same layer
			if (person_following(s_persons[i], person))
				continue;  // ignore own followers
			base = person_get_base(s_persons[i]);
			if (do_rects_intersect(my_base, base) && !person_ignoring(person, s_persons[i])) {
				is_obstructed = true;
				if (out_obstructing_person)
					*out_obstructing_person = s_persons[i];
				break;
			}
		}
	}

	// no obstructing person, check map-defined obstructions
	obsmap = map_obsmap(layer);
	if (obsmap_test_rect(obsmap, my_base))
		is_obstructed = true;

	// check for obstructing tiles
	// for performance reasons, the search is constrained to the immediate vicinity
	// of the person's sprite base.
	if (!person->ignore_all_tiles) {
		tileset = map_tileset();
		tileset_get_size(tileset, &tile_w, &tile_h);
		area.x1 = my_base.x1 / tile_w;
		area.y1 = my_base.y1 / tile_h;
		area.x2 = area.x1 + (my_base.x2 - my_base.x1) / tile_w + 2;
		area.y2 = area.y1 + (my_base.y2 - my_base.y1) / tile_h + 2;
		for (i_x = area.x1; i_x < area.x2; ++i_x) for (i_y = area.y1; i_y < area.y2; ++i_y) {
			base = translate_rect(my_base, -(i_x * tile_w), -(i_y * tile_h));
			obsmap = tileset_obsmap(tileset, map_tile_index(i_x, i_y, layer));
			if (obsmap != NULL && obsmap_test_rect(obsmap, base)) {
				is_obstructed = true;
				if (out_tile_index)
					*out_tile_index = map_tile_index(i_x, i_y, layer);
				break;
			}
		}
	}

	return is_obstructed;
}

double
person_get_angle(const person_t* person)
{
	return person->theta;
}

rect_t
person_get_base(const person_t* person)
{
	rect_t base_rect;
	int    base_x, base_y;
	double x, y;

	base_rect = zoom_rect(spriteset_get_base(person->sprite), person->scale_x, person->scale_y);
	person_get_xy(person, &x, &y, true);
	base_x = x - (base_rect.x1 + (base_rect.x2 - base_rect.x1) / 2);
	base_y = y - (base_rect.y1 + (base_rect.y2 - base_rect.y1) / 2);
	base_rect.x1 += base_x; base_rect.x2 += base_x;
	base_rect.y1 += base_y; base_rect.y2 += base_y;
	return base_rect;
}

color_t
person_get_color(const person_t* person)
{
	return person->mask;
}

void
person_get_scale(const person_t* person, double* out_scale_x, double* out_scale_y)
{
	*out_scale_x = person->scale_x;
	*out_scale_y = person->scale_y;
}

void
person_get_speed(const person_t* person, double* out_x_speed, double* out_y_speed)
{
	if (out_x_speed) *out_x_speed = person->speed_x;
	if (out_y_speed) *out_y_speed = person->speed_y;
}

spriteset_t*
person_get_spriteset(person_t* person)
{
	return person->sprite;
}

void
person_get_xy(const person_t* person, double* out_x, double* out_y, bool want_normalize)
{
	*out_x = person->x;
	*out_y = person->y;
	if (want_normalize)
		map_normalize_xy(out_x, out_y, person->layer);
}

void
person_get_xyz(const person_t* person, double* out_x, double* out_y, int* out_layer, bool want_normalize)
{
	*out_x = person->x;
	*out_y = person->y;
	*out_layer = person->layer;
	if (want_normalize)
		map_normalize_xy(out_x, out_y, *out_layer);
}

void
person_set_angle(person_t* person, double theta)
{
	person->theta = theta;
}

void
person_set_color(person_t* person, color_t mask)
{
	person->mask = mask;
}

void
person_set_scale(person_t* person, double scale_x, double scale_y)
{
	person->scale_x = scale_x;
	person->scale_y = scale_y;
}

void
person_set_script(person_t* person, int type, script_t* script)
{
	script_free(person->scripts[type]);
	person->scripts[type] = script;
}

void
person_set_speed(person_t* person, double x_speed, double y_speed)
{
	person->speed_x = x_speed;
	person->speed_y = y_speed;
}

void
person_set_spriteset(person_t* person, spriteset_t* spriteset)
{
	spriteset_t* old_spriteset;

	old_spriteset = person->sprite;
	person->sprite = spriteset_ref(spriteset);
	person->anim_frames = spriteset_frame_delay(person->sprite, person->direction, 0);
	person->frame = 0;
	spriteset_free(old_spriteset);
}

void
person_set_xyz(person_t* person, double x, double y, int layer)
{
	person->x = x;
	person->y = y;
	person->layer = layer;
	sort_persons();
}

bool
person_call_script(const person_t* person, person_op_t op, bool use_default)
{
	const person_t* last_person;

	last_person = s_current_person;
	s_current_person = person;
	if (use_default)
		script_run(s_def_person_scripts[op], false);
	if (does_person_exist(person))
		script_run(person->scripts[op], false);
	s_current_person = last_person;
	return true;
}

bool
person_compile_script(person_t* person, int type, const lstring_t* codestring)
{
	script_t*   script;
	const char* script_name;

	script_name = type == PERSON_SCRIPT_ON_CREATE ? "onCreate"
		: type == PERSON_SCRIPT_ON_DESTROY ? "onDestroy"
		: type == PERSON_SCRIPT_ON_TOUCH ? "onTouch"
		: type == PERSON_SCRIPT_ON_TALK ? "onTalk"
		: type == PERSON_SCRIPT_GENERATOR ? "genCommands"
		: NULL;
	if (script_name == NULL)
		return false;
	script = script_new(codestring, "%s/%s/%s.js", map_name(), person->name, script_name);
	person_set_script(person, type, script);
	return true;
}

person_t*
person_find(const char* name)
{
	int i;

	for (i = 0; i < s_num_persons; ++i) {
		if (strcmp(name, s_persons[i]->name) == 0)
			return s_persons[i];
	}
	return NULL;
}

bool
person_follow(person_t* person, person_t* leader, int distance)
{
	const person_t* node;

	// prevent circular follower chains from forming
	if (leader != NULL) {
		node = leader;
		do
			if (node == person) return false;
		while (node = node->leader);
	}

	// add the person as a follower (or sever existing link if leader==NULL)
	if (leader != NULL) {
		if (!enlarge_step_history(leader, distance))
			return false;
		person->leader = leader;
		person->follow_distance = distance;
	}
	person->leader = leader;
	return true;
}

bool
person_queue_command(person_t* person, int command, bool is_immediate)
{
	struct command* commands;
	bool            is_aok = true;

	switch (command) {
	case COMMAND_MOVE_NORTHEAST:
		is_aok &= person_queue_command(person, COMMAND_MOVE_NORTH, true);
		is_aok &= person_queue_command(person, COMMAND_MOVE_EAST, is_immediate);
		return is_aok;
	case COMMAND_MOVE_SOUTHEAST:
		is_aok &= person_queue_command(person, COMMAND_MOVE_SOUTH, true);
		is_aok &= person_queue_command(person, COMMAND_MOVE_EAST, is_immediate);
		return is_aok;
	case COMMAND_MOVE_SOUTHWEST:
		is_aok &= person_queue_command(person, COMMAND_MOVE_SOUTH, true);
		is_aok &= person_queue_command(person, COMMAND_MOVE_WEST, is_immediate);
		return is_aok;
	case COMMAND_MOVE_NORTHWEST:
		is_aok &= person_queue_command(person, COMMAND_MOVE_NORTH, true);
		is_aok &= person_queue_command(person, COMMAND_MOVE_WEST, is_immediate);
		return is_aok;
	default:
		++person->num_commands;
		if (person->num_commands > person->max_commands) {
			if (!(commands = realloc(person->commands, person->num_commands * 2 * sizeof(struct command))))
				return false;
			person->max_commands = person->num_commands * 2;
			person->commands = commands;
		}
		person->commands[person->num_commands - 1].type = command;
		person->commands[person->num_commands - 1].is_immediate = is_immediate;
		person->commands[person->num_commands - 1].script = NULL;
		return true;
	}
}

bool
person_queue_script(person_t* person, script_t* script, bool is_immediate)
{
	++person->num_commands;
	if (person->num_commands > person->max_commands) {
		person->max_commands = person->num_commands * 2;
		if (!(person->commands = realloc(person->commands, person->max_commands * sizeof(struct command))))
			return false;
	}
	person->commands[person->num_commands - 1].type = COMMAND_RUN_SCRIPT;
	person->commands[person->num_commands - 1].is_immediate = is_immediate;
	person->commands[person->num_commands - 1].script = script;
	return true;
}

void
person_talk(const person_t* person)
{
	const person_t* last_active;
	rect_t          map_rect;
	person_t*       target_person;
	double          talk_x, talk_y;

	map_rect = map_bounds();

	// check if anyone else is within earshot
	person_get_xy(person, &talk_x, &talk_y, true);
	if (strstr(person->direction, "north"))
		talk_y -= s_talk_distance;
	if (strstr(person->direction, "east"))
		talk_x += s_talk_distance;
	if (strstr(person->direction, "south"))
		talk_y += s_talk_distance;
	if (strstr(person->direction, "west"))
		talk_x -= s_talk_distance;
	person_obstructed_at(person, talk_x, talk_y, &target_person, NULL);

	// if so, call their talk script
	last_active = s_acting_person;
	s_acting_person = person;
	if (target_person != NULL)
		person_call_script(target_person, PERSON_SCRIPT_ON_TALK, true);
	s_acting_person = last_active;
}

void
trigger_get_xyz(int trigger_index, int* out_x, int* out_y, int* out_layer)
{
	struct map_trigger* trigger;

	trigger = vector_get(s_map->triggers, trigger_index);
	if (out_x != NULL)
		*out_x = trigger->x;
	if (out_y != NULL)
		*out_y = trigger->y;
	if (out_layer) *out_layer = trigger->z;
}

void
trigger_set_layer(int trigger_index, int layer)
{
	struct map_trigger* trigger;

	trigger = vector_get(s_map->triggers, trigger_index);
	trigger->z = layer;
}

void
trigger_set_script(int trigger_index, script_t* script)
{
	script_t*           old_script;
	struct map_trigger* trigger;

	trigger = vector_get(s_map->triggers, trigger_index);
	old_script = trigger->script;
	trigger->script = script_ref(script);
	script_free(old_script);
}

void
trigger_set_xy(int trigger_index, int x, int y)
{
	struct map_trigger* trigger;

	trigger = vector_get(s_map->triggers, trigger_index);
	trigger->x = x;
	trigger->y = y;
}

void
trigger_activate(int trigger_index)
{
	int                 last_trigger;
	struct map_trigger* trigger;

	trigger = vector_get(s_map->triggers, trigger_index);
	last_trigger = s_current_trigger;
	s_current_trigger = trigger_index;
	script_run(trigger->script, true);
	s_current_trigger = last_trigger;
}

rect_t
zone_get_bounds(int zone_index)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	return zone->bounds;
}

int
zone_get_layer(int zone_index)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	return zone->layer;
}

int
zone_get_steps(int zone_index)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	return zone->interval;
}

void
zone_set_bounds(int zone_index, rect_t bounds)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	normalize_rect(&bounds);
	zone->bounds = bounds;
}

void
zone_set_layer(int zone_index, int layer)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	zone->layer = layer;
}

void
zone_set_script(int zone_index, script_t* script)
{
	script_t*        old_script;
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	old_script = zone->script;
	zone->script = script_ref(script);
	script_free(old_script);
}

void
zone_set_steps(int zone_index, int interval)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	zone->interval = interval;
	zone->steps_left = 0;
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
		map_call_script(MAP_SCRIPT_ON_LEAVE, true);
	}

	// close out old map and prep for new one
	free_map(s_map); free(s_map_filename);
	for (i = 0; i < s_num_delay_scripts; ++i)
		script_free(s_delay_scripts[i].script);
	s_num_delay_scripts = 0;
	s_map = map; s_map_filename = strdup(filename);
	reset_persons(preserve_persons);

	// populate persons
	for (i = 0; i < s_map->num_persons; ++i) {
		person_info = &s_map->persons[i];
		path = fs_make_path(lstr_cstr(person_info->spriteset), "spritesets", true);
		spriteset = spriteset_load(path_cstr(path));
		path_free(path);
		if (spriteset == NULL)
			goto on_error;
		if (!(person = person_new(lstr_cstr(person_info->name), spriteset, false, NULL)))
			goto on_error;
		spriteset_free(spriteset);
		person_set_xyz(person, person_info->x, person_info->y, person_info->z);
		person_compile_script(person, PERSON_SCRIPT_ON_CREATE, person_info->create_script);
		person_compile_script(person, PERSON_SCRIPT_ON_DESTROY, person_info->destroy_script);
		person_compile_script(person, PERSON_SCRIPT_ON_TOUCH, person_info->touch_script);
		person_compile_script(person, PERSON_SCRIPT_ON_TALK, person_info->talk_script);
		person_compile_script(person, PERSON_SCRIPT_GENERATOR, person_info->command_script);

		// normally this is handled by person_new(), but since in this case the
		// person-specific create script isn't compiled until after the person is created,
		// the map engine gets the responsibility.
		person_call_script(person, PERSON_SCRIPT_ON_CREATE, false);
	}

	// set camera over starting position
	s_camera_x = s_map->origin.x;
	s_camera_y = s_map->origin.y;

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
	map_call_script(MAP_SCRIPT_ON_ENTER, true);

	s_frames = 0;
	return true;

on_error:
	spriteset_free(spriteset);
	free_map(s_map);
	return false;
}

static void
command_person(person_t* person, int command)
{
	const person_t* last_active;
	double          new_x, new_y;
	person_t*       person_to_touch;

	new_x = person->x;
	new_y = person->y;
	switch (command) {
	case COMMAND_ANIMATE:
		person->revert_frames = person->revert_delay;
		if (person->anim_frames > 0 && --person->anim_frames == 0) {
			++person->frame;
			person->anim_frames = spriteset_frame_delay(person->sprite, person->direction, person->frame);
		}
		break;
	case COMMAND_FACE_NORTH:
		set_person_direction(person, "north");
		break;
	case COMMAND_FACE_NORTHEAST:
		set_person_direction(person, "northeast");
		break;
	case COMMAND_FACE_EAST:
		set_person_direction(person, "east");
		break;
	case COMMAND_FACE_SOUTHEAST:
		set_person_direction(person, "southeast");
		break;
	case COMMAND_FACE_SOUTH:
		set_person_direction(person, "south");
		break;
	case COMMAND_FACE_SOUTHWEST:
		set_person_direction(person, "southwest");
		break;
	case COMMAND_FACE_WEST:
		set_person_direction(person, "west");
		break;
	case COMMAND_FACE_NORTHWEST:
		set_person_direction(person, "northwest");
		break;
	case COMMAND_MOVE_NORTH:
		new_y = person->y - person->speed_y;
		break;
	case COMMAND_MOVE_EAST:
		new_x = person->x + person->speed_x;
		break;
	case COMMAND_MOVE_SOUTH:
		new_y = person->y + person->speed_y;
		break;
	case COMMAND_MOVE_WEST:
		new_x = person->x - person->speed_x;
		break;
	}
	if (new_x != person->x || new_y != person->y) {
		// person is trying to move, make sure the path is clear of obstructions
		if (!person_obstructed_at(person, new_x, new_y, &person_to_touch, NULL)) {
			if (new_x != person->x)
				person->mv_x = new_x > person->x ? 1 : -1;
			if (new_y != person->y)
				person->mv_y = new_y > person->y ? 1 : -1;
			person->x = new_x; person->y = new_y;
		}
		else {
			// if not, and we collided with a person, call that person's touch script
			last_active = s_acting_person;
			s_acting_person = person;
			if (person_to_touch != NULL)
				person_call_script(person_to_touch, PERSON_SCRIPT_ON_TOUCH, true);
			s_acting_person = last_active;
		}
	}
}

static int
compare_persons(const void* a, const void* b)
{
	person_t* p1 = *(person_t**)a;
	person_t* p2 = *(person_t**)b;

	double x, y_p1, y_p2;
	int    y_delta;

	person_get_xy(p1, &x, &y_p1, true);
	person_get_xy(p2, &x, &y_p2, true);
	y_delta = (y_p1 + p1->y_offset) - (y_p2 + p2->y_offset);
	if (y_delta != 0)
		return y_delta;
	else if (person_following(p1, p2))
		return -1;
	else if (person_following(p2, p1))
		return 1;
	else
		return p1->id - p2->id;
}

static bool
does_person_exist(const person_t* person)
{
	int i;

	for (i = 0; i < s_num_persons; ++i)
		if (person == s_persons[i]) return true;
	return false;
}

void
draw_persons(int layer, bool is_flipped, int cam_x, int cam_y)
{
	person_t*    person;
	spriteset_t* sprite;
	int          w, h;
	double       x, y;
	int          i;

	for (i = 0; i < s_num_persons; ++i) {
		person = s_persons[i];
		if (!person->is_visible || person->layer != layer)
			continue;
		sprite = person->sprite;
		w = spriteset_width(sprite);
		h = spriteset_height(sprite);
		person_get_xy(person, &x, &y, true);
		x -= cam_x - person->x_offset;
		y -= cam_y - person->y_offset;
		spriteset_draw(sprite, person->mask, is_flipped, person->theta, person->scale_x, person->scale_y,
			person->direction, x, y, person->frame);
	}
}

static bool
enlarge_step_history(person_t* person, int new_size)
{
	struct step *new_steps;
	size_t      pastmost;
	double      last_x;
	double      last_y;

	int i;

	if (new_size > person->max_history) {
		if (!(new_steps = realloc(person->steps, new_size * sizeof(struct step))))
			return false;

		// when enlarging the history buffer, fill new slots with pastmost values
		// (kind of like sign extension)
		pastmost = person->max_history - 1;
		last_x = person->steps != NULL ? person->steps[pastmost].x : person->x;
		last_y = person->steps != NULL ? person->steps[pastmost].y : person->y;
		for (i = person->max_history; i < new_size; ++i) {
			new_steps[i].x = last_x;
			new_steps[i].y = last_y;
		}
		person->steps = new_steps;
		person->max_history = new_size;
	}

	return true;
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
free_map(struct map* map)
{
	struct map_trigger* trigger;
	struct map_zone*    zone;

	iter_t iter;
	int    i;

	if (map == NULL)
		return;
	for (i = 0; i < MAP_SCRIPT_MAX; ++i)
		script_free(map->scripts[i]);
	for (i = 0; i < map->num_layers; ++i) {
		script_free(map->layers[i].render_script);
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
		script_free(trigger->script);
	iter = vector_enum(s_map->zones);
	while (zone = vector_next(&iter))
		script_free(zone->script);
	lstr_free(s_map->bgm_file);
	tileset_free(map->tileset);
	free(map->layers);
	free(map->persons);
	vector_free(map->triggers);
	vector_free(map->zones);
	free(map);
}

static void
free_person(person_t* person)
{
	int i;

	free(person->steps);
	for (i = 0; i < PERSON_SCRIPT_MAX; ++i)
		script_free(person->scripts[i]);
	spriteset_free(person->sprite);
	free(person->commands);
	free(person->name);
	free(person->direction);
	free(person);
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
			if (out_index != NULL)
				*out_index = (int)iter.index;
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
				trigger.script = script_new(script, "%s/trig%d", filename, vector_len(map->triggers));
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
			zone.script = script_new(script, "%s/zone%d", filename, vector_len(map->zones));
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
			map->scripts[MAP_SCRIPT_ON_ENTER] = script_new(strings[3], "%s/onEnter", filename);
			map->scripts[MAP_SCRIPT_ON_LEAVE] = script_new(strings[4], "%s/onLeave", filename);
		}
		if (rmp.num_strings >= 9) {
			map->scripts[MAP_SCRIPT_ON_LEAVE_NORTH] = script_new(strings[5], "%s/onLeave", filename);
			map->scripts[MAP_SCRIPT_ON_LEAVE_EAST] = script_new(strings[6], "%s/onLeaveEast", filename);
			map->scripts[MAP_SCRIPT_ON_LEAVE_SOUTH] = script_new(strings[7], "%s/onLeaveSouth", filename);
			map->scripts[MAP_SCRIPT_ON_LEAVE_WEST] = script_new(strings[8], "%s/onLeaveWest", filename);
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
		bounds = map_bounds();
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
		bounds = map_bounds();
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
					person_talk(person);
				s_players[i].is_talk_allowed = false;
			}
			else // allow talking again only after key is released
				s_players[i].is_talk_allowed = true;
			mv_x = 0; mv_y = 0;
			if (!person_busy(person)) {  // allow player control only if input person is idle
				if (kb_is_key_down(get_player_key(i, PLAYER_KEY_UP)) || joy_position(i, 1) <= -0.5)
					mv_y = -1;
				if (kb_is_key_down(get_player_key(i, PLAYER_KEY_RIGHT)) || joy_position(i, 0) >= 0.5)
					mv_x = 1;
				if (kb_is_key_down(get_player_key(i, PLAYER_KEY_DOWN)) || joy_position(i, 1) >= 0.5)
					mv_y = 1;
				if (kb_is_key_down(get_player_key(i, PLAYER_KEY_LEFT)) || joy_position(i, 0) <= -0.5)
					mv_x = -1;
			}
			switch (mv_x + mv_y * 3) {
			case -3: // north
				person_queue_command(person, COMMAND_MOVE_NORTH, true);
				person_queue_command(person, COMMAND_FACE_NORTH, true);
				person_queue_command(person, COMMAND_ANIMATE, false);
				break;
			case -2: // northeast
				person_queue_command(person, COMMAND_MOVE_NORTHEAST, true);
				person_queue_command(person, COMMAND_FACE_NORTHEAST, true);
				person_queue_command(person, COMMAND_ANIMATE, false);
				break;
			case 1: // east
				person_queue_command(person, COMMAND_MOVE_EAST, true);
				person_queue_command(person, COMMAND_FACE_EAST, true);
				person_queue_command(person, COMMAND_ANIMATE, false);
				break;
			case 4: // southeast
				person_queue_command(person, COMMAND_MOVE_SOUTHEAST, true);
				person_queue_command(person, COMMAND_FACE_SOUTHEAST, true);
				person_queue_command(person, COMMAND_ANIMATE, false);
				break;
			case 3: // south
				person_queue_command(person, COMMAND_MOVE_SOUTH, true);
				person_queue_command(person, COMMAND_FACE_SOUTH, true);
				person_queue_command(person, COMMAND_ANIMATE, false);
				break;
			case 2: // southwest
				person_queue_command(person, COMMAND_MOVE_SOUTHWEST, true);
				person_queue_command(person, COMMAND_FACE_SOUTHWEST, true);
				person_queue_command(person, COMMAND_ANIMATE, false);
				break;
			case -1: // west
				person_queue_command(person, COMMAND_MOVE_WEST, true);
				person_queue_command(person, COMMAND_FACE_WEST, true);
				person_queue_command(person, COMMAND_ANIMATE, false);
				break;
			case -4: // northwest
				person_queue_command(person, COMMAND_MOVE_NORTHWEST, true);
				person_queue_command(person, COMMAND_FACE_NORTHWEST, true);
				person_queue_command(person, COMMAND_ANIMATE, false);
				break;
			}
		}
	}

	update_bound_keys(true);
}

static void
record_step(person_t* person)
{
	struct step* p_step;

	if (person->max_history <= 0)
		return;
	memmove(&person->steps[1], &person->steps[0], (person->max_history - 1) * sizeof(struct step));
	p_step = &person->steps[0];
	p_step->x = person->x;
	p_step->y = person->y;
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
		map_screen_to_layer(z, s_camera_x, s_camera_y, &off_x, &off_y);

		// render person reflections if layer is reflective
		al_hold_bitmap_drawing(true);
		if (layer->is_reflective) {
			if (is_repeating) {  // for small repeating maps, persons need to be repeated as well
				for (y = 0; y < g_res_y / layer_height + 2; ++y) for (x = 0; x < g_res_x / layer_width + 2; ++x)
					draw_persons(z, true, off_x - x * layer_width, off_y - y * layer_height);
			}
			else {
				draw_persons(z, true, off_x, off_y);
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
				draw_persons(z, false, off_x - x * layer_width, off_y - y * layer_height);
		}
		else {
			draw_persons(z, false, off_x, off_y);
		}
		al_hold_bitmap_drawing(false);

		script_run(layer->render_script, false);
	}

	overlay_color = al_map_rgba(s_color_mask.r, s_color_mask.g, s_color_mask.b, s_color_mask.a);
	al_draw_filled_rectangle(0, 0, g_res_x, g_res_y, overlay_color);
	script_run(s_render_script, false);
}

void
reset_persons(bool keep_existing)
{
	unsigned int id;
	point3_t     origin;
	person_t*    person;

	int i, j;

	origin = map_origin();
	for (i = 0; i < s_num_persons; ++i) {
		person = s_persons[i];
		id = person->id;
		if (!keep_existing)
			person->num_commands = 0;
		if (person->is_persistent || keep_existing) {
			person->x = origin.x;
			person->y = origin.y;
			person->layer = origin.z;
		}
		else {
			person_call_script(person, PERSON_SCRIPT_ON_DESTROY, true);
			free_person(person);
			--s_num_persons;
			for (j = i; j < s_num_persons; ++j) s_persons[j] = s_persons[j + 1];
			--i;
		}
	}
	sort_persons();
}

static void
set_person_direction(person_t* person, const char* direction)
{
	person->direction = realloc(person->direction, (strlen(direction) + 1) * sizeof(char));
	strcpy(person->direction, direction);
}

static void
set_person_name(person_t* person, const char* name)
{
	person->name = realloc(person->name, (strlen(name) + 1) * sizeof(char));
	strcpy(person->name, name);
}

static void
sort_persons(void)
{
	qsort(s_persons, s_num_persons, sizeof(person_t*), compare_persons);
}

static void
update_map_engine(bool in_main_loop)
{
	bool                has_moved;
	int                 index;
	bool                is_sort_needed = false;
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
		person_get_xy(s_players[i].person, &start_x[i], &start_y[i], false);
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->leader != NULL)
			continue;  // skip followers for now
		update_person(s_persons[i], &has_moved);
		is_sort_needed |= has_moved;
	}
	if (is_sort_needed)
		sort_persons();

	// update color mask fade level
	if (s_fade_progress < s_fade_frames) {
		++s_fade_progress;
		s_color_mask = color_mix(s_fade_color_to, s_fade_color_from,
			s_fade_progress, s_fade_frames - s_fade_progress);
	}

	// update camera
	if (s_camera_person != NULL) {
		person_get_xy(s_camera_person, &x, &y, true);
		s_camera_x = x; s_camera_y = y;
	}

	// run edge script if the camera has moved past the edge of the map
	// note: only applies for non-repeating maps
	if (in_main_loop && !s_map->is_repeating) {
		script_type = s_camera_y < 0 ? MAP_SCRIPT_ON_LEAVE_NORTH
			: s_camera_x >= map_w ? MAP_SCRIPT_ON_LEAVE_EAST
			: s_camera_y >= map_h ? MAP_SCRIPT_ON_LEAVE_SOUTH
			: s_camera_x < 0 ? MAP_SCRIPT_ON_LEAVE_WEST
			: MAP_SCRIPT_MAX;
		if (script_type < MAP_SCRIPT_MAX)
			map_call_script(script_type, true);
	}

	// if there are any input persons, check for trigger activation
	for (i = 0; i < MAX_PLAYERS; ++i) if (s_players[i].person != NULL) {
		// did we step on a trigger or move to a new one?
		person_get_xyz(s_players[i].person, &x, &y, &layer, true);
		trigger = get_trigger_at(x, y, layer, &index);
		if (trigger != s_on_trigger) {
			last_trigger = s_current_trigger;
			s_current_trigger = index;
			s_on_trigger = trigger;
			if (trigger != NULL)
				script_run(trigger->script, false);
			s_current_trigger = last_trigger;
		}
	}

	// update any zones occupied by the input person
	// note: a zone's step count is in reality a pixel count, so a zone
	//       may be updated multiple times in a single frame.
	for (k = 0; k < MAX_PLAYERS; ++k) if (s_players[k].person != NULL) {
		person_get_xy(s_players[k].person, &x, &y, false);
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
					script_run(zone->script, true);
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
			--s_num_delay_scripts;
			script_run(script_to_run, false);
			script_free(script_to_run);
			--i;
		}
	}

	// now that everything else is in order, we can run the
	// update script!
	script_run(s_update_script, false);
}

static void
update_person(person_t* person, bool* out_has_moved)
{
	struct command  command;
	double          delta_x, delta_y;
	int             facing;
	bool            has_moved;
	bool            is_finished;
	const person_t* last_person;
	struct step     step;
	int             vector;

	int i;

	person->mv_x = 0; person->mv_y = 0;
	if (person->revert_frames > 0 && --person->revert_frames <= 0)
		person->frame = 0;
	if (person->leader == NULL) {  // no leader; use command queue
								   // call the command generator if the queue is empty
		if (person->num_commands == 0)
			person_call_script(person, PERSON_SCRIPT_GENERATOR, true);

		// run through the queue, stopping after the first non-immediate command
		is_finished = !does_person_exist(person) || person->num_commands == 0;
		while (!is_finished) {
			command = person->commands[0];
			--person->num_commands;
			for (i = 0; i < person->num_commands; ++i)
				person->commands[i] = person->commands[i + 1];
			last_person = s_current_person;
			s_current_person = person;
			if (command.type != COMMAND_RUN_SCRIPT)
				command_person(person, command.type);
			else
				script_run(command.script, false);
			s_current_person = last_person;
			script_free(command.script);
			is_finished = !does_person_exist(person)  // stop if person was destroyed
				|| !command.is_immediate || person->num_commands == 0;
		}
	}
	else {  // leader set; follow the leader!
		step = person->leader->steps[person->follow_distance - 1];
		delta_x = step.x - person->x;
		delta_y = step.y - person->y;
		if (fabs(delta_x) > person->speed_x)
			command_person(person, delta_x > 0 ? COMMAND_MOVE_EAST : COMMAND_MOVE_WEST);
		if (!does_person_exist(person)) return;
		if (fabs(delta_y) > person->speed_y)
			command_person(person, delta_y > 0 ? COMMAND_MOVE_SOUTH : COMMAND_MOVE_NORTH);
		if (!does_person_exist(person)) return;
		vector = person->mv_x + person->mv_y * 3;
		facing = vector == -3 ? COMMAND_FACE_NORTH
			: vector == -2 ? COMMAND_FACE_NORTHEAST
			: vector == 1 ? COMMAND_FACE_EAST
			: vector == 4 ? COMMAND_FACE_SOUTHEAST
			: vector == 3 ? COMMAND_FACE_SOUTH
			: vector == 2 ? COMMAND_FACE_SOUTHWEST
			: vector == -1 ? COMMAND_FACE_WEST
			: vector == -4 ? COMMAND_FACE_NORTHWEST
			: COMMAND_WAIT;
		if (facing != COMMAND_WAIT)
			command_person(person, COMMAND_ANIMATE);
		if (!does_person_exist(person)) return;
		command_person(person, facing);
	}

	// check that the person didn't mysteriously disappear...
	if (!does_person_exist(person))
		return;  // they probably got eaten by a hunger-pig or something.

				 // if the person's position changed, record it in their step history
	*out_has_moved = person_has_moved(person);
	if (*out_has_moved)
		record_step(person);

	// recursively update the follower chain
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->leader != person)
			continue;
		update_person(s_persons[i], &has_moved);
		*out_has_moved |= has_moved;
	}
}

void
init_map_engine_api(duk_context* ctx)
{
	duk_push_global_stash(ctx);
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, "personData");
	duk_pop(ctx);

	api_define_function(ctx, NULL, "AddTrigger", js_AddTrigger);
	api_define_function(ctx, NULL, "AddZone", js_AddZone);
	api_define_function(ctx, NULL, "AreZonesAt", js_AreZonesAt);
	api_define_function(ctx, NULL, "AttachCamera", js_AttachCamera);
	api_define_function(ctx, NULL, "AttachInput", js_AttachInput);
	api_define_function(ctx, NULL, "AttachPlayerInput", js_AttachPlayerInput);
	api_define_function(ctx, NULL, "CallDefaultMapScript", js_CallDefaultMapScript);
	api_define_function(ctx, NULL, "CallDefaultPersonScript", js_CallDefaultPersonScript);
	api_define_function(ctx, NULL, "CallMapScript", js_CallMapScript);
	api_define_function(ctx, NULL, "CallPersonScript", js_CallPersonScript);
	api_define_function(ctx, NULL, "ChangeMap", js_ChangeMap);
	api_define_function(ctx, NULL, "ClearPersonCommands", js_ClearPersonCommands);
	api_define_function(ctx, NULL, "CreatePerson", js_CreatePerson);
	api_define_function(ctx, NULL, "DestroyPerson", js_DestroyPerson);
	api_define_function(ctx, NULL, "DetachCamera", js_DetachCamera);
	api_define_function(ctx, NULL, "DetachInput", js_DetachInput);
	api_define_function(ctx, NULL, "DetachPlayerInput", js_DetachPlayerInput);
	api_define_function(ctx, NULL, "DoesPersonExist", js_DoesPersonExist);
	api_define_function(ctx, NULL, "ExecuteTrigger", js_ExecuteTrigger);
	api_define_function(ctx, NULL, "ExecuteZoneScript", js_ExecuteZoneScript);
	api_define_function(ctx, NULL, "ExecuteZones", js_ExecuteZones);
	api_define_function(ctx, NULL, "ExitMapEngine", js_ExitMapEngine);
	api_define_function(ctx, NULL, "FollowPerson", js_FollowPerson);
	api_define_function(ctx, NULL, "GetActingPerson", js_GetActingPerson);
	api_define_function(ctx, NULL, "GetCameraPerson", js_GetCameraPerson);
	api_define_function(ctx, NULL, "GetCameraX", js_GetCameraX);
	api_define_function(ctx, NULL, "GetCameraY", js_GetCameraY);
	api_define_function(ctx, NULL, "GetCurrentMap", js_GetCurrentMap);
	api_define_function(ctx, NULL, "GetCurrentPerson", js_GetCurrentPerson);
	api_define_function(ctx, NULL, "GetCurrentTrigger", js_GetCurrentTrigger);
	api_define_function(ctx, NULL, "GetCurrentZone", js_GetCurrentZone);
	api_define_function(ctx, NULL, "GetInputPerson", js_GetInputPerson);
	api_define_function(ctx, NULL, "GetLayerHeight", js_GetLayerHeight);
	api_define_function(ctx, NULL, "GetLayerMask", js_GetLayerMask);
	api_define_function(ctx, NULL, "GetLayerName", js_GetLayerName);
	api_define_function(ctx, NULL, "GetLayerWidth", js_GetLayerWidth);
	api_define_function(ctx, NULL, "GetMapEngineFrameRate", js_GetMapEngineFrameRate);
	api_define_function(ctx, NULL, "GetNextAnimatedTile", js_GetNextAnimatedTile);
	api_define_function(ctx, NULL, "GetNumLayers", js_GetNumLayers);
	api_define_function(ctx, NULL, "GetNumTiles", js_GetNumTiles);
	api_define_function(ctx, NULL, "GetNumTriggers", js_GetNumTriggers);
	api_define_function(ctx, NULL, "GetNumZones", js_GetNumZones);
	api_define_function(ctx, NULL, "GetObstructingPerson", js_GetObstructingPerson);
	api_define_function(ctx, NULL, "GetObstructingTile", js_GetObstructingTile);
	api_define_function(ctx, NULL, "GetPersonAngle", js_GetPersonAngle);
	api_define_function(ctx, NULL, "GetPersonBase", js_GetPersonBase);
	api_define_function(ctx, NULL, "GetPersonData", js_GetPersonData);
	api_define_function(ctx, NULL, "GetPersonDirection", js_GetPersonDirection);
	api_define_function(ctx, NULL, "GetPersonFollowDistance", js_GetPersonFollowDistance);
	api_define_function(ctx, NULL, "GetPersonFollowers", js_GetPersonFollowers);
	api_define_function(ctx, NULL, "GetPersonFrame", js_GetPersonFrame);
	api_define_function(ctx, NULL, "GetPersonFrameNext", js_GetPersonFrameNext);
	api_define_function(ctx, NULL, "GetPersonFrameRevert", js_GetPersonFrameRevert);
	api_define_function(ctx, NULL, "GetPersonIgnoreList", js_GetPersonIgnoreList);
	api_define_function(ctx, NULL, "GetPersonLayer", js_GetPersonLayer);
	api_define_function(ctx, NULL, "GetPersonLeader", js_GetPersonLeader);
	api_define_function(ctx, NULL, "GetPersonList", js_GetPersonList);
	api_define_function(ctx, NULL, "GetPersonMask", js_GetPersonMask);
	api_define_function(ctx, NULL, "GetPersonOffsetX", js_GetPersonOffsetX);
	api_define_function(ctx, NULL, "GetPersonOffsetY", js_GetPersonOffsetY);
	api_define_function(ctx, NULL, "GetPersonSpeedX", js_GetPersonSpeedX);
	api_define_function(ctx, NULL, "GetPersonSpeedY", js_GetPersonSpeedY);
	api_define_function(ctx, NULL, "GetPersonSpriteset", js_GetPersonSpriteset);
	api_define_function(ctx, NULL, "GetPersonValue", js_GetPersonValue);
	api_define_function(ctx, NULL, "GetPersonX", js_GetPersonX);
	api_define_function(ctx, NULL, "GetPersonXFloat", js_GetPersonXFloat);
	api_define_function(ctx, NULL, "GetPersonY", js_GetPersonY);
	api_define_function(ctx, NULL, "GetPersonYFloat", js_GetPersonYFloat);
	api_define_function(ctx, NULL, "GetTalkActivationButton", js_GetTalkActivationButton);
	api_define_function(ctx, NULL, "GetTalkActivationKey", js_GetTalkActivationKey);
	api_define_function(ctx, NULL, "GetTalkDistance", js_GetTalkDistance);
	api_define_function(ctx, NULL, "GetTile", js_GetTile);
	api_define_function(ctx, NULL, "GetTileDelay", js_GetTileDelay);
	api_define_function(ctx, NULL, "GetTileImage", js_GetTileImage);
	api_define_function(ctx, NULL, "GetTileHeight", js_GetTileHeight);
	api_define_function(ctx, NULL, "GetTileName", js_GetTileName);
	api_define_function(ctx, NULL, "GetTileSurface", js_GetTileSurface);
	api_define_function(ctx, NULL, "GetTileWidth", js_GetTileWidth);
	api_define_function(ctx, NULL, "GetTriggerLayer", js_GetTriggerLayer);
	api_define_function(ctx, NULL, "GetTriggerX", js_GetTriggerX);
	api_define_function(ctx, NULL, "GetTriggerY", js_GetTriggerY);
	api_define_function(ctx, NULL, "GetZoneHeight", js_GetZoneHeight);
	api_define_function(ctx, NULL, "GetZoneLayer", js_GetZoneLayer);
	api_define_function(ctx, NULL, "GetZoneSteps", js_GetZoneSteps);
	api_define_function(ctx, NULL, "GetZoneWidth", js_GetZoneWidth);
	api_define_function(ctx, NULL, "GetZoneX", js_GetZoneX);
	api_define_function(ctx, NULL, "GetZoneY", js_GetZoneY);
	api_define_function(ctx, NULL, "IgnorePersonObstructions", js_IgnorePersonObstructions);
	api_define_function(ctx, NULL, "IgnoreTileObstructions", js_IgnoreTileObstructions);
	api_define_function(ctx, NULL, "IsCameraAttached", js_IsCameraAttached);
	api_define_function(ctx, NULL, "IsCommandQueueEmpty", js_IsCommandQueueEmpty);
	api_define_function(ctx, NULL, "IsIgnoringPersonObstructions", js_IsIgnoringPersonObstructions);
	api_define_function(ctx, NULL, "IsIgnoringTileObstructions", js_IsIgnoringTileObstructions);
	api_define_function(ctx, NULL, "IsInputAttached", js_IsInputAttached);
	api_define_function(ctx, NULL, "IsLayerReflective", js_IsLayerReflective);
	api_define_function(ctx, NULL, "IsLayerVisible", js_IsLayerVisible);
	api_define_function(ctx, NULL, "IsMapEngineRunning", js_IsMapEngineRunning);
	api_define_function(ctx, NULL, "IsPersonObstructed", js_IsPersonObstructed);
	api_define_function(ctx, NULL, "IsPersonVisible", js_IsPersonVisible);
	api_define_function(ctx, NULL, "IsTriggerAt", js_IsTriggerAt);
	api_define_function(ctx, NULL, "MapEngine", js_MapEngine);
	api_define_function(ctx, NULL, "MapToScreenX", js_MapToScreenX);
	api_define_function(ctx, NULL, "MapToScreenY", js_MapToScreenY);
	api_define_function(ctx, NULL, "QueuePersonCommand", js_QueuePersonCommand);
	api_define_function(ctx, NULL, "QueuePersonScript", js_QueuePersonScript);
	api_define_function(ctx, NULL, "RemoveTrigger", js_RemoveTrigger);
	api_define_function(ctx, NULL, "RemoveZone", js_RemoveZone);
	api_define_function(ctx, NULL, "RenderMap", js_RenderMap);
	api_define_function(ctx, NULL, "ReplaceTilesOnLayer", js_ReplaceTilesOnLayer);
	api_define_function(ctx, NULL, "ScreenToMapX", js_ScreenToMapX);
	api_define_function(ctx, NULL, "ScreenToMapY", js_ScreenToMapY);
	api_define_function(ctx, NULL, "SetCameraX", js_SetCameraX);
	api_define_function(ctx, NULL, "SetCameraY", js_SetCameraY);
	api_define_function(ctx, NULL, "SetColorMask", js_SetColorMask);
	api_define_function(ctx, NULL, "SetDefaultMapScript", js_SetDefaultMapScript);
	api_define_function(ctx, NULL, "SetDefaultPersonScript", js_SetDefaultPersonScript);
	api_define_function(ctx, NULL, "SetDelayScript", js_SetDelayScript);
	api_define_function(ctx, NULL, "SetLayerHeight", js_SetLayerHeight);
	api_define_function(ctx, NULL, "SetLayerMask", js_SetLayerMask);
	api_define_function(ctx, NULL, "SetLayerReflective", js_SetLayerReflective);
	api_define_function(ctx, NULL, "SetLayerRenderer", js_SetLayerRenderer);
	api_define_function(ctx, NULL, "SetLayerSize", js_SetLayerSize);
	api_define_function(ctx, NULL, "SetLayerVisible", js_SetLayerVisible);
	api_define_function(ctx, NULL, "SetLayerWidth", js_SetLayerWidth);
	api_define_function(ctx, NULL, "SetMapEngineFrameRate", js_SetMapEngineFrameRate);
	api_define_function(ctx, NULL, "SetNextAnimatedTile", js_SetNextAnimatedTile);
	api_define_function(ctx, NULL, "SetPersonAngle", js_SetPersonAngle);
	api_define_function(ctx, NULL, "SetPersonData", js_SetPersonData);
	api_define_function(ctx, NULL, "SetPersonDirection", js_SetPersonDirection);
	api_define_function(ctx, NULL, "SetPersonFollowDistance", js_SetPersonFollowDistance);
	api_define_function(ctx, NULL, "SetPersonFrame", js_SetPersonFrame);
	api_define_function(ctx, NULL, "SetPersonFrameNext", js_SetPersonFrameNext);
	api_define_function(ctx, NULL, "SetPersonFrameRevert", js_SetPersonFrameRevert);
	api_define_function(ctx, NULL, "SetPersonIgnoreList", js_SetPersonIgnoreList);
	api_define_function(ctx, NULL, "SetPersonLayer", js_SetPersonLayer);
	api_define_function(ctx, NULL, "SetPersonMask", js_SetPersonMask);
	api_define_function(ctx, NULL, "SetPersonOffsetX", js_SetPersonOffsetX);
	api_define_function(ctx, NULL, "SetPersonOffsetY", js_SetPersonOffsetY);
	api_define_function(ctx, NULL, "SetPersonScaleAbsolute", js_SetPersonScaleAbsolute);
	api_define_function(ctx, NULL, "SetPersonScaleFactor", js_SetPersonScaleFactor);
	api_define_function(ctx, NULL, "SetPersonScript", js_SetPersonScript);
	api_define_function(ctx, NULL, "SetPersonSpeed", js_SetPersonSpeed);
	api_define_function(ctx, NULL, "SetPersonSpeedXY", js_SetPersonSpeedXY);
	api_define_function(ctx, NULL, "SetPersonSpriteset", js_SetPersonSpriteset);
	api_define_function(ctx, NULL, "SetPersonValue", js_SetPersonValue);
	api_define_function(ctx, NULL, "SetPersonVisible", js_SetPersonVisible);
	api_define_function(ctx, NULL, "SetPersonX", js_SetPersonX);
	api_define_function(ctx, NULL, "SetPersonXYFloat", js_SetPersonXYFloat);
	api_define_function(ctx, NULL, "SetPersonY", js_SetPersonY);
	api_define_function(ctx, NULL, "SetRenderScript", js_SetRenderScript);
	api_define_function(ctx, NULL, "SetTalkActivationButton", js_SetTalkActivationButton);
	api_define_function(ctx, NULL, "SetTalkActivationKey", js_SetTalkActivationKey);
	api_define_function(ctx, NULL, "SetTalkDistance", js_SetTalkDistance);
	api_define_function(ctx, NULL, "SetTile", js_SetTile);
	api_define_function(ctx, NULL, "SetTileDelay", js_SetTileDelay);
	api_define_function(ctx, NULL, "SetTileImage", js_SetTileImage);
	api_define_function(ctx, NULL, "SetTileName", js_SetTileName);
	api_define_function(ctx, NULL, "SetTileSurface", js_SetTileSurface);
	api_define_function(ctx, NULL, "SetTriggerLayer", js_SetTriggerLayer);
	api_define_function(ctx, NULL, "SetTriggerScript", js_SetTriggerScript);
	api_define_function(ctx, NULL, "SetTriggerXY", js_SetTriggerXY);
	api_define_function(ctx, NULL, "SetUpdateScript", js_SetUpdateScript);
	api_define_function(ctx, NULL, "SetZoneLayer", js_SetZoneLayer);
	api_define_function(ctx, NULL, "SetZoneMetrics", js_SetZoneMetrics);
	api_define_function(ctx, NULL, "SetZoneScript", js_SetZoneScript);
	api_define_function(ctx, NULL, "SetZoneSteps", js_SetZoneSteps);
	api_define_function(ctx, NULL, "UpdateMapEngine", js_UpdateMapEngine);

	// map script types
	api_define_const(ctx, NULL, "SCRIPT_ON_ENTER_MAP", MAP_SCRIPT_ON_ENTER);
	api_define_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP", MAP_SCRIPT_ON_LEAVE);
	api_define_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_NORTH", MAP_SCRIPT_ON_LEAVE_NORTH);
	api_define_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_EAST", MAP_SCRIPT_ON_LEAVE_EAST);
	api_define_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_SOUTH", MAP_SCRIPT_ON_LEAVE_SOUTH);
	api_define_const(ctx, NULL, "SCRIPT_ON_LEAVE_MAP_WEST", MAP_SCRIPT_ON_LEAVE_WEST);

	// movement script specifier constants
	api_define_const(ctx, NULL, "SCRIPT_ON_CREATE", PERSON_SCRIPT_ON_CREATE);
	api_define_const(ctx, NULL, "SCRIPT_ON_DESTROY", PERSON_SCRIPT_ON_DESTROY);
	api_define_const(ctx, NULL, "SCRIPT_ON_ACTIVATE_TOUCH", PERSON_SCRIPT_ON_TOUCH);
	api_define_const(ctx, NULL, "SCRIPT_ON_ACTIVATE_TALK", PERSON_SCRIPT_ON_TALK);
	api_define_const(ctx, NULL, "SCRIPT_COMMAND_GENERATOR", PERSON_SCRIPT_GENERATOR);

	// person movement commands
	api_define_const(ctx, NULL, "COMMAND_WAIT", COMMAND_WAIT);
	api_define_const(ctx, NULL, "COMMAND_ANIMATE", COMMAND_ANIMATE);
	api_define_const(ctx, NULL, "COMMAND_FACE_NORTH", COMMAND_FACE_NORTH);
	api_define_const(ctx, NULL, "COMMAND_FACE_NORTHEAST", COMMAND_FACE_NORTHEAST);
	api_define_const(ctx, NULL, "COMMAND_FACE_EAST", COMMAND_FACE_EAST);
	api_define_const(ctx, NULL, "COMMAND_FACE_SOUTHEAST", COMMAND_FACE_SOUTHEAST);
	api_define_const(ctx, NULL, "COMMAND_FACE_SOUTH", COMMAND_FACE_SOUTH);
	api_define_const(ctx, NULL, "COMMAND_FACE_SOUTHWEST", COMMAND_FACE_SOUTHWEST);
	api_define_const(ctx, NULL, "COMMAND_FACE_WEST", COMMAND_FACE_WEST);
	api_define_const(ctx, NULL, "COMMAND_FACE_NORTHWEST", COMMAND_FACE_NORTHWEST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_NORTH", COMMAND_MOVE_NORTH);
	api_define_const(ctx, NULL, "COMMAND_MOVE_NORTHEAST", COMMAND_MOVE_NORTHEAST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_EAST", COMMAND_MOVE_EAST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_SOUTHEAST", COMMAND_MOVE_SOUTHEAST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_SOUTH", COMMAND_MOVE_SOUTH);
	api_define_const(ctx, NULL, "COMMAND_MOVE_SOUTHWEST", COMMAND_MOVE_SOUTHWEST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_WEST", COMMAND_MOVE_WEST);
	api_define_const(ctx, NULL, "COMMAND_MOVE_NORTHWEST", COMMAND_MOVE_NORTHWEST);
}

static int
duk_require_map_layer(duk_context* ctx, duk_idx_t index)
{
	long        strtol_out;
	int         layer_index;
	const char* name;
	char*       p_end;

	duk_require_type_mask(ctx, index, DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER);
	
	if (duk_is_number(ctx, index)) {
		layer_index = duk_get_int(ctx, index);
		goto have_index;
	}
	else {
		// don't anyone ever say I'm not dedicated to compatibility!  there are a few
		// poorly written Sphere 1.5 games that pass layer IDs as strings.  usually this
		// would fail because miniSphere supports named layers, but here I go out of my
		// way to support it anyway.
		name = duk_get_string(ctx, index);
		strtol_out = strtol(name, &p_end, 0);
		if ((layer_index = find_layer(name)) >= 0)
			goto have_index;
		else if (name[0] != '\0' && *p_end == '\0') {
			layer_index = (int)strtol_out;
			goto have_index;
		}
		else
			duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "layer name does not exist '%s'", name);
	}
	
have_index:
	if (layer_index < 0 || layer_index >= map_num_layers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "layer index out of range");
	return layer_index;
}

static duk_ret_t
js_AddTrigger(duk_context* ctx)
{
	rect_t     bounds;
	int        layer;
	script_t*  script;
	lstring_t* script_name;
	int        x;
	int        y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	bounds = map_bounds();
	if (x < bounds.x1 || y < bounds.y1 || x >= bounds.x2 || y >= bounds.y2)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "X/Y out of bounds");
	script_name = lstr_newf("%s/trig%d", map_name(), map_num_triggers());
	script = duk_require_sphere_script(ctx, 3, lstr_cstr(script_name));
	if (!map_add_trigger(x, y, layer, script))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to add trigger to map");
	duk_push_number(ctx, map_num_triggers() - 1);
	lstr_free(script_name);
	return 1;
}

static duk_ret_t
js_AddZone(duk_context* ctx)
{
	rect_t     bounds;
	int        height;
	int        layer;
	script_t*  script;
	lstring_t* script_name;
	int        width;
	int        x;
	int        y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	width = duk_to_int(ctx, 2);
	height = duk_to_int(ctx, 3);
	layer = duk_require_map_layer(ctx, 4);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	bounds = map_bounds();
	if (width <= 0 || height <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone width/height");
	if (x < bounds.x1 || y < bounds.y1 || x + width > bounds.x2 || y + height > bounds.y2)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "zone out of bounds");
	script_name = lstr_newf("%s/zone%d", map_name(), map_num_zones());
	script = duk_require_sphere_script(ctx, 5, lstr_cstr(script_name));
	if (!map_add_zone(new_rect(x, y, width, height), layer, script, 8))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to add zone to map");
	lstr_free(script_name);
	duk_push_number(ctx, map_num_zones() - 1);
	return 1;
}

static duk_ret_t
js_AreZonesAt(duk_context* ctx)
{
	int layer;
	int x;
	int y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);

	duk_push_boolean(ctx, map_zones_at(x, y, layer, NULL));
	return 1;
}

static duk_ret_t
js_AttachCamera(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);

	if (!(person = person_find(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "person `%s` doesn't exist", name);
	s_camera_person = person;
	return 0;
}

static duk_ret_t
js_AttachInput(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	int i;

	name = duk_require_string(ctx, 0);

	if (!(person = person_find(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "person `%s` doesn't exist", name);
	
	// detach person from other players
	for (i = 0; i < MAX_PLAYERS; ++i) {
		if (s_players[i].person == person)
			s_players[i].person = NULL;
	}
	
	s_players[0].person = person;
	return 0;
}

static duk_ret_t
js_AttachPlayerInput(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	int         player;

	int i;

	name = duk_require_string(ctx, 0);
	player = duk_to_int(ctx, 1);

	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "player number out of range");
	if (!(person = person_find(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "person `%s` doesn't exist", name);
	for (i = 0; i < MAX_PLAYERS; ++i)  // detach person from other players
		if (s_players[i].person == person) s_players[i].person = NULL;
	s_players[player].person = person;
	return 0;
}

static duk_ret_t
js_CallDefaultMapScript(duk_context* ctx)
{
	int type = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid script type constant");
	script_run(s_def_map_scripts[type], false);
	return 0;
}

static duk_ret_t
js_CallDefaultPersonScript(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int type = duk_require_int(ctx, 1);

	const person_t* last_person;
	person_t*       person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid script type constant");
	last_person = s_current_person;
	s_current_person = person;
	script_run(s_def_map_scripts[type], false);
	s_current_person = last_person;
	return 0;
}

static duk_ret_t
js_CallMapScript(duk_context* ctx)
{
	int type = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (type < 0 || type >= MAP_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid script type constant");
	script_run(s_map->scripts[type], false);
	return 0;
}

static duk_ret_t
js_CallPersonScript(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int type = duk_require_int(ctx, 1);

	person_t*   person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid script type constant");
	person_call_script(person, type, false);
	return 0;
}

static duk_ret_t
js_ClearPersonCommands(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person->num_commands = 0;
	return 0;
}

static duk_ret_t
js_ChangeMap(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, "maps", true, false);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (!change_map(filename, false))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't load map `%s`", filename);
	return 0;
}

static duk_ret_t
js_CreatePerson(duk_context* ctx)
{
	bool         destroy_with_map;
	const char*  filename;
	const char*  name;
	person_t*    person;
	spriteset_t* spriteset;

	name = duk_require_string(ctx, 0);
	destroy_with_map = duk_to_boolean(ctx, 2);

	if (duk_is_class_obj(ctx, 1, "ssSpriteset"))
		// ref the spriteset so we can safely free it later. this avoids
		// having to check the argument type again.
		spriteset = duk_require_sphere_spriteset(ctx, 1);
	else {
		filename = duk_require_path(ctx, 1, "spritesets", true, false);
		if (!(spriteset = spriteset_load(filename)))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't load spriteset `%s`", filename);
	}

	// create the person and its JS-side data object
	person = person_new(name, spriteset, !destroy_with_map, NULL);
	spriteset_free(spriteset);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "personData");
	duk_push_object(ctx); duk_put_prop_string(ctx, -2, name);
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
js_DestroyPerson(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_free(person);
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

	if (duk_get_top(ctx) < 1) {
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "player number or string expected as argument");
	}
	else if (duk_is_string(ctx, 0)) {
		name = duk_get_string(ctx, 0);
		if (!(person = person_find(name)))
			duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "person `%s` doesn't exist", name);
		player = -1;
		for (i = MAX_PLAYERS - 1; i >= 0; --i)  // ensures Sphere semantics
			player = s_players[i].person == person ? i : player;
		if (player == -1)
			return 0;
	}
	else if (duk_is_number(ctx, 0)) {
		player = duk_get_int(ctx, 0);
	}
	else {
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "player number or string expected as argument");
	}
	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "player number out of range (%d)", player);
	s_players[player].person = NULL;
	return 0;
}

static duk_ret_t
js_DoesPersonExist(duk_context* ctx)
{
	const char* name;
	
	name = duk_require_string(ctx, 0);

	duk_push_boolean(ctx, person_find(name) != NULL);
	return 1;
}

static duk_ret_t
js_ExecuteTrigger(duk_context* ctx)
{
	int index;
	int layer;
	int x;
	int y;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if ((index = map_trigger_at(x, y, layer)) >= 0)
		trigger_activate(index);
	return 0;
}

static duk_ret_t
js_ExecuteZoneScript(duk_context* ctx)
{
	int              last_zone;
	struct map_zone* zone;
	int              zone_index;

	zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= map_num_zones())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");

	zone = vector_get(s_map->zones, zone_index);
	last_zone = s_current_zone;
	s_current_zone = zone_index;
	script_run(zone->script, true);
	s_current_zone = last_zone;
	return 0;
}

static duk_ret_t
js_ExecuteZones(duk_context* ctx)
{
	int              index;
	int              last_zone;
	int              layer;
	int              x;
	int              y;
	struct map_zone* zone;

	int i;

	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	layer = duk_require_map_layer(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	i = 0;
	while (zone = get_zone_at(x, y, layer, i++, &index)) {
		last_zone = s_current_zone;
		s_current_zone = index;
		script_run(zone->script, true);
		s_current_zone = last_zone;
	}
	return 0;
}

static duk_ret_t
js_ExitMapEngine(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	map_engine_exit();
	return 0;
}

static duk_ret_t
js_FollowPerson(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	const char* leader_name = duk_is_null(ctx, 1) ? "" : duk_require_string(ctx, 1);
	int distance = leader_name[0] != '\0' ? duk_require_int(ctx, 2) : 0;

	person_t* leader = NULL;
	person_t* person;

	if (!(person = person_find(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (!(leader_name[0] == '\0' || (leader = person_find(leader_name))))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", leader_name);
	if (distance <= 0 && leader_name[0] != '\0')
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid distance", distance);
	if (!person_follow(person, leader, distance))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "circular chain not allowed");
	return 0;
}

static duk_ret_t
js_GetActingPerson(duk_context* ctx)
{
	if (s_acting_person == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "must be called from person activation script (touch/talk)");
	duk_push_string(ctx, person_name(s_acting_person));
	return 1;
}

static duk_ret_t
js_GetCameraPerson(duk_context* ctx)
{
	if (s_camera_person == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid operation, camera not attached");
	duk_push_string(ctx, person_name(s_camera_person));
	return 1;
}

static duk_ret_t
js_GetCameraX(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	duk_push_int(ctx, s_camera_x);
	return 1;
}

static duk_ret_t
js_GetCameraY(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	duk_push_int(ctx, s_camera_y);
	return 1;
}

static duk_ret_t
js_GetCurrentMap(duk_context* ctx)
{
	path_t* map_name;
	path_t* origin;
	
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	
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
js_GetCurrentPerson(duk_context* ctx)
{
	if (s_current_person == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "must be called from a person script");
	duk_push_string(ctx, person_name(s_current_person));
	return 1;
}

static duk_ret_t
js_GetCurrentTrigger(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (s_current_trigger == -1)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot call outside of a trigger script");
	duk_push_int(ctx, s_current_trigger);
	return 1;
}

static duk_ret_t
js_GetCurrentZone(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (s_current_zone == -1)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "cannot call outside of a zone script");
	duk_push_int(ctx, s_current_zone);
	return 1;
}

static duk_ret_t
js_GetInputPerson(duk_context* ctx)
{
	int num_args;
	int player;

	num_args = duk_get_top(ctx);
	player = num_args >= 1 ? duk_to_int(ctx, 0) : 0;

	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "player number out of range (%d)", player);
	if (s_players[player].person == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "input not attached for player %d", player + 1);
	duk_push_string(ctx, person_name(s_players[player].person));
	return 1;
}

static duk_ret_t
js_GetLayerHeight(duk_context* ctx)
{
	int layer;

	layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	duk_push_int(ctx, s_map->layers[layer].height);
	return 1;
}

static duk_ret_t
js_GetLayerMask(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	duk_push_sphere_color(ctx, s_map->layers[layer].color_mask);
	return 1;
}

static duk_ret_t
js_GetLayerName(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	duk_push_lstring_t(ctx, s_map->layers[layer].name);
	return 1;
}

static duk_ret_t
js_GetLayerWidth(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
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
	int        tile_index;
	tileset_t* tileset;
	
	tile_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");

	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index %d", tile_index);
	duk_push_int(ctx, tileset_get_next(tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetNumLayers(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	duk_push_int(ctx, map_num_layers());
	return 1;
}

static duk_ret_t
js_GetNumTiles(duk_context* ctx)
{
	tileset_t* tileset;
	
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	
	tileset = map_tileset();
	duk_push_int(ctx, tileset_len(tileset));
	return 1;
}

static duk_ret_t
js_GetNumTriggers(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	
	duk_push_number(ctx, map_num_triggers());
	return 1;
}

static duk_ret_t
js_GetNumZones(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	
	duk_push_number(ctx, map_num_zones());
	return 1;
}

static duk_ret_t
js_GetObstructingPerson(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	person_t* obs_person = NULL;
	person_t* person;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine must be running");
	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_obstructed_at(person, x, y, &obs_person, NULL);
	duk_push_string(ctx, obs_person != NULL ? person_name(obs_person) : "");
	return 1;
}

static duk_ret_t
js_GetObstructingTile(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	person_t* person;
	int       tile_index;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine must be running");
	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_obstructed_at(person, x, y, NULL, &tile_index);
	duk_push_int(ctx, tile_index);
	return 1;
}

static duk_ret_t
js_GetPersonAngle(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_number(ctx, person_get_angle(person));
	return 1;
}

static duk_ret_t
js_GetPersonBase(duk_context* ctx)
{
	rect_t      base;
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if (!(person = person_find(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	base = spriteset_get_base(person_get_spriteset(person));
	duk_push_object(ctx);
	duk_push_int(ctx, base.x1); duk_put_prop_string(ctx, -2, "x1");
	duk_push_int(ctx, base.y1); duk_put_prop_string(ctx, -2, "y1");
	duk_push_int(ctx, base.x2); duk_put_prop_string(ctx, -2, "x2");
	duk_push_int(ctx, base.y2); duk_put_prop_string(ctx, -2, "y2");
	return 1;
}

static duk_ret_t
js_GetPersonData(duk_context* ctx)
{
	int          height;
	const char*  name;
	int          num_directions;
	int          num_frames;
	person_t*    person;
	spriteset_t* spriteset;
	int          width;

	name = duk_require_string(ctx, 0);

	if (!(person = person_find(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	spriteset = person->sprite;
	width = spriteset_width(spriteset);
	height = spriteset_height(spriteset);
	num_directions = spriteset_num_poses(spriteset);
	num_frames = spriteset_num_frames(spriteset, person->direction);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "personData");
	duk_get_prop_string(ctx, -1, name);
	duk_push_int(ctx, num_frames); duk_put_prop_string(ctx, -2, "num_frames");
	duk_push_int(ctx, num_directions); duk_put_prop_string(ctx, -2, "num_directions");
	duk_push_int(ctx, width); duk_put_prop_string(ctx, -2, "width");
	duk_push_int(ctx, height); duk_put_prop_string(ctx, -2, "height");
	duk_push_string(ctx, person->leader ? person->leader->name : ""); duk_put_prop_string(ctx, -2, "leader");
	duk_remove(ctx, -2); duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_GetPersonDirection(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if (!(person = person_find(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_string(ctx, person->direction);
	return 1;
}

static duk_ret_t
js_GetPersonFollowDistance(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if (!(person = person_find(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (person->leader == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "person has no leader");
	duk_push_int(ctx, person->follow_distance);
	return 1;
}

static duk_ret_t
js_GetPersonFollowers(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	duk_uarridx_t index = 0;
	person_t*     person;

	int i;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_array(ctx);
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->leader == person) {
			duk_push_string(ctx, s_persons[i]->name);
			duk_put_prop_index(ctx, -2, index++);
		}
	}
	return 1;
}

static duk_ret_t
js_GetPersonFrame(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	int       num_frames;
	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	num_frames = spriteset_num_frames(person->sprite, person->direction);
	duk_push_int(ctx, person->frame % num_frames);
	return 1;
}

static duk_ret_t
js_GetPersonFrameNext(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_int(ctx, person->anim_frames);
	return 1;
}

static duk_ret_t
js_GetPersonFrameRevert(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_int(ctx, person->revert_delay);
	return 1;
}

static duk_ret_t
js_GetPersonIgnoreList(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	int i;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_array(ctx);
	for (i = 0; i < person->num_ignores; ++i) {
		duk_push_string(ctx, person->ignores[i]);
		duk_put_prop_index(ctx, -2, i);
	}
	return 1;
}

static duk_ret_t
js_GetPersonLayer(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_int(ctx, person->layer);
	return 1;
}

static duk_ret_t
js_GetPersonLeader(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_string(ctx, person->leader != NULL ? person->leader->name : "");
	return 1;
}

static duk_ret_t
js_GetPersonMask(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_sphere_color(ctx, person_get_color(person));
	return 1;
}

static duk_ret_t
js_GetPersonOffsetX(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_int(ctx, person->x_offset);
	return 1;
}

static duk_ret_t
js_GetPersonOffsetY(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_int(ctx, person->y_offset);
	return 1;
}

static duk_ret_t
js_GetPersonList(duk_context* ctx)
{
	int i;

	duk_push_array(ctx);
	for (i = 0; i < s_num_persons; ++i) {
		duk_push_string(ctx, s_persons[i]->name);
		duk_put_prop_index(ctx, -2, i);
	}
	return 1;
}

static duk_ret_t
js_GetPersonSpriteset(duk_context* ctx)
{
	const char*  name;
	spriteset_t* spriteset;
	person_t*    person;

	name = duk_require_string(ctx, 0);

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	spriteset = person_get_spriteset(person);
	duk_push_sphere_spriteset(ctx, spriteset);
	return 1;
}

static duk_ret_t
js_GetPersonSpeedX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*  person;
	double     x_speed;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_speed(person, &x_speed, NULL);
	duk_push_number(ctx, x_speed);
	return 1;
}

static duk_ret_t
js_GetPersonSpeedY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*  person;
	double     y_speed;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_speed(person, NULL, &y_speed);
	duk_push_number(ctx, y_speed);
	return 1;
}

static duk_ret_t
js_GetPersonValue(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	const char* key = duk_to_string(ctx, 1);

	person_t* person;

	duk_require_type_mask(ctx, 1, DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER);
	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "personData");
	if (!duk_get_prop_string(ctx, -1, name)) {
		duk_pop(ctx);
		duk_push_object(ctx);
		duk_put_prop_string(ctx, -2, name);
		duk_get_prop_string(ctx, -1, name);
	}
	duk_get_prop_string(ctx, -1, key);
	duk_remove(ctx, -2); duk_remove(ctx, -2); duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_GetPersonX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;
	double      x, y;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_xy(person, &x, &y, true);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_GetPersonXFloat(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;
	double      x, y;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_xy(person, &x, &y, true);
	duk_push_number(ctx, x);
	return 1;
}

static duk_ret_t
js_GetPersonY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;
	double      x, y;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_xy(person, &x, &y, true);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_GetPersonYFloat(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;
	double      x, y;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_get_xy(person, &x, &y, true);
	duk_push_number(ctx, y);
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
	int player = n_args >= 1 ? duk_to_int(ctx, 0) : 0;

	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "player number out of range (%d)", player);
	duk_push_int(ctx, s_players[player].talk_key);
	return 1;
}

static duk_ret_t
js_GetTalkDistance(duk_context* ctx)
{
	duk_push_int(ctx, s_talk_distance);
	return 1;
}

static duk_ret_t
js_GetTile(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);

	int              layer_w, layer_h;
	struct map_tile* tilemap;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	layer_w = s_map->layers[layer].width;
	layer_h = s_map->layers[layer].height;
	tilemap = s_map->layers[layer].tilemap;
	duk_push_int(ctx, tilemap[x + y * layer_w].tile_index);
	return 1;
}

static duk_ret_t
js_GetTileDelay(duk_context* ctx)
{
	int tile_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index %d", tile_index);
	duk_push_int(ctx, tileset_get_delay(s_map->tileset, tile_index));
	return 1;
}

static duk_ret_t
js_GetTileHeight(duk_context* ctx)
{
	int w, h;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	tileset_get_size(s_map->tileset, &w, &h);
	duk_push_int(ctx, h);
	return 1;
}

static duk_ret_t
js_GetTileImage(duk_context* ctx)
{
	int tile_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index %d", tile_index);
	duk_push_class_obj(ctx, "ssImage", image_ref(tileset_get_image(s_map->tileset, tile_index)));
	return 1;
}

static duk_ret_t
js_GetTileName(duk_context* ctx)
{
	int        tile_index = duk_to_int(ctx, 0);
	
	const lstring_t* tile_name;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index %d", tile_index);
	tile_name = tileset_get_name(s_map->tileset, tile_index);
	duk_push_lstring_t(ctx, tile_name);
	return 1;
}

static duk_ret_t
js_GetTileSurface(duk_context* ctx)
{
	int tile_index = duk_to_int(ctx, 0);

	image_t* image;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index %d", tile_index);
	if ((image = image_clone(tileset_get_image(s_map->tileset, tile_index))) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to create new surface image");
	duk_push_class_obj(ctx, "ssSurface", image);
	return 1;
}

static duk_ret_t
js_GetTileWidth(duk_context* ctx)
{
	int w, h;
	
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	tileset_get_size(s_map->tileset, &w, &h);
	duk_push_int(ctx, w);
	return 1;
}

static duk_ret_t
js_GetTriggerLayer(duk_context* ctx)
{
	int trigger_index = duk_to_int(ctx, 0);

	int layer;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index %d", trigger_index);
	trigger_get_xyz(trigger_index, NULL, NULL, &layer);
	duk_push_int(ctx, layer);
	return 1;
}

static duk_ret_t
js_GetTriggerX(duk_context* ctx)
{
	int trigger_index = duk_to_int(ctx, 0);

	int x;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index %d", trigger_index);
	trigger_get_xyz(trigger_index, &x, NULL, NULL);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_GetTriggerY(duk_context* ctx)
{
	int trigger_index = duk_to_int(ctx, 0);

	int y;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index %d", trigger_index);
	trigger_get_xyz(trigger_index, NULL, &y, NULL);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_GetZoneHeight(duk_context* ctx)
{
	int zone_index = duk_to_int(ctx, 0);

	rect_t bounds;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index %d", zone_index);
	bounds = zone_get_bounds(zone_index);
	duk_push_int(ctx, bounds.y2 - bounds.y1);
	return 1;
}

static duk_ret_t
js_GetZoneLayer(duk_context* ctx)
{
	int zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index %d", zone_index);
	duk_push_int(ctx, zone_get_layer(zone_index));
	return 1;
}

static duk_ret_t
js_GetZoneSteps(duk_context* ctx)
{
	int zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index %d", zone_index);
	duk_push_int(ctx, zone_get_steps(zone_index));
	return 1;
}

static duk_ret_t
js_GetZoneWidth(duk_context* ctx)
{
	int zone_index = duk_to_int(ctx, 0);

	rect_t bounds;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index %d", zone_index);
	bounds = zone_get_bounds(zone_index);
	duk_push_int(ctx, bounds.x2 - bounds.x1);
	return 1;
}

static duk_ret_t
js_GetZoneX(duk_context* ctx)
{
	int zone_index = duk_to_int(ctx, 0);

	rect_t bounds;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index %d", zone_index);
	bounds = zone_get_bounds(zone_index);
	duk_push_int(ctx, bounds.x1);
	return 1;
}

static duk_ret_t
js_GetZoneY(duk_context* ctx)
{
	int zone_index = duk_to_int(ctx, 0);

	rect_t bounds;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index %d", zone_index);
	bounds = zone_get_bounds(zone_index);
	duk_push_int(ctx, bounds.y1);
	return 1;
}

static duk_ret_t
js_IgnorePersonObstructions(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	bool is_ignoring = duk_to_boolean(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person->ignore_all_persons = is_ignoring;
	return 0;
}

static duk_ret_t
js_IgnoreTileObstructions(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	bool is_ignoring = duk_to_boolean(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person->ignore_all_tiles = is_ignoring;
	return 0;
}

static duk_ret_t
js_IsCameraAttached(duk_context* ctx)
{
	duk_push_boolean(ctx, s_camera_person != NULL);
	return 1;
}

static duk_ret_t
js_IsCommandQueueEmpty(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_boolean(ctx, person->num_commands <= 0);
	return 1;
}

static duk_ret_t
js_IsIgnoringPersonObstructions(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_boolean(ctx, person->ignore_all_persons);
	return 1;
}

static duk_ret_t
js_IsIgnoringTileObstructions(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_boolean(ctx, person->ignore_all_tiles);
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
		if (!(person = person_find(name)))
			duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "person `%s` doesn't exist", name);
		player = -1;
		for (i = MAX_PLAYERS - 1; i >= 0; --i)  // ensures Sphere semantics
			player = s_players[i].person == person ? i : player;
		if (player == -1) {
			duk_push_false(ctx);
			return 1;
		}
	}
	else if (duk_is_number(ctx, 0))
		player = duk_get_int(ctx, 0);
	else
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "player number or string expected");
	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "player number out of range (%d)", player);
	duk_push_boolean(ctx, s_players[player].person != NULL);
	return 1;
}

static duk_ret_t
js_IsLayerReflective(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	duk_push_boolean(ctx, s_map->layers[layer].is_reflective);
	return 1;
}

static duk_ret_t
js_IsLayerVisible(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
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
js_IsPersonObstructed(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_boolean(ctx, person_obstructed_at(person, x, y, NULL, NULL));
	return 1;
}

static duk_ret_t
js_IsPersonVisible(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_boolean(ctx, person->is_visible);
	return 1;
}

static duk_ret_t
js_IsTriggerAt(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);

	duk_push_boolean(ctx, get_trigger_at(x, y, layer, NULL) != NULL);
	return 1;
}

static duk_ret_t
js_MapEngine(duk_context* ctx)
{
	const char* filename;
	int         framerate;
	int         num_args;

	num_args = duk_get_top(ctx);
	filename = duk_require_path(ctx, 0, "maps", true, false);
	framerate = num_args >= 2 ? duk_to_int(ctx, 1)
		: g_framerate;

	s_is_map_running = true;
	s_exiting = false;
	s_color_mask = color_new(0, 0, 0, 0);
	s_fade_color_to = s_fade_color_from = s_color_mask;
	s_fade_progress = s_fade_frames = 0;
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	s_framerate = framerate;
	if (!change_map(filename, true))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to load map `%s`", filename);
	while (!s_exiting) {
		update_map_engine(true);
		process_map_input();
		render_map();
		screen_flip(g_screen, s_framerate);
	}
	reset_persons(false);
	s_is_map_running = false;
	return 0;
}

static duk_ret_t
js_MapToScreenX(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	double x = duk_to_int(ctx, 1);

	int offset_x;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	offset_x = 0;
	map_screen_to_map(s_camera_x, s_camera_y, &offset_x, NULL);
	duk_push_int(ctx, x - offset_x);
	return 1;
}

static duk_ret_t
js_MapToScreenY(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	double y = duk_to_int(ctx, 1);

	int offset_y;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	offset_y = 0;
	map_screen_to_map(s_camera_x, s_camera_y, NULL, &offset_y);
	duk_push_int(ctx, y - offset_y);
	return 1;
}

static duk_ret_t
js_QueuePersonCommand(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* name = duk_require_string(ctx, 0);
	int command = duk_require_int(ctx, 1);
	bool is_immediate = n_args >= 3 ? duk_to_boolean(ctx, 2) : false;

	person_t* person;

	if (!(person = person_find(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (command < 0 || command >= COMMAND_RUN_SCRIPT)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid command type constant");
	if (command >= COMMAND_MOVE_NORTH && command <= COMMAND_MOVE_NORTHWEST) {
		if (!person_queue_command(person, COMMAND_ANIMATE, true))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to queue command");
	}
	if (!person_queue_command(person, command, is_immediate))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to queue command");
	return 0;
}

static duk_ret_t
js_QueuePersonScript(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* name = duk_require_string(ctx, 0);
	lstring_t* script_name = lstr_newf("%s/%s/queueScript.js", map_name(), name, s_queued_id++);
	script_t* script = duk_require_sphere_script(ctx, 1, lstr_cstr(script_name));
	bool is_immediate = n_args >= 3 ? duk_to_boolean(ctx, 2) : false;

	person_t* person;

	lstr_free(script_name);
	if (!(person = person_find(name)))
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (!person_queue_script(person, script, is_immediate))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to enqueue script");
	return 0;
}

static duk_ret_t
js_RemoveTrigger(duk_context* ctx)
{
	int trigger_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (trigger_index < 0 || trigger_index >= (int)vector_len(s_map->triggers))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index %d", trigger_index);
	map_remove_trigger(trigger_index);
	return 0;
}

static duk_ret_t
js_RemoveZone(duk_context* ctx)
{
	int zone_index = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index %d", zone_index);
	map_remove_zone(zone_index);
	return 0;
}

static duk_ret_t
js_RenderMap(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	render_map();
	return 0;
}

static duk_ret_t
js_ReplaceTilesOnLayer(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	int old_index = duk_to_int(ctx, 1);
	int new_index = duk_to_int(ctx, 2);

	int              layer_w, layer_h;
	struct map_tile* p_tile;

	int i_x, i_y;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (old_index < 0 || old_index >= tileset_len(s_map->tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "old invalid tile index %d", old_index);
	if (new_index < 0 || new_index >= tileset_len(s_map->tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "new invalid tile index %d", new_index);
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
	int x = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	map_screen_to_map(s_camera_x, s_camera_y, &x, NULL);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_ScreenToMapY(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	int y = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	map_screen_to_map(s_camera_x, s_camera_y, NULL, &y);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_SetCameraX(duk_context* ctx)
{
	int new_x = duk_to_int(ctx, 0);
	
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	s_camera_x = new_x;
	return 0;
}

static duk_ret_t
js_SetCameraY(duk_context* ctx)
{
	int new_y = duk_to_int(ctx, 0);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	s_camera_y = new_y;
	return 0;
}

static duk_ret_t
js_SetColorMask(duk_context* ctx)
{
	color_t new_mask;
	int     num_args;
	int     num_frames;

	num_args = duk_get_top(ctx);
	new_mask = duk_require_sphere_color(ctx, 0);
	num_frames = num_args >= 2 ? duk_to_int(ctx, 1) : 0;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (num_frames < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid number of frames");
	if (num_frames > 0) {
		s_fade_color_to = new_mask;
		s_fade_color_from = s_color_mask;
		s_fade_frames = num_frames;
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
	int script_type = duk_to_int(ctx, 0);
	const char* script_name = (script_type == MAP_SCRIPT_ON_ENTER) ? "synth:onLoadMap.js"
		: (script_type == MAP_SCRIPT_ON_LEAVE) ? "synth:onUnloadMap.js"
		: (script_type == MAP_SCRIPT_ON_LEAVE_NORTH) ? "synth:onLeaveMapNorth.js"
		: (script_type == MAP_SCRIPT_ON_LEAVE_EAST) ? "synth:onLeaveMapEast.js"
		: (script_type == MAP_SCRIPT_ON_LEAVE_SOUTH) ? "synth:onLeaveMapSouth.js"
		: (script_type == MAP_SCRIPT_ON_LEAVE_WEST) ? "synth:onLeaveMapWest.js"
		: NULL;
	script_t* script = duk_require_sphere_script(ctx, 1, script_name);

	if (script_type < 0 || script_type >= MAP_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid map script constant");
	script_free(s_def_map_scripts[script_type]);
	s_def_map_scripts[script_type] = script;
	return 0;
}

static duk_ret_t
js_SetDefaultPersonScript(duk_context* ctx)
{
	int type = duk_require_int(ctx, 0);
	const char* script_name = (type == PERSON_SCRIPT_ON_CREATE) ? "synth:onCreatePerson.js"
		: (type == PERSON_SCRIPT_ON_DESTROY) ? "synth:onDestroyPerson.js"
		: (type == PERSON_SCRIPT_ON_TALK) ? "synth:onTalk.js"
		: (type == PERSON_SCRIPT_ON_TOUCH) ? "synth:onTouchPerson.js"
		: (type == PERSON_SCRIPT_GENERATOR) ? "synth:onGenCommands.js"
		: NULL;
	script_t* script = duk_require_sphere_script(ctx, 1, script_name);

	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid script type constant");
	script_free(s_def_map_scripts[type]);
	s_def_map_scripts[type] = script;
	return 0;
}

static duk_ret_t
js_SetDelayScript(duk_context* ctx)
{
	int frames = duk_to_int(ctx, 0);
	script_t* script = duk_require_sphere_script(ctx, 1, "[delay script]");

	struct delay_script* delay;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (frames < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "frames must be positive");
	if (++s_num_delay_scripts > s_max_delay_scripts) {
		s_max_delay_scripts = s_num_delay_scripts * 2;
		if (!(s_delay_scripts = realloc(s_delay_scripts, s_max_delay_scripts * sizeof(struct delay_script))))
			duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to enlarge delay script queue");
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
	new_height = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (new_height <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "height must be positive and nonzero (got: %d)", new_height);
	if (!map_resize(layer, s_map->layers[layer].width, new_height))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to resize layer %d", layer);
	return 0;
}

static duk_ret_t
js_SetLayerMask(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	color_t mask = duk_require_sphere_color(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	s_map->layers[layer].color_mask = mask;
	return 0;
}

static duk_ret_t
js_SetLayerReflective(duk_context* ctx)
{
	int layer = duk_require_map_layer(ctx, 0);
	bool is_reflective = duk_to_boolean(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	s_map->layers[layer].is_reflective = is_reflective;
	return 0;
}

static duk_ret_t
js_SetLayerRenderer(duk_context* ctx)
{
	int       layer;
	char      script_name[50];
	script_t* script;

	layer = duk_require_map_layer(ctx, 0);
	
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	sprintf(script_name, "[layer %d render script]", layer);
	script = duk_require_sphere_script(ctx, 1, script_name);
	script_free(s_map->layers[layer].render_script);
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
	new_width = duk_to_int(ctx, 1);
	new_height = duk_to_int(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (new_width <= 0 || new_height <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid layer dimensions");
	if (!map_resize(layer, new_width, new_height))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "couldn't resize layer");
	return 0;
}

static duk_ret_t
js_SetLayerVisible(duk_context* ctx)
{
	bool is_visible;
	int  layer;

	layer = duk_require_map_layer(ctx, 0);
	is_visible = duk_to_boolean(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	s_map->layers[layer].is_visible = is_visible;
	return 0;
}

static duk_ret_t
js_SetLayerWidth(duk_context* ctx)
{
	int layer;
	int new_width;

	layer = duk_require_map_layer(ctx, 0);
	new_width = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (new_width <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "width must be positive and nonzero (got: %d)", new_width);
	if (!map_resize(layer, new_width, s_map->layers[layer].height))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "unable to resize layer %d", layer);
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
	int tile_index = duk_to_int(ctx, 0);
	int next_index = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (tile_index < 0 || tile_index >= tileset_len(s_map->tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index %d", tile_index);
	if (next_index < 0 || next_index >= tileset_len(s_map->tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index for next tile (%d)", tile_index);
	tileset_set_next(s_map->tileset, tile_index, next_index);
	return 0;
}

static duk_ret_t
js_SetPersonAngle(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	double theta = duk_to_number(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_angle(person, theta);
	return 0;
}

static duk_ret_t
js_SetPersonData(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	duk_require_object_coercible(ctx, 1);
	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "personData");
	duk_dup(ctx, 1); duk_put_prop_string(ctx, -2, name);
	return 0;
}

static duk_ret_t
js_SetPersonDirection(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	const char* new_dir = duk_require_string(ctx, 1);

	person_t*   person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	set_person_direction(person, new_dir);
	return 0;
}

static duk_ret_t
js_SetPersonFollowDistance(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int distance = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (person->leader == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "person has no leader");
	if (distance <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid distance");
	enlarge_step_history(person->leader, distance);
	person->follow_distance = distance;
	return 0;
}

static duk_ret_t
js_SetPersonFrame(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int frame_index = duk_require_int(ctx, 1);

	int       num_frames;
	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	num_frames = spriteset_num_frames(person->sprite, person->direction);
	person->frame = (frame_index % num_frames + num_frames) % num_frames;
	person->anim_frames = spriteset_frame_delay(person->sprite, person->direction, person->frame);
	person->revert_frames = person->revert_delay;
	return 0;
}

static duk_ret_t
js_SetPersonFrameNext(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int frames = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (frames < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid delay value");
	person->anim_frames = frames;
	person->revert_frames = person->revert_delay;
	return 0;
}

static duk_ret_t
js_SetPersonFrameRevert(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int frames = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (frames < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid delay value");
	person->revert_delay = frames;
	person->revert_frames = person->revert_delay;
	return 0;
}

static duk_ret_t
js_SetPersonIgnoreList(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	size_t    list_size;
	person_t* person;

	int i;

	duk_require_object_coercible(ctx, 1);
	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (!duk_is_array(ctx, 1))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "ignore_list argument must be an array");
	list_size = duk_get_length(ctx, 1);
	if (list_size > INT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "list is too large");
	for (i = 0; i < person->num_ignores; ++i) {
		free(person->ignores[i]);
	}
	person->ignores = realloc(person->ignores, list_size * sizeof(char*));
	person->num_ignores = (int)list_size;
	for (i = 0; i < (int)list_size; ++i) {
		duk_get_prop_index(ctx, 1, (duk_uarridx_t)i);
		person->ignores[i] = strdup(duk_require_string(ctx, -1));
		duk_pop(ctx);
	}
	return 0;
}

static duk_ret_t
js_SetPersonLayer(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int layer = duk_require_map_layer(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person->layer = layer;
	return 0;
}

static duk_ret_t
js_SetPersonMask(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	color_t mask = duk_require_sphere_color(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_color(person, mask);
	return 0;
}

static duk_ret_t
js_SetPersonOffsetX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int offset = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person->x_offset = offset;
	return 0;
}

static duk_ret_t
js_SetPersonOffsetY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int offset = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person->y_offset = offset;
	return 0;
}

static duk_ret_t
js_SetPersonScaleAbsolute(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int width = duk_require_int(ctx, 1);
	int height = duk_require_int(ctx, 2);

	person_t*    person;
	int          sprite_h;
	int          sprite_w;
	spriteset_t* spriteset;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (width < 0 || height < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid scale dimensions");
	spriteset = person_get_spriteset(person);
	sprite_w = spriteset_width(spriteset);
	sprite_h = spriteset_height(spriteset);
	person_set_scale(person, width / sprite_w, height / sprite_h);
	return 0;
}

static duk_ret_t
js_SetPersonScaleFactor(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	double scale_x = duk_to_number(ctx, 1);
	double scale_y = duk_to_number(ctx, 2);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (scale_x < 0.0 || scale_y < 0.0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "scale must be positive (got X: %f, Y: %f })", scale_x, scale_y);
	person_set_scale(person, scale_x, scale_y);
	return 0;
}

static duk_ret_t
js_SetPersonScript(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int type = duk_require_int(ctx, 1);

	lstring_t* codestring;
	person_t*  person;
	script_t*  script;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "invalid script type constant");
	if (duk_is_string(ctx, 2)) {
		codestring = duk_require_lstring_t(ctx, 2);
		person_compile_script(person, type, codestring);
		lstr_free(codestring);
	}
	else {
		script = duk_require_sphere_script(ctx, 2, "[person script]");
		person_set_script(person, type, script);
	}
	return 0;
}

static duk_ret_t
js_SetPersonSpeed(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	double speed = duk_to_number(ctx, 1);

	person_t*  person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_speed(person, speed, speed);
	return 0;
}

static duk_ret_t
js_SetPersonSpeedXY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	double x_speed = duk_to_number(ctx, 1);
	double y_speed = duk_to_number(ctx, 2);

	person_t*  person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person_set_speed(person, x_speed, y_speed);
	return 0;
}

static duk_ret_t
js_SetPersonSpriteset(duk_context* ctx)
{
	const char*  name;
	person_t*    person;
	spriteset_t* spriteset;

	name = duk_require_string(ctx, 0);
	spriteset = duk_require_sphere_spriteset(ctx, 1);

	if ((person = person_find(name)) == NULL) {
		spriteset_free(spriteset);
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	}
	person_set_spriteset(person, spriteset);
	spriteset_free(spriteset);
	return 0;
}

static duk_ret_t
js_SetPersonValue(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	const char* key = duk_to_string(ctx, 1);

	person_t* person;

	duk_require_valid_index(ctx, 2);
	duk_require_type_mask(ctx, 1, DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER);
	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "personData");
	if (!duk_get_prop_string(ctx, -1, name)) {
		duk_pop(ctx);
		duk_push_object(ctx);
		duk_put_prop_string(ctx, -2, name);
		duk_get_prop_string(ctx, -1, name);
	}
	duk_dup(ctx, 2); duk_put_prop_string(ctx, -2, key);
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
js_SetPersonVisible(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	bool is_visible = duk_to_boolean(ctx, 1);

	person_t* person;

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person->is_visible = is_visible;
	return 0;
}

static duk_ret_t
js_SetPersonX(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	int         x;

	name = duk_require_string(ctx, 0);
	x = duk_to_int(ctx, 1);

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person->x = x;
	return 0;
}

static duk_ret_t
js_SetPersonXYFloat(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	double      x;
	double      y;

	name = duk_require_string(ctx, 0);
	x = duk_to_number(ctx, 1);
	y = duk_to_number(ctx, 2);

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person->x = x;
	person->y = y;
	return 0;
}

static duk_ret_t
js_SetPersonY(duk_context* ctx)
{
	const char* name;
	person_t*   person;
	int         y;

	name = duk_require_string(ctx, 0);
	y = duk_to_int(ctx, 1);

	if ((person = person_find(name)) == NULL)
		duk_error_blame(ctx, -1, DUK_ERR_REFERENCE_ERROR, "no such person `%s`", name);
	person->y = y;
	return 0;
}

static duk_ret_t
js_SetRenderScript(duk_context* ctx)
{
	script_t* script;
	
	script = duk_require_sphere_script(ctx, 0, "default/renderScript");

	script_free(s_render_script);
	s_render_script = script;
	return 0;
}

static duk_ret_t
js_SetTalkActivationButton(duk_context* ctx)
{
	int button = duk_to_int(ctx, 0);

	s_talk_button = button;
	return 0;
}

static duk_ret_t
js_SetTalkActivationKey(duk_context* ctx)
{
	int key;
	int num_args;
	int player = 0;
	
	num_args = duk_get_top(ctx);
	key = duk_to_int(ctx, 0);
	if (num_args >= 2)
		player = duk_to_int(ctx, 1);

	if (player < 0 || player >= MAX_PLAYERS)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "player number out of range");
	
	s_players[player].talk_key = key;
	return 0;
}

static duk_ret_t
js_SetTalkDistance(duk_context* ctx)
{
	int pixels;
	
	pixels = duk_to_int(ctx, 0);

	if (pixels < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid talk distance");
	
	s_talk_distance = pixels;
	return 0;
}
static duk_ret_t
js_SetTile(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int layer = duk_require_map_layer(ctx, 2);
	int tile_index = duk_to_int(ctx, 3);

	int              layer_w, layer_h;
	struct map_tile* tilemap;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
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
	int        delay;
	int        tile_index;
	tileset_t* tileset;
	
	tile_index = duk_to_int(ctx, 0);
	delay = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	if (delay < 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid frame count");
	tileset_set_delay(tileset, tile_index, delay);
	return 0;
}

static duk_ret_t
js_SetTileImage(duk_context* ctx)
{
	image_t*   image;
	int        image_h;
	int        image_w;
	int        tile_h;
	int        tile_index;
	int        tile_w;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);
	image = duk_require_class_obj(ctx, 1, "ssImage");

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	tileset_get_size(tileset, &tile_w, &tile_h);
	image_w = image_width(image);
	image_h = image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "image/tile size mismatch");
	tileset_set_image(tileset, tile_index, image);
	return 0;
}

static duk_ret_t
js_SetTileName(duk_context* ctx)
{
	lstring_t* name;
	int        tile_index;
	tileset_t* tileset;
	
	tile_index = duk_to_int(ctx, 0);
	name = duk_require_lstring_t(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	tileset_set_name(tileset, tile_index, name);
	lstr_free(name);
	return 0;
}

static duk_ret_t
js_SetTileSurface(duk_context* ctx)
{
	image_t*   image;
	int        image_h;
	int        image_w;
	int        tile_h;
	int        tile_index;
	int        tile_w;
	tileset_t* tileset;

	tile_index = duk_to_int(ctx, 0);
	image = duk_require_class_obj(ctx, 1, "ssSurface");

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	tileset = map_tileset();
	if (tile_index < 0 || tile_index >= tileset_len(tileset))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid tile index");
	tileset_get_size(tileset, &tile_w, &tile_h);
	image_w = image_width(image);
	image_h = image_height(image);
	if (image_w != tile_w || image_h != tile_h)
		duk_error_blame(ctx, -1, DUK_ERR_TYPE_ERROR, "surface/tile size mismatch");
	tileset_set_image(tileset, tile_index, image);
	return 0;
}

static duk_ret_t
js_SetTriggerLayer(duk_context* ctx)
{
	int layer;
	int trigger_index;

	trigger_index = duk_to_int(ctx, 0);
	layer = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index");
	trigger_set_layer(trigger_index, layer);
	return 0;
}

static duk_ret_t
js_SetTriggerScript(duk_context* ctx)
{
	script_t*  script;
	lstring_t* script_name;
	int        trigger_index;

	trigger_index = duk_to_int(ctx, 0);
	
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index");
	script_name = lstr_newf("%s/trigger~%d/onStep", map_name(), trigger_index);
	script = duk_require_sphere_script(ctx, 1, lstr_cstr(script_name));
	trigger_set_script(trigger_index, script);
	script_free(script);
	lstr_free(script_name);
	return 0;
}

static duk_ret_t
js_SetTriggerXY(duk_context* ctx)
{
	int trigger_index;
	int x;
	int y;

	trigger_index = duk_to_int(ctx, 0);
	x = duk_to_int(ctx, 1);
	y = duk_to_int(ctx, 2);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (trigger_index < 0 || trigger_index >= map_num_triggers())
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid trigger index");
	trigger_set_xy(trigger_index, x, y);
	return 0;
}

static duk_ret_t
js_SetUpdateScript(duk_context* ctx)
{
	script_t* script = duk_require_sphere_script(ctx, 0, "default/updateScript");

	script_free(s_update_script);
	s_update_script = script;
	return 0;
}

static duk_ret_t
js_SetZoneLayer(duk_context* ctx)
{
	int zone_index = duk_to_int(ctx, 0);
	int layer = duk_require_map_layer(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index %d", zone_index);
	zone_set_layer(zone_index, layer);
	return 0;
}

static duk_ret_t
js_SetZoneMetrics(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int zone_index = duk_to_int(ctx, 0);
	int x = duk_to_int(ctx, 1);
	int y = duk_to_int(ctx, 2);
	int width = duk_to_int(ctx, 3);
	int height = duk_to_int(ctx, 4);
	int layer = n_args >= 6 ? duk_require_map_layer(ctx, 5) : -1;

	rect_t bounds;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index");
	bounds = map_bounds();
	if (width <= 0 || height <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone size");
	if (x < bounds.x1 || y < bounds.y1 || x + width > bounds.x2 || y + height > bounds.y2)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "zone is out of bounds");
	zone_set_bounds(zone_index, new_rect(x, y, x + width, y + height));
	if (layer >= 0)
		zone_set_layer(zone_index, layer);
	return 0;
}

static duk_ret_t
js_SetZoneScript(duk_context* ctx)
{
	int zone_index = duk_to_int(ctx, 0);
	script_t* script;
	
	lstring_t* script_name;

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index `%d`", zone_index);
	if (!(script_name = lstr_newf("%s/zone%d", map_name(), zone_index)))
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "error compiling zone script");
	script = duk_require_sphere_script(ctx, 1, lstr_cstr(script_name));
	lstr_free(script_name);
	zone_set_script(zone_index, script);
	script_free(script);
	return 0;
}

static duk_ret_t
js_SetZoneSteps(duk_context* ctx)
{
	int zone_index = duk_to_int(ctx, 0);
	int steps = duk_to_int(ctx, 1);

	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	if (zone_index < 0 || zone_index >= (int)vector_len(s_map->zones))
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid zone index `%d`", zone_index);
	if (steps <= 0)
		duk_error_blame(ctx, -1, DUK_ERR_RANGE_ERROR, "steps must be positive (got: %d)", steps);
	zone_set_steps(zone_index, steps);
	return 0;
}

static duk_ret_t
js_UpdateMapEngine(duk_context* ctx)
{
	if (!map_engine_running())
		duk_error_blame(ctx, -1, DUK_ERR_ERROR, "map engine not running");
	update_map_engine(false);
	return 0;
}
