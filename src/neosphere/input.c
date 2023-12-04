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
#include "input.h"

#include "debugger.h"
#include "jsal.h"
#include "kev_file.h"
#include "script.h"
#include "vector.h"

struct key_queue
{
	int num_keys;
	int keys[255];
};

static void queue_key       (int keycode);
static void queue_mouse_key (mouse_key_t key, int x, int y, int delta);

static vector_t*            s_bound_buttons;
static vector_t*            s_bound_keys;
static int                  s_default_key_map[4][PLAYER_KEY_MAX];
static ALLEGRO_EVENT_QUEUE* s_event_queue;
static bool                 s_has_keymap_changed = false;
static bool                 s_have_joystick;
static bool                 s_have_mouse;
static ALLEGRO_JOYSTICK*    s_joy_handles[MAX_JOYSTICKS];
static int                  s_key_map[4][PLAYER_KEY_MAX];
static struct key_queue     s_key_queue;
static bool                 s_key_state[ALLEGRO_KEY_MAX];
static int                  s_keymod_state;
static int                  s_last_wheel_pos = 0;
static mouse_event_t        s_mouse_queue[255];
static int                  s_num_joysticks = 0;
static int                  s_num_mouse_buttons;
static int                  s_num_mouse_events = 0;
static bool                 s_was_button_down[MAX_MOUSE_BUTTONS];

struct bound_button
{
	int       joystick_id;
	int       button;
	bool      is_pressed;
	script_t* on_down_script;
	script_t* on_up_script;
};

struct bound_key
{
	int       keycode;
	bool      is_pressed;
	script_t* on_down_script;
	script_t* on_up_script;
};

