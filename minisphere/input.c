#include "minisphere.h"
#include "api.h"
#include "input.h"

static duk_ret_t js_AreKeysLeft     (duk_context* ctx);
static duk_ret_t js_IsAnyKeyPressed (duk_context* ctx);
static duk_ret_t js_IsKeyPressed    (duk_context* ctx);
static duk_ret_t js_GetKey          (duk_context* ctx);
static duk_ret_t js_GetKeyString    (duk_context* ctx);
static duk_ret_t js_GetPlayerKey    (duk_context* ctx);
static duk_ret_t js_GetToggleState  (duk_context* ctx);
static duk_ret_t js_BindKey         (duk_context* ctx);
static duk_ret_t js_UnbindKey       (duk_context* ctx);

static int  s_key_down_scripts[ALLEGRO_KEY_MAX];
static int  s_key_up_scripts[ALLEGRO_KEY_MAX];
static bool s_last_key_state[ALLEGRO_KEY_MAX];

void
init_input_api(duk_context* ctx)
{
	memset(s_key_down_scripts, 0, ALLEGRO_KEY_MAX * sizeof(int));
	memset(s_key_up_scripts, 0, ALLEGRO_KEY_MAX * sizeof(int));
	memset(s_last_key_state, 0, ALLEGRO_KEY_MAX * sizeof(bool));

	register_api_const(ctx, "PLAYER_1", 0);
	register_api_const(ctx, "PLAYER_2", 1);
	register_api_const(ctx, "PLAYER_3", 2);
	register_api_const(ctx, "PLAYER_4", 3);
	register_api_const(ctx, "PLAYER_KEY_MENU", PLAYER_KEY_MENU);
	register_api_const(ctx, "PLAYER_KEY_UP", PLAYER_KEY_UP);
	register_api_const(ctx, "PLAYER_KEY_DOWN", PLAYER_KEY_DOWN);
	register_api_const(ctx, "PLAYER_KEY_LEFT", PLAYER_KEY_LEFT);
	register_api_const(ctx, "PLAYER_KEY_RIGHT", PLAYER_KEY_RIGHT);
	register_api_const(ctx, "PLAYER_KEY_A", PLAYER_KEY_A);
	register_api_const(ctx, "PLAYER_KEY_B", PLAYER_KEY_B);
	register_api_const(ctx, "PLAYER_KEY_X", PLAYER_KEY_X);
	register_api_const(ctx, "PLAYER_KEY_Y", PLAYER_KEY_Y);
	register_api_const(ctx, "KEY_SHIFT", ALLEGRO_KEY_LSHIFT);
	register_api_const(ctx, "KEY_CTRL", ALLEGRO_KEY_LCTRL);
	register_api_const(ctx, "KEY_ALT", ALLEGRO_KEY_ALT);
	register_api_const(ctx, "KEY_TAB", ALLEGRO_KEY_TAB);
	register_api_const(ctx, "KEY_SPACE", ALLEGRO_KEY_SPACE);
	register_api_const(ctx, "KEY_ENTER", ALLEGRO_KEY_ENTER);
	register_api_const(ctx, "KEY_ESCAPE", ALLEGRO_KEY_ESCAPE);
	register_api_const(ctx, "KEY_UP", ALLEGRO_KEY_UP);
	register_api_const(ctx, "KEY_DOWN", ALLEGRO_KEY_DOWN);
	register_api_const(ctx, "KEY_LEFT", ALLEGRO_KEY_LEFT);
	register_api_const(ctx, "KEY_RIGHT", ALLEGRO_KEY_RIGHT);
	register_api_const(ctx, "KEY_PAGEUP", ALLEGRO_KEY_PGUP);
	register_api_const(ctx, "KEY_PAGEDOWN", ALLEGRO_KEY_PGDN);
	register_api_const(ctx, "KEY_HOME", ALLEGRO_KEY_HOME);
	register_api_const(ctx, "KEY_END", ALLEGRO_KEY_END);
	register_api_const(ctx, "KEY_INSERT", ALLEGRO_KEY_INSERT);
	register_api_const(ctx, "KEY_DELETE", ALLEGRO_KEY_DELETE);
	register_api_const(ctx, "KEY_BACKSPACE", ALLEGRO_KEY_BACKSPACE);
	register_api_const(ctx, "KEY_TILDE", ALLEGRO_KEY_TILDE);
	register_api_const(ctx, "KEY_F1", ALLEGRO_KEY_F1);
	register_api_const(ctx, "KEY_F2", ALLEGRO_KEY_F2);
	register_api_const(ctx, "KEY_F3", ALLEGRO_KEY_F3);
	register_api_const(ctx, "KEY_F4", ALLEGRO_KEY_F4);
	register_api_const(ctx, "KEY_F5", ALLEGRO_KEY_F5);
	register_api_const(ctx, "KEY_F6", ALLEGRO_KEY_F6);
	register_api_const(ctx, "KEY_F7", ALLEGRO_KEY_F7);
	register_api_const(ctx, "KEY_F8", ALLEGRO_KEY_F8);
	register_api_const(ctx, "KEY_F9", ALLEGRO_KEY_F9);
	register_api_const(ctx, "KEY_F10", ALLEGRO_KEY_F10);
	register_api_const(ctx, "KEY_F11", ALLEGRO_KEY_F11);
	register_api_const(ctx, "KEY_F12", ALLEGRO_KEY_F12);
	register_api_const(ctx, "KEY_0", ALLEGRO_KEY_0);
	register_api_const(ctx, "KEY_1", ALLEGRO_KEY_1);
	register_api_const(ctx, "KEY_2", ALLEGRO_KEY_2);
	register_api_const(ctx, "KEY_3", ALLEGRO_KEY_3);
	register_api_const(ctx, "KEY_4", ALLEGRO_KEY_4);
	register_api_const(ctx, "KEY_5", ALLEGRO_KEY_5);
	register_api_const(ctx, "KEY_6", ALLEGRO_KEY_6);
	register_api_const(ctx, "KEY_7", ALLEGRO_KEY_7);
	register_api_const(ctx, "KEY_8", ALLEGRO_KEY_8);
	register_api_const(ctx, "KEY_9", ALLEGRO_KEY_9);
	register_api_const(ctx, "KEY_A", ALLEGRO_KEY_A);
	register_api_const(ctx, "KEY_B", ALLEGRO_KEY_B);
	register_api_const(ctx, "KEY_C", ALLEGRO_KEY_C);
	register_api_const(ctx, "KEY_D", ALLEGRO_KEY_D);
	register_api_const(ctx, "KEY_E", ALLEGRO_KEY_E);
	register_api_const(ctx, "KEY_F", ALLEGRO_KEY_F);
	register_api_const(ctx, "KEY_G", ALLEGRO_KEY_G);
	register_api_const(ctx, "KEY_H", ALLEGRO_KEY_H);
	register_api_const(ctx, "KEY_I", ALLEGRO_KEY_I);
	register_api_const(ctx, "KEY_J", ALLEGRO_KEY_J);
	register_api_const(ctx, "KEY_K", ALLEGRO_KEY_K);
	register_api_const(ctx, "KEY_L", ALLEGRO_KEY_L);
	register_api_const(ctx, "KEY_M", ALLEGRO_KEY_M);
	register_api_const(ctx, "KEY_N", ALLEGRO_KEY_N);
	register_api_const(ctx, "KEY_O", ALLEGRO_KEY_O);
	register_api_const(ctx, "KEY_P", ALLEGRO_KEY_P);
	register_api_const(ctx, "KEY_Q", ALLEGRO_KEY_Q);
	register_api_const(ctx, "KEY_R", ALLEGRO_KEY_R);
	register_api_const(ctx, "KEY_S", ALLEGRO_KEY_S);
	register_api_const(ctx, "KEY_T", ALLEGRO_KEY_T);
	register_api_const(ctx, "KEY_U", ALLEGRO_KEY_U);
	register_api_const(ctx, "KEY_V", ALLEGRO_KEY_V);
	register_api_const(ctx, "KEY_W", ALLEGRO_KEY_W);
	register_api_const(ctx, "KEY_X", ALLEGRO_KEY_X);
	register_api_const(ctx, "KEY_Y", ALLEGRO_KEY_Y);
	register_api_const(ctx, "KEY_Z", ALLEGRO_KEY_Z);

	register_api_func(ctx, NULL, "AreKeysLeft", js_AreKeysLeft);
	register_api_func(ctx, NULL, "IsAnyKeyPressed", js_IsAnyKeyPressed);
	register_api_func(ctx, NULL, "IsKeyPressed", js_IsKeyPressed);
	register_api_func(ctx, NULL, "GetKey", js_GetKey);
	register_api_func(ctx, NULL, "GetKeyString", js_GetKeyString);
	register_api_func(ctx, NULL, "GetPlayerKey", js_GetPlayerKey);
	register_api_func(ctx, NULL, "GetToggleState", js_GetToggleState);
	register_api_func(ctx, NULL, "BindKey", js_BindKey);
	register_api_func(ctx, NULL, "UnbindKey", js_UnbindKey);
}

