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

#ifndef SPHERE__MAP_ENGINE_H__INCLUDED
#define SPHERE__MAP_ENGINE_H__INCLUDED

#include "color.h"
#include "geometry.h"
#include "obstruction.h"
#include "script.h"
#include "spriteset.h"
#include "tileset.h"

typedef struct person person_t;

typedef
enum player_id
{
	PLAYER_1,
	PLAYER_2,
	PLAYER_3,
	PLAYER_4,
	PLAYER_MAX
} player_id_t;

typedef
enum command_op
{
	COMMAND_WAIT,
	COMMAND_ANIMATE,
	COMMAND_FACE_NORTH,
	COMMAND_FACE_NORTHEAST,
	COMMAND_FACE_EAST,
	COMMAND_FACE_SOUTHEAST,
	COMMAND_FACE_SOUTH,
	COMMAND_FACE_SOUTHWEST,
	COMMAND_FACE_WEST,
	COMMAND_FACE_NORTHWEST,
	COMMAND_MOVE_NORTH,
	COMMAND_MOVE_NORTHEAST,
	COMMAND_MOVE_EAST,
	COMMAND_MOVE_SOUTHEAST,
	COMMAND_MOVE_SOUTH,
	COMMAND_MOVE_SOUTHWEST,
	COMMAND_MOVE_WEST,
	COMMAND_MOVE_NORTHWEST,
	COMMAND_RUN_SCRIPT
} command_op_t;

typedef
enum map_op
{
	MAP_SCRIPT_ON_ENTER,
	MAP_SCRIPT_ON_LEAVE,
	MAP_SCRIPT_ON_LEAVE_NORTH,
	MAP_SCRIPT_ON_LEAVE_EAST,
	MAP_SCRIPT_ON_LEAVE_SOUTH,
	MAP_SCRIPT_ON_LEAVE_WEST,
	MAP_SCRIPT_MAX
} map_op_t;

typedef
enum person_op
{
	PERSON_SCRIPT_ON_CREATE,
	PERSON_SCRIPT_ON_DESTROY,
	PERSON_SCRIPT_ON_TOUCH,
	PERSON_SCRIPT_ON_TALK,
	PERSON_SCRIPT_GENERATOR,
	PERSON_SCRIPT_MAX
} person_op_t;