void
initialize_input(void)
{
	int i;

	console_log(1, "initializing input subsystem");

	al_install_keyboard();
	if (!(s_have_mouse = al_install_mouse()))
		console_log(1, "  mouse initialization failed");
	if (!(s_have_joystick = al_install_joystick()))
		console_log(1, "  joystick initialization failed");

	s_num_mouse_buttons = (int)al_get_mouse_num_buttons();
	if (s_num_mouse_buttons > MAX_MOUSE_BUTTONS)
		s_num_mouse_buttons = MAX_MOUSE_BUTTONS;

	memset(s_key_state, 0, sizeof s_key_state);
	memset(s_was_button_down, 0, sizeof s_was_button_down);

	s_event_queue = al_create_event_queue();
	al_register_event_source(s_event_queue, al_get_keyboard_event_source());
	if (s_have_mouse)
		al_register_event_source(s_event_queue, al_get_mouse_event_source());
	if (s_have_joystick)
		al_register_event_source(s_event_queue, al_get_joystick_event_source());

	// look for active joysticks
	if (s_have_joystick) {
		s_num_joysticks = fmin(MAX_JOYSTICKS, al_get_num_joysticks());
		for (i = 0; i < MAX_JOYSTICKS; ++i)
			s_joy_handles[i] = i < s_num_joysticks ? al_get_joystick(i) : NULL;
	}

	// create bound key vectors
	s_bound_buttons = vector_new(sizeof(struct bound_button));
	s_bound_keys = vector_new(sizeof(struct bound_key));

	// fill in default player key map
	memset(s_key_map, 0, sizeof(s_key_map));
	s_default_key_map[0][PLAYER_KEY_UP] = ALLEGRO_KEY_UP;
	s_default_key_map[0][PLAYER_KEY_DOWN] = ALLEGRO_KEY_DOWN;
	s_default_key_map[0][PLAYER_KEY_LEFT] = ALLEGRO_KEY_LEFT;
	s_default_key_map[0][PLAYER_KEY_RIGHT] = ALLEGRO_KEY_RIGHT;
	s_default_key_map[0][PLAYER_KEY_A] = ALLEGRO_KEY_Z;
	s_default_key_map[0][PLAYER_KEY_B] = ALLEGRO_KEY_X;
	s_default_key_map[0][PLAYER_KEY_X] = ALLEGRO_KEY_C;
	s_default_key_map[0][PLAYER_KEY_Y] = ALLEGRO_KEY_V;
	s_default_key_map[0][PLAYER_KEY_MENU] = ALLEGRO_KEY_TAB;
	s_default_key_map[1][PLAYER_KEY_UP] = ALLEGRO_KEY_W;
	s_default_key_map[1][PLAYER_KEY_DOWN] = ALLEGRO_KEY_S;
	s_default_key_map[1][PLAYER_KEY_LEFT] = ALLEGRO_KEY_A;
	s_default_key_map[1][PLAYER_KEY_RIGHT] = ALLEGRO_KEY_D;
	s_default_key_map[1][PLAYER_KEY_A] = ALLEGRO_KEY_1;
	s_default_key_map[1][PLAYER_KEY_B] = ALLEGRO_KEY_2;
	s_default_key_map[1][PLAYER_KEY_X] = ALLEGRO_KEY_3;
	s_default_key_map[1][PLAYER_KEY_Y] = ALLEGRO_KEY_4;
	s_default_key_map[1][PLAYER_KEY_MENU] = ALLEGRO_KEY_TAB;
	s_default_key_map[2][PLAYER_KEY_UP] = ALLEGRO_KEY_PAD_8;
	s_default_key_map[2][PLAYER_KEY_DOWN] = ALLEGRO_KEY_PAD_2;
	s_default_key_map[2][PLAYER_KEY_LEFT] = ALLEGRO_KEY_PAD_4;
	s_default_key_map[2][PLAYER_KEY_RIGHT] = ALLEGRO_KEY_PAD_6;
	s_default_key_map[2][PLAYER_KEY_A] = ALLEGRO_KEY_PAD_PLUS;
	s_default_key_map[2][PLAYER_KEY_B] = ALLEGRO_KEY_PAD_MINUS;
	s_default_key_map[2][PLAYER_KEY_X] = ALLEGRO_KEY_PAD_0;
	s_default_key_map[2][PLAYER_KEY_Y] = ALLEGRO_KEY_PAD_DELETE;
	s_default_key_map[2][PLAYER_KEY_MENU] = ALLEGRO_KEY_TAB;
	s_default_key_map[3][PLAYER_KEY_UP] = ALLEGRO_KEY_I;
	s_default_key_map[3][PLAYER_KEY_DOWN] = ALLEGRO_KEY_K;
	s_default_key_map[3][PLAYER_KEY_LEFT] = ALLEGRO_KEY_J;
	s_default_key_map[3][PLAYER_KEY_RIGHT] = ALLEGRO_KEY_L;
	s_default_key_map[3][PLAYER_KEY_A] = ALLEGRO_KEY_7;
	s_default_key_map[3][PLAYER_KEY_B] = ALLEGRO_KEY_8;
	s_default_key_map[3][PLAYER_KEY_X] = ALLEGRO_KEY_9;
	s_default_key_map[3][PLAYER_KEY_Y] = ALLEGRO_KEY_0;
	s_default_key_map[3][PLAYER_KEY_MENU] = ALLEGRO_KEY_TAB;

	// load global key mappings
	kb_load_keymap();
}

void
shutdown_input(void)
{
	struct bound_button* bound_button;
	struct bound_key*    bound_key;

	iter_t iter;

	// save player key mappings
	console_log(1, "shutting down input subsystem");

	// free bound key scripts
	iter = vector_enum(s_bound_buttons);
	while ((bound_button = iter_next(&iter))) {
		script_unref(bound_button->on_down_script);
		script_unref(bound_button->on_up_script);
	}
	iter = vector_enum(s_bound_keys);
	while ((bound_key = iter_next(&iter))) {
		script_unref(bound_key->on_down_script);
		script_unref(bound_key->on_up_script);
	}
	vector_free(s_bound_buttons);
	vector_free(s_bound_keys);

	// shut down Allegro input
	al_destroy_event_queue(s_event_queue);
	al_uninstall_joystick();
	al_uninstall_mouse();
	al_uninstall_keyboard();
}

bool
joy_is_button_down(int joy_index, int button)
{
	ALLEGRO_JOYSTICK_STATE joy_state;
	ALLEGRO_JOYSTICK*      joystick;

	if (!s_have_joystick || joy_index < 0)
		return false;
	if (!(joystick = s_joy_handles[joy_index]))
		return 0.0;
	al_get_joystick_state(joystick, &joy_state);
	return joy_state.button[button] > 0;
}