void
check_bound_keys(void)
{
	bool                   is_down;
	ALLEGRO_KEYBOARD_STATE kb_state;

	int i;

	al_get_keyboard_state(&kb_state);
	for (i = 0; i < ALLEGRO_KEY_MAX; ++i) {
		is_down = al_key_down(&kb_state, i);
		if (is_down && !s_last_key_state[i])
			run_script(s_key_down_scripts[i], false);
		else if (!is_down && s_last_key_state[i])
			run_script(s_key_up_scripts[i], false);
		s_last_key_state[i] = is_down;
	}
}

static duk_ret_t
js_AreKeysLeft(duk_context* ctx)
{
	duk_push_boolean(ctx, g_key_queue.num_keys > 0);
	return 1;
}

static duk_ret_t
js_IsAnyKeyPressed(duk_context* ctx)
{
	ALLEGRO_KEYBOARD_STATE kb_state;

	int i_key;
	
	al_get_keyboard_state(&kb_state);
	for (i_key = 0; i_key < ALLEGRO_KEY_MAX; ++i_key) {
		if (al_key_down(&kb_state, i_key)) {
			duk_push_true(ctx);
			return 1;
		}
	}
	duk_push_false(ctx);
	return 1;
}

static duk_ret_t
js_IsKeyPressed(duk_context* ctx)
{
	int                    code;
	ALLEGRO_KEYBOARD_STATE kb_state;

	code = duk_require_int(ctx, 0);
	al_get_keyboard_state(&kb_state);
	duk_push_boolean(ctx, kb_state.display == g_display
		&& al_key_down(&kb_state, code));
	return 1;
}

