#ifndef MINISPHERE__INPUT_H__INCLUDED
#define MINISPHERE__INPUT_H__INCLUDED

#define MAX_JOYSTICKS   4
#define MAX_JOY_BUTTONS 32

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
	MOUSE_KEY_WHEEL_UP,
	MOUSE_KEY_WHEEL_DOWN,
	MOUSE_KEY_MAX,
} mouse_key_t;

typedef
struct mouse_event
{
	mouse_key_t key;
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

#endif // MINISPHERE__INPUT_H__INCLUDED
