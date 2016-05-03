#ifndef MINISPHERE__INPUT_H__INCLUDED
#define MINISPHERE__INPUT_H__INCLUDED

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

void  initialize_input     (void);
void  shutdown_input       (void);
bool  is_any_key_down      (void);
bool  is_joy_button_down   (int joy_index, int button);
bool  is_key_down          (int keycode);
float get_joy_axis         (int joy_index, int axis_index);
int   get_joy_axis_count   (int joy_index);
int   get_joy_button_count (int joy_index);
int   get_player_key       (int player, player_key_t vkey);
void  set_player_key       (int player, player_key_t vkey, int keycode);
void  attach_input_display (void);
void  clear_key_queue      (void);
void  load_key_map         (void);
void  save_key_map         (void);
void  update_bound_keys    (bool use_map_keys);
void  update_input         (void);

void init_input_api (void);

#endif // MINISPHERE__INPUT_H__INCLUDED
