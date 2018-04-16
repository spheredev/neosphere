/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2018, Fat Cerberus
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
 *  * Neither the name of miniSphere nor the names of its contributors may be
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

#include "minisphere.h"
#include "map_engine.h"

#include "api.h"
#include "audio.h"
#include "color.h"
#include "dispatch.h"
#include "geometry.h"
#include "image.h"
#include "input.h"
#include "jsal.h"
#include "obstruction.h"
#include "script.h"
#include "spriteset.h"
#include "tileset.h"
#include "vanilla.h"
#include "vector.h"

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
static int                 s_frame_rate = 0;
static unsigned int        s_frames = 0;
static bool                s_is_map_running = false;
static lstring_t*          s_last_bgm_file = NULL;
static struct map*         s_map = NULL;
static sound_t*            s_map_bgm_stream = NULL;
static char*               s_map_filename = NULL;
static int                 s_max_deferreds = 0;
static int                 s_max_persons = 0;
static unsigned int        s_next_person_id = 0;
static int                 s_num_deferreds = 0;
static int                 s_num_persons = 0;
static struct map_trigger* s_on_trigger = NULL;
static unsigned int        s_queued_id = 0;
static vector_t*           s_person_list = NULL;
static struct player*      s_players;
static script_t*           s_render_script = NULL;
static int                 s_talk_button = 0;
static int                 s_talk_distance = 8;
static script_t*           s_update_script = NULL;
static struct deferred     *s_deferreds = NULL;
static person_t*           *s_persons = NULL;

struct deferred
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
	vector_t*       ignore_list;
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
static void                detach_person        (const person_t* person);
static bool                does_person_exist    (const person_t* person);
static void                draw_persons         (int layer, bool is_flipped, int cam_x, int cam_y);
static bool                enlarge_step_history (person_t* person, int new_size);
static void                free_map             (struct map* map);
static void                free_person          (person_t* person);
static struct map_trigger* get_trigger_at       (int x, int y, int layer, int* out_index);
static struct map_zone*    get_zone_at          (int x, int y, int layer, int which, int* out_index);
static struct map*         load_map             (const char* path);
static void                map_screen_to_layer  (int layer, int camera_x, int camera_y, int* inout_x, int* inout_y);
static void                map_screen_to_map    (int camera_x, int camera_y, int* inout_x, int* inout_y);
static void                process_map_input    (void);
static void                record_step          (person_t* person);
static void                reset_persons        (bool keep_existing);
static void                set_person_name      (person_t* person, const char* name);
static void                sort_persons         (void);
static void                update_map_engine    (bool is_main_loop);
static void                update_person        (person_t* person, bool* out_has_moved);

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
	s_players = calloc(PLAYER_MAX, sizeof(struct player));
	for (i = 0; i < PLAYER_MAX; ++i)
		s_players[i].is_talk_allowed = true;
	s_current_trigger = -1;
	s_current_zone = -1;
	s_render_script = NULL;
	s_update_script = NULL;
	s_num_deferreds = s_max_deferreds = 0;
	s_deferreds = NULL;
	s_talk_button = 0;
	s_is_map_running = false;
	s_color_mask = mk_color(0, 0, 0, 0);
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

	vector_free(s_person_list);

	for (i = 0; i < s_num_deferreds; ++i)
		script_unref(s_deferreds[i].script);
	free(s_deferreds);
	for (i = 0; i < MAP_SCRIPT_MAX; ++i)
		script_unref(s_def_map_scripts[i]);
	script_unref(s_update_script);
	script_unref(s_render_script);
	free_map(s_map);
	free(s_players);

	for (i = 0; i < s_num_persons; ++i)
		free_person(s_persons[i]);
	for (i = 0; i < PERSON_SCRIPT_MAX; ++i)
		script_unref(s_def_person_scripts[i]);
	free(s_persons);

	mixer_unref(s_bgm_mixer);

	audio_uninit();
}

void
map_engine_on_map_event(map_op_t op, script_t* script)
{
	script_t* old_script;

	old_script = s_def_map_scripts[op];
	s_def_map_scripts[op] = script_ref(script);
	script_unref(old_script);
}

void
map_engine_on_person_event(person_op_t op, script_t* script)
{
	script_t* old_script;

	old_script = s_def_person_scripts[op];
	s_def_person_scripts[op] = script_ref(script);
	script_unref(old_script);
}

void
map_engine_on_render(script_t* script)
{
	script_unref(s_render_script);
	s_render_script = script_ref(script);
}

void
map_engine_on_update(script_t* script)
{
	script_unref(s_update_script);
	s_update_script = script_ref(script);
}

const person_t*
map_engine_acting_person(void)
{
	return s_acting_person;
}

const person_t*
map_engine_active_person(void)
{
	return s_current_person;
}

int
map_engine_active_trigger(void)
{
	return s_current_trigger;
}

int
map_engine_active_zone(void)
{
	return s_current_zone;
}

vector_t*
map_engine_persons(void)
{
	int i;

	if (s_person_list == NULL)
		s_person_list = vector_new(sizeof(person_t*));
	vector_clear(s_person_list);
	for (i = 0; i < s_num_persons; ++i)
		vector_push(s_person_list, &s_persons[i]);
	return s_person_list;
}