static duk_ret_t
js_GetKey(duk_context* ctx)
{
	int keycode;

	while (g_key_queue.num_keys <= 0) {
		if (!do_events()) bail_out_game();
	}
	keycode = g_key_queue.keys[0];
	--g_key_queue.num_keys;
	memmove(g_key_queue.keys, &g_key_queue.keys[1], sizeof(int) * g_key_queue.num_keys);
	duk_push_int(ctx, keycode);
	return 1;
}

static duk_ret_t
js_GetKeyString(duk_context* ctx)
{
	int keycode = duk_require_int(ctx, 0);
	bool shift = duk_require_boolean(ctx, 1);

	switch (keycode) {
	case ALLEGRO_KEY_A: duk_push_string(ctx, shift ? "A" : "a"); break;
	case ALLEGRO_KEY_B: duk_push_string(ctx, shift ? "B" : "b"); break;
	case ALLEGRO_KEY_C: duk_push_string(ctx, shift ? "C" : "c"); break;
	case ALLEGRO_KEY_D: duk_push_string(ctx, shift ? "D" : "d"); break;
	case ALLEGRO_KEY_E: duk_push_string(ctx, shift ? "E" : "e"); break;
	case ALLEGRO_KEY_F: duk_push_string(ctx, shift ? "F" : "f"); break;
	case ALLEGRO_KEY_G: duk_push_string(ctx, shift ? "G" : "g"); break;
	case ALLEGRO_KEY_H: duk_push_string(ctx, shift ? "H" : "h"); break;
	case ALLEGRO_KEY_I: duk_push_string(ctx, shift ? "I" : "i"); break;
	case ALLEGRO_KEY_J: duk_push_string(ctx, shift ? "J" : "j"); break;
	case ALLEGRO_KEY_K: duk_push_string(ctx, shift ? "K" : "k"); break;
	case ALLEGRO_KEY_L: duk_push_string(ctx, shift ? "L" : "l"); break;
	case ALLEGRO_KEY_M: duk_push_string(ctx, shift ? "M" : "m"); break;
	case ALLEGRO_KEY_N: duk_push_string(ctx, shift ? "N" : "n"); break;
	case ALLEGRO_KEY_O: duk_push_string(ctx, shift ? "O" : "o"); break;
	case ALLEGRO_KEY_P: duk_push_string(ctx, shift ? "P" : "p"); break;
	case ALLEGRO_KEY_Q: duk_push_string(ctx, shift ? "Q" : "q"); break;
	case ALLEGRO_KEY_R: duk_push_string(ctx, shift ? "R" : "r"); break;
	case ALLEGRO_KEY_S: duk_push_string(ctx, shift ? "S" : "s"); break;
	case ALLEGRO_KEY_T: duk_push_string(ctx, shift ? "T" : "t"); break;
	case ALLEGRO_KEY_U: duk_push_string(ctx, shift ? "U" : "u"); break;
	case ALLEGRO_KEY_V: duk_push_string(ctx, shift ? "V" : "v"); break;
	case ALLEGRO_KEY_W: duk_push_string(ctx, shift ? "W" : "w"); break;
	case ALLEGRO_KEY_X: duk_push_string(ctx, shift ? "X" : "x"); break;
	case ALLEGRO_KEY_Y: duk_push_string(ctx, shift ? "Y" : "y"); break;
	case ALLEGRO_KEY_Z: duk_push_string(ctx, shift ? "Z" : "z"); break;
	case ALLEGRO_KEY_1: duk_push_string(ctx, shift ? "1" : "!"); break;
	case ALLEGRO_KEY_2: duk_push_string(ctx, shift ? "2" : "@"); break;
	case ALLEGRO_KEY_3: duk_push_string(ctx, shift ? "3" : "#"); break;
	case ALLEGRO_KEY_4: duk_push_string(ctx, shift ? "4" : "$"); break;
	case ALLEGRO_KEY_5: duk_push_string(ctx, shift ? "5" : "%"); break;
	case ALLEGRO_KEY_6: duk_push_string(ctx, shift ? "6" : "^"); break;
	case ALLEGRO_KEY_7: duk_push_string(ctx, shift ? "7" : "&"); break;
	case ALLEGRO_KEY_8: duk_push_string(ctx, shift ? "8" : "*"); break;
	case ALLEGRO_KEY_9: duk_push_string(ctx, shift ? "9" : "("); break;
	case ALLEGRO_KEY_0: duk_push_string(ctx, shift ? "0" : ")"); break;
	case ALLEGRO_KEY_BACKSLASH: duk_push_string(ctx, shift ? "\\" : "|"); break;
	case ALLEGRO_KEY_CLOSEBRACE: duk_push_string(ctx, shift ? "." : ">"); break;
	case ALLEGRO_KEY_COMMA: duk_push_string(ctx, shift ? "," : "<"); break;
	case ALLEGRO_KEY_EQUALS: duk_push_string(ctx, shift ? "=" : "+"); break;
	case ALLEGRO_KEY_MINUS: duk_push_string(ctx, shift ? "-" : "_"); break;
	case ALLEGRO_KEY_QUOTE: duk_push_string(ctx, shift ? "'" : "\""); break;
	case ALLEGRO_KEY_SEMICOLON: duk_push_string(ctx, shift ? ";" : ":"); break;
	case ALLEGRO_KEY_SLASH: duk_push_string(ctx, shift ? "/" : "?"); break;
	case ALLEGRO_KEY_SPACE: duk_push_string(ctx, " "); break;
	case ALLEGRO_KEY_TAB: duk_push_string(ctx, "\t"); break;
	case ALLEGRO_KEY_TILDE: duk_push_string(ctx, shift ? "`" : "~"); break;
	default:
		duk_push_string(ctx, "");
	}
	return 1;
}

