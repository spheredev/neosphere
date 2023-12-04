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

#ifndef SPHERE__INPUT_H__INCLUDED
#define SPHERE__INPUT_H__INCLUDED

#include "script.h"

#define MAX_JOYSTICKS     16
#define MAX_JOY_BUTTONS   64
#define MAX_MOUSE_BUTTONS 32

typedef
enum player_key
{
	PLAYER_KEY_MENU,
	PLAYER_KEY_UP,
	PLAYER_KEY_DOWN,
	PLAYER_KEY_LEFT,
	PLAYER_KEY_RIGHT,
	PLAYER_KEY_A,
	PLAYER_KEY_B,
	PLAYER_KEY_X,
	PLAYER_KEY_Y,
	PLAYER_KEY_MAX
} player_key_t;

typedef
enum mouse_button
{
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE
} mouse_button_t;

typedef
enum mouse_key
{
	MOUSE_KEY_NONE,
	MOUSE_KEY_LEFT,
	MOUSE_KEY_RIGHT,
	MOUSE_KEY_MIDDLE,
	MOUSE_KEY_BACK,
	MOUSE_KEY_FORWARD,
	MOUSE_KEY_WHEEL_UP,
	MOUSE_KEY_WHEEL_DOWN,
	MOUSE_KEY_MAX,
} mouse_key_t;

typedef
struct mouse_event
{
	mouse_key_t key;
	int         delta;
	int         x;
	int         y;
} mouse_event_t;

void initialize_input   (void);
void shutdown_input     (void);

bool          joy_is_button_down (int joy_index, int button);
const char*   joy_name           (int joy_index);
int           joy_num_axes       (int joy_index);
int           joy_num_buttons    (int joy_index);
int           joy_num_devices    (void);
float         joy_position       (int joy_index, int axis_index);
void          joy_bind_button    (int joy_index, int button, script_t* on_down_script, script_t* on_up_script);
bool          kb_is_any_key_down (void);
bool          kb_is_key_down     (int keycode);
bool          kb_is_toggled      (int keycode);
int           kb_queue_len       (void);
void          kb_bind_key        (int keycode, script_t* on_down_script, script_t* on_up_script);
void          kb_clear_queue     (void);
int           kb_get_key         (void);
void          kb_load_keymap     (void);
void          kb_save_keymap     (void);
bool          mouse_is_key_down  (mouse_key_t key);
int           mouse_queue_len    (void);
void          mouse_clear_queue  (void);
mouse_event_t mouse_get_event    (void);

int   get_player_key       (int player, player_key_t vkey);
void  set_player_key       (int player, player_key_t vkey, int keycode);
void  attach_input_display (void);
void  update_bound_keys    (bool use_map_keys);
void  update_input         (void);

#endif // SPHERE__INPUT_H__INCLUDED