const char*
joy_name(int joy_index)
{
	ALLEGRO_JOYSTICK* joystick;

	if (!s_have_joystick || joy_index < 0 || !(joystick = s_joy_handles[joy_index]))
		return "null joystick";
	return al_get_joystick_name(joystick);
}

int
joy_num_axes(int joy_index)
{
	ALLEGRO_JOYSTICK* joystick;
	int               n_axes;
	int               n_sticks;

	int i;

	if (!s_have_joystick || joy_index < 0 || !(joystick = s_joy_handles[joy_index]))
		return MAX_JOY_BUTTONS;
	n_sticks = al_get_joystick_num_sticks(joystick);
	n_axes = 0;
	for (i = 0; i < n_sticks; ++i)
		n_axes += al_get_joystick_num_axes(joystick, i);
	return n_axes;
}

int
joy_num_buttons(int joy_index)
{
	ALLEGRO_JOYSTICK* joystick;

	if (!s_have_joystick || joy_index < 0 || !(joystick = s_joy_handles[joy_index]))
		return MAX_JOY_BUTTONS;
	return al_get_joystick_num_buttons(joystick);
}

int
joy_num_devices(void)
{
	return s_num_joysticks;
}

float
joy_position(int joy_index, int axis_index)
{
	ALLEGRO_JOYSTICK_STATE joy_state;
	ALLEGRO_JOYSTICK*      joystick;
	int                    n_stick_axes;
	int                    n_sticks;

	int i;

	if (!s_have_joystick || joy_index < 0 || !(joystick = s_joy_handles[joy_index]))
		return 0.0;
	al_get_joystick_state(joystick, &joy_state);
	n_sticks = al_get_joystick_num_sticks(joystick);
	for (i = 0; i < n_sticks; ++i) {
		n_stick_axes = al_get_joystick_num_axes(joystick, i);
		if (axis_index < n_stick_axes)
			return joy_state.stick[i].axis[axis_index];
		axis_index -= n_stick_axes;
	}
	return 0.0;
}

void
joy_bind_button(int joy_index, int button, script_t* on_down_script, script_t* on_up_script)
{
	struct bound_button* bound_button;
	bool                 is_new_entry = true;
	struct bound_button  new_binding;
	script_t*            old_down_script;
	script_t*            old_up_script;

	iter_t iter;

	new_binding.joystick_id = joy_index;
	new_binding.button = button;
	new_binding.is_pressed = false;
	new_binding.on_down_script = on_down_script;
	new_binding.on_up_script = on_up_script;
	iter = vector_enum(s_bound_buttons);
	while ((bound_button = iter_next(&iter))) {
		if (bound_button->joystick_id == joy_index && bound_button->button == button) {
			bound_button->is_pressed = false;
			old_down_script = bound_button->on_down_script;
			old_up_script = bound_button->on_up_script;
			memcpy(bound_button, &new_binding, sizeof(struct bound_button));
			if (old_down_script != bound_button->on_down_script)
				script_unref(old_down_script);
			if (old_up_script != bound_button->on_up_script)
				script_unref(old_up_script);
			is_new_entry = false;
		}
	}
	if (is_new_entry)
		vector_push(s_bound_buttons, &new_binding);
}

bool
kb_is_any_key_down(void)
{
	int i_key;

	update_input();
	for (i_key = 0; i_key < ALLEGRO_KEY_MAX; ++i_key)
		if (s_key_state[i_key]) return true;
	return false;
}

bool
kb_is_key_down(int keycode)
{
	bool is_pressed;

	update_input();
	switch (keycode) {
	case ALLEGRO_KEY_LSHIFT:
		is_pressed = s_key_state[ALLEGRO_KEY_LSHIFT]
			|| s_key_state[ALLEGRO_KEY_RSHIFT];
		break;
	case ALLEGRO_KEY_LCTRL:
		is_pressed = s_key_state[ALLEGRO_KEY_LCTRL]
			|| s_key_state[ALLEGRO_KEY_RCTRL];
		break;
	case ALLEGRO_KEY_ALT:
		is_pressed = s_key_state[ALLEGRO_KEY_ALT]
			|| s_key_state[ALLEGRO_KEY_ALTGR];
		break;
	default:
		is_pressed = s_key_state[keycode];
	}
	return is_pressed;
}

