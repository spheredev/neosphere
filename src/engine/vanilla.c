#include "minisphere.h"
#include "vanilla.h"

#include "animation.h"
#include "api.h"
#include "async.h"
#include "audio.h"
#include "bytearray.h"
#include "debugger.h"
#include "font.h"
#include "logger.h"
#include "sockets.h"
#include "spriteset.h"
#include "windowstyle.h"

static duk_ret_t js_IsSkippedFrame             (duk_context* ctx);
static duk_ret_t js_GetClippingRectangle       (duk_context* ctx);
static duk_ret_t js_GetFrameRate               (duk_context* ctx);
static duk_ret_t js_GetGameList                (duk_context* ctx);
static duk_ret_t js_GetGameManifest            (duk_context* ctx);
static duk_ret_t js_GetLocalAddress            (duk_context* ctx);
static duk_ret_t js_GetLocalName               (duk_context* ctx);
static duk_ret_t js_GetMaxFrameSkips           (duk_context* ctx);
static duk_ret_t js_GetScreenHeight            (duk_context* ctx);
static duk_ret_t js_GetScreenWidth             (duk_context* ctx);
static duk_ret_t js_GetSystemFont              (duk_context* ctx);
static duk_ret_t js_GetSystemWindowStyle       (duk_context* ctx);
static duk_ret_t js_GetTime                    (duk_context* ctx);
static duk_ret_t js_GetVersion                 (duk_context* ctx);
static duk_ret_t js_GetVersionString           (duk_context* ctx);
static duk_ret_t js_SetClippingRectangle       (duk_context* ctx);
static duk_ret_t js_SetFrameRate               (duk_context* ctx);
static duk_ret_t js_SetMaxFrameSkips           (duk_context* ctx);
static duk_ret_t js_SetScreenSize              (duk_context* ctx);
static duk_ret_t js_Abort                      (duk_context* ctx);
static duk_ret_t js_Alert                      (duk_context* ctx);
static duk_ret_t js_ApplyColorMask             (duk_context* ctx);
static duk_ret_t js_Assert                     (duk_context* ctx);
static duk_ret_t js_CreateStringFromCode       (duk_context* ctx);
static duk_ret_t js_CreateSurface              (duk_context* ctx);
static duk_ret_t js_DebugPrint                 (duk_context* ctx);
static duk_ret_t js_Delay                      (duk_context* ctx);
static duk_ret_t js_DispatchScript             (duk_context* ctx);
static duk_ret_t js_DoEvents                   (duk_context* ctx);
static duk_ret_t js_EvaluateScript             (duk_context* ctx);
static duk_ret_t js_EvaluateSystemScript       (duk_context* ctx);
static duk_ret_t js_ExecuteGame                (duk_context* ctx);
static duk_ret_t js_Exit                       (duk_context* ctx);
static duk_ret_t js_FlipScreen                 (duk_context* ctx);
static duk_ret_t js_GarbageCollect             (duk_context* ctx);
static duk_ret_t js_GrabSurface                (duk_context* ctx);
static duk_ret_t js_GradientCircle             (duk_context* ctx);
static duk_ret_t js_GradientRectangle          (duk_context* ctx);
static duk_ret_t js_Line                       (duk_context* ctx);
static duk_ret_t js_LineSeries                 (duk_context* ctx);
static duk_ret_t js_ListenOnPort               (duk_context* ctx);
static duk_ret_t js_LoadAnimation              (duk_context* ctx);
static duk_ret_t js_LoadFont                   (duk_context* ctx);
static duk_ret_t js_LoadSound                  (duk_context* ctx);
static duk_ret_t js_LoadSpriteset              (duk_context* ctx);
static duk_ret_t js_LoadSurface                (duk_context* ctx);
static duk_ret_t js_LoadWindowStyle            (duk_context* ctx);
static duk_ret_t js_OpenAddress                (duk_context* ctx);
static duk_ret_t js_OpenLog                    (duk_context* ctx);
static duk_ret_t js_OutlinedCircle             (duk_context* ctx);
static duk_ret_t js_OutlinedRectangle          (duk_context* ctx);
static duk_ret_t js_OutlinedRoundRectangle     (duk_context* ctx);
static duk_ret_t js_Point                      (duk_context* ctx);
static duk_ret_t js_PointSeries                (duk_context* ctx);
static duk_ret_t js_Print                      (duk_context* ctx);
static duk_ret_t js_Rectangle                  (duk_context* ctx);
static duk_ret_t js_RequireScript              (duk_context* ctx);
static duk_ret_t js_RequireSystemScript        (duk_context* ctx);
static duk_ret_t js_RestartGame                (duk_context* ctx);
static duk_ret_t js_RoundRectangle             (duk_context* ctx);
static duk_ret_t js_Triangle                   (duk_context* ctx);
static duk_ret_t js_UnskipFrame                (duk_context* ctx);
static duk_ret_t js_Animation_finalize         (duk_context* ctx);
static duk_ret_t js_Animation_get_height       (duk_context* ctx);
static duk_ret_t js_Animation_get_width        (duk_context* ctx);
static duk_ret_t js_Animation_getDelay         (duk_context* ctx);
static duk_ret_t js_Animation_getNumFrames     (duk_context* ctx);
static duk_ret_t js_Animation_drawFrame        (duk_context* ctx);
static duk_ret_t js_Animation_drawZoomedFrame  (duk_context* ctx);
static duk_ret_t js_Animation_readNextFrame    (duk_context* ctx);
static duk_ret_t js_Font_finalize              (duk_context* ctx);
static duk_ret_t js_Font_toString              (duk_context* ctx);
static duk_ret_t js_Font_get_colorMask         (duk_context* ctx);
static duk_ret_t js_Font_set_colorMask         (duk_context* ctx);
static duk_ret_t js_Font_get_height            (duk_context* ctx);
static duk_ret_t js_Font_getCharacterImage     (duk_context* ctx);
static duk_ret_t js_Font_setCharacterImage     (duk_context* ctx);
static duk_ret_t js_Font_getStringHeight       (duk_context* ctx);
static duk_ret_t js_Font_getStringWidth        (duk_context* ctx);
static duk_ret_t js_Font_clone                 (duk_context* ctx);
static duk_ret_t js_Font_drawText              (duk_context* ctx);
static duk_ret_t js_Font_drawTextBox           (duk_context* ctx);
static duk_ret_t js_Font_drawZoomedText        (duk_context* ctx);
static duk_ret_t js_Font_wordWrapString        (duk_context* ctx);
static duk_ret_t js_Logger_finalize            (duk_context* ctx);
static duk_ret_t js_Logger_toString            (duk_context* ctx);
static duk_ret_t js_Logger_beginBlock          (duk_context* ctx);
static duk_ret_t js_Logger_endBlock            (duk_context* ctx);
static duk_ret_t js_Logger_write               (duk_context* ctx);
static duk_ret_t js_Socket_finalize            (duk_context* ctx);
static duk_ret_t js_Socket_get_remoteAddress   (duk_context* ctx);
static duk_ret_t js_Socket_get_remotePort      (duk_context* ctx);
static duk_ret_t js_Socket_toString            (duk_context* ctx);
static duk_ret_t js_Socket_isConnected         (duk_context* ctx);
static duk_ret_t js_Socket_getPendingReadSize  (duk_context* ctx);
static duk_ret_t js_Socket_close               (duk_context* ctx);
static duk_ret_t js_Socket_read                (duk_context* ctx);
static duk_ret_t js_Socket_readString          (duk_context* ctx);
static duk_ret_t js_Socket_write               (duk_context* ctx);
static duk_ret_t js_Sound_finalize             (duk_context* ctx);
static duk_ret_t js_Sound_toString             (duk_context* ctx);
static duk_ret_t js_Sound_getVolume            (duk_context* ctx);
static duk_ret_t js_Sound_setVolume            (duk_context* ctx);
static duk_ret_t js_Sound_get_length           (duk_context* ctx);
static duk_ret_t js_Sound_get_pan              (duk_context* ctx);
static duk_ret_t js_Sound_set_pan              (duk_context* ctx);
static duk_ret_t js_Sound_get_pitch            (duk_context* ctx);
static duk_ret_t js_Sound_set_pitch            (duk_context* ctx);
static duk_ret_t js_Sound_get_playing          (duk_context* ctx);
static duk_ret_t js_Sound_get_position         (duk_context* ctx);
static duk_ret_t js_Sound_set_position         (duk_context* ctx);
static duk_ret_t js_Sound_get_repeat           (duk_context* ctx);
static duk_ret_t js_Sound_set_repeat           (duk_context* ctx);
static duk_ret_t js_Sound_get_seekable         (duk_context* ctx);
static duk_ret_t js_Sound_get_volume           (duk_context* ctx);
static duk_ret_t js_Sound_set_volume           (duk_context* ctx);
static duk_ret_t js_Sound_pause                (duk_context* ctx);
static duk_ret_t js_Sound_play                 (duk_context* ctx);
static duk_ret_t js_Sound_reset                (duk_context* ctx);
static duk_ret_t js_Sound_stop                 (duk_context* ctx);
static duk_ret_t js_Spriteset_finalize         (duk_context* ctx);
static duk_ret_t js_Spriteset_get_filename     (duk_context* ctx);
static duk_ret_t js_Spriteset_toString         (duk_context* ctx);
static duk_ret_t js_Spriteset_clone            (duk_context* ctx);
static duk_ret_t js_Spriteset_get_image        (duk_context* ctx);
static duk_ret_t js_Spriteset_set_image        (duk_context* ctx);
static duk_ret_t js_Surface_finalize           (duk_context* ctx);
static duk_ret_t js_Surface_get_height         (duk_context* ctx);
static duk_ret_t js_Surface_get_width          (duk_context* ctx);
static duk_ret_t js_Surface_toString           (duk_context* ctx);
static duk_ret_t js_Surface_getPixel           (duk_context* ctx);
static duk_ret_t js_Surface_setAlpha           (duk_context* ctx);
static duk_ret_t js_Surface_setBlendMode       (duk_context* ctx);
static duk_ret_t js_Surface_setPixel           (duk_context* ctx);
static duk_ret_t js_Surface_applyColorFX       (duk_context* ctx);
static duk_ret_t js_Surface_applyColorFX4      (duk_context* ctx);
static duk_ret_t js_Surface_applyLookup        (duk_context* ctx);
static duk_ret_t js_Surface_blit               (duk_context* ctx);
static duk_ret_t js_Surface_blitMaskSurface    (duk_context* ctx);
static duk_ret_t js_Surface_blitSurface        (duk_context* ctx);
static duk_ret_t js_Surface_clone              (duk_context* ctx);
static duk_ret_t js_Surface_cloneSection       (duk_context* ctx);
static duk_ret_t js_Surface_createImage        (duk_context* ctx);
static duk_ret_t js_Surface_drawText           (duk_context* ctx);
static duk_ret_t js_Surface_filledCircle       (duk_context* ctx);
static duk_ret_t js_Surface_flipHorizontally   (duk_context* ctx);
static duk_ret_t js_Surface_flipVertically     (duk_context* ctx);
static duk_ret_t js_Surface_gradientCircle     (duk_context* ctx);
static duk_ret_t js_Surface_gradientRectangle  (duk_context* ctx);
static duk_ret_t js_Surface_line               (duk_context* ctx);
static duk_ret_t js_Surface_outlinedCircle     (duk_context* ctx);
static duk_ret_t js_Surface_outlinedRectangle  (duk_context* ctx);
static duk_ret_t js_Surface_pointSeries        (duk_context* ctx);
static duk_ret_t js_Surface_rotate             (duk_context* ctx);
static duk_ret_t js_Surface_rectangle          (duk_context* ctx);
static duk_ret_t js_Surface_replaceColor       (duk_context* ctx);
static duk_ret_t js_Surface_rescale            (duk_context* ctx);
static duk_ret_t js_Surface_save               (duk_context* ctx);
static duk_ret_t js_WindowStyle_finalize       (duk_context* ctx);
static duk_ret_t js_WindowStyle_get_colorMask  (duk_context* ctx);
static duk_ret_t js_WindowStyle_set_colorMask  (duk_context* ctx);
static duk_ret_t js_WindowStyle_toString       (duk_context* ctx);
static duk_ret_t js_WindowStyle_drawWindow     (duk_context* ctx);

