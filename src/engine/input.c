#include "minisphere.h"
#include "input.h"

#include "debugger.h"
#include "kevfile.h"
#include "vector.h"

struct key_queue
{
	int num_keys;
	int keys[255];
};

static void queue_key         (int keycode);
static void queue_wheel_event (int event);

static vector_t*            s_bound_buttons;
static vector_t*            s_bound_keys;
static int                  s_default_key_map[4][PLAYER_KEY_MAX];
static ALLEGRO_EVENT_QUEUE* s_events;
static bool                 s_have_joystick;
static bool                 s_have_mouse;
static ALLEGRO_JOYSTICK*    s_joy_handles[MAX_JOYSTICKS];
static int                  s_key_map[4][PLAYER_KEY_MAX];
static struct key_queue     s_key_queue;
static bool                 s_key_state[ALLEGRO_KEY_MAX];
static int                  s_keymod_state;
static int                  s_last_wheel_pos = 0;
static int                  s_num_joysticks = 0;
static int                  s_num_wheel_events = 0;
static bool                 s_has_keymap_changed = false;
static mouse_key_t          s_wheel_queue[255];

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

	s_events = al_create_event_queue();
	al_register_event_source(s_events, al_get_keyboard_event_source());
	if (s_have_mouse)
		al_register_event_source(s_events, al_get_mouse_event_source());
	if (s_have_joystick)
		al_register_event_source(s_events, al_get_joystick_event_source());

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
	struct bound_button* pbutton;
	struct bound_key*    pkey;
	
	iter_t iter;

	// save player key mappings
	console_log(1, "shutting down input subsystem");

	// free bound key scripts
	iter = vector_enum(s_bound_buttons);
	while (pbutton = vector_next(&iter)) {
		free_script(pbutton->on_down_script);
		free_script(pbutton->on_up_script);
	}
	iter = vector_enum(s_bound_keys);
	while (pkey = vector_next(&iter)) {
		free_script(pkey->on_down_script);
		free_script(pkey->on_up_script);
	}
	vector_free(s_bound_buttons);
	vector_free(s_bound_keys);

	// shut down Allegro input
	al_destroy_event_queue(s_events);
	al_uninstall_joystick();
	al_uninstall_mouse();
	al_uninstall_keyboard();
}

bool
joy_is_button_down(int joy_index, int button)
{
	ALLEGRO_JOYSTICK_STATE joy_state;
	ALLEGRO_JOYSTICK*      joystick;

	if (!s_have_joystick)
		return false;
	if (!(joystick = s_joy_handles[joy_index]))
		return 0.0;
	al_get_joystick_state(joystick, &joy_state);
	return joy_state.button[button] > 0;
}

int
joy_num_axes(int joy_index)
{
	ALLEGRO_JOYSTICK* joystick;
	int               n_axes;
	int               n_sticks;

	int i;

	if (!s_have_joystick || !(joystick = s_joy_handles[joy_index]))
		return 0;
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

	if (!s_have_joystick || !(joystick = s_joy_handles[joy_index]))
		return 0;
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

	if (!s_have_joystick || !(joystick = s_joy_handles[joy_index]))
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
	bool                 is_new_entry = true;
	struct bound_button* bound;
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
	while (bound = vector_next(&iter)) {
		if (bound->joystick_id == joy_index && bound->button == button) {
			bound->is_pressed = false;
			old_down_script = bound->on_down_script;
			old_up_script = bound->on_up_script;
			memcpy(bound, &new_binding, sizeof(struct bound_button));
			if (old_down_script != bound->on_down_script)
				free_script(old_down_script);
			if (old_up_script != bound->on_up_script)
				free_script(old_up_script);
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
	kevfile_t*     file;
	const char* key_name;
	char*       filename;
	lstring_t*  setting;

	int i, j;

	filename = g_fs != NULL ? "keymap.kev" : "#/minisphere.conf";
	if (!(file = kev_open(g_fs, filename, true)))
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
		setting = lstr_newf("keymap_Player%i_%s", i + 1, key_name);
		s_key_map[i][j] = kev_read_float(file, lstr_cstr(setting), s_default_key_map[i][j]);
		lstr_free(setting);
	}
	kev_close(file);
}

void
kb_save_keymap(void)
{
	kevfile_t*     file;
	const char* key_name;
	lstring_t*  setting;

	int i, j;

	if (!s_has_keymap_changed || g_game_path == NULL)
		return;
	console_log(1, "saving player key mappings");
	file = kev_open(g_fs, "keymap.kev", true);
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
		setting = lstr_newf("keymap_Player%i_%s", i + 1, key_name);
		kev_write_float(file, lstr_cstr(setting), s_key_map[i][j]);
		lstr_free(setting);
	}
	kev_close(file);
}

int
mouse_queue_len(void)
{
	return s_num_wheel_events;
}