bool
kb_is_toggled(int keycode)
{
	int flag;

	flag = keycode == ALLEGRO_KEY_CAPSLOCK ? ALLEGRO_KEYMOD_CAPSLOCK
		: keycode == ALLEGRO_KEY_NUMLOCK ? ALLEGRO_KEYMOD_NUMLOCK
		: keycode == ALLEGRO_KEY_SCROLLLOCK ? ALLEGRO_KEYMOD_SCROLLLOCK
		: 0x0;
	return (s_keymod_state & flag) != 0;
}

int
kb_queue_len(void)
{
	return s_key_queue.num_keys;
}

void
kb_clear_queue(void)
{
	s_key_queue.num_keys = 0;
}

int
kb_get_key(void)
{
	int keycode;

	if (s_key_queue.num_keys == 0)
		return 0;
	keycode = s_key_queue.keys[0];
	--s_key_queue.num_keys;
	memmove(s_key_queue.keys, &s_key_queue.keys[1], sizeof(int) * s_key_queue.num_keys);
	return keycode;
}

void
kb_load_keymap(void)
{
	kev_file_t* file;
	const char* key_name;
	char*       filename;
	lstring_t*  setting;

	int i, j;

	filename = g_game != NULL ? "@/keymap.conf" : "#/neoSphere.conf";
	if (!(file = kev_open(g_game, filename, true)))
		return;
	for (i = 0; i < 4; ++i) for (j = 0; j < PLAYER_KEY_MAX; ++j) {
		key_name = j == PLAYER_KEY_UP ? "UP"
			: j == PLAYER_KEY_DOWN ? "DOWN"
			: j == PLAYER_KEY_LEFT ? "LEFT"
			: j == PLAYER_KEY_RIGHT ? "RIGHT"
			: j == PLAYER_KEY_A ? "A"
			: j == PLAYER_KEY_B ? "B"
			: j == PLAYER_KEY_X ? "X"
			: j == PLAYER_KEY_Y ? "Y"
			: j == PLAYER_KEY_MENU ? "MENU"
			: "8:12";
		setting = lstr_newf("keymap_Player%d_%s", i + 1, key_name);
		s_key_map[i][j] = kev_read_float(file, lstr_cstr(setting), s_default_key_map[i][j]);
		lstr_free(setting);
	}
	kev_close(file);
}

void
kb_save_keymap(void)
{
	kev_file_t* file;
	const char* key_name;
	lstring_t*  setting;

	int i, j;

	if (!s_has_keymap_changed || g_game == NULL)
		return;
	console_log(1, "saving player key mappings");
	file = kev_open(g_game, "@/keymap.conf", true);
	for (i = 0; i < 4; ++i) for (j = 0; j < PLAYER_KEY_MAX; ++j) {
		key_name = j == PLAYER_KEY_UP ? "UP"
			: j == PLAYER_KEY_DOWN ? "DOWN"
			: j == PLAYER_KEY_LEFT ? "LEFT"
			: j == PLAYER_KEY_RIGHT ? "RIGHT"
			: j == PLAYER_KEY_A ? "A"
			: j == PLAYER_KEY_B ? "B"
			: j == PLAYER_KEY_X ? "X"
			: j == PLAYER_KEY_Y ? "Y"
			: j == PLAYER_KEY_MENU ? "MENU"
			: "8:12";
		setting = lstr_newf("keymap_Player%d_%s", i + 1, key_name);
		kev_write_float(file, lstr_cstr(setting), s_key_map[i][j]);
		lstr_free(setting);
	}
	kev_close(file);
}

int
mouse_queue_len(void)
{
	return s_num_mouse_events;
}

void
mouse_clear_queue(void)
{
	s_num_mouse_events = 0;
}

mouse_event_t
mouse_get_event(void)
{
	mouse_event_t event;

	int i;

	event = s_mouse_queue[0];
	--s_num_mouse_events;
	for (i = 0; i < s_num_mouse_events; ++i)
		s_mouse_queue[i] = s_mouse_queue[i + 1];
	return event;
}