enum blend_mode
{
	BLEND_BLEND,
	BLEND_REPLACE,
	BLEND_RGB_ONLY,
	BLEND_ALPHA_ONLY,
	BLEND_ADD,
	BLEND_SUBTRACT,
	BLEND_MULTIPLY,
	BLEND_AVERAGE,
	BLEND_INVERT,
	BLEND_MAX
};

enum line_series_type
{
	LINE_MULTIPLE,
	LINE_STRIP,
	LINE_LOOP
};

static mixer_t*       s_def_mixer;
static unsigned int   s_next_async_id = 1;
static windowstyle_t* s_sys_winstyle;

void
initialize_vanilla_api(duk_context* ctx)
{
	const char* filename;
	
	console_log(1, "initializing Vanilla API (v1.5)");

	s_def_mixer = mixer_new(44100, 16, 2);
	
	// load system window style
	if (g_sys_conf != NULL) {
		filename = kev_read_string(g_sys_conf, "WindowStyle", "system.rws");
		s_sys_winstyle = load_windowstyle(systempath(filename));
	}
	
	// add polyfills for __defineGetter__/__defineSetter__.  Sphere 1.x predates ES5
	// and as a result there are a lot of games that expect these to exist.
	duk_eval_string(ctx, "Object.defineProperty(Object.prototype, '__defineGetter__', { value: function(name, func) {"
		"Object.defineProperty(this, name, { get: func, configurable: true }); } });");
	duk_eval_string(ctx, "Object.defineProperty(Object.prototype, '__defineSetter__', { value: function(name, func) {"
		"Object.defineProperty(this, name, { set: func, configurable: true }); } });");

	// set up a dictionary to track RequireScript() calls
	duk_push_global_stash(ctx);
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, "RequireScript");
	duk_pop(ctx);

	// register the absolutely massive Sphere 1.x API
	api_register_static_func(ctx, NULL, "IsSkippedFrame", js_IsSkippedFrame);
	api_register_static_func(ctx, NULL, "GetClippingRectangle", js_GetClippingRectangle);
	api_register_static_func(ctx, NULL, "GetFrameRate", js_GetFrameRate);
	api_register_static_func(ctx, NULL, "GetGameManifest", js_GetGameManifest);
	api_register_static_func(ctx, NULL, "GetGameList", js_GetGameList);
	api_register_static_func(ctx, NULL, "GetLocalAddress", js_GetLocalAddress);
	api_register_static_func(ctx, NULL, "GetLocalName", js_GetLocalName);
	api_register_static_func(ctx, NULL, "GetMaxFrameSkips", js_GetMaxFrameSkips);
	api_register_static_func(ctx, NULL, "GetScreenHeight", js_GetScreenHeight);
	api_register_static_func(ctx, NULL, "GetScreenWidth", js_GetScreenWidth);
	api_register_static_func(ctx, NULL, "GetSystemFont", js_GetSystemFont);
	api_register_static_func(ctx, NULL, "GetSystemWindowStyle", js_GetSystemWindowStyle);
	api_register_static_func(ctx, NULL, "GetTime", js_GetTime);
	api_register_static_func(ctx, NULL, "GetVersion", js_GetVersion);
	api_register_static_func(ctx, NULL, "GetVersionString", js_GetVersionString);
	api_register_static_func(ctx, NULL, "SetClippingRectangle", js_SetClippingRectangle);
	api_register_static_func(ctx, NULL, "SetFrameRate", js_SetFrameRate);
	api_register_static_func(ctx, NULL, "SetMaxFrameSkips", js_SetMaxFrameSkips);
	api_register_static_func(ctx, NULL, "SetScreenSize", js_SetScreenSize);
	api_register_static_func(ctx, NULL, "Abort", js_Abort);
	api_register_static_func(ctx, NULL, "Alert", js_Alert);
	api_register_static_func(ctx, NULL, "ApplyColorMask", js_ApplyColorMask);
	api_register_static_func(ctx, NULL, "Assert", js_Assert);
	api_register_static_func(ctx, NULL, "CreateStringFromCode", js_CreateStringFromCode);
	api_register_static_func(ctx, NULL, "CreateSurface", js_CreateSurface);
	api_register_static_func(ctx, NULL, "DebugPrint", js_DebugPrint);
	api_register_static_func(ctx, NULL, "Delay", js_Delay);
	api_register_static_func(ctx, NULL, "DispatchScript", js_DispatchScript);
	api_register_static_func(ctx, NULL, "DoEvents", js_DoEvents);
	api_register_static_func(ctx, NULL, "EvaluateScript", js_EvaluateScript);
	api_register_static_func(ctx, NULL, "EvaluateSystemScript", js_EvaluateSystemScript);
	api_register_static_func(ctx, NULL, "Exit", js_Exit);
	api_register_static_func(ctx, NULL, "ExecuteGame", js_ExecuteGame);
	api_register_static_func(ctx, NULL, "FlipScreen", js_FlipScreen);
	api_register_static_func(ctx, NULL, "GarbageCollect", js_GarbageCollect);
	api_register_static_func(ctx, NULL, "GrabSurface", js_GrabSurface);
	api_register_static_func(ctx, NULL, "GradientCircle", js_GradientCircle);
	api_register_static_func(ctx, NULL, "GradientRectangle", js_GradientRectangle);
	api_register_static_func(ctx, NULL, "Line", js_Line);
	api_register_static_func(ctx, NULL, "LineSeries", js_LineSeries);
	api_register_static_func(ctx, NULL, "ListenOnPort", js_ListenOnPort);
	api_register_static_func(ctx, NULL, "LoadAnimation", js_LoadAnimation);
	api_register_static_func(ctx, NULL, "LoadFont", js_LoadFont);
	api_register_static_func(ctx, NULL, "LoadSound", js_LoadSound);
	api_register_static_func(ctx, NULL, "LoadSpriteset", js_LoadSpriteset);
	api_register_static_func(ctx, NULL, "LoadSurface", js_LoadSurface);
	api_register_static_func(ctx, NULL, "LoadWindowStyle", js_LoadWindowStyle);
	api_register_static_func(ctx, NULL, "OpenAddress", js_OpenAddress);
	api_register_static_func(ctx, NULL, "OpenLog", js_OpenLog);
	api_register_static_func(ctx, NULL, "OutlinedCircle", js_OutlinedCircle);
	api_register_static_func(ctx, NULL, "OutlinedRectangle", js_OutlinedRectangle);
	api_register_static_func(ctx, NULL, "OutlinedRoundRectangle", js_OutlinedRoundRectangle);
	api_register_static_func(ctx, NULL, "Point", js_Point);
	api_register_static_func(ctx, NULL, "PointSeries", js_PointSeries);
	api_register_static_func(ctx, NULL, "Print", js_Print);
	api_register_static_func(ctx, NULL, "Rectangle", js_Rectangle);
	api_register_static_func(ctx, NULL, "RequireScript", js_RequireScript);
	api_register_static_func(ctx, NULL, "RequireSystemScript", js_RequireSystemScript);
	api_register_static_func(ctx, NULL, "RestartGame", js_RestartGame);
	api_register_static_func(ctx, NULL, "RoundRectangle", js_RoundRectangle);
	api_register_static_func(ctx, NULL, "Triangle", js_Triangle);
	api_register_static_func(ctx, NULL, "UnskipFrame", js_UnskipFrame);

	api_register_type(ctx, "Animation", js_Animation_finalize);
	api_register_prop(ctx, "Animation", "width", js_Animation_get_width, NULL);
	api_register_prop(ctx, "Animation", "height", js_Animation_get_height, NULL);
	api_register_method(ctx, "Animation", "getDelay", js_Animation_getDelay);
	api_register_method(ctx, "Animation", "getNumFrames", js_Animation_getNumFrames);
	api_register_method(ctx, "Animation", "drawFrame", js_Animation_drawFrame);
	api_register_method(ctx, "Animation", "drawZoomedFrame", js_Animation_drawZoomedFrame);
	api_register_method(ctx, "Animation", "readNextFrame", js_Animation_readNextFrame);

	api_register_type(ctx, "Font", js_Font_finalize);
	api_register_prop(ctx, "Font", "colorMask", js_Font_get_colorMask, js_Font_set_colorMask);
	api_register_prop(ctx, "Font", "height", js_Font_get_height, NULL);
	api_register_method(ctx, "Font", "getCharacterImage", js_Font_getCharacterImage);
	api_register_method(ctx, "Font", "getColorMask", js_Font_get_colorMask);
	api_register_method(ctx, "Font", "getHeight", js_Font_get_height);
	api_register_method(ctx, "Font", "getStringHeight", js_Font_getStringHeight);
	api_register_method(ctx, "Font", "getStringWidth", js_Font_getStringWidth);
	api_register_method(ctx, "Font", "setCharacterImage", js_Font_setCharacterImage);
	api_register_method(ctx, "Font", "setColorMask", js_Font_set_colorMask);
	api_register_method(ctx, "Font", "toString", js_Font_toString);
	api_register_method(ctx, "Font", "clone", js_Font_clone);
	api_register_method(ctx, "Font", "drawText", js_Font_drawText);
	api_register_method(ctx, "Font", "drawTextBox", js_Font_drawTextBox);
	api_register_method(ctx, "Font", "drawZoomedText", js_Font_drawZoomedText);
	api_register_method(ctx, "Font", "wordWrapString", js_Font_wordWrapString);
	
	api_register_type(ctx, "Logger", js_Logger_finalize);
	api_register_method(ctx, "Logger", "toString", js_Logger_toString);
	api_register_method(ctx, "Logger", "beginBlock", js_Logger_beginBlock);
	api_register_method(ctx, "Logger", "endBlock", js_Logger_endBlock);
	api_register_method(ctx, "Logger", "write", js_Logger_write);

	api_register_type(g_duk, "Socket", js_Socket_finalize);
	api_register_method(g_duk, "Socket", "toString", js_Socket_toString);
	api_register_method(g_duk, "Socket", "isConnected", js_Socket_isConnected);
	api_register_method(g_duk, "Socket", "getPendingReadSize", js_Socket_getPendingReadSize);
	api_register_method(g_duk, "Socket", "close", js_Socket_close);
	api_register_method(g_duk, "Socket", "read", js_Socket_read);
	api_register_method(g_duk, "Socket", "readString", js_Socket_readString);
	api_register_method(g_duk, "Socket", "write", js_Socket_write);

	api_register_type(g_duk, "Sound", js_Sound_finalize);
	api_register_prop(g_duk, "Sound", "length", js_Sound_get_length, NULL);
	api_register_prop(g_duk, "Sound", "pan", js_Sound_get_pan, js_Sound_set_pan);
	api_register_prop(g_duk, "Sound", "pitch", js_Sound_get_pitch, js_Sound_set_pitch);
	api_register_prop(g_duk, "Sound", "playing", js_Sound_get_playing, NULL);
	api_register_prop(g_duk, "Sound", "position", js_Sound_get_position, js_Sound_set_position);
	api_register_prop(g_duk, "Sound", "repeat", js_Sound_get_repeat, js_Sound_set_repeat);
	api_register_prop(g_duk, "Sound", "seekable", js_Sound_get_seekable, NULL);
	api_register_prop(g_duk, "Sound", "volume", js_Sound_get_volume, js_Sound_set_volume);
	api_register_method(g_duk, "Sound", "isPlaying", js_Sound_get_playing);
	api_register_method(g_duk, "Sound", "isSeekable", js_Sound_get_seekable);
	api_register_method(g_duk, "Sound", "getLength", js_Sound_get_length);
	api_register_method(g_duk, "Sound", "getPan", js_Sound_get_pan);
	api_register_method(g_duk, "Sound", "getPitch", js_Sound_get_pitch);
	api_register_method(g_duk, "Sound", "getPosition", js_Sound_get_position);
	api_register_method(g_duk, "Sound", "getRepeat", js_Sound_get_repeat);
	api_register_method(g_duk, "Sound", "getVolume", js_Sound_getVolume);
	api_register_method(g_duk, "Sound", "setPan", js_Sound_set_pan);
	api_register_method(g_duk, "Sound", "setPitch", js_Sound_set_pitch);
	api_register_method(g_duk, "Sound", "setPosition", js_Sound_set_position);
	api_register_method(g_duk, "Sound", "setRepeat", js_Sound_set_repeat);
	api_register_method(g_duk, "Sound", "setVolume", js_Sound_setVolume);
	api_register_method(g_duk, "Sound", "pause", js_Sound_pause);
	api_register_method(g_duk, "Sound", "play", js_Sound_play);
	api_register_method(g_duk, "Sound", "reset", js_Sound_reset);
	api_register_method(g_duk, "Sound", "stop", js_Sound_stop);
	api_register_method(g_duk, "Sound", "toString", js_Sound_toString);

	api_register_type(ctx, "Spriteset", js_Spriteset_finalize);
	api_register_prop(ctx, "Spriteset", "filename", js_Spriteset_get_filename, NULL);
	api_register_method(ctx, "Spriteset", "toString", js_Spriteset_toString);
	api_register_method(ctx, "Spriteset", "clone", js_Spriteset_clone);

	api_register_type(ctx, "Surface", js_Surface_finalize);
	api_register_prop(ctx, "Surface", "height", js_Surface_get_height, NULL);
	api_register_prop(ctx, "Surface", "width", js_Surface_get_width, NULL);
	api_register_method(ctx, "Surface", "getPixel", js_Surface_getPixel);
	api_register_method(ctx, "Surface", "setAlpha", js_Surface_setAlpha);
	api_register_method(ctx, "Surface", "setBlendMode", js_Surface_setBlendMode);
	api_register_method(ctx, "Surface", "setPixel", js_Surface_setPixel);
	api_register_method(ctx, "Surface", "applyColorFX", js_Surface_applyColorFX);
	api_register_method(ctx, "Surface", "applyColorFX4", js_Surface_applyColorFX4);
	api_register_method(ctx, "Surface", "applyLookup", js_Surface_applyLookup);
	api_register_method(ctx, "Surface", "blit", js_Surface_blit);
	api_register_method(ctx, "Surface", "blitMaskSurface", js_Surface_blitMaskSurface);
	api_register_method(ctx, "Surface", "blitSurface", js_Surface_blitSurface);
	api_register_method(ctx, "Surface", "clone", js_Surface_clone);
	api_register_method(ctx, "Surface", "cloneSection", js_Surface_cloneSection);
	api_register_method(ctx, "Surface", "createImage", js_Surface_createImage);
	api_register_method(ctx, "Surface", "drawText", js_Surface_drawText);
	api_register_method(ctx, "Surface", "filledCircle", js_Surface_filledCircle);
	api_register_method(ctx, "Surface", "flipHorizontally", js_Surface_flipHorizontally);
	api_register_method(ctx, "Surface", "flipVertically", js_Surface_flipVertically);
	api_register_method(ctx, "Surface", "gradientCircle", js_Surface_gradientCircle);
	api_register_method(ctx, "Surface", "gradientRectangle", js_Surface_gradientRectangle);
	api_register_method(ctx, "Surface", "line", js_Surface_line);
	api_register_method(ctx, "Surface", "outlinedCircle", js_Surface_outlinedCircle);
	api_register_method(ctx, "Surface", "outlinedRectangle", js_Surface_outlinedRectangle);
	api_register_method(ctx, "Surface", "pointSeries", js_Surface_pointSeries);
	api_register_method(ctx, "Surface", "rotate", js_Surface_rotate);
	api_register_method(ctx, "Surface", "rectangle", js_Surface_rectangle);
	api_register_method(ctx, "Surface", "replaceColor", js_Surface_replaceColor);
	api_register_method(ctx, "Surface", "rescale", js_Surface_rescale);
	api_register_method(ctx, "Surface", "save", js_Surface_save);
	api_register_method(ctx, "Surface", "toString", js_Surface_toString);

	api_register_type(g_duk, "WindowStyle", js_WindowStyle_finalize);
	api_register_prop(g_duk, "WindowStyle", "colorMask", js_WindowStyle_get_colorMask, js_WindowStyle_set_colorMask);
	api_register_method(g_duk, "WindowStyle", "toString", js_WindowStyle_toString);
	api_register_method(g_duk, "WindowStyle", "getColorMask", js_WindowStyle_get_colorMask);
	api_register_method(g_duk, "WindowStyle", "setColorMask", js_WindowStyle_set_colorMask);
	api_register_method(g_duk, "WindowStyle", "drawWindow", js_WindowStyle_drawWindow);
	
	// blend modes for Surfaces
	api_register_const(ctx, NULL, "BLEND", BLEND_BLEND);
	api_register_const(ctx, NULL, "REPLACE", BLEND_REPLACE);
	api_register_const(ctx, NULL, "RGB_ONLY", BLEND_RGB_ONLY);
	api_register_const(ctx, NULL, "ALPHA_ONLY", BLEND_ALPHA_ONLY);
	api_register_const(ctx, NULL, "ADD", BLEND_ADD);
	api_register_const(ctx, NULL, "SUBTRACT", BLEND_SUBTRACT);
	api_register_const(ctx, NULL, "MULTIPLY", BLEND_MULTIPLY);
	api_register_const(ctx, NULL, "AVERAGE", BLEND_AVERAGE);
	api_register_const(ctx, NULL, "INVERT", BLEND_INVERT);
	
	// LineSeries() modes
	api_register_const(ctx, NULL, "LINE_MULTIPLE", LINE_MULTIPLE);
	api_register_const(ctx, NULL, "LINE_STRIP", LINE_STRIP);
	api_register_const(ctx, NULL, "LINE_LOOP", LINE_LOOP);
}