mouse_key_t
mouse_get_key(void)
{
	mouse_key_t key;

	int i;

	key = s_wheel_queue[0];
	--s_num_wheel_events;
	for (i = 0; i < s_num_wheel_events; ++i)
		s_wheel_queue[i] = s_wheel_queue[i + 1];
	return key;
}

int
get_player_key(int player, player_key_t vkey)
{
	return s_key_map[player][vkey];
}

void
attach_input_display(void)
{
	al_register_event_source(s_events,
		al_get_display_event_source(screen_display(g_screen)));
}

void
set_player_key(int player, player_key_t vkey, int keycode)
{
	s_key_map[player][vkey] = keycode;
	s_has_keymap_changed = g_game_path != NULL;
}

void
update_bound_keys(bool use_map_keys)
{
	struct bound_button* button;
	struct bound_key*    key;
	bool                 is_down;
	
	iter_t iter;

	// check bound keyboard keys
	if (use_map_keys) {
		iter = vector_enum(s_bound_keys);
		while (key = vector_next(&iter)) {
			is_down = s_key_state[key->keycode];
			if (is_down && !key->is_pressed)
				run_script(key->on_down_script, false);
			if (!is_down && key->is_pressed)
				run_script(key->on_up_script, false);
			key->is_pressed = is_down;
		}
	}

	// check bound joystick buttons
	iter = vector_enum(s_bound_buttons);
	while (button = vector_next(&iter)) {
		is_down = joy_is_button_down(button->joystick_id, button->button);
		if (is_down && !button->is_pressed)
			run_script(button->on_down_script, false);
		if (!is_down && button->is_pressed)
			run_script(button->on_up_script, false);
		button->is_pressed = is_down;
	}
}

void
update_input(void)
{
	ALLEGRO_EVENT          event;
	int                    keycode;
	ALLEGRO_MOUSE_STATE    mouse_state;
	
	// process Allegro input events
	while (al_get_next_event(s_events, &event)) {
		switch (event.type) {
		case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
			// Alt+Tabbing out can cause keys to get "stuck", this works around it
			// by clearing the states when switching away.
			memset(s_key_state, 0, ALLEGRO_KEY_MAX * sizeof(bool));
			break;
		case ALLEGRO_EVENT_KEY_DOWN:
			keycode = event.keyboard.keycode;
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
			s_key_state[event.keyboard.keycode] = false;
			break;
		case ALLEGRO_EVENT_KEY_CHAR:
			s_keymod_state = event.keyboard.modifiers;
			switch (event.keyboard.keycode) {
			case ALLEGRO_KEY_ENTER:
				if (event.keyboard.modifiers & ALLEGRO_KEYMOD_ALT
				 || event.keyboard.modifiers & ALLEGRO_KEYMOD_ALTGR)
				{
					screen_toggle_fullscreen(g_screen);
				}
				else {
					queue_key(event.keyboard.keycode);
				}
				break;
			case ALLEGRO_KEY_F10:
				screen_toggle_fullscreen(g_screen);
				break;
			case ALLEGRO_KEY_F11:
				screen_toggle_fps(g_screen);
				break;
			case ALLEGRO_KEY_F12:
				if (is_debugger_attached())
					duk_debugger_pause(g_duk);
				else
					screen_queue_screenshot(g_screen);
				break;
			default:
				queue_key(event.keyboard.keycode);
				break;
			}
		}
	}
	
	// check whether mouse wheel moved since last update
	if (s_have_mouse) {
		al_get_mouse_state(&mouse_state);
		if (mouse_state.z > s_last_wheel_pos)
			queue_wheel_event(MOUSE_KEY_WHEEL_UP);
		if (mouse_state.z < s_last_wheel_pos)
			queue_wheel_event(MOUSE_KEY_WHEEL_DOWN);
		s_last_wheel_pos = mouse_state.z;
	}
}

void
kb_bind_key(int keycode, script_t* on_down_script, script_t* on_up_script)
{
	bool              is_new_key = true;
	struct bound_key* key;
	struct bound_key  new_binding;
	script_t*         old_down_script;
	script_t*         old_up_script;

	iter_t iter;

	new_binding.keycode = keycode;
	new_binding.is_pressed = false;
	new_binding.on_down_script = on_down_script;
	new_binding.on_up_script = on_up_script;
	iter = vector_enum(s_bound_keys);
	while (key = vector_next(&iter)) {
		if (key->keycode == keycode) {
			key->is_pressed = false;
			old_down_script = key->on_down_script;
			old_up_script = key->on_up_script;
			memcpy(key, &new_binding, sizeof(struct bound_key));
			if (old_down_script != key->on_down_script)
				free_script(old_down_script);
			if (old_up_script != key->on_up_script)
				free_script(old_up_script);
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
queue_wheel_event(int event)
{
	if (s_num_wheel_events < 255) {
		s_wheel_queue[s_num_wheel_events] = event;
		++s_num_wheel_events;
	}
}
