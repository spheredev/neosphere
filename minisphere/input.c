#include "minisphere.h"
#include "api.h"

static duk_ret_t _js_AreKeysLeft     (duk_context* ctx);
static duk_ret_t _js_GetKey          (duk_context* ctx);
static duk_ret_t _js_GetKeyString    (duk_context* ctx);
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
	register_api_func(ctx, NULL, "AreKeysLeft", &_js_AreKeysLeft);
	register_api_func(ctx, NULL, "GetKey", &_js_GetKey);
	register_api_func(ctx, NULL, "GetKeyString", &_js_GetKeyString);
	register_api_func(ctx, NULL, "GetToggleState", &_js_GetToggleState);
	register_api_func(ctx, NULL, "IsKeyPressed", &_js_IsKeyPressed);
	register_api_func(ctx, NULL, "IsAnyKeyPressed", &_js_IsAnyKeyPressed);
}

static duk_ret_t
_js_AreKeysLeft(duk_context* ctx)
{
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
	duk_push_false(ctx);
	return 1;
}