void
duk_push_sphere_spriteset(duk_context* ctx, spriteset_t* spriteset)
{
	char prop_name[20];

	int i, j;

	duk_push_sphere_obj(ctx, "Spriteset", ref_spriteset(spriteset));

	// Spriteset:base
	duk_push_object(ctx);
	duk_push_int(ctx, spriteset->base.x1); duk_put_prop_string(ctx, -2, "x1");
	duk_push_int(ctx, spriteset->base.y1); duk_put_prop_string(ctx, -2, "y1");
	duk_push_int(ctx, spriteset->base.x2); duk_put_prop_string(ctx, -2, "x2");
	duk_push_int(ctx, spriteset->base.y2); duk_put_prop_string(ctx, -2, "y2");
	duk_put_prop_string(ctx, -2, "base");

	// Spriteset:images
	duk_push_array(ctx);
	for (i = 0; i < spriteset->num_images; ++i) {
		sprintf(prop_name, "%i", i);
		duk_push_string(ctx, prop_name);
		duk_push_c_function(ctx, js_Spriteset_get_image, DUK_VARARGS);
		duk_get_prop_string(ctx, -1, "bind"); duk_dup(ctx, -2);
		duk_dup(ctx, -6);
		duk_call_method(ctx, 1);
		duk_remove(ctx, -2);
		duk_push_c_function(ctx, js_Spriteset_set_image, DUK_VARARGS);
		duk_get_prop_string(ctx, -1, "bind"); duk_dup(ctx, -2);
		duk_dup(ctx, -7);
		duk_call_method(ctx, 1);
		duk_remove(ctx, -2);
		duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER
			| DUK_DEFPROP_HAVE_ENUMERABLE | DUK_DEFPROP_ENUMERABLE);
	}
	duk_put_prop_string(ctx, -2, "images");

	// Spriteset:directions
	duk_push_array(ctx);
	for (i = 0; i < spriteset->num_poses; ++i) {
		duk_push_object(ctx);
		duk_push_lstring_t(ctx, spriteset->poses[i].name);
		duk_put_prop_string(ctx, -2, "name");
		duk_push_array(ctx);
		for (j = 0; j < spriteset->poses[i].num_frames; ++j) {
			duk_push_object(ctx);
			duk_push_int(ctx, spriteset->poses[i].frames[j].image_idx); duk_put_prop_string(ctx, -2, "index");
			duk_push_int(ctx, spriteset->poses[i].frames[j].delay); duk_put_prop_string(ctx, -2, "delay");
			duk_put_prop_index(ctx, -2, j);
		}
		duk_put_prop_string(ctx, -2, "frames");
		duk_put_prop_index(ctx, -2, i);
	}
	duk_put_prop_string(ctx, -2, "directions");
}

static void
duk_push_sphere_windowstyle(duk_context* ctx, windowstyle_t* winstyle)
{
	duk_push_sphere_obj(ctx, "WindowStyle", ref_windowstyle(winstyle));
	duk_push_sphere_color(ctx, color_new(255, 255, 255, 255)); duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
}

static uint8_t*
duk_require_rgba_lut(duk_context* ctx, duk_idx_t index)
{
	uint8_t* lut;
	int      length;

	int i;

	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	lut = malloc(sizeof(uint8_t) * 256);
	length = fmin(duk_get_length(ctx, index), 256);
	for (i = length; i < 256; ++i) lut[i] = i;
	for (i = 0; i < length; ++i) {
		duk_get_prop_index(ctx, index, i);
		lut[i] = fmin(fmax(duk_require_int(ctx, -1), 0), 255);
		duk_pop(ctx);
	}
	return lut;
}

