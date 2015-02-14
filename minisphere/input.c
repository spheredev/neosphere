#include "minisphere.h"
#include "api.h"
#include "input.h"

static duk_ret_t _js_AreKeysLeft     (duk_context* ctx);
static duk_ret_t _js_GetKey          (duk_context* ctx);
static duk_ret_t _js_GetKeyString    (duk_context* ctx);
static duk_ret_t _js_GetPlayerKey    (duk_context* ctx);
static duk_ret_t _js_GetToggleState  (duk_context* ctx);
static duk_ret_t _js_IsKeyPressed    (duk_context* ctx);
static duk_ret_t _js_IsAnyKeyPressed (duk_context* ctx);

void
init_input_api(duk_context* ctx)
{
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
	
	register_api_func(ctx, NULL, "AreKeysLeft", &_js_AreKeysLeft);
	register_api_func(ctx, NULL, "GetKey", &_js_GetKey);
	register_api_func(ctx, NULL, "GetKeyString", &_js_GetKeyString);
	register_api_func(ctx, NULL, "GetPlayerKey", &_js_GetPlayerKey);
	register_api_func(ctx, NULL, "GetToggleState", &_js_GetToggleState);
	register_api_func(ctx, NULL, "IsKeyPressed", &_js_IsKeyPressed);
	register_api_func(ctx, NULL, "IsAnyKeyPressed", &_js_IsAnyKeyPressed);
}

static duk_ret_t
_js_AreKeysLeft(duk_context* ctx)
{
	ALLEGRO_KEYBOARD_STATE kbd_state;
	al_get_keyboard_state(&kbd_state);
	duk_push_false(ctx);
	return 1;
}

static duk_ret_t
_js_GetKey(duk_context* ctx)
{
	return 0;
}

static duk_ret_t
_js_GetKeyString(duk_context* ctx)
{
	duk_push_string(ctx, "");
	return 1;
}

static duk_ret_t
_js_GetPlayerKey(duk_context* ctx)
{
	int key_type;
	int player;
	
	player = duk_require_int(ctx, 0);
	key_type = duk_require_int(ctx, 1);
	switch (key_type) {
		case PLAYER_KEY_MENU: duk_push_int(ctx, ALLEGRO_KEY_ENTER); break;
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
_js_GetToggleState(duk_context* ctx)
{
	duk_push_false(ctx);
	return 1;
}

static duk_ret_t
_js_IsAnyKeyPressed(duk_context* ctx)
{
	duk_push_false(ctx);
	return 1;
}

static duk_ret_t
_js_IsKeyPressed(duk_context* ctx)
{
	int                    code;
	ALLEGRO_KEYBOARD_STATE kb_state;

	code = duk_require_int(ctx, 0);
	al_get_keyboard_state(&kb_state);
	duk_push_boolean(ctx, kb_state.display == g_display
		&& al_key_down(&kb_state, code));
	return 1;
}