bool
map_engine_running(void)
{
	return s_is_map_running;
}

int
map_engine_get_framerate(void)
{
	return s_frame_rate;
}

person_t*
map_engine_get_player(player_id_t player_id)
{
	return s_players[player_id].person;
}

person_t*
map_engine_get_subject(void)
{
	return s_camera_person;
}

int
map_engine_get_talk_button(void)
{
	return s_talk_button;
}

int
map_engine_get_talk_distance(void)
{
	return s_talk_distance;
}

int
map_engine_get_talk_key(player_id_t player_id)
{
	return s_players[player_id].talk_key;
}

void
map_engine_set_framerate(int framerate)
{
	s_frame_rate = framerate;
}

void
map_engine_set_player(player_id_t player_id, person_t* person)
{
	int i;

	// detach person from any other players
	for (i = 0; i < PLAYER_MAX; ++i) {
		if (s_players[i].person == person)
			s_players[i].person = NULL;
	}

	s_players[player_id].person = person;
}

void
map_engine_set_subject(person_t* person)
{
	s_camera_person = person;
}

void
map_engine_set_talk_button(int button_id)
{
	s_talk_button = button_id;
}

void
map_engine_set_talk_distance(int distance)
{
	s_talk_distance = distance;
}

void
map_engine_set_talk_key(player_id_t player_id, int key)
{
	s_players[player_id].talk_key = key;
}

bool
map_engine_change_map(const char* filename)
{
	return change_map(filename, false);
}

void
map_engine_defer(script_t* script, int num_frames)
{
	struct deferred* deferred;

	if (++s_num_deferreds > s_max_deferreds) {
		s_max_deferreds = s_num_deferreds * 2;
		s_deferreds = realloc(s_deferreds, s_max_deferreds * sizeof(struct deferred));
	}
	deferred = &s_deferreds[s_num_deferreds - 1];
	deferred->script = script;
	deferred->frames_left = num_frames;
}