static void
apply_blend_mode(int blend_mode)
{
	switch (blend_mode) {
	case BLEND_BLEND:
		al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
			ALLEGRO_ADD, ALLEGRO_INVERSE_DEST_COLOR, ALLEGRO_ONE);
		break;
	case BLEND_REPLACE:
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
		break;
	case BLEND_ADD:
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
		break;
	case BLEND_SUBTRACT:
		al_set_blender(ALLEGRO_DEST_MINUS_SRC, ALLEGRO_ONE, ALLEGRO_ONE);
		break;
	case BLEND_MULTIPLY:
		al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_DEST_COLOR, ALLEGRO_ZERO,
			ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_ONE);
		break;
	case BLEND_INVERT:
		al_set_separate_blender(ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_INVERSE_SRC_COLOR,
			ALLEGRO_ADD, ALLEGRO_ZERO, ALLEGRO_ONE);
		break;
	}
}

static void
reset_blender(void)
{
	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
}

static duk_ret_t
js_IsSkippedFrame(duk_context* ctx)
{
	duk_push_boolean(ctx, screen_is_skipframe(g_screen));
	return 1;
}

static duk_ret_t
js_GetClippingRectangle(duk_context* ctx)
{
	rect_t clip;

	clip = screen_get_clipping(g_screen);
	duk_push_object(ctx);
	duk_push_int(ctx, clip.x1); duk_put_prop_string(ctx, -2, "x");
	duk_push_int(ctx, clip.y1); duk_put_prop_string(ctx, -2, "y");
	duk_push_int(ctx, clip.x2 - clip.x1); duk_put_prop_string(ctx, -2, "width");
	duk_push_int(ctx, clip.y2 - clip.y1); duk_put_prop_string(ctx, -2, "height");
	return 1;
}

static duk_ret_t
js_GetFrameRate(duk_context* ctx)
{
	duk_push_int(ctx, g_framerate);
	return 1;
}

static duk_ret_t
js_GetGameList(duk_context* ctx)
{
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_FS_ENTRY* fse;
	path_t*           path = NULL;
	path_t*           paths[2];
	sandbox_t*        sandbox;

	int i, j = 0;

	// build search paths
	paths[0] = path_rebase(path_new("games/"), enginepath());
	paths[1] = path_rebase(path_new("minisphere/games/"), homepath());

	// search for supported games
	duk_push_array(ctx);
	for (i = sizeof paths / sizeof(path_t*) - 1; i >= 0; --i) {
		fse = al_create_fs_entry(path_cstr(paths[i]));
		if (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fse)) {
			while (file_info = al_read_directory(fse)) {
				path = path_new(al_get_fs_entry_name(file_info));
				if (sandbox = new_sandbox(path_cstr(path))) {
					duk_push_lstring_t(ctx, get_game_manifest(sandbox));
					duk_json_decode(ctx, -1);
					duk_push_string(ctx, path_cstr(path));
					duk_put_prop_string(ctx, -2, "directory");
					duk_put_prop_index(ctx, -2, j++);
					free_sandbox(sandbox);
				}
				path_free(path);
			}
		}
		al_destroy_fs_entry(fse);
		path_free(paths[i]);
	}
	return 1;
}

static duk_ret_t
js_GetGameManifest(duk_context* ctx)
{
	duk_push_lstring_t(ctx, get_game_manifest(g_fs));
	duk_json_decode(ctx, -1);
	duk_push_string(ctx, path_cstr(get_game_path(g_fs)));
	duk_put_prop_string(ctx, -2, "directory");
	return 1;
}

static duk_ret_t
js_GetLocalAddress(duk_context* ctx)
{
	duk_push_string(ctx, "127.0.0.1");
	return 1;
}

static duk_ret_t
js_GetLocalName(duk_context* ctx)
{
	duk_push_string(ctx, "localhost");
	return 1;
}

static duk_ret_t
js_GetMaxFrameSkips(duk_context* ctx)
{
	duk_push_int(ctx, screen_get_frameskip(g_screen));
	return 1;
}

static duk_ret_t
js_GetScreenHeight(duk_context* ctx)
{
	duk_push_int(ctx, g_res_y);
	return 1;
}

static duk_ret_t
js_GetScreenWidth(duk_context* ctx)
{
	duk_push_int(ctx, g_res_x);
	return 1;
}

static duk_ret_t
js_GetSystemFont(duk_context* ctx)
{
	duk_push_sphere_font(ctx, g_sys_font);
	return 1;
}

static duk_ret_t
js_GetSystemWindowStyle(duk_context* ctx)
{
	if (s_sys_winstyle == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "missing system windowstyle");

	duk_push_sphere_windowstyle(ctx, s_sys_winstyle);
	return 1;
}

static duk_ret_t
js_GetTime(duk_context* ctx)
{
	duk_push_number(ctx, floor(al_get_time() * 1000));
	return 1;
}

static duk_ret_t
js_GetVersion(duk_context* ctx)
{
	duk_push_number(ctx, 1.5);
	return 1;
}

static duk_ret_t
js_GetVersionString(duk_context* ctx)
{
	duk_push_sprintf(ctx, "v1.5 (%s %s)", PRODUCT_NAME, VERSION_NAME);
	return 1;
}

static duk_ret_t
js_SetClippingRectangle(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int width = duk_require_int(ctx, 2);
	int height = duk_require_int(ctx, 3);

	screen_set_clipping(g_screen, new_rect(x, y, x + width, y + height));
	return 0;
}

static duk_ret_t
js_SetFrameRate(duk_context* ctx)
{
	int framerate = duk_require_int(ctx, 0);

	if (framerate < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetFrameRate(): framerate must be positive (got: %d)", framerate);
	g_framerate = framerate;
	return 0;
}

static duk_ret_t
js_SetMaxFrameSkips(duk_context* ctx)
{
	int max_skips = duk_require_int(ctx, 0);

	if (max_skips < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetMaxFrameSkips(): value cannot be negative (%d)", max_skips);
	screen_set_frameskip(g_screen, max_skips);
	return 0;
}

static duk_ret_t
js_SetScreenSize(duk_context* ctx)
{
	int  res_width;
	int  res_height;

	res_width = duk_require_int(ctx, 0);
	res_height = duk_require_int(ctx, 1);

	if (res_width < 0 || res_height < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetScreenSize(): dimensions cannot be negative (got X: %d, Y: %d)",
			res_width, res_height);
	screen_resize(g_screen, res_width, res_height);
	return 0;
}

static duk_ret_t
js_Alert(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* text = n_args >= 1 && !duk_is_null_or_undefined(ctx, 0)
		? duk_to_string(ctx, 0) : "It's 8:12... do you know where the pig is?\n\nIt's...\n\n\n\n\n\n\nBEHIND YOU! *MUNCH*";
	int stack_offset = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	const char* caller_info;
	const char* filename;
	int         line_number;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Alert(): stack offset must be negative");

	// get filename and line number of Alert() call
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Duktape");
	duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3 + stack_offset); duk_call(ctx, 1);
	if (!duk_is_object(ctx, -1)) {
		duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3); duk_call(ctx, 1);
	}
	duk_remove(ctx, -2);
	duk_get_prop_string(ctx, -1, "lineNumber"); line_number = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "function");
	duk_get_prop_string(ctx, -1, "fileName"); filename = duk_get_string(ctx, -1); duk_pop(ctx);
	duk_pop_2(ctx);

	// show the message
	screen_show_mouse(g_screen, true);
	caller_info =
		duk_push_sprintf(ctx, "%s (line %i)", filename, line_number),
		duk_get_string(ctx, -1);
	al_show_native_message_box(screen_display(g_screen), "Alert from Sphere game", caller_info, text, NULL, 0x0);
	screen_show_mouse(g_screen, false);

	return 0;
}

static duk_ret_t
js_Abort(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* message = n_args >= 1 ? duk_to_string(ctx, 0) : "Some type of weird pig just ate your game!\n\n\n\n\n\n\n\n...and you*munch*";
	int stack_offset = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Abort(): stack offset must be negative");
	duk_error_ni(ctx, -1 + stack_offset, DUK_ERR_ERROR, "%s", message);
}

static duk_ret_t
js_ApplyColorMask(duk_context* ctx)
{
	color_t color;

	color = duk_require_sphere_color(ctx, 0);

	if (!screen_is_skipframe(g_screen))
		al_draw_filled_rectangle(0, 0, g_res_x, g_res_y, nativecolor(color));
	return 0;
}

static duk_ret_t
js_Assert(duk_context* ctx)
{
	const char* filename;
	int         line_number;
	const char* message;
	int         num_args;
	bool        result;
	int         stack_offset;
	lstring_t*  text;

	num_args = duk_get_top(ctx);
	result = duk_to_boolean(ctx, 0);
	message = duk_require_string(ctx, 1);
	stack_offset = num_args >= 3 ? duk_require_int(ctx, 2)
		: 0;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Assert(): stack offset must be negative");

	if (!result) {
		// get the offending script and line number from the call stack
		duk_push_global_object(ctx);
		duk_get_prop_string(ctx, -1, "Duktape");
		duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3 + stack_offset); duk_call(ctx, 1);
		if (!duk_is_object(ctx, -1)) {
			duk_pop(ctx);
			duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3); duk_call(ctx, 1);
		}
		duk_remove(ctx, -2);
		duk_get_prop_string(ctx, -1, "lineNumber"); line_number = duk_get_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "function");
		duk_get_prop_string(ctx, -1, "fileName"); filename = duk_get_string(ctx, -1); duk_pop(ctx);
		duk_pop_2(ctx);
		fprintf(stderr, "ASSERT: `%s:%i` : %s\n", filename, line_number, message);

		// if an assertion fails in a game being debugged:
		//   - the user may choose to ignore it, in which case execution continues.  this is useful
		//     in some debugging scenarios.
		//   - if the user chooses not to continue, a prompt breakpoint will be triggered, turning
		//     over control to the attached debugger.
		if (is_debugger_attached()) {
			text = lstr_newf("%s (line: %i)\n%s\n\nYou can ignore the error, or pause execution, turning over control to the attached debugger.  If you choose to debug, execution will pause at the statement following the failed Assert().\n\nIgnore the error and continue?", filename, line_number, message);
			if (!al_show_native_message_box(screen_display(g_screen), "Script Error", "Assertion failed!",
				lstr_cstr(text), NULL, ALLEGRO_MESSAGEBOX_WARN | ALLEGRO_MESSAGEBOX_YES_NO))
			{
				duk_debugger_pause(ctx);
			}
			lstr_free(text);
		}
	}
	duk_dup(ctx, 0);
	return 1;
}

static duk_ret_t
js_CreateStringFromCode(duk_context* ctx)
{
	int code = duk_require_int(ctx, 0);

	char cstr[2];

	if (code < 0 || code > 255)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "CreateStringFromCode(): character code is out of ASCII range (%i)", code);
	cstr[0] = (char)code; cstr[1] = '\0';
	duk_push_string(ctx, cstr);
	return 1;
}