bool
mouse_is_key_down(mouse_key_t key)
{
	ALLEGRO_DISPLAY*    display;
	ALLEGRO_MOUSE_STATE state;

	display = screen_display(g_screen);
	al_get_mouse_state(&state);
	if (state.display != display)
		return false;
	switch (key) {
	case MOUSE_KEY_LEFT:
		return al_mouse_button_down(&state, 1);
	case MOUSE_KEY_MIDDLE:
		return al_mouse_button_down(&state, 3);
	case MOUSE_KEY_RIGHT:
		return al_mouse_button_down(&state, 2);
	case MOUSE_KEY_BACK:
		return al_mouse_button_down(&state, 4);
	case MOUSE_KEY_FORWARD:
		return al_mouse_button_down(&state, 5);
	default:
		return false;
	}
}

int
get_player_key(int player, player_key_t vkey)
{
	return s_key_map[player][vkey];
}

void
attach_input_display(void)
{
	al_register_event_source(s_event_queue,
		al_get_display_event_source(screen_display(g_screen)));
}

void
set_player_key(int player, player_key_t vkey, int keycode)
{
	s_key_map[player][vkey] = keycode;
	s_has_keymap_changed = g_game != NULL;
}

void
update_bound_keys(bool use_map_keys)
{
	struct bound_button* bound_button;
	struct bound_key*    bound_key;
	bool                 is_down;

	iter_t iter;

	// check bound keyboard keys
	if (use_map_keys) {
		iter = vector_enum(s_bound_keys);
		while ((bound_key = iter_next(&iter))) {
			is_down = s_key_state[bound_key->keycode];
			if (is_down && !bound_key->is_pressed)
				script_run(bound_key->on_down_script, false);
			if (!is_down && bound_key->is_pressed)
				script_run(bound_key->on_up_script, false);
			bound_key->is_pressed = is_down;
		}
	}

	// check bound joystick buttons
	iter = vector_enum(s_bound_buttons);
	while ((bound_button = iter_next(&iter))) {
		is_down = joy_is_button_down(bound_button->joystick_id, bound_button->button);
		if (is_down && !bound_button->is_pressed)
			script_run(bound_button->on_down_script, false);
		if (!is_down && bound_button->is_pressed)
			script_run(bound_button->on_up_script, false);
		bound_button->is_pressed = is_down;
	}
}

