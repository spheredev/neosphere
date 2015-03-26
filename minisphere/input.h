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
	PLAYER_KEY_Y
};

extern void  initialize_input     (void);
extern void  shutdown_input       (void);
extern bool  is_joy_button_down   (int joy_index, int button);
extern float get_joy_axis         (int joy_index, int axis_index);
extern int   get_joy_axis_count   (int joy_index);
extern int   get_joy_button_count (int joy_index);
extern void  clear_key_queue      (void);
extern void  update_input         (void);

extern void init_input_api (void);