static duk_ret_t
js_CreateSurface(duk_context* ctx)
{
	color_t     fill_color;
	int         height;
	image_t*    image;
	int         num_args;
	int         width;

	num_args = duk_get_top(ctx);
	width = duk_require_int(ctx, 0);
	height = duk_require_int(ctx, 1);
	fill_color = num_args >= 3 ? duk_require_sphere_color(ctx, 2) : color_new(0, 0, 0, 0);

	if (!(image = image_new(width, height)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create surface");
	image_fill(image, fill_color);
	duk_push_sphere_obj(ctx, "Surface", image);
	return 1;
}

static duk_ret_t
js_DebugPrint(duk_context* ctx)
{
	int num_items;

	num_items = duk_get_top(ctx);

	// separate printed values with a space
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_NORMAL);
	return 0;
}

static duk_ret_t
js_Delay(duk_context* ctx)
{
	double millisecs = floor(duk_require_number(ctx, 0));

	if (millisecs < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Delay(): delay must be positive (got: %.0f)", millisecs);
	delay(millisecs / 1000);
	return 0;
}

static duk_ret_t
js_DispatchScript(duk_context* ctx)
{
	script_t* script;
	char*     script_name;

	script_name = strnewf("synth:async~%u.js", s_next_async_id++);
	script = duk_require_sphere_script(ctx, 0, script_name);
	free(script_name);

	if (!queue_async_script(script))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to dispatch async script");
	return 0;
}

static duk_ret_t
js_DoEvents(duk_context* ctx)
{
	do_events();
	duk_push_boolean(ctx, true);
	return 1;
}

static duk_ret_t
js_EvaluateScript(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, "scripts", true);
	if (!sfs_fexist(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "EvaluateScript(): file `%s` not found", filename);
	if (!evaluate_script(filename, false))
		duk_throw(ctx);
	return 1;
}

static duk_ret_t
js_EvaluateSystemScript(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	char path[SPHERE_PATH_MAX];

	sprintf(path, "scripts/lib/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		sprintf(path, "#/scripts/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "EvaluateSystemScript(): system script `%s` not found", filename);
	if (!evaluate_script(path, false))
		duk_throw(ctx);
	return 1;
}

static duk_ret_t
js_ExecuteGame(duk_context* ctx)
{
	path_t*     games_path;
	const char* filename;

	filename = duk_require_string(ctx, 0);

	// store the old game path so we can relaunch when the chained game exits
	g_last_game_path = path_dup(get_game_path(g_fs));

	// if the passed-in path is relative, resolve it relative to <engine>/games.
	// this is done for compatibility with Sphere 1.x.
	g_game_path = path_new(filename);
	games_path = path_rebase(path_new("games/"), enginepath());
	path_rebase(g_game_path, games_path);
	path_free(games_path);

	restart_engine();
}

static duk_ret_t
js_Exit(duk_context* ctx)
{
	exit_game(false);
}

static duk_ret_t
js_FlipScreen(duk_context* ctx)
{
	screen_flip(g_screen, g_framerate);
	return 0;
}

static duk_ret_t
js_GarbageCollect(duk_context* ctx)
{
	duk_gc(ctx, 0x0);
	duk_gc(ctx, 0x0);
	return 0;
}

static duk_ret_t
js_GrabSurface(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);

	image_t* image;

	if (!(image = screen_grab(g_screen, x, y, w, h)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GrabSurface(): unable to grab backbuffer image");
	duk_push_sphere_obj(ctx, "Surface", image);
	return 1;
}

static duk_ret_t
js_GradientCircle(duk_context* ctx)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);
	int radius = duk_require_number(ctx, 2);
	color_t in_color = duk_require_sphere_color(ctx, 3);
	color_t out_color = duk_require_sphere_color(ctx, 4);

	double phi;
	int    vcount;

	int i;

	if (screen_is_skipframe(g_screen))
		return 0;
	vcount = fmin(radius, 126);
	s_vbuf[0].x = x; s_vbuf[0].y = y; s_vbuf[0].z = 0;
	s_vbuf[0].color = nativecolor(in_color);
	for (i = 0; i < vcount; ++i) {
		phi = 2 * M_PI * i / vcount;
		s_vbuf[i + 1].x = x + cos(phi) * radius;
		s_vbuf[i + 1].y = y - sin(phi) * radius;
		s_vbuf[i + 1].z = 0;
		s_vbuf[i + 1].color = nativecolor(out_color);
	}
	s_vbuf[i + 1].x = x + cos(0) * radius;
	s_vbuf[i + 1].y = y - sin(0) * radius;
	s_vbuf[i + 1].z = 0;
	s_vbuf[i + 1].color = nativecolor(out_color);
	al_draw_prim(s_vbuf, NULL, NULL, 0, vcount + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return 0;
}

static duk_ret_t
js_GradientRectangle(duk_context* ctx)
{
	int x1 = duk_require_int(ctx, 0);
	int y1 = duk_require_int(ctx, 1);
	int x2 = x1 + duk_require_int(ctx, 2);
	int y2 = y1 + duk_require_int(ctx, 3);
	color_t color_ul = duk_require_sphere_color(ctx, 4);
	color_t color_ur = duk_require_sphere_color(ctx, 5);
	color_t color_lr = duk_require_sphere_color(ctx, 6);
	color_t color_ll = duk_require_sphere_color(ctx, 7);

	if (!screen_is_skipframe(g_screen)) {
		ALLEGRO_VERTEX verts[] = {
			{ x1, y1, 0, 0, 0, nativecolor(color_ul) },
			{ x2, y1, 0, 0, 0, nativecolor(color_ur) },
			{ x1, y2, 0, 0, 0, nativecolor(color_ll) },
			{ x2, y2, 0, 0, 0, nativecolor(color_lr) }
		};
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	}
	return 0;
}

static duk_ret_t
js_Line(duk_context* ctx)
{
	float x1 = duk_require_int(ctx, 0) + 0.5;
	float y1 = duk_require_int(ctx, 1) + 0.5;
	float x2 = duk_require_int(ctx, 2) + 0.5;
	float y2 = duk_require_int(ctx, 3) + 0.5;
	color_t color = duk_require_sphere_color(ctx, 4);

	if (!screen_is_skipframe(g_screen))
		al_draw_line(x1, y1, x2, y2, nativecolor(color), 1);
	return 0;
}

static duk_ret_t
js_LineSeries(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	color_t color = duk_require_sphere_color(ctx, 1);
	int type = n_args >= 3 ? duk_require_int(ctx, 2) : LINE_MULTIPLE;

	size_t          num_points;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	size_t i;

	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LineSeries(): first argument must be an array");
	duk_get_prop_string(ctx, 0, "length"); num_points = duk_get_uint(ctx, 0); duk_pop(ctx);
	if (num_points < 2)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "LineSeries(): two or more vertices required");
	if (num_points > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "LineSeries(): too many vertices");
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LineSeries(): unable to allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, (duk_uarridx_t)i);
		duk_get_prop_string(ctx, 0, "x"); x = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, 0, "y"); y = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points,
		type == LINE_STRIP ? ALLEGRO_PRIM_LINE_STRIP
		: type == LINE_LOOP ? ALLEGRO_PRIM_LINE_LOOP
		: ALLEGRO_PRIM_LINE_LIST
	);
	free(vertices);
	return 0;
}