void
update_input(void)
{
	ALLEGRO_EVENT       event;
	bool                is_button_down;
	int                 keycode;
	mouse_key_t         mouse_key;
	ALLEGRO_MOUSE_STATE mouse_state;

	int i;

	// process Allegro input events
	while (al_get_next_event(s_event_queue, &event)) {
		switch (event.type) {
		case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
			// Alt+Tabbing out can cause keys to get "stuck", this works around it
			// by clearing the states when switching away.
			memset(s_key_state, 0, ALLEGRO_KEY_MAX * sizeof(bool));
			break;
		case ALLEGRO_EVENT_KEY_DOWN:
			keycode = event.keyboard.keycode;
			if (keycode == ALLEGRO_KEY_BACKQUOTE)
				keycode = ALLEGRO_KEY_TILDE;
			s_key_state[keycode] = true;

			// queue Ctrl/Alt/Shift keys (Sphere compatibility hack)
			if (keycode == ALLEGRO_KEY_LCTRL || keycode == ALLEGRO_KEY_RCTRL
				|| keycode == ALLEGRO_KEY_ALT || keycode == ALLEGRO_KEY_ALTGR
				|| keycode == ALLEGRO_KEY_LSHIFT || keycode == ALLEGRO_KEY_RSHIFT)
			{
				if (keycode == ALLEGRO_KEY_LCTRL || keycode == ALLEGRO_KEY_RCTRL)
					queue_key(ALLEGRO_KEY_LCTRL);
				if (keycode == ALLEGRO_KEY_ALT || keycode == ALLEGRO_KEY_ALTGR)
					queue_key(ALLEGRO_KEY_ALT);
				if (keycode == ALLEGRO_KEY_LSHIFT || keycode == ALLEGRO_KEY_RSHIFT)
					queue_key(ALLEGRO_KEY_LSHIFT);
			}

			break;
		case ALLEGRO_EVENT_KEY_UP:
			keycode = event.keyboard.keycode;
			if (keycode == ALLEGRO_KEY_BACKQUOTE)
				keycode = ALLEGRO_KEY_TILDE;
			s_key_state[keycode] = false;
			break;
		case ALLEGRO_EVENT_KEY_CHAR:
			keycode = event.keyboard.keycode;
			if (keycode == ALLEGRO_KEY_BACKQUOTE)
				keycode = ALLEGRO_KEY_TILDE;
			s_keymod_state = event.keyboard.modifiers;
			switch (keycode) {
			case ALLEGRO_KEY_ENTER:
				if (event.keyboard.modifiers & ALLEGRO_KEYMOD_ALT
				 || event.keyboard.modifiers & ALLEGRO_KEYMOD_ALTGR)
				{
					screen_toggle_fullscreen(g_screen);
				}
				else {
					queue_key(keycode);
				}
				break;
			case ALLEGRO_KEY_F10:
				screen_toggle_fullscreen(g_screen);
				break;
			case ALLEGRO_KEY_F11:
				screen_toggle_fps(g_screen);
				break;
			case ALLEGRO_KEY_F12:
				if (!debugger_attached())
					screen_queue_screenshot(g_screen);
				else
					jsal_debug_breakpoint_inject();
				break;
			default:
				queue_key(keycode);
				break;
			}
		}
	}

	if (s_have_mouse) {
		// check for mouse wheel movement
		al_get_mouse_state(&mouse_state);
		if (mouse_state.z > s_last_wheel_pos)
			queue_mouse_key(MOUSE_KEY_WHEEL_UP, mouse_state.x, mouse_state.y, mouse_state.z - s_last_wheel_pos);
		if (mouse_state.z < s_last_wheel_pos)
			queue_mouse_key(MOUSE_KEY_WHEEL_DOWN, mouse_state.x, mouse_state.y, s_last_wheel_pos - mouse_state.z);
		s_last_wheel_pos = mouse_state.z;

		// check for clicks on all mouse buttons
		if (mouse_state.display == screen_display(g_screen)) {
			for (i = 0; i < s_num_mouse_buttons; ++i) {
				is_button_down = al_mouse_button_down(&mouse_state, 1 + i);
				mouse_key = i == 0 ? MOUSE_KEY_LEFT
					: i == 1 ? MOUSE_KEY_RIGHT
					: i == 2 ? MOUSE_KEY_MIDDLE
					: i == 3 ? MOUSE_KEY_BACK
					: i == 4 ? MOUSE_KEY_FORWARD
					: MOUSE_KEY_NONE;
				if (is_button_down && !s_was_button_down[i]) {
					if (i < 5)
						queue_mouse_key(mouse_key, mouse_state.x, mouse_state.y, 0);
				}
				s_was_button_down[i] = is_button_down;
			}
		}
	}
}

void
kb_bind_key(int keycode, script_t* on_down_script, script_t* on_up_script)
{
	struct bound_key* bound_key;
	bool              is_new_key = true;
	struct bound_key  new_binding;
	script_t*         old_down_script;
	script_t*         old_up_script;

	iter_t iter;

	new_binding.keycode = keycode;
	new_binding.is_pressed = false;
	new_binding.on_down_script = on_down_script;
	new_binding.on_up_script = on_up_script;
	iter = vector_enum(s_bound_keys);
	while ((bound_key = iter_next(&iter))) {
		if (bound_key->keycode == keycode) {
			bound_key->is_pressed = false;
			old_down_script = bound_key->on_down_script;
			old_up_script = bound_key->on_up_script;
			memcpy(bound_key, &new_binding, sizeof(struct bound_key));
			if (old_down_script != bound_key->on_down_script)
				script_unref(old_down_script);
			if (old_up_script != bound_key->on_up_script)
				script_unref(old_up_script);
			is_new_key = false;
		}
	}
	if (is_new_key)
		vector_push(s_bound_keys, &new_binding);
}

static void
queue_key(int keycode)
{
	int key_index;

	if (s_key_queue.num_keys < 255) {
		key_index = s_key_queue.num_keys;
		++s_key_queue.num_keys;
		s_key_queue.keys[key_index] = keycode;
	}
}

static void
queue_mouse_key(mouse_key_t key, int x, int y, int delta)
{
	mouse_event_t* p_event;

	if (s_num_mouse_events < 255) {
		p_event = &s_mouse_queue[s_num_mouse_events];
		p_event->key = key;
		p_event->delta = delta;
		p_event->x = x;
		p_event->y = y;
		++s_num_mouse_events;
	}
}