static duk_ret_t
js_GetPlayerKey(duk_context* ctx)
{
	int key_type;
	int player;

	player = duk_require_int(ctx, 0);
	key_type = duk_require_int(ctx, 1);
	switch (key_type) {
	case PLAYER_KEY_MENU: duk_push_int(ctx, ALLEGRO_KEY_ESCAPE); break;
	case PLAYER_KEY_UP: duk_push_int(ctx, ALLEGRO_KEY_UP); break;
	case PLAYER_KEY_DOWN: duk_push_int(ctx, ALLEGRO_KEY_DOWN); break;
	case PLAYER_KEY_LEFT: duk_push_int(ctx, ALLEGRO_KEY_LEFT); break;
	case PLAYER_KEY_RIGHT: duk_push_int(ctx, ALLEGRO_KEY_RIGHT); break;
	case PLAYER_KEY_A: duk_push_int(ctx, ALLEGRO_KEY_Z); break;
	case PLAYER_KEY_B: duk_push_int(ctx, ALLEGRO_KEY_X); break;
	case PLAYER_KEY_X: duk_push_int(ctx, ALLEGRO_KEY_C); break;
	case PLAYER_KEY_Y: duk_push_int(ctx, ALLEGRO_KEY_V); break;
	}
	return 1;
}

static duk_ret_t
js_GetToggleState(duk_context* ctx)
{
	duk_push_false(ctx);
	return 1;
}

static js_BindKey(duk_context* ctx)
{
	int keycode = duk_require_int(ctx, 0);
	lstring_t* key_down_script = !duk_is_null(ctx, 1)
		? duk_require_lstring_t(ctx, 1) : lstring_from_cstr("");
	lstring_t* key_up_script = !duk_is_null(ctx, 2)
		? duk_require_lstring_t(ctx, 2) : lstring_from_cstr("");

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "BindKey(): Invalid key constant");
	s_key_down_scripts[keycode] = compile_script(key_down_script, "[key-down script]");
	s_key_up_scripts[keycode] = compile_script(key_up_script, "[key-down script]");
	free_lstring(key_down_script);
	free_lstring(key_up_script);
	return 0;
}

static duk_ret_t
js_UnbindKey(duk_context* ctx)
{
	int keycode = duk_require_int(ctx, 0);

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "UnbindKey(): Invalid key constant");
	free_script(s_key_down_scripts[keycode]);
	free_script(s_key_up_scripts[keycode]);
	s_key_down_scripts[keycode] = 0;
	s_key_up_scripts[keycode] = 0;
	return 0;
}