static duk_ret_t
js_ListenOnPort(duk_context* ctx)
{
	int       port;
	socket_t* socket;
	
	port = duk_require_int(ctx, 0);

	if (socket = listen_on_port(NULL, port, 1024, 0))
		duk_push_sphere_obj(ctx, "Socket", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_LoadAnimation(duk_context* ctx)
{
	animation_t* anim;
	const char*  filename;

	filename = duk_require_path(ctx, 0, "animations", true);
	if (!(anim = animation_new(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LoadAnimation(): unable to load animation file `%s`", filename);
	duk_push_sphere_obj(ctx, "Animation", anim);
	return 1;
}

static duk_ret_t
js_LoadFont(duk_context* ctx)
{
	const char* filename;
	font_t*     font;

	filename = duk_require_path(ctx, 0, "fonts", true);
	if (!(font = font_load(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to load font `%s`", filename);
	duk_push_sphere_font(ctx, font);
	font_free(font);
	return 1;
}

static duk_ret_t
js_LoadSound(duk_context* ctx)
{
	const char* filename;
	sound_t*    sound;

	filename = duk_require_path(ctx, 0, "sounds", true);

	if (!(sound = sound_new(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to load sound `%s`", filename);
	duk_push_sphere_obj(ctx, "Sound", sound);
	return 1;
}

static duk_ret_t
js_LoadSpriteset(duk_context* ctx)
{
	const char*  filename;
	spriteset_t* spriteset;

	filename = duk_require_path(ctx, 0, "spritesets", true);
	if ((spriteset = load_spriteset(filename)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Spriteset(): unable to load spriteset file `%s`", filename);
	duk_push_sphere_spriteset(ctx, spriteset);
	free_spriteset(spriteset);
	return 1;
}

static duk_ret_t
js_LoadSurface(duk_context* ctx)
{
	const char* filename;
	image_t*    image;

	filename = duk_require_path(ctx, 0, "images", true);
	if (!(image = image_load(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LoadSurface(): unable to load image file `%s`", filename);
	duk_push_sphere_obj(ctx, "Surface", image);
	return 1;
}

static duk_ret_t
js_LoadWindowStyle(duk_context* ctx)
{
	const char*    filename;
	windowstyle_t* winstyle;

	filename = duk_require_path(ctx, 0, "windowstyles", true);
	if (!(winstyle = load_windowstyle(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to load windowstyle `%s`", filename);
	duk_push_sphere_windowstyle(ctx, winstyle);
	free_windowstyle(winstyle);
	return 1;
}

static duk_ret_t
js_OpenAddress(duk_context* ctx)
{
	const char* hostname;
	int         port;
	socket_t*   socket;
	
	hostname = duk_require_string(ctx, 0);
	port = duk_require_int(ctx, 1);
	
	if ((socket = connect_to_host(hostname, port, 1024)) != NULL)
		duk_push_sphere_obj(ctx, "Socket", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_OpenLog(duk_context* ctx)
{
	const char* filename;
	logger_t*   logger;

	filename = duk_require_path(ctx, 0, "logs", true);
	if (!(logger = log_open(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to open log `%s`", filename);
	duk_push_sphere_obj(ctx, "Logger", logger);
	return 1;
}

static duk_ret_t
js_OutlinedCircle(duk_context* ctx)
{
	float x = duk_require_int(ctx, 0) + 0.5;
	float y = duk_require_int(ctx, 1) + 0.5;
	float radius = duk_require_int(ctx, 2);
	color_t color = duk_require_sphere_color(ctx, 3);

	if (!screen_is_skipframe(g_screen))
		al_draw_circle(x, y, radius, nativecolor(color), 1);
	return 0;
}

static duk_ret_t
js_OutlinedRectangle(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float x1 = duk_require_int(ctx, 0) + 0.5;
	float y1 = duk_require_int(ctx, 1) + 0.5;
	float x2 = x1 + duk_require_int(ctx, 2) - 1;
	float y2 = y1 + duk_require_int(ctx, 3) - 1;
	color_t color = duk_require_sphere_color(ctx, 4);
	int thickness = n_args >= 6 ? duk_require_int(ctx, 5) : 1;

	if (!screen_is_skipframe(g_screen))
		al_draw_rectangle(x1, y1, x2, y2, nativecolor(color), thickness);
	return 0;
}

static duk_ret_t
js_OutlinedRoundRectangle(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float x = duk_require_int(ctx, 0) + 0.5;
	float y = duk_require_int(ctx, 1) + 0.5;
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	float radius = duk_require_number(ctx, 4);
	color_t color = duk_require_sphere_color(ctx, 5);
	int thickness = n_args >= 7 ? duk_require_int(ctx, 6) : 1;

	if (!screen_is_skipframe(g_screen))
		al_draw_rounded_rectangle(x, y, x + w - 1, y + h - 1, radius, radius, nativecolor(color), thickness);
	return 0;
}

static duk_ret_t
js_Point(duk_context* ctx)
{
	float x = duk_require_int(ctx, 0) + 0.5;
	float y = duk_require_int(ctx, 1) + 0.5;
	color_t color = duk_require_sphere_color(ctx, 2);

	if (!screen_is_skipframe(g_screen))
		al_draw_pixel(x, y, nativecolor(color));
	return 0;
}

static duk_ret_t
js_PointSeries(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 1);

	size_t          num_points;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	size_t i;

	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "PointSeries(): first argument must be an array");
	duk_get_prop_string(ctx, 0, "length"); num_points = duk_get_uint(ctx, 0); duk_pop(ctx);
	if (num_points < 1)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "PointSeries(): one or more vertices required");
	if (num_points > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "PointSeries(): too many vertices");
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "PointSeries(): unable to allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, (duk_uarridx_t)i);
		duk_get_prop_string(ctx, 0, "x"); x = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, 0, "y"); y = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points, ALLEGRO_PRIM_POINT_LIST);
	free(vertices);
	return 0;
}

static duk_ret_t
js_Print(duk_context* ctx)
{
	int num_items;

	num_items = duk_get_top(ctx);

	// separate printed values with a space
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	printf("%s\n", duk_get_string(ctx, -1));
	return 0;
}

static duk_ret_t
js_Rectangle(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);

	if (!screen_is_skipframe(g_screen))
		al_draw_filled_rectangle(x, y, x + w, y + h, nativecolor(color));
	return 0;
}

static duk_ret_t
js_RequireScript(duk_context* ctx)
{
	const char* filename;
	bool        is_required;

	filename = duk_require_path(ctx, 0, "scripts", true);
	if (!sfs_fexist(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RequireScript(): file `%s` not found", filename);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, filename);
	is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_push_true(ctx);
		duk_put_prop_string(ctx, -2, filename);
		if (!evaluate_script(filename, false))
			duk_throw(ctx);
	}
	duk_pop_3(ctx);
	return 0;
}

static duk_ret_t
js_RequireSystemScript(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	bool is_required;
	char path[SPHERE_PATH_MAX];

	sprintf(path, "scripts/lib/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		sprintf(path, "#/scripts/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RequireSystemScript(): system script `%s` not found", filename);

	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, path);
	is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_push_true(ctx);
		duk_put_prop_string(ctx, -2, path);
		if (!evaluate_script(path, false))
			duk_throw(ctx);
	}
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
js_RestartGame(duk_context* ctx)
{
	restart_engine();
}

static duk_ret_t
js_RoundRectangle(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	float radius = duk_require_number(ctx, 4);
	color_t color = duk_require_sphere_color(ctx, 5);

	if (!screen_is_skipframe(g_screen))
		al_draw_filled_rounded_rectangle(x, y, x + w, y + h, radius, radius, nativecolor(color));
	return 0;
}

static duk_ret_t
js_Triangle(duk_context* ctx)
{
	int x1 = duk_require_int(ctx, 0);
	int y1 = duk_require_int(ctx, 1);
	int x2 = duk_require_int(ctx, 2);
	int y2 = duk_require_int(ctx, 3);
	int x3 = duk_require_int(ctx, 4);
	int y3 = duk_require_int(ctx, 5);
	color_t color = duk_require_sphere_color(ctx, 6);

	if (!screen_is_skipframe(g_screen))
		al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, nativecolor(color));
	return 0;
}

static duk_ret_t
js_UnskipFrame(duk_context* ctx)
{
	screen_unskip_frame(g_screen);
	return 0;
}

static duk_ret_t
js_Animation_finalize(duk_context* ctx)
{
	animation_t* anim;

	anim = duk_require_sphere_obj(ctx, 0, "Animation");
	animation_free(anim);
	return 0;
}

static duk_ret_t
js_Animation_get_height(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");

	duk_push_int(ctx, animation_height(anim));
	return 1;
}

static duk_ret_t
js_Animation_get_width(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");

	duk_push_int(ctx, animation_width(anim));
	return 1;
}

static duk_ret_t
js_Animation_getDelay(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	duk_push_int(ctx, animation_delay(anim));
	return 1;
}

static duk_ret_t
js_Animation_getNumFrames(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");

	duk_push_int(ctx, animation_num_frames(anim));
	return 1;
}

static duk_ret_t
js_Animation_drawFrame(duk_context* ctx)
{
	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);

	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");

	image_draw(animation_frame(anim), x, y);
	return 0;
}

static duk_ret_t
js_Animation_drawZoomedFrame(duk_context* ctx)
{
	animation_t* anim;
	int          height;
	double       scale;
	int          width;
	int          x;
	int          y;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	x = duk_require_number(ctx, 0);
	y = duk_require_number(ctx, 1);
	scale = duk_require_number(ctx, 2);

	if (scale < 0.0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "zoom must be positive");
	width = animation_width(anim);
	height = animation_height(anim);
	image_draw_scaled(animation_frame(anim), x, y, width * scale, height * scale);
	return 0;
}

static duk_ret_t
js_Animation_readNextFrame(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	animation_update(anim);
	return 0;
}

static duk_ret_t
js_Font_finalize(duk_context* ctx)
{
	font_t* font;

	font = duk_require_sphere_obj(ctx, 0, "Font");
	font_free(font);
	return 0;
}

static duk_ret_t
js_Font_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object font]");
	return 1;
}

static duk_ret_t
js_Font_getCharacterImage(duk_context* ctx)
{
	uint32_t cp;
	font_t*  font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	cp = duk_require_uint(ctx, 0);

	duk_push_sphere_image(ctx, font_glyph(font, cp));
	return 1;
}

static duk_ret_t
js_Font_get_colorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_Font_set_colorMask(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "color_mask"); duk_pop(ctx);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Font_get_height(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	duk_push_int(ctx, font_height(font));
	return 1;
}

static duk_ret_t
js_Font_setCharacterImage(duk_context* ctx)
{
	int cp = duk_require_int(ctx, 0);
	image_t* image = duk_require_sphere_image(ctx, 1);

	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");

	font_set_glyph(font, cp, image);
	return 0;
}

static duk_ret_t
js_Font_clone(duk_context* ctx)
{
	font_t* dolly_font;
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	if (!(dolly_font = font_clone(font)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Font:clone(): unable to clone font");
	duk_push_sphere_font(ctx, dolly_font);
	return 1;
}

static duk_ret_t
js_Font_drawText(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	const char* text = duk_to_string(ctx, 2);

	font_t* font;
	color_t mask;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen))
		font_draw_text(font, mask, x, y, TEXT_ALIGN_LEFT, text);
	return 0;
}

static duk_ret_t
js_Font_drawZoomedText(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	float scale = duk_require_number(ctx, 2);
	const char* text = duk_to_string(ctx, 3);

	ALLEGRO_BITMAP* bitmap;
	font_t*         font;
	color_t         mask;
	int             text_w, text_h;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen)) {
		text_w = font_get_width(font, text);
		text_h = font_height(font);
		bitmap = al_create_bitmap(text_w, text_h);
		al_set_target_bitmap(bitmap);
		font_draw_text(font, mask, 0, 0, TEXT_ALIGN_LEFT, text);
		al_set_target_backbuffer(screen_display(g_screen));
		al_draw_scaled_bitmap(bitmap, 0, 0, text_w, text_h, x, y, text_w * scale, text_h * scale, 0x0);
		al_destroy_bitmap(bitmap);
	}
	return 0;
}

static duk_ret_t
js_Font_drawTextBox(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	int offset = duk_require_int(ctx, 4);
	const char* text = duk_to_string(ctx, 5);

	font_t*     font;
	int         line_height;
	const char* line_text;
	color_t     mask;
	int         num_lines;

	int i;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen)) {
		duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS);
		duk_push_this(ctx);
		duk_push_string(ctx, text);
		duk_push_int(ctx, w);
		duk_call_method(ctx, 2);
		duk_get_prop_string(ctx, -1, "length"); num_lines = duk_get_int(ctx, -1); duk_pop(ctx);
		line_height = font_height(font);
		for (i = 0; i < num_lines; ++i) {
			duk_get_prop_index(ctx, -1, i); line_text = duk_get_string(ctx, -1); duk_pop(ctx);
			font_draw_text(font, mask, x + offset, y, TEXT_ALIGN_LEFT, line_text);
			y += line_height;
		}
		duk_pop(ctx);
	}
	return 0;
}

static duk_ret_t
js_Font_getStringHeight(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	int width = duk_require_int(ctx, 1);

	font_t* font;
	int     num_lines;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS);
	duk_push_this(ctx);
	duk_push_string(ctx, text);
	duk_push_int(ctx, width);
	duk_call_method(ctx, 2);
	duk_get_prop_string(ctx, -1, "length"); num_lines = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, font_height(font) * num_lines);
	return 1;
}

static duk_ret_t
js_Font_getStringWidth(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);

	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	duk_push_int(ctx, font_get_width(font, text));
	return 1;
}

static duk_ret_t
js_Font_wordWrapString(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	int         width = duk_require_int(ctx, 1);

	font_t*     font;
	int         num_lines;
	wraptext_t* wraptext;

	int i;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	wraptext = wraptext_new(text, font, width);
	num_lines = wraptext_len(wraptext);
	duk_push_array(ctx);
	for (i = 0; i < num_lines; ++i) {
		duk_push_string(ctx, wraptext_line(wraptext, i));
		duk_put_prop_index(ctx, -2, i);
	}
	wraptext_free(wraptext);
	return 1;
}

static duk_ret_t
js_Logger_finalize(duk_context* ctx)
{
	logger_t* logger;

	logger = duk_require_sphere_obj(ctx, 0, "Logger");
	log_close(logger);
	return 0;
}

static duk_ret_t
js_Logger_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object log]");
	return 1;
}

static duk_ret_t
js_Logger_beginBlock(duk_context* ctx)
{
	const char* title = duk_to_string(ctx, 0);

	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	if (!log_begin_block(logger, title))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Log:beginBlock(): unable to create new log block");
	return 0;
}

static duk_ret_t
js_Logger_endBlock(duk_context* ctx)
{
	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	log_end_block(logger);
	return 0;
}

static duk_ret_t
js_Logger_write(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);

	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	log_write(logger, NULL, text);
	return 0;
}

static duk_ret_t
js_Socket_finalize(duk_context* ctx)
{
	socket_t* socket;

	socket = duk_require_sphere_obj(ctx, 0, "Socket");
	free_socket(socket);
	return 1;
}

static duk_ret_t
js_Socket_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object socket]");
	return 1;
}

static duk_ret_t
js_Socket_isConnected(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket != NULL)
		duk_push_boolean(ctx, is_socket_live(socket));
	else
		duk_push_false(ctx);
	return 1;
}

static duk_ret_t
js_Socket_getPendingReadSize(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getPendingReadSize(): socket has been closed");
	duk_push_uint(ctx, (duk_uint_t)peek_socket(socket));
	return 1;
}

static duk_ret_t
js_Socket_close(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_push_null(ctx); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	if (socket != NULL)
		free_socket(socket);
	return 1;
}

static duk_ret_t
js_Socket_read(duk_context* ctx)
{
	int length = duk_require_int(ctx, 0);

	bytearray_t* array;
	void*        read_buffer;
	socket_t*    socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (length <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Socket:read(): must read at least 1 byte (got: %i)", length);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): socket is not connected");
	if (!(read_buffer = malloc(length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): unable to allocate read buffer");
	read_socket(socket, read_buffer, length);
	if (!(array = bytearray_from_buffer(read_buffer, length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): unable to create byte array");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_Socket_readString(duk_context* ctx)
{
	size_t length = duk_require_uint(ctx, 0);

	uint8_t*  buffer;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): socket is not connected");
	if (!(buffer = malloc(length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): unable to allocate read buffer");
	read_socket(socket, buffer, length);
	duk_push_lstring(ctx, (char*)buffer, length);
	free(buffer);
	return 1;
}

static duk_ret_t
js_Socket_write(duk_context* ctx)
{
	bytearray_t*   array;
	const uint8_t* payload;
	socket_t*      socket;
	size_t         write_size;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (duk_is_string(ctx, 0))
		payload = (uint8_t*)duk_get_lstring(ctx, 0, &write_size);
	else {
		array = duk_require_sphere_bytearray(ctx, 0);
		payload = get_bytearray_buffer(array);
		write_size = get_bytearray_size(array);
	}
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): socket is not connected");
	write_socket(socket, payload, write_size);
	return 0;
}

static duk_ret_t
js_Sound_finalize(duk_context* ctx)
{
	sound_t* sound;

	sound = duk_require_sphere_obj(ctx, 0, "Sound");
	sound_free(sound);
	return 0;
}

static duk_ret_t
js_Sound_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object sound]");
	return 1;
}

static duk_ret_t
js_Sound_getVolume(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	duk_push_int(ctx, sound_gain(sound) * 255);
	return 1;
}

static duk_ret_t
js_Sound_setVolume(duk_context* ctx)
{
	int volume = duk_require_int(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	volume = volume < 0 ? 0 : volume > 255 ? 255 : volume;
	sound_set_gain(sound, (float)volume / 255);
	return 0;
}

static duk_ret_t
js_Sound_get_length(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	duk_push_number(ctx, floor(sound_len(sound) * 1000000));
	return 1;
}

static duk_ret_t
js_Sound_get_pan(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	duk_push_int(ctx, sound_pan(sound) * 255);
	return 1;
}

static duk_ret_t
js_Sound_set_pan(duk_context* ctx)
{
	int new_pan = duk_require_int(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	sound_set_pan(sound, (float)new_pan / 255);
	return 0;
}

static duk_ret_t
js_Sound_get_pitch(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	duk_push_number(ctx, sound_speed(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_pitch(duk_context* ctx)
{
	float new_pitch = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	sound_set_speed(sound, new_pitch);
	return 0;
}

static duk_ret_t
js_Sound_get_playing(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	duk_push_boolean(ctx, sound_playing(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_position(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	duk_push_number(ctx, floor(sound_tell(sound) * 1000000));
	return 1;
}

static duk_ret_t
js_Sound_set_position(duk_context* ctx)
{
	double   new_pos;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	new_pos = duk_require_number(ctx, 0);

	sound_seek(sound, floor(new_pos) / 1000000);
	return 0;
}

static duk_ret_t
js_Sound_get_repeat(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	duk_push_boolean(ctx, sound_repeat(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_repeat(duk_context* ctx)
{
	bool is_looped = duk_require_boolean(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	sound_set_repeat(sound, is_looped);
	return 0;
}

static duk_ret_t
js_Sound_get_seekable(duk_context* ctx)
{
	duk_push_true(ctx);
	return 1;
}

static duk_ret_t
js_Sound_get_volume(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	duk_push_number(ctx, sound_gain(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_volume(duk_context* ctx)
{
	float volume = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	sound_set_gain(sound, volume);
	return 0;
}

static duk_ret_t
js_Sound_pause(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	sound_pause(sound, true);
	return 0;
}

static duk_ret_t
js_Sound_play(duk_context* ctx)
{
	mixer_t* mixer;
	int      num_args;
	sound_t* sound;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	// Sound:play() has some really convoluted argument semantics.
	// unfortunately, this is necessary to be able to maintain Sphere 1.x compatibility
	// and still support mixers.
	if (num_args >= 1 && duk_is_sphere_obj(ctx, 0, "Mixer")) {
		if (num_args >= 2)
			sound_set_repeat(sound, duk_require_boolean(ctx, 1));
		else
			sound_set_repeat(sound, false);
		mixer = duk_require_sphere_obj(ctx, 0, "Mixer");
		sound_play(sound, mixer);
	}
	else {
		if (num_args >= 1) {
			sound_set_repeat(sound, duk_require_boolean(ctx, 0));
			sound_play(sound, s_def_mixer);
		}
		else {
			sound_pause(sound, false);
			if (!sound_playing(sound))
				sound_play(sound, s_def_mixer);
		}
	}

	return 0;
}

static duk_ret_t
js_Sound_reset(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	sound_seek(sound, 0.0);
	return 0;
}

static duk_ret_t
js_Sound_stop(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	sound_stop(sound);
	return 0;
}

static duk_ret_t
js_Spriteset_finalize(duk_context* ctx)
{
	spriteset_t* spriteset;

	spriteset = duk_require_sphere_obj(ctx, 0, "Spriteset");
	free_spriteset(spriteset);
	return 0;
}

static duk_ret_t
js_Spriteset_get_filename(duk_context* ctx)
{
	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "Spriteset");
	duk_pop(ctx);
	duk_push_string(ctx, spriteset->filename);
	return 1;
}

static duk_ret_t
js_Spriteset_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object spriteset]");
	return 1;
}

static duk_ret_t
js_Spriteset_clone(duk_context* ctx)
{
	spriteset_t* new_spriteset;
	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "Spriteset");
	duk_pop(ctx);
	if ((new_spriteset = clone_spriteset(spriteset)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Spriteset:clone(): unable to create new spriteset");
	duk_push_sphere_spriteset(ctx, new_spriteset);
	free_spriteset(new_spriteset);
	return 1;
}

static duk_ret_t
js_Spriteset_get_image(duk_context* ctx)
{
	duk_uarridx_t index = duk_to_int(ctx, 0);

	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "Spriteset");
	duk_pop(ctx);
	duk_push_sphere_image(ctx, get_spriteset_image(spriteset, index));
	return 1;
}

static duk_ret_t
js_Spriteset_set_image(duk_context* ctx)
{
	image_t* image = duk_require_sphere_image(ctx, 0);
	duk_uarridx_t index = duk_to_int(ctx, 1);

	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "Spriteset");
	duk_pop(ctx);
	set_spriteset_image(spriteset, index, image);
	return 0;
}

static duk_ret_t
js_Surface_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_sphere_obj(ctx, 0, "Surface");
	image_free(image);
	return 0;
}

static duk_ret_t
js_Surface_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	if (image != NULL)
		duk_push_int(ctx, image_height(image));
	else
		duk_push_int(ctx, g_res_y);
	return 1;
}

static duk_ret_t
js_Surface_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	if (image != NULL)
		duk_push_int(ctx, image_width(image));
	else
		duk_push_int(ctx, g_res_x);
	return 1;
}

static duk_ret_t
js_Surface_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object surface]");
	return 1;
}

static duk_ret_t
js_Surface_setPixel(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	color_t color = duk_require_sphere_color(ctx, 2);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	image_set_pixel(image, x, y, color);
	return 0;
}

static duk_ret_t
js_Surface_getPixel(duk_context* ctx)
{
	int      height;
	image_t* image;
	color_t  pixel;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);

	width = image_width(image);
	height = image_height(image);
	if (x < 0 || x >= width || y < 0 || y >= height)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Surface:getPixel(): X/Y out of range (%i,%i) for %ix%i surface", x, y, width, height);
	pixel = image_get_pixel(image, x, y);
	duk_push_sphere_color(ctx, pixel);
	return 1;
}

static duk_ret_t
js_Surface_applyColorFX(duk_context* ctx)
{
	colormatrix_t matrix;
	int           height;
	int           width;
	int           x;
	int           y;

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);
	width = duk_require_int(ctx, 2);
	height = duk_require_int(ctx, 3);
	matrix = duk_require_sphere_colormatrix(ctx, 4);

	if (x < 0 || y < 0 || x + width > image_width(image) || y + height > image_height(image))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Surface:applyColorFX(): area of effect extends past image (%i,%i,%i,%i)", x, y, width, height);
	if (!image_apply_colormat(image, matrix, x, y, width, height))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:applyColorFX(): unable to apply transformation");
	return 0;
}

static duk_ret_t
js_Surface_applyColorFX4(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	colormatrix_t ul_mat = duk_require_sphere_colormatrix(ctx, 4);
	colormatrix_t ur_mat = duk_require_sphere_colormatrix(ctx, 5);
	colormatrix_t ll_mat = duk_require_sphere_colormatrix(ctx, 6);
	colormatrix_t lr_mat = duk_require_sphere_colormatrix(ctx, 7);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	if (x < 0 || y < 0 || x + w > image_width(image) || y + h > image_height(image))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Surface:applyColorFX4(): area of effect extends past image (%i,%i,%i,%i)", x, y, w, h);
	if (!image_apply_colormat_4(image, ul_mat, ur_mat, ll_mat, lr_mat, x, y, w, h))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:applyColorFX4(): unable to apply transformation");
	return 0;
}

static duk_ret_t
js_Surface_applyLookup(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	uint8_t* red_lu = duk_require_rgba_lut(ctx, 4);
	uint8_t* green_lu = duk_require_rgba_lut(ctx, 5);
	uint8_t* blue_lu = duk_require_rgba_lut(ctx, 6);
	uint8_t* alpha_lu = duk_require_rgba_lut(ctx, 7);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	if (x < 0 || y < 0 || x + w > image_width(image) || y + h > image_height(image))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Surface:applyColorFX(): area of effect extends past image (%i,%i,%i,%i)", x, y, w, h);
	if (!image_apply_lookup(image, x, y, w, h, red_lu, green_lu, blue_lu, alpha_lu))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:applyLookup(): unable to apply lookup transformation");
	free(red_lu);
	free(green_lu);
	free(blue_lu);
	free(alpha_lu);
	return 0;
}

static duk_ret_t
js_Surface_blit(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	if (!screen_is_skipframe(g_screen))
		al_draw_bitmap(image_bitmap(image), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Surface_blitMaskSurface(duk_context* ctx)
{
	image_t* src_image = duk_require_sphere_obj(ctx, 0, "Surface");
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);
	color_t mask = duk_require_sphere_color(ctx, 3);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);

	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));
	al_draw_tinted_bitmap(image_bitmap(src_image), nativecolor(mask), x, y, 0x0);
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_blitSurface(duk_context* ctx)
{
	image_t* src_image = duk_require_sphere_obj(ctx, 0, "Surface");
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);

	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));
	al_draw_bitmap(image_bitmap(src_image), x, y, 0x0);
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_clone(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	if ((new_image = image_clone(image)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:clone() - Unable to create new surface image");
	duk_push_sphere_obj(ctx, "Surface", new_image);
	return 1;
}

static duk_ret_t
js_Surface_cloneSection(duk_context* ctx)
{
	int      height;
	image_t* image;
	image_t* new_image;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);
	width = duk_require_int(ctx, 2);
	height = duk_require_int(ctx, 3);

	if ((new_image = image_new(width, height)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:cloneSection(): unable to create surface");
	al_set_target_bitmap(image_bitmap(new_image));
	al_draw_bitmap_region(image_bitmap(image), x, y, width, height, 0, 0, 0x0);
	al_set_target_backbuffer(screen_display(g_screen));
	duk_push_sphere_obj(ctx, "Surface", new_image);
	return 1;
}

static duk_ret_t
js_Surface_createImage(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	if ((new_image = image_clone(image)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:createImage(): unable to create image");
	duk_push_sphere_image(ctx, new_image);
	image_free(new_image);
	return 1;
}

static duk_ret_t
js_Surface_drawText(duk_context* ctx)
{
	font_t* font = duk_require_sphere_obj(ctx, 0, "Font");
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);
	const char* text = duk_to_string(ctx, 3);

	int      blend_mode;
	color_t  color;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);

	duk_get_prop_string(ctx, 0, "\xFF" "color_mask"); color = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));
	font_draw_text(font, color, x, y, TEXT_ALIGN_LEFT, text);
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_filledCircle(duk_context* ctx)
{
	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);
	int radius = duk_require_number(ctx, 2);
	color_t color = duk_require_sphere_color(ctx, 3);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));
	al_draw_filled_circle(x, y, radius, nativecolor(color));
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_flipHorizontally(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_pop(ctx);
	image_flip(image, true, false);
	return 0;
}

static duk_ret_t
js_Surface_flipVertically(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_pop(ctx);
	image_flip(image, false, true);
	return 0;
}

static duk_ret_t
js_Surface_gradientCircle(duk_context* ctx)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);
	int radius = duk_require_number(ctx, 2);
	color_t in_color = duk_require_sphere_color(ctx, 3);
	color_t out_color = duk_require_sphere_color(ctx, 4);

	int      blend_mode;
	image_t* image;
	double   phi;
	int      vcount;

	int i;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));
	vcount = fmin(radius, 126);
	s_vbuf[0].x = x; s_vbuf[0].y = y; s_vbuf[0].z = 0;
	s_vbuf[0].color = nativecolor(in_color);
	for (i = 0; i < vcount; ++i) {
		phi = 2 * M_PI * i / vcount;
		s_vbuf[i + 1].x = x + cos(phi) * radius;
		s_vbuf[i + 1].y = y - sin(phi) * radius;
		s_vbuf[i + 1].z = 0;
		s_vbuf[i + 1].color = nativecolor(out_color);
	}
	s_vbuf[i + 1].x = x + cos(0) * radius;
	s_vbuf[i + 1].y = y - sin(0) * radius;
	s_vbuf[i + 1].z = 0;
	s_vbuf[i + 1].color = nativecolor(out_color);
	al_draw_prim(s_vbuf, NULL, NULL, 0, vcount + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_gradientRectangle(duk_context* ctx)
{
	int x1 = duk_require_int(ctx, 0);
	int y1 = duk_require_int(ctx, 1);
	int x2 = x1 + duk_require_int(ctx, 2);
	int y2 = y1 + duk_require_int(ctx, 3);
	color_t color_ul = duk_require_sphere_color(ctx, 4);
	color_t color_ur = duk_require_sphere_color(ctx, 5);
	color_t color_lr = duk_require_sphere_color(ctx, 6);
	color_t color_ll = duk_require_sphere_color(ctx, 7);

	int           blend_mode;
	image_t*      image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));

	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, nativecolor(color_ul) },
		{ x2, y1, 0, 0, 0, nativecolor(color_ur) },
		{ x1, y2, 0, 0, 0, nativecolor(color_ll) },
		{ x2, y2, 0, 0, 0, nativecolor(color_lr) }
	};
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_line(duk_context* ctx)
{
	float x1 = duk_require_int(ctx, 0) + 0.5;
	float y1 = duk_require_int(ctx, 1) + 0.5;
	float x2 = duk_require_int(ctx, 2) + 0.5;
	float y2 = duk_require_int(ctx, 3) + 0.5;
	color_t color = duk_require_sphere_color(ctx, 4);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));
	al_draw_line(x1, y1, x2, y2, nativecolor(color), 1);
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_outlinedCircle(duk_context* ctx)
{
	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);
	int radius = duk_require_number(ctx, 2);
	color_t color = duk_require_sphere_color(ctx, 3);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));
	al_draw_circle(x, y, radius, nativecolor(color), 1);
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_pointSeries(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 1);

	int             blend_mode;
	image_t*        image;
	size_t          num_points;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	unsigned int i;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:pointSeries(): first argument must be an array");
	duk_get_prop_string(ctx, 0, "length"); num_points = duk_get_uint(ctx, 0); duk_pop(ctx);
	if (num_points > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Surface:pointSeries(): too many vertices (%u)", num_points);
	vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX));
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, i);
		duk_get_prop_string(ctx, 0, "x"); x = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, 0, "y"); y = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points, ALLEGRO_PRIM_POINT_LIST);
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	free(vertices);
	return 0;
}