void
map_engine_draw_map(void)
{
	bool              is_repeating;
	int               cell_x;
	int               cell_y;
	int               first_cell_x;
	int               first_cell_y;
	struct map_layer* layer;
	int               layer_height;
	int               layer_width;
	size2_t           resolution;
	int               tile_height;
	int               tile_index;
	int               tile_width;
	int               off_x;
	int               off_y;

	int x, y, z;

	if (screen_skipping_frame(g_screen))
		return;

	resolution = screen_size(g_screen);
	tileset_get_size(s_map->tileset, &tile_width, &tile_height);

	// render map layers from bottom to top (+Z = up)
	for (z = 0; z < s_map->num_layers; ++z) {
		layer = &s_map->layers[z];
		is_repeating = s_map->is_repeating || layer->is_parallax;
		layer_width = layer->width * tile_width;
		layer_height = layer->height * tile_height;
		off_x = 0;
		off_y = 0;
		map_screen_to_layer(z, s_camera_x, s_camera_y, &off_x, &off_y);

		// render person reflections if layer is reflective
		al_hold_bitmap_drawing(true);
		if (layer->is_reflective) {
			if (is_repeating) {  // for small repeating maps, persons need to be repeated as well
				for (y = 0; y < resolution.height / layer_height + 2; ++y) for (x = 0; x < resolution.width / layer_width + 2; ++x)
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
			for (y = 0; y < resolution.height / tile_height + 2; ++y) for (x = 0; x < resolution.width / tile_width + 2; ++x) {
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
			for (y = 0; y < resolution.height / layer_height + 2; ++y) for (x = 0; x < resolution.width / layer_width + 2; ++x)
				draw_persons(z, false, off_x - x * layer_width, off_y - y * layer_height);
		}
		else {
			draw_persons(z, false, off_x, off_y);
		}
		al_hold_bitmap_drawing(false);

		script_run(layer->render_script, false);
	}

	al_draw_filled_rectangle(0, 0, resolution.width, resolution.height, nativecolor(s_color_mask));
	script_run(s_render_script, false);
}

void
map_engine_exit(void)
{
	s_exiting = true;
}

void
map_engine_fade_to(color_t color_mask, int num_frames)
{
	if (num_frames > 0) {
		s_fade_color_to = color_mask;
		s_fade_color_from = s_color_mask;
		s_fade_frames = num_frames;
		s_fade_progress = 0;
	}
	else {
		s_color_mask = color_mask;
		s_fade_color_to = s_fade_color_from = color_mask;
		s_fade_progress = s_fade_frames = 0;
	}
}

bool
map_engine_start(const char* filename, int framerate)
{
	s_is_map_running = true;
	s_exiting = false;
	s_color_mask = mk_color(0, 0, 0, 0);
	s_fade_color_to = s_fade_color_from = s_color_mask;
	s_fade_progress = s_fade_frames = 0;
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	s_frame_rate = framerate;
	if (!change_map(filename, true))
		goto on_error;
	while (!s_exiting && jsal_vm_enabled()) {
		sphere_heartbeat(true, 1);

		// order of operations matches Sphere 1.x.  not sure why, but Sphere 1.x
		// checks for input AFTER an update for some reason...
		update_map_engine(true);
		process_map_input();
		map_engine_draw_map();

		// don't clear the backbuffer.  the Sphere 1.x map engine has a bug where it doesn't
		// clear the backbuffer between frames; as it turns out, a good deal of of v1 code relies
		// on that behavior.
		sphere_tick(1, false, s_frame_rate);
	}
	reset_persons(false);
	s_is_map_running = false;
	return true;

on_error:
	s_is_map_running = false;
	return false;
}

void
map_engine_update(void)
{
	update_map_engine(false);
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

int
map_layer_by_name(const char* name)
{
	int i;

	for (i = 0; i < s_map->num_layers; ++i) {
		if (strcmp(name, lstr_cstr(s_map->layers[0].name)) == 0)
			return i;
	}
	return -1;
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

point3_t
map_origin(void)
{
	return s_map != NULL ? s_map->origin
		: mk_point3(0, 0, 0);
}

const char*
map_pathname(void)
{
	return s_map ? s_map_filename : NULL;
}

person_t*
map_person_by_name(const char* name)
{
	int i;

	for (i = 0; i < s_num_persons; ++i) {
		if (strcmp(name, s_persons[i]->name) == 0)
			return s_persons[i];
	}
	return NULL;
}

int
map_tile_at(int x, int y, int layer)
{
	int layer_h;
	int layer_w;

	layer_w = s_map->layers[layer].width;
	layer_h = s_map->layers[layer].height;

	if (s_map->is_repeating || s_map->layers[layer].is_parallax) {
		x = (x % layer_w + layer_w) % layer_w;
		y = (y % layer_h + layer_h) % layer_h;
	}
	if (x < 0 || y < 0 || x >= layer_w || y >= layer_h)
		return -1;
	return layer_get_tile(layer, x, y);
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
	while ((trigger = iter_next(&iter))) {
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

point2_t
map_xy_from_screen(point2_t screen_xy)
{
	int x;
	int y;

	x = screen_xy.x;
	y = screen_xy.y;
	map_screen_to_map(s_camera_x, s_camera_y, &x, &y);
	return mk_point2(x, y);
}

int
map_zone_at(int x, int y, int layer, int which)
{
	struct map_zone* zone;

	iter_t iter;

	iter = vector_enum(s_map->zones);
	while ((zone = iter_next(&iter))) {
		if (zone->layer != layer && false)  // layer ignored for compatibility
			continue;
		if (is_point_in_rect(x, y, zone->bounds) && --which < 0)
			return iter.index;
	}
	return -1;
}

point2_t
map_get_camera_xy(void)
{
	return mk_point2(s_camera_x, s_camera_y);
}

void
map_set_camera_xy(point2_t where)
{
	s_camera_x = where.x;
	s_camera_y = where.y;
}

void
map_activate(map_op_t op, bool use_default)
{
	if (use_default)
		script_run(s_def_map_scripts[op], false);
	script_run(s_map->scripts[op], false);
}

bool
map_add_trigger(int x, int y, int layer, script_t* script)
{
	struct map_trigger trigger;

	console_log(2, "creating trigger #%d on map '%s'", vector_len(s_map->triggers), s_map_filename);
	console_log(3, "    location: '%s' @ (%d,%d)", lstr_cstr(s_map->layers[layer].name), x, y);

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

	console_log(2, "creating %u-step zone #%d on map '%s'", steps, vector_len(s_map->zones), s_map_filename);
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

void
map_call_default(map_op_t op)
{
	script_run(s_def_map_scripts[op], false);
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

void
layer_on_render(int layer, script_t* script)
{
	script_unref(s_map->layers[layer].render_script);
	s_map->layers[layer].render_script = script_ref(script);
}

const char*
layer_name(int layer)
{
	return lstr_cstr(s_map->layers[layer].name);
}

const obsmap_t*
layer_obsmap(int layer)
{
	return s_map->layers[layer].obsmap;
}

size2_t
layer_size(int layer)
{
	struct map_layer* layer_data;

	layer_data = &s_map->layers[layer];
	return mk_size2(layer_data->width, layer_data->height);
}

color_t
layer_get_color_mask(int layer)
{
	return s_map->layers[layer].color_mask;
}

bool
layer_get_reflective(int layer)
{
	return s_map->layers[layer].is_reflective;
}

int
layer_get_tile(int layer, int x, int y)
{
	struct map_tile* tile;
	int              width;

	width = s_map->layers[layer].width;
	tile = &s_map->layers[layer].tilemap[x + y * width];
	return tile->tile_index;
}

bool
layer_get_visible(int layer)
{
	return s_map->layers[layer].is_visible;
}

void
layer_set_color_mask(int layer, color_t color)
{
	s_map->layers[layer].color_mask = color;
}

void
layer_set_reflective(int layer, bool reflective)
{
	s_map->layers[layer].is_reflective = reflective;
}

void
layer_set_tile(int layer, int x, int y, int tile_index)
{
	struct map_tile* tile;
	int              width;

	width = s_map->layers[layer].width;
	tile = &s_map->layers[layer].tilemap[x + y * width];
	tile->tile_index = tile_index;
	tile->frames_left = tileset_get_delay(s_map->tileset, tile_index);
}

void
layer_set_visible(int layer, bool visible)
{
	s_map->layers[layer].is_visible = visible;
}

void
layer_replace_tiles(int layer, int old_index, int new_index)
{
	int              layer_h;
	int              layer_w;
	struct map_tile* tile;

	int i_x, i_y;

	layer_w = s_map->layers[layer].width;
	layer_h = s_map->layers[layer].height;
	for (i_x = 0; i_x < layer_w; ++i_x) for (i_y = 0; i_y < layer_h; ++i_y) {
		tile = &s_map->layers[layer].tilemap[i_x + i_y * layer_w];
		if (tile->tile_index == old_index)
			tile->tile_index = new_index;
	}
}

bool
layer_resize(int layer, int x_size, int y_size)
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
			if (x < old_width && y < old_height) {
				tilemap[x + y * x_size] = s_map->layers[layer].tilemap[x + y * old_width];
			}
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
	person_set_pose(person, spriteset_pose_name(spriteset, 0));
	person->is_persistent = is_persistent;
	person->is_visible = true;
	person->x = origin.x;
	person->y = origin.y;
	person->layer = origin.z;
	person->speed_x = 1.0;
	person->speed_y = 1.0;
	person->anim_frames = spriteset_frame_delay(person->sprite, person->direction, 0);
	person->mask = mk_color(255, 255, 255, 255);
	person->scale_x = person->scale_y = 1.0;
	person->scripts[PERSON_SCRIPT_ON_CREATE] = create_script;
	person_activate(person, PERSON_SCRIPT_ON_CREATE, NULL, true);
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
	person_activate(person, PERSON_SCRIPT_ON_DESTROY, NULL, true);
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->leader == person)
			s_persons[i]->leader = NULL;
	}

	// remove the person from the engine
	detach_person(person);
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i] == person) {
			for (j = i; j < s_num_persons - 1; ++j)
				s_persons[j] = s_persons[j + 1];
			--s_num_persons;
			--i;
		}
	}

	vector_free(person->ignore_list);

	free_person(person);
	sort_persons();
}

rect_t
person_base(const person_t* person)
{
	rect_t base_rect;
	int    base_x;
	int    base_y;
	double x;
	double y;

	base_rect = rect_zoom(spriteset_get_base(person->sprite), person->scale_x, person->scale_y);
	person_get_xy(person, &x, &y, true);
	base_x = x - (base_rect.x1 + (base_rect.x2 - base_rect.x1) / 2);
	base_y = y - (base_rect.y1 + (base_rect.y2 - base_rect.y1) / 2);
	base_rect.x1 += base_x; base_rect.x2 += base_x;
	base_rect.y1 += base_y; base_rect.y2 += base_y;
	return base_rect;
}

bool
person_following(const person_t* person, const person_t* leader)
{
	const person_t* node;

	node = person;
	while ((node = node->leader))
		if (node == leader) return true;
	return false;
}

bool
person_has_moved(const person_t* person)
{
	return person->mv_x != 0 || person->mv_y != 0;
}

vector_t*
person_ignore_list(person_t* person)
{
	// note: the returned vector is an array of C strings.  these should be treated
	//       as const char*; in other words, don't free them!

	int i;

	if (person->ignore_list == NULL)
		person->ignore_list = vector_new(sizeof(const char*));
	vector_clear(person->ignore_list);
	for (i = 0; i < person->num_ignores; ++i)
		vector_push(person->ignore_list, &person->ignores[i]);
	return person->ignore_list;
}

bool
person_ignored_by(const person_t* person, const person_t* other)
{
	// note: commutative; if either person ignores the other, the function will return true

	int i;

	if (other->ignore_all_persons || person->ignore_all_persons)
		return true;
	for (i = 0; i < other->num_ignores; ++i)
		if (strcmp(other->ignores[i], person->name) == 0) return true;
	for (i = 0; i < person->num_ignores; ++i)
		if (strcmp(person->ignores[i], other->name) == 0) return true;
	return false;
}

bool
person_moving(const person_t* person)
{
	return person->num_commands > 0;
}

const char*
person_name(const person_t* person)
{
	return person != NULL ? person->name : "";
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
	my_base = rect_translate(person_base(person), x - cur_x, y - cur_y);
	if (out_obstructing_person != NULL)
		*out_obstructing_person = NULL;
	if (out_tile_index != NULL)
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
			base = person_base(s_persons[i]);
			if (do_rects_overlap(my_base, base) && !person_ignored_by(person, s_persons[i])) {
				is_obstructed = true;
				if (out_obstructing_person)
					*out_obstructing_person = s_persons[i];
				break;
			}
		}
	}

	// no obstructing person, check map-defined obstructions
	obsmap = layer_obsmap(layer);
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
			base = rect_translate(my_base, -(i_x * tile_w), -(i_y * tile_h));
			obsmap = tileset_obsmap(tileset, map_tile_at(i_x, i_y, layer));
			if (obsmap != NULL && obsmap_test_rect(obsmap, base)) {
				is_obstructed = true;
				if (out_tile_index)
					*out_tile_index = map_tile_at(i_x, i_y, layer);
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

color_t
person_get_color(const person_t* person)
{
	return person->mask;
}

int
person_get_frame(const person_t* person)
{
	int num_frames;

	num_frames = spriteset_num_frames(person->sprite, person->direction);
	return person->frame % num_frames;
}

int
person_get_frame_delay(const person_t* person)
{
	return person->anim_frames;
}

bool
person_get_ignore_persons(const person_t* person)
{
	return person->ignore_all_persons;
}

bool
person_get_ignore_tiles(const person_t* person)
{
	return person->ignore_all_tiles;
}

int
person_get_layer(const person_t* person)
{
	return person->layer;
}

person_t*
person_get_leader(const person_t* person)
{
	return person->leader;
}

point2_t
person_get_offset(const person_t* person)
{
	return mk_point2(person->x_offset, person->y_offset);
}

const char*
person_get_pose(const person_t* person)
{
	return person->direction;
}

int
person_get_revert_delay(const person_t* person)
{
	return person->revert_delay;
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
person_get_spriteset(const person_t* person)
{
	return person->sprite;
}

int
person_get_trailing(const person_t* person)
{
	return person->follow_distance;
}

bool
person_get_visible(const person_t* person)
{
	return person->is_visible;
}

void
person_get_xy(const person_t* person, double* out_x, double* out_y, bool normalize)
{
	*out_x = person->x;
	*out_y = person->y;
	if (normalize)
		map_normalize_xy(out_x, out_y, person->layer);
}

void
person_get_xyz(const person_t* person, double* out_x, double* out_y, int* out_layer, bool normalize)
{
	*out_x = person->x;
	*out_y = person->y;
	*out_layer = person->layer;
	if (normalize)
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
person_set_frame(person_t* person, int frame_index)
{
	int num_frames;

	num_frames = spriteset_num_frames(person->sprite, person->direction);
	person->frame = (frame_index % num_frames + num_frames) % num_frames;
	person->anim_frames = spriteset_frame_delay(person->sprite, person->direction, person->frame);
	person->revert_frames = person->revert_delay;
}

void
person_set_frame_delay(person_t* person, int num_frames)
{
	person->anim_frames = num_frames;
	person->revert_frames = person->revert_delay;
}

void
person_set_ignore_persons(person_t* person, bool ignoring)
{
	person->ignore_all_persons = ignoring;
}

void
person_set_ignore_tiles (person_t* person, bool ignoring)
{
	person->ignore_all_tiles = ignoring;
}

void
person_set_layer(person_t* person, int layer)
{
	person->layer = layer;
}

bool
person_set_leader(person_t* person, person_t* leader, int distance)
{
	const person_t* node;

	// prevent circular follower chains from forming
	if (leader != NULL) {
		node = leader;
		do {
			if (node == person)
				return false;
		} while ((node = node->leader));
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

void
person_set_offset(person_t* person, point2_t offset)
{
	person->x_offset = offset.x;
	person->y_offset = offset.y;
}

void
person_set_pose(person_t* person, const char* pose_name)
{
	person->direction = realloc(person->direction, (strlen(pose_name) + 1) * sizeof(char));
	strcpy(person->direction, pose_name);
}

void
person_set_revert_delay(person_t* person, int num_frames)
{
	person->revert_delay = num_frames;
	person->revert_frames = num_frames;
}

void
person_set_scale(person_t* person, double scale_x, double scale_y)
{
	person->scale_x = scale_x;
	person->scale_y = scale_y;
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
	spriteset_unref(old_spriteset);
}

void
person_set_trailing(person_t* person, int distance)
{
	enlarge_step_history(person->leader, distance);
	person->follow_distance = distance;
}

void
person_set_visible(person_t* person, bool visible)
{
	person->is_visible = visible;
}

void
person_set_xyz(person_t* person, double x, double y, int layer)
{
	person->x = x;
	person->y = y;
	person->layer = layer;
	sort_persons();
}

void
person_on_event(person_t* person, int type, script_t* script)
{
	script_unref(person->scripts[type]);
	person->scripts[type] = script;
}

void
person_activate(const person_t* person, person_op_t op, const person_t* acting_person, bool use_default)
{
	const person_t* last_acting;
	const person_t* last_current;

	last_acting = s_acting_person;
	last_current = s_current_person;
	s_acting_person = acting_person;
	s_current_person = person;
	if (use_default)
		script_run(s_def_person_scripts[op], false);
	if (does_person_exist(person))
		script_run(person->scripts[op], false);
	s_acting_person = last_acting;
	s_current_person = last_current;
}

void
person_call_default(const person_t* person, person_op_t op, const person_t* acting_person)
{
	const person_t* last_acting;
	const person_t* last_current;

	last_acting = s_acting_person;
	last_current = s_current_person;
	s_acting_person = acting_person;
	s_current_person = person;
	script_run(s_def_person_scripts[op], false);
	s_acting_person = last_acting;
	s_current_person = last_current;
}

void
person_clear_ignores(person_t* person)
{
	int i;

	for (i = 0; i < person->num_ignores; ++i)
		free(person->ignores[i]);
	person->num_ignores = 0;
}

void
person_clear_queue(person_t* person)
{
	person->num_commands = 0;
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
	script = script_new(codestring, "%s/%s/%s.js", map_pathname(), person->name, script_name);
	person_on_event(person, type, script);
	return true;
}

void
person_ignore_name(person_t* person, const char* name)
{
	int index;

	index = person->num_ignores++;
	person->ignores = realloc(person->ignores, person->num_ignores * sizeof(char*));
	person->ignores[index] = strdup(name);

	// ignore list changed, delete cache
	vector_free(person->ignore_list);
	person->ignore_list = NULL;
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
	if (target_person != NULL)
		person_activate(target_person, PERSON_SCRIPT_ON_TALK, person, true);
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
	script_unref(old_script);
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
	rect_normalize(&bounds);
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
	script_unref(old_script);
}

void
zone_set_steps(int zone_index, int interval)
{
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	zone->interval = interval;
	zone->steps_left = 0;
}

void
zone_activate(int zone_index)
{
	int              last_zone;
	struct map_zone* zone;

	zone = vector_get(s_map->zones, zone_index);
	last_zone = s_current_zone;
	s_current_zone = zone_index;
	script_run(zone->script, true);
	s_current_zone = last_zone;
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

	console_log(2, "changing current map to '%s'", filename);

	map = load_map(filename);
	if (map == NULL) return false;
	if (s_map != NULL) {
		// run map exit scripts first, before loading new map
		map_activate(MAP_SCRIPT_ON_LEAVE, true);
	}

	// close out old map and prep for new one
	free_map(s_map); free(s_map_filename);
	for (i = 0; i < s_num_deferreds; ++i)
		script_unref(s_deferreds[i].script);
	s_num_deferreds = 0;
	s_map = map; s_map_filename = strdup(filename);
	reset_persons(preserve_persons);

	// populate persons
	for (i = 0; i < s_map->num_persons; ++i) {
		person_info = &s_map->persons[i];
		path = game_full_path(g_game, lstr_cstr(person_info->spriteset), "spritesets", true);
		spriteset = spriteset_load(path_cstr(path));
		path_free(path);
		if (spriteset == NULL)
			goto on_error;
		if (!(person = person_new(lstr_cstr(person_info->name), spriteset, false, NULL)))
			goto on_error;
		spriteset_unref(spriteset);
		person_set_xyz(person, person_info->x, person_info->y, person_info->z);
		person_compile_script(person, PERSON_SCRIPT_ON_CREATE, person_info->create_script);
		person_compile_script(person, PERSON_SCRIPT_ON_DESTROY, person_info->destroy_script);
		person_compile_script(person, PERSON_SCRIPT_ON_TOUCH, person_info->touch_script);
		person_compile_script(person, PERSON_SCRIPT_ON_TALK, person_info->talk_script);
		person_compile_script(person, PERSON_SCRIPT_GENERATOR, person_info->command_script);

		// normally this is handled by person_new(), but since in this case the
		// person-specific create script isn't compiled until after the person is created,
		// the map engine gets the responsibility.
		person_activate(person, PERSON_SCRIPT_ON_CREATE, NULL, false);
	}

	// set camera over starting position
	s_camera_x = s_map->origin.x;
	s_camera_y = s_map->origin.y;

	// start up map BGM (if same as previous, leave alone)
	if (s_map->bgm_file == NULL && s_map_bgm_stream != NULL) {
		sound_unref(s_map_bgm_stream);
		lstr_free(s_last_bgm_file);
		s_map_bgm_stream = NULL;
		s_last_bgm_file = NULL;
	}
	else if (s_map->bgm_file != NULL
		&& (s_last_bgm_file == NULL || lstr_cmp(s_map->bgm_file, s_last_bgm_file) != 0))
	{
		sound_unref(s_map_bgm_stream);
		lstr_free(s_last_bgm_file);
		s_last_bgm_file = lstr_dup(s_map->bgm_file);
		path = game_full_path(g_game, lstr_cstr(s_map->bgm_file), "sounds", true);
		if ((s_map_bgm_stream = sound_new(path_cstr(path)))) {
			sound_set_repeat(s_map_bgm_stream, true);
			sound_play(s_map_bgm_stream, s_bgm_mixer);
		}
		path_free(path);
	}

	// run map entry scripts
	map_activate(MAP_SCRIPT_ON_ENTER, true);

	s_frames = 0;
	return true;

on_error:
	spriteset_unref(spriteset);
	free_map(s_map);
	return false;
}

static void
command_person(person_t* person, int command)
{
	double    new_x;
	double    new_y;
	person_t* person_to_touch;

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
		person_set_pose(person, "north");
		break;
	case COMMAND_FACE_NORTHEAST:
		person_set_pose(person, "northeast");
		break;
	case COMMAND_FACE_EAST:
		person_set_pose(person, "east");
		break;
	case COMMAND_FACE_SOUTHEAST:
		person_set_pose(person, "southeast");
		break;
	case COMMAND_FACE_SOUTH:
		person_set_pose(person, "south");
		break;
	case COMMAND_FACE_SOUTHWEST:
		person_set_pose(person, "southwest");
		break;
	case COMMAND_FACE_WEST:
		person_set_pose(person, "west");
		break;
	case COMMAND_FACE_NORTHWEST:
		person_set_pose(person, "northwest");
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
			person->x = new_x;
			person->y = new_y;
		}
		else {
			// if not, and we collided with a person, call that person's touch script
			if (person_to_touch != NULL)
				person_activate(person_to_touch, PERSON_SCRIPT_ON_TOUCH, person, true);
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
	y_delta = y_p1 - y_p2;
	if (y_delta != 0)
		return y_delta;
	else if (person_following(p1, p2))
		return -1;
	else if (person_following(p2, p1))
		return 1;
	else
		return p1->id - p2->id;
}

static void
detach_person(const person_t* person)
{
	int i;

	if (s_camera_person == person)
		s_camera_person = NULL;
	for (i = 0; i < PLAYER_MAX; ++i) {
		if (s_players[i].person == person)
			s_players[i].person = NULL;
	}
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
			person->direction, trunc(x), trunc(y), person->frame);
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
		script_unref(map->scripts[i]);
	for (i = 0; i < map->num_layers; ++i) {
		script_unref(map->layers[i].render_script);
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
	while ((trigger = iter_next(&iter)))
		script_unref(trigger->script);
	iter = vector_enum(s_map->zones);
	while ((zone = iter_next(&iter)))
		script_unref(zone->script);
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
		script_unref(person->scripts[i]);
	spriteset_unref(person->sprite);
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
	while ((trigger = iter_next(&iter))) {
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
	while ((zone = iter_next(&iter))) {
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
	file_t*                  file = NULL;
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

	console_log(2, "constructing new map from '%s'", filename);

	memset(&rmp, 0, sizeof(struct rmp_header));

	if (!(file = file_open(g_game, filename, "rb")))
		goto on_error;
	map = calloc(1, sizeof(struct map));
	if (file_read(file, &rmp, 1, sizeof(struct rmp_header)) != 1)
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
			if (file_read(file, &layer_hdr, 1, sizeof(struct rmp_layer_header)) != 1)
				goto on_error;
			layer = &map->layers[i];
			layer->is_parallax = (layer_hdr.flags & 2) != 0x0;
			layer->is_reflective = layer_hdr.is_reflective;
			layer->is_visible = (layer_hdr.flags & 1) == 0x0;
			layer->color_mask = mk_color(255, 255, 255, 255);
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
			if ((tile_data = malloc(num_tiles * 2)) == NULL)
				goto on_error;
			if (file_read(file, tile_data, num_tiles, 2) != num_tiles)
				goto on_error;
			for (j = 0; j < num_tiles; ++j)
				layer->tilemap[j].tile_index = tile_data[j];
			for (j = 0; j < layer_hdr.num_segments; ++j) {
				if (!fread_rect32(file, &segment)) goto on_error;
				obsmap_add_line(layer->obsmap, segment);
			}
			free(tile_data);
			tile_data = NULL;
		}

		// if either dimension is zero, the map has no non-parallax layers and is thus malformed
		if (map->width == 0 || map->height == 0)
			goto on_error;

		// load entities
		map->num_persons = 0;
		for (i = 0; i < rmp.num_entities; ++i) {
			if (file_read(file, &entity_hdr, 1, sizeof(struct rmp_entity_header)) != 1)
				goto on_error;
			if (entity_hdr.z < 0 || entity_hdr.z >= rmp.num_layers)
				entity_hdr.z = 0;
			switch (entity_hdr.type) {
			case 1:  // person
				++map->num_persons;
				person = &map->persons[map->num_persons - 1];
				memset(person, 0, sizeof(struct map_person));
				if (!(person->name = read_lstring(file, true)))
					goto on_error;
				if (!(person->spriteset = read_lstring(file, true)))
					goto on_error;
				person->x = entity_hdr.x; person->y = entity_hdr.y; person->z = entity_hdr.z;
				if (file_read(file, &count, 1, 2) != 1 || count < 5)
					goto on_error;
				person->create_script = read_lstring(file, false);
				person->destroy_script = read_lstring(file, false);
				person->touch_script = read_lstring(file, false);
				person->talk_script = read_lstring(file, false);
				person->command_script = read_lstring(file, false);
				for (j = 5; j < count; ++j)
					lstr_free(read_lstring(file, true));
				file_seek(file, 16, WHENCE_CUR);
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
			if (file_read(file, &zone_hdr, 1, sizeof(struct rmp_zone_header)) != 1)
				goto on_error;
			if ((script = read_lstring(file, false)) == NULL) goto on_error;
			if (zone_hdr.layer < 0 || zone_hdr.layer >= rmp.num_layers)
				zone_hdr.layer = 0;
			zone.layer = zone_hdr.layer;
			zone.bounds = mk_rect(zone_hdr.x1, zone_hdr.y1, zone_hdr.x2, zone_hdr.y2);
			zone.interval = zone_hdr.interval;
			zone.steps_left = 0;
			zone.script = script_new(script, "%s/zone%d", filename, vector_len(map->zones));
			rect_normalize(&zone.bounds);
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
	file_close(file);
	return map;

on_error:
	if (file != NULL) file_close(file);
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

void
map_screen_to_layer(int layer, int camera_x, int camera_y, int* inout_x, int* inout_y)
{
	rect_t  bounds;
	int     center_x;
	int     center_y;
	int     layer_h;
	int     layer_w;
	float   plx_offset_x = 0.0;
	int     plx_offset_y = 0.0;
	size2_t resolution;
	int     tile_w;
	int     tile_h;
	int     x_offset;
	int     y_offset;

	// get layer and screen metrics
	resolution = screen_size(g_screen);
	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	layer_w = s_map->layers[layer].width * tile_w;
	layer_h = s_map->layers[layer].height * tile_h;
	center_x = resolution.width / 2;
	center_y = resolution.height / 2;

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
		// if the map is smaller than the screen, align to top left.  centering
		// would be better aesthetically, but there are a couple Sphere 1.x games
		// that depend on top-left justification.
		if (layer_w < resolution.width)
			x_offset = 0;
		if (layer_h < resolution.height)
			y_offset = 0;
	}
	if (inout_x != NULL)
		*inout_x += x_offset;
	if (inout_y != NULL)
		*inout_y += y_offset;

	// normalize coordinates. this simplifies rendering calculations.
	if (s_map->is_repeating || s_map->layers[layer].is_parallax) {
		if (inout_x) *inout_x = (*inout_x % layer_w + layer_w) % layer_w;
		if (inout_y) *inout_y = (*inout_y % layer_h + layer_h) % layer_h;
	}
}

static void
map_screen_to_map(int camera_x, int camera_y, int* inout_x, int* inout_y)
{
	rect_t  bounds;
	int     center_x;
	int     center_y;
	int     map_h;
	int     map_w;
	size2_t resolution;
	int     tile_h;
	int     tile_w;
	int     x_offset;
	int     y_offset;

	// get layer and screen metrics
	resolution = screen_size(g_screen);
	tileset_get_size(s_map->tileset, &tile_w, &tile_h);
	map_w = s_map->width * tile_w;
	map_h = s_map->height * tile_h;
	center_x = resolution.width / 2;
	center_y = resolution.height / 2;

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
		// if the map is smaller than the screen, align to top left.  centering
		// would be better aesthetically, but there are a couple Sphere 1.x games
		// that depend on top-left justification.
		if (map_w < resolution.width)
			x_offset = 0;
		if (map_h < resolution.height)
			y_offset = 0;
	}
	if (inout_x != NULL)
		*inout_x += x_offset;
	if (inout_y != NULL)
		*inout_y += y_offset;

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
	for (i = 0; i < PLAYER_MAX; ++i) {
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
			else {
				// allow talking again only after key is released
				s_players[i].is_talk_allowed = true;
			}
			mv_x = 0; mv_y = 0;
			if (person->num_commands == 0 && person->leader == NULL) {
				// allow player control only if the input person is idle and not being led around
				// by someone else.
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
			person_activate(person, PERSON_SCRIPT_ON_DESTROY, NULL, true);
			free_person(person);
			--s_num_persons;
			for (j = i; j < s_num_persons; ++j)
				s_persons[j] = s_persons[j + 1];
			--i;
		}
	}
	sort_persons();
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
	double              start_x[PLAYER_MAX];
	double              start_y[PLAYER_MAX];
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

	for (i = 0; i < PLAYER_MAX; ++i) if (s_players[i].person != NULL)
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
			map_activate(script_type, true);
	}

	// if there are any input persons, check for trigger activation
	for (i = 0; i < PLAYER_MAX; ++i) if (s_players[i].person != NULL) {
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
	for (k = 0; k < PLAYER_MAX; ++k) if (s_players[k].person != NULL) {
		person_get_xy(s_players[k].person, &x, &y, false);
		px = fabs(x - start_x[k]);
		py = fabs(y - start_y[k]);
		num_zone_steps = px > py ? px : py;
		for (i = 0; i < num_zone_steps; ++i) {
			j = 0;
			while ((zone = get_zone_at(x, y, layer, j++, &index))) {
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

	// check if there are any deferred scripts due to run this frame
	// and run the ones that are
	for (i = 0; i < s_num_deferreds; ++i) {
		if (s_deferreds[i].frames_left-- <= 0) {
			script_to_run = s_deferreds[i].script;
			for (j = i; j < s_num_deferreds - 1; ++j)
				s_deferreds[j] = s_deferreds[j + 1];
			--s_num_deferreds;
			script_run(script_to_run, false);
			script_unref(script_to_run);
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
			person_activate(person, PERSON_SCRIPT_GENERATOR, NULL, true);

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
			script_unref(command.script);
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
		return;  // they probably got eaten by a pig.

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