bool             map_engine_init              (void);
void             map_engine_uninit            (void);
void             map_engine_on_render         (script_t* script);
void             map_engine_on_update         (script_t* script);
const person_t*  map_engine_acting_person     (void);
const person_t*  map_engine_active_person     (void);
int              map_engine_active_trigger    (void);
int              map_engine_active_zone       (void);
vector_t*        map_engine_persons           (void);
bool             map_engine_running           (void);
int              map_engine_get_framerate     (void);
person_t*        map_engine_get_player        (player_id_t player_id);
person_t*        map_engine_get_subject       (void);
int              map_engine_get_talk_button   (void);
int              map_engine_get_talk_distance (void);
int              map_engine_get_talk_key      (player_id_t player_id);
void             map_engine_on_map_event      (map_op_t op, script_t* script);
void             map_engine_on_person_event   (person_op_t op, script_t* script);
void             map_engine_set_framerate     (int framerate);
void             map_engine_set_player        (player_id_t player_id, person_t* person);
void             map_engine_set_subject       (person_t* person);
void             map_engine_set_talk_button   (int button_id);
void             map_engine_set_talk_distance (int distance);
void             map_engine_set_talk_key      (player_id_t player_id, int key);
bool             map_engine_change_map        (const char* filename);
void             map_engine_defer             (script_t* script, int num_frames);
void             map_engine_draw_map          (void);
void             map_engine_exit              (void);
void             map_engine_fade_to           (color_t mask_color, int num_frames);
bool             map_engine_start             (const char* filename, int framerate);
void             map_engine_update            (void);
rect_t           map_bounds                   (void);
int              map_layer_by_name            (const char* name);
int              map_num_layers               (void);
int              map_num_persons              (void);
int              map_num_triggers             (void);
int              map_num_zones                (void);
point3_t         map_origin                   (void);
const char*      map_pathname                 (void);
person_t*        map_person_by_name           (const char* name);
int              map_tile_at                  (int x, int y, int layer);
tileset_t*       map_tileset                  (void);
int              map_trigger_at               (int x, int y, int layer);
point2_t         map_xy_from_screen           (point2_t screen_xy);
int              map_zone_at                  (int x, int y, int layer, int which);
point2_t         map_get_camera_xy            (void);
void             map_set_camera_xy            (point2_t where);
void             map_activate                 (map_op_t op, bool use_default);
bool             map_add_trigger              (int x, int y, int layer, script_t* script);
bool             map_add_zone                 (rect_t bounds, int layer, script_t* script, int steps);
void             map_call_default             (map_op_t op);
void             map_normalize_xy             (double* inout_x, double* inout_y, int layer);
void             map_remove_trigger           (int trigger_index);
void             map_remove_zone              (int zone_index);
void             layer_on_render              (int layer, script_t* script);
const char*      layer_name                   (int layer);
const obsmap_t*  layer_obsmap                 (int layer);
size2_t          layer_size                   (int layer);
color_t          layer_get_color_mask         (int layer);
bool             layer_get_reflective         (int layer);
int              layer_get_tile               (int layer, int x, int y);
bool             layer_get_visible            (int layer);
void             layer_set_color_mask         (int layer, color_t color);
void             layer_set_reflective         (int layer, bool reflective);
void             layer_set_tile               (int layer, int x, int y, int tile_index);
void             layer_set_visible            (int layer, bool visible);
void             layer_replace_tiles          (int layer, int old_index, int new_index);
bool             layer_resize                 (int layer, int x_size, int y_size);
person_t*        person_new                   (const char* name, spriteset_t* spriteset, bool is_persistent, script_t* create_script);
void             person_free                  (person_t* person);
rect_t           person_base                  (const person_t* person);
bool             person_following             (const person_t* person, const person_t* leader);
bool             person_has_moved             (const person_t* person);
vector_t*        person_ignore_list           (person_t* person);
bool             person_ignored_by            (const person_t* person, const person_t* other);
bool             person_moving                (const person_t* person);
const char*      person_name                  (const person_t* person);
bool             person_obstructed_at         (const person_t* person, double x, double y, person_t** out_obstructing_person, int* out_tile_index);
double           person_get_angle             (const person_t* person);
color_t          person_get_color             (const person_t* person);
int              person_get_frame             (const person_t* person);
int              person_get_frame_delay       (const person_t* person);
bool             person_get_ignore_persons    (const person_t* person);
bool             person_get_ignore_tiles      (const person_t* person);
int              person_get_layer             (const person_t* person);
person_t*        person_get_leader            (const person_t* person);
point2_t         person_get_offset            (const person_t* person);
const char*      person_get_pose              (const person_t* person);
int              person_get_revert_delay      (const person_t* person);
void             person_get_scale             (const person_t*, double* out_scale_x, double* out_scale_y);
void             person_get_speed             (const person_t* person, double* out_x_speed, double* out_y_speed);
spriteset_t*     person_get_spriteset         (const person_t* person);
int              person_get_trailing          (const person_t* person);
bool             person_get_visible           (const person_t* person);
void             person_get_xy                (const person_t* person, double* out_x, double* out_y, bool normalize);
void             person_get_xyz               (const person_t* person, double* out_x, double* out_y, int* out_layer, bool normalize);
void             person_set_angle             (person_t* person, double theta);
void             person_set_color             (person_t* person, color_t mask);
void             person_set_frame             (person_t* person, int frame_index);
void             person_set_frame_delay       (person_t* person, int num_frames);
void             person_set_ignore_persons    (person_t* person, bool ignoring);
void             person_set_ignore_tiles      (person_t* person, bool ignoring);
void             person_set_layer             (person_t* person, int layer);
bool             person_set_leader            (person_t* person, person_t* leader, int distance);
void             person_set_offset            (person_t* person, point2_t offset);
void             person_set_pose              (person_t* person, const char* pose_name);
void             person_set_revert_delay      (person_t* person, int num_frames);
void             person_set_scale             (person_t*, double scale_x, double scale_y);
void             person_set_speed             (person_t* person, double x_speed, double y_speed);
void             person_set_spriteset         (person_t* person, spriteset_t* spriteset);
void             person_set_trailing          (person_t* person, int distance);
void             person_set_visible           (person_t* person, bool visible);
void             person_set_xyz               (person_t* person, double x, double y, int layer);
void             person_on_event              (person_t* person, int type, script_t* script);
void             person_activate              (const person_t* person, person_op_t op, const person_t* acting_person, bool use_default);
void             person_call_default          (const person_t* person, person_op_t op, const person_t* acting_person);
void             person_clear_queue           (person_t* person);
bool             person_compile_script        (person_t* person, int type, const lstring_t* codestring);
void             person_clear_ignores         (person_t* person);
void             person_ignore_name           (person_t* person, const char* name);
bool             person_queue_command         (person_t* person, int command, bool is_immediate);
bool             person_queue_script          (person_t* person, script_t* script, bool is_immediate);
void             person_talk                  (const person_t* person);
void             trigger_get_xyz              (int trigger_index, int* out_x, int* out_y, int* out_layer);
void             trigger_set_layer            (int trigger_index, int layer);
void             trigger_set_script           (int trigger_index, script_t* script);
void             trigger_set_xy               (int trigger_index, int x, int y);
void             trigger_activate             (int trigger_index);
rect_t           zone_get_bounds              (int zone_index);
int              zone_get_layer               (int zone_index);
int              zone_get_steps               (int zone_index);
void             zone_set_bounds              (int zone_index, rect_t bounds);
void             zone_set_layer               (int zone_index, int layer);
void             zone_set_script              (int zone_index, script_t* script);
void             zone_set_steps               (int zone_index, int steps);
void             zone_activate                (int zone_index);

#endif // SPHERE__MAP_ENGINE_H__INCLUDED