static duk_ret_t
js_Surface_outlinedRectangle(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float x1 = duk_require_int(ctx, 0) + 0.5;
	float y1 = duk_require_int(ctx, 1) + 0.5;
	float x2 = x1 + duk_require_int(ctx, 2) - 1;
	float y2 = y1 + duk_require_int(ctx, 3) - 1;
	color_t color = duk_require_sphere_color(ctx, 4);
	int thickness = n_args >= 6 ? duk_require_int(ctx, 5) : 1;

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));
	al_draw_rectangle(x1, y1, x2, y2, nativecolor(color), thickness);
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_replaceColor(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 0);
	color_t new_color = duk_require_sphere_color(ctx, 1);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_pop(ctx);
	if (!image_replace_color(image, color, new_color))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:replaceColor() - Failed to perform replacement");
	return 0;
}

static duk_ret_t
js_Surface_rescale(duk_context* ctx)
{
	int width = duk_require_int(ctx, 0);
	int height = duk_require_int(ctx, 1);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_pop(ctx);
	if (!image_rescale(image, width, height))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:rescale() - Failed to rescale image");
	duk_push_this(ctx);
	return 1;
}

static duk_ret_t
js_Surface_rotate(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float angle = duk_require_number(ctx, 0);
	bool want_resize = n_args >= 2 ? duk_require_boolean(ctx, 1) : true;

	image_t* image;
	image_t* new_image;
	int      new_w, new_h;
	int      w, h;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_pop(ctx);
	w = new_w = image_width(image);
	h = new_h = image_height(image);
	if (want_resize) {
		// TODO: implement in-place resizing for Surface:rotate()
	}
	if ((new_image = image_new(new_w, new_h)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:rotate() - Failed to create new surface bitmap");
	al_set_target_bitmap(image_bitmap(new_image));
	al_draw_rotated_bitmap(image_bitmap(image), (float)w / 2, (float)h / 2, (float)new_w / 2, (float)new_h / 2, angle, 0x0);
	al_set_target_backbuffer(screen_display(g_screen));

	// free old image and replace internal image pointer
	// at one time this was an acceptable thing to do; now it's just a hack
	image_free(image);
	duk_push_this(ctx);
	duk_push_pointer(ctx, new_image); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	return 1;
}

static duk_ret_t
js_Surface_rectangle(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);

	image_t* image;
	int      blend_mode;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_get_prop_string(ctx, -1, "\xFF" "blend_mode");
	blend_mode = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	apply_blend_mode(blend_mode);
	al_set_target_bitmap(image_bitmap(image));
	al_draw_filled_rectangle(x, y, x + w, y + h, nativecolor(color));
	al_set_target_backbuffer(screen_display(g_screen));
	reset_blender();
	return 0;
}

static duk_ret_t
js_Surface_save(duk_context* ctx)
{
	const char* filename;
	image_t*    image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_pop(ctx);
	filename = duk_require_path(ctx, 0, "images", true);
	image_save(image, filename);
	return 1;
}

static duk_ret_t
js_Surface_setAlpha(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int alpha = duk_require_int(ctx, 0);
	bool want_all = n_args >= 2 ? duk_require_boolean(ctx, 1) : true;

	image_t*      image;
	image_lock_t* lock;
	color_t*      pixel;
	int           w, h;

	int i_x, i_y;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");
	duk_pop(ctx);
	if (!(lock = image_lock(image)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:setAlpha(): unable to lock surface");
	w = image_width(image);
	h = image_height(image);
	alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;
	for (i_y = h - 1; i_y >= 0; --i_y) for (i_x = w - 1; i_x >= 0; --i_x) {
		pixel = &lock->pixels[i_x + i_y * lock->pitch];
		pixel->alpha = want_all || pixel->alpha != 0
			? alpha : pixel->alpha;
	}
	image_unlock(image, lock);
	return 0;
}

static duk_ret_t
js_Surface_setBlendMode(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "blend_mode");
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_WindowStyle_finalize(duk_context* ctx)
{
	windowstyle_t* winstyle;

	winstyle = duk_require_sphere_obj(ctx, 0, "WindowStyle");
	free_windowstyle(winstyle);
	return 0;
}

static duk_ret_t
js_WindowStyle_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object windowstyle]");
	return 1;
}

static duk_ret_t
js_WindowStyle_get_colorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_require_sphere_obj(ctx, -1, "WindowStyle");
	duk_get_prop_string(ctx, -2, "\xFF" "color_mask");
	duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_WindowStyle_set_colorMask(duk_context* ctx)
{
	color_t mask = duk_require_sphere_color(ctx, 0);

	duk_push_this(ctx);
	duk_require_sphere_obj(ctx, -1, "WindowStyle");
	duk_push_sphere_color(ctx, mask);
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_WindowStyle_drawWindow(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);

	color_t        mask;
	windowstyle_t* winstyle;

	duk_push_this(ctx);
	winstyle = duk_require_sphere_obj(ctx, -1, "WindowStyle");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	draw_window(winstyle, mask, x, y, w, h);
	return 0;
}
