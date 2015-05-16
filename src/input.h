typedef enum player_key player_key_t;

extern void  initialize_input     (void);
extern void  shutdown_input       (void);
extern bool  is_any_key_down      (void);
extern bool  is_joy_button_down   (int joy_index, int button);
extern bool  is_key_down          (int keycode);
extern float get_joy_axis         (int joy_index, int axis_index);
extern int   get_joy_axis_count   (int joy_index);
extern int   get_joy_button_count (int joy_index);
extern int   get_player_key       (int player, player_key_t vkey);
extern void  set_player_key       (int player, player_key_t vkey, int keycode);
extern void  clear_key_queue      (void);
extern void  load_key_map         (void);
extern void  save_key_map         (void);
extern void  update_bound_keys    (bool use_map_keys);
extern void  update_input         (void);

extern void init_input_api (void);

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
};
