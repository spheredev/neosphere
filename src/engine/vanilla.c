#include "minisphere.h"
#include "vanilla.h"

#include "animation.h"
#include "api.h"
#include "async.h"
#include "audio.h"
#include "bytearray.h"
#include "debugger.h"
#include "font.h"
#include "image.h"
#include "input.h"
#include "logger.h"
#include "sockets.h"
#include "spriteset.h"
#include "windowstyle.h"

#define API_VERSION        1.5
#define API_VERSION_STRING "v1.5"

static duk_ret_t js_AreKeysLeft                (duk_context* ctx);
static duk_ret_t js_IsAnyKeyPressed            (duk_context* ctx);
static duk_ret_t js_IsJoystickButtonPressed    (duk_context* ctx);
static duk_ret_t js_IsKeyPressed               (duk_context* ctx);
static duk_ret_t js_IsMouseButtonPressed       (duk_context* ctx);
static duk_ret_t js_GetClippingRectangle       (duk_context* ctx);
static duk_ret_t js_GetDirectoryList           (duk_context* ctx);
static duk_ret_t js_GetFileList                (duk_context* ctx);
static duk_ret_t js_GetFrameRate               (duk_context* ctx);
static duk_ret_t js_GetGameList                (duk_context* ctx);
static duk_ret_t js_GetLocalAddress            (duk_context* ctx);
static duk_ret_t js_GetLocalName               (duk_context* ctx);
static duk_ret_t js_GetJoystickAxis            (duk_context* ctx);
static duk_ret_t js_GetKey                     (duk_context* ctx);
static duk_ret_t js_GetKeyString               (duk_context* ctx);
static duk_ret_t js_GetMouseWheelEvent         (duk_context* ctx);
static duk_ret_t js_GetMouseX                  (duk_context* ctx);
static duk_ret_t js_GetMouseY                  (duk_context* ctx);
static duk_ret_t js_GetNumJoysticks            (duk_context* ctx);
static duk_ret_t js_GetNumJoystickAxes         (duk_context* ctx);
static duk_ret_t js_GetNumJoystickButtons      (duk_context* ctx);
static duk_ret_t js_GetNumMouseWheelEvents     (duk_context* ctx);
static duk_ret_t js_GetPlayerKey               (duk_context* ctx);
static duk_ret_t js_GetScreenHeight            (duk_context* ctx);
static duk_ret_t js_GetScreenWidth             (duk_context* ctx);
static duk_ret_t js_GetSystemArrow             (duk_context* ctx);
static duk_ret_t js_GetSystemDownArrow         (duk_context* ctx);
static duk_ret_t js_GetSystemFont              (duk_context* ctx);
static duk_ret_t js_GetSystemUpArrow           (duk_context* ctx);
static duk_ret_t js_GetSystemWindowStyle       (duk_context* ctx);
static duk_ret_t js_GetTime                    (duk_context* ctx);
static duk_ret_t js_GetToggleState             (duk_context* ctx);
static duk_ret_t js_GetVersion                 (duk_context* ctx);
static duk_ret_t js_GetVersionString           (duk_context* ctx);
static duk_ret_t js_SetClippingRectangle       (duk_context* ctx);
static duk_ret_t js_SetFrameRate               (duk_context* ctx);
static duk_ret_t js_SetMousePosition           (duk_context* ctx);
static duk_ret_t js_Abort                      (duk_context* ctx);
static duk_ret_t js_ApplyColorMask             (duk_context* ctx);
static duk_ret_t js_BindJoystickButton         (duk_context* ctx);
static duk_ret_t js_BindKey                    (duk_context* ctx);
static duk_ret_t js_BlendColors                (duk_context* ctx);
static duk_ret_t js_CreateByteArray            (duk_context* ctx);
static duk_ret_t js_CreateByteArrayFromString  (duk_context* ctx);
static duk_ret_t js_CreateColor                (duk_context* ctx);
static duk_ret_t js_CreateColorMatrix          (duk_context* ctx);
static duk_ret_t js_CreateStringFromByteArray  (duk_context* ctx);
static duk_ret_t js_CreateDirectory            (duk_context* ctx);
static duk_ret_t js_CreateStringFromCode       (duk_context* ctx);
static duk_ret_t js_CreateSurface              (duk_context* ctx);
static duk_ret_t js_DeflateByteArray           (duk_context* ctx);
static duk_ret_t js_DoesFileExist              (duk_context* ctx);
static duk_ret_t js_Delay                      (duk_context* ctx);
static duk_ret_t js_EvaluateScript             (duk_context* ctx);
static duk_ret_t js_EvaluateSystemScript       (duk_context* ctx);
static duk_ret_t js_ExecuteGame                (duk_context* ctx);
static duk_ret_t js_Exit                       (duk_context* ctx);
static duk_ret_t js_FlipScreen                 (duk_context* ctx);
static duk_ret_t js_GarbageCollect             (duk_context* ctx);
static duk_ret_t js_GrabImage                  (duk_context* ctx);
static duk_ret_t js_GrabSurface                (duk_context* ctx);
static duk_ret_t js_GradientCircle             (duk_context* ctx);
static duk_ret_t js_GradientRectangle          (duk_context* ctx);
static duk_ret_t js_HashByteArray              (duk_context* ctx);
static duk_ret_t js_HashFromFile               (duk_context* ctx);
static duk_ret_t js_InflateByteArray           (duk_context* ctx);
static duk_ret_t js_Line                       (duk_context* ctx);
static duk_ret_t js_LineSeries                 (duk_context* ctx);
static duk_ret_t js_ListenOnPort               (duk_context* ctx);
static duk_ret_t js_LoadAnimation              (duk_context* ctx);
static duk_ret_t js_LoadFont                   (duk_context* ctx);
static duk_ret_t js_LoadImage                  (duk_context* ctx);
static duk_ret_t js_LoadSound                  (duk_context* ctx);
static duk_ret_t js_LoadSpriteset              (duk_context* ctx);
static duk_ret_t js_LoadSurface                (duk_context* ctx);
static duk_ret_t js_LoadWindowStyle            (duk_context* ctx);
static duk_ret_t js_OpenAddress                (duk_context* ctx);
static duk_ret_t js_OpenFile                   (duk_context* ctx);
static duk_ret_t js_OpenLog                    (duk_context* ctx);
static duk_ret_t js_OpenRawFile                (duk_context* ctx);
static duk_ret_t js_OutlinedCircle             (duk_context* ctx);
static duk_ret_t js_OutlinedRectangle          (duk_context* ctx);
static duk_ret_t js_OutlinedRoundRectangle     (duk_context* ctx);
static duk_ret_t js_Point                      (duk_context* ctx);
static duk_ret_t js_PointSeries                (duk_context* ctx);
static duk_ret_t js_Print                      (duk_context* ctx);
static duk_ret_t js_Rectangle                  (duk_context* ctx);
static duk_ret_t js_RemoveDirectory            (duk_context* ctx);
static duk_ret_t js_RemoveFile                 (duk_context* ctx);
static duk_ret_t js_Rename                     (duk_context* ctx);
static duk_ret_t js_RequireScript              (duk_context* ctx);
static duk_ret_t js_RequireSystemScript        (duk_context* ctx);
static duk_ret_t js_RestartGame                (duk_context* ctx);
static duk_ret_t js_RoundRectangle             (duk_context* ctx);
static duk_ret_t js_Triangle                   (duk_context* ctx);
static duk_ret_t js_UnbindJoystickButton       (duk_context* ctx);
static duk_ret_t js_UnbindKey                  (duk_context* ctx);
static duk_ret_t js_Animation_finalize         (duk_context* ctx);
static duk_ret_t js_Animation_get_height       (duk_context* ctx);
static duk_ret_t js_Animation_get_width        (duk_context* ctx);
static duk_ret_t js_Animation_drawFrame        (duk_context* ctx);
static duk_ret_t js_Animation_drawZoomedFrame  (duk_context* ctx);
static duk_ret_t js_Animation_getDelay         (duk_context* ctx);
static duk_ret_t js_Animation_getNumFrames     (duk_context* ctx);
static duk_ret_t js_Animation_readNextFrame    (duk_context* ctx);
static duk_ret_t js_ByteArray_finalize         (duk_context* ctx);
static duk_ret_t js_ByteArray_proxy_get        (duk_context* ctx);
static duk_ret_t js_ByteArray_proxy_set        (duk_context* ctx);
static duk_ret_t js_ByteArray_get_length       (duk_context* ctx);
static duk_ret_t js_ByteArray_concat           (duk_context* ctx);
static duk_ret_t js_ByteArray_slice            (duk_context* ctx);
static duk_ret_t js_ByteArray_toString         (duk_context* ctx);
static duk_ret_t js_Color_toString             (duk_context* ctx);
static duk_ret_t js_ColorMatrix_toString       (duk_context* ctx);
static duk_ret_t js_File_finalize              (duk_context* ctx);
static duk_ret_t js_File_close                 (duk_context* ctx);
static duk_ret_t js_File_flush                 (duk_context* ctx);
static duk_ret_t js_File_getKey                (duk_context* ctx);
static duk_ret_t js_File_getNumKeys            (duk_context* ctx);
static duk_ret_t js_File_read                  (duk_context* ctx);
static duk_ret_t js_File_toString              (duk_context* ctx);
static duk_ret_t js_File_write                 (duk_context* ctx);
static duk_ret_t js_Font_finalize              (duk_context* ctx);
static duk_ret_t js_Font_clone                 (duk_context* ctx);
static duk_ret_t js_Font_drawText              (duk_context* ctx);
static duk_ret_t js_Font_drawTextBox           (duk_context* ctx);
static duk_ret_t js_Font_drawZoomedText        (duk_context* ctx);
static duk_ret_t js_Font_getCharacterImage     (duk_context* ctx);
static duk_ret_t js_Font_getColorMask          (duk_context* ctx);
static duk_ret_t js_Font_getHeight             (duk_context* ctx);
static duk_ret_t js_Font_getStringHeight       (duk_context* ctx);
static duk_ret_t js_Font_getStringWidth        (duk_context* ctx);
static duk_ret_t js_Font_setCharacterImage     (duk_context* ctx);
static duk_ret_t js_Font_setColorMask          (duk_context* ctx);
static duk_ret_t js_Font_toString              (duk_context* ctx);
static duk_ret_t js_Font_wordWrapString        (duk_context* ctx);
static duk_ret_t js_Image_finalize             (duk_context* ctx);
static duk_ret_t js_Image_get_height           (duk_context* ctx);
static duk_ret_t js_Image_get_width            (duk_context* ctx);
static duk_ret_t js_Image_blit                 (duk_context* ctx);
static duk_ret_t js_Image_blitMask             (duk_context* ctx);
static duk_ret_t js_Image_createSurface        (duk_context* ctx);
static duk_ret_t js_Image_rotateBlit           (duk_context* ctx);
static duk_ret_t js_Image_rotateBlitMask       (duk_context* ctx);
static duk_ret_t js_Image_toString             (duk_context* ctx);
static duk_ret_t js_Image_transformBlit        (duk_context* ctx);
static duk_ret_t js_Image_transformBlitMask    (duk_context* ctx);
static duk_ret_t js_Image_zoomBlit             (duk_context* ctx);
static duk_ret_t js_Image_zoomBlitMask         (duk_context* ctx);
static duk_ret_t js_Logger_finalize            (duk_context* ctx);
static duk_ret_t js_Logger_beginBlock          (duk_context* ctx);
static duk_ret_t js_Logger_endBlock            (duk_context* ctx);
static duk_ret_t js_Logger_toString            (duk_context* ctx);
static duk_ret_t js_Logger_write               (duk_context* ctx);
static duk_ret_t js_RawFile_finalize           (duk_context* ctx);
static duk_ret_t js_RawFile_close              (duk_context* ctx);
static duk_ret_t js_RawFile_getPosition        (duk_context* ctx);
static duk_ret_t js_RawFile_getSize            (duk_context* ctx);
static duk_ret_t js_RawFile_read               (duk_context* ctx);
static duk_ret_t js_RawFile_setPosition        (duk_context* ctx);
static duk_ret_t js_RawFile_toString           (duk_context* ctx);
static duk_ret_t js_RawFile_write              (duk_context* ctx);
static duk_ret_t js_Socket_finalize            (duk_context* ctx);
static duk_ret_t js_Socket_close               (duk_context* ctx);
static duk_ret_t js_Socket_getPendingReadSize  (duk_context* ctx);
static duk_ret_t js_Socket_isConnected         (duk_context* ctx);
static duk_ret_t js_Socket_read                (duk_context* ctx);
static duk_ret_t js_Socket_toString            (duk_context* ctx);
static duk_ret_t js_Socket_write               (duk_context* ctx);
static duk_ret_t js_Sound_finalize             (duk_context* ctx);
static duk_ret_t js_Sound_getLength            (duk_context* ctx);
static duk_ret_t js_Sound_getPan               (duk_context* ctx);
static duk_ret_t js_Sound_getPitch             (duk_context* ctx);
static duk_ret_t js_Sound_getPosition          (duk_context* ctx);
static duk_ret_t js_Sound_getRepeat            (duk_context* ctx);
static duk_ret_t js_Sound_getVolume            (duk_context* ctx);
static duk_ret_t js_Sound_isPlaying            (duk_context* ctx);
static duk_ret_t js_Sound_isSeekable           (duk_context* ctx);
static duk_ret_t js_Sound_pause                (duk_context* ctx);
static duk_ret_t js_Sound_play                 (duk_context* ctx);
static duk_ret_t js_Sound_reset                (duk_context* ctx);
static duk_ret_t js_Sound_setPan               (duk_context* ctx);
static duk_ret_t js_Sound_setPitch             (duk_context* ctx);
static duk_ret_t js_Sound_setPosition          (duk_context* ctx);
static duk_ret_t js_Sound_setRepeat            (duk_context* ctx);
static duk_ret_t js_Sound_setVolume            (duk_context* ctx);
static duk_ret_t js_Sound_stop                 (duk_context* ctx);
static duk_ret_t js_Sound_toString             (duk_context* ctx);
static duk_ret_t js_Spriteset_finalize         (duk_context* ctx);
static duk_ret_t js_Spriteset_get_filename     (duk_context* ctx);
static duk_ret_t js_Spriteset_get_image        (duk_context* ctx);
static duk_ret_t js_Spriteset_set_image        (duk_context* ctx);
static duk_ret_t js_Spriteset_clone            (duk_context* ctx);
static duk_ret_t js_Spriteset_toString         (duk_context* ctx);
static duk_ret_t js_Surface_finalize           (duk_context* ctx);
static duk_ret_t js_Surface_get_height         (duk_context* ctx);
static duk_ret_t js_Surface_get_width          (duk_context* ctx);
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
static duk_ret_t js_Surface_getPixel           (duk_context* ctx);
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
static duk_ret_t js_Surface_setAlpha           (duk_context* ctx);
static duk_ret_t js_Surface_setBlendMode       (duk_context* ctx);
static duk_ret_t js_Surface_setPixel           (duk_context* ctx);
static duk_ret_t js_Surface_toString           (duk_context* ctx);
static duk_ret_t js_WindowStyle_finalize       (duk_context* ctx);
static duk_ret_t js_WindowStyle_drawWindow     (duk_context* ctx);
static duk_ret_t js_WindowStyle_getColorMask   (duk_context* ctx);
static duk_ret_t js_WindowStyle_setColorMask   (duk_context* ctx);
static duk_ret_t js_WindowStyle_toString       (duk_context* ctx);

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

static unsigned int   s_next_async_id = 1;
static mixer_t*       s_sound_mixer;
static image_t*       s_sys_arrow = NULL;
static image_t*       s_sys_dn_arrow = NULL;
static image_t*       s_sys_up_arrow = NULL;
static windowstyle_t* s_sys_winstyle;

void
initialize_vanilla_api(duk_context* ctx)
{
	const char* filename;
	
	console_log(1, "initializing Sphere G1 API %s", API_VERSION_STRING);

	s_sound_mixer = mixer_new(44100, 16, 2);
	
	// load system-provided images
	if (g_sys_conf != NULL) {
		filename = kev_read_string(g_sys_conf, "Arrow", "pointer.png");
		s_sys_arrow = image_load(systempath(filename));
		filename = kev_read_string(g_sys_conf, "UpArrow", "up_arrow.png");
		s_sys_up_arrow = image_load(systempath(filename));
		filename = kev_read_string(g_sys_conf, "DownArrow", "down_arrow.png");
		s_sys_dn_arrow = image_load(systempath(filename));
	}

	// load system window style
	if (g_sys_conf != NULL) {
		filename = kev_read_string(g_sys_conf, "ssWindowStyle", "system.rws");
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
	api_register_static_func(ctx, NULL, "AreKeysLeft", js_AreKeysLeft);
	api_register_static_func(ctx, NULL, "IsAnyKeyPressed", js_IsAnyKeyPressed);
	api_register_static_func(ctx, NULL, "IsJoystickButtonPressed", js_IsJoystickButtonPressed);
	api_register_static_func(ctx, NULL, "IsKeyPressed", js_IsKeyPressed);
	api_register_static_func(ctx, NULL, "IsMouseButtonPressed", js_IsMouseButtonPressed);
	api_register_static_func(ctx, NULL, "GetClippingRectangle", js_GetClippingRectangle);
	api_register_static_func(ctx, NULL, "GetDirectoryList", js_GetDirectoryList);
	api_register_static_func(ctx, NULL, "GetFileList", js_GetFileList);
	api_register_static_func(ctx, NULL, "GetFrameRate", js_GetFrameRate);
	api_register_static_func(ctx, NULL, "GetGameList", js_GetGameList);
	api_register_static_func(ctx, NULL, "GetJoystickAxis", js_GetJoystickAxis);
	api_register_static_func(ctx, NULL, "GetLocalAddress", js_GetLocalAddress);
	api_register_static_func(ctx, NULL, "GetLocalName", js_GetLocalName);
	api_register_static_func(ctx, NULL, "GetKey", js_GetKey);
	api_register_static_func(ctx, NULL, "GetKeyString", js_GetKeyString);
	api_register_static_func(ctx, NULL, "GetMouseWheelEvent", js_GetMouseWheelEvent);
	api_register_static_func(ctx, NULL, "GetMouseX", js_GetMouseX);
	api_register_static_func(ctx, NULL, "GetMouseY", js_GetMouseY);
	api_register_static_func(ctx, NULL, "GetNumJoysticks", js_GetNumJoysticks);
	api_register_static_func(ctx, NULL, "GetNumJoystickAxes", js_GetNumJoystickAxes);
	api_register_static_func(ctx, NULL, "GetNumJoystickButtons", js_GetNumJoystickButtons);
	api_register_static_func(ctx, NULL, "GetNumMouseWheelEvents", js_GetNumMouseWheelEvents);
	api_register_static_func(ctx, NULL, "GetPlayerKey", js_GetPlayerKey);
	api_register_static_func(ctx, NULL, "GetScreenHeight", js_GetScreenHeight);
	api_register_static_func(ctx, NULL, "GetScreenWidth", js_GetScreenWidth);
	api_register_static_func(ctx, NULL, "GetSystemArrow", js_GetSystemArrow);
	api_register_static_func(ctx, NULL, "GetSystemDownArrow", js_GetSystemDownArrow);
	api_register_static_func(ctx, NULL, "GetSystemFont", js_GetSystemFont);
	api_register_static_func(ctx, NULL, "GetSystemUpArrow", js_GetSystemUpArrow);
	api_register_static_func(ctx, NULL, "GetSystemWindowStyle", js_GetSystemWindowStyle);
	api_register_static_func(ctx, NULL, "GetTime", js_GetTime);
	api_register_static_func(ctx, NULL, "GetToggleState", js_GetToggleState);
	api_register_static_func(ctx, NULL, "GetVersion", js_GetVersion);
	api_register_static_func(ctx, NULL, "GetVersionString", js_GetVersionString);
	api_register_static_func(ctx, NULL, "SetClippingRectangle", js_SetClippingRectangle);
	api_register_static_func(ctx, NULL, "SetFrameRate", js_SetFrameRate);
	api_register_static_func(ctx, NULL, "SetMousePosition", js_SetMousePosition);
	api_register_static_func(ctx, NULL, "Abort", js_Abort);
	api_register_static_func(ctx, NULL, "ApplyColorMask", js_ApplyColorMask);
	api_register_static_func(ctx, NULL, "BindJoystickButton", js_BindJoystickButton);
	api_register_static_func(ctx, NULL, "BindKey", js_BindKey);
	api_register_static_func(ctx, NULL, "BlendColors", js_BlendColors);
	api_register_static_func(ctx, NULL, "BlendColorsWeighted", js_BlendColors);
	api_register_static_func(ctx, NULL, "CreateByteArray", js_CreateByteArray);
	api_register_static_func(ctx, NULL, "CreateByteArrayFromString", js_CreateByteArrayFromString);
	api_register_static_func(ctx, NULL, "CreateColor", js_CreateColor);
	api_register_static_func(ctx, NULL, "CreateColorMatrix", js_CreateColorMatrix);
	api_register_static_func(ctx, NULL, "CreateDirectory", js_CreateDirectory);
	api_register_static_func(ctx, NULL, "CreateStringFromByteArray", js_CreateStringFromByteArray);
	api_register_static_func(ctx, NULL, "CreateStringFromCode", js_CreateStringFromCode);
	api_register_static_func(ctx, NULL, "CreateSurface", js_CreateSurface);
	api_register_static_func(ctx, NULL, "DeflateByteArray", js_DeflateByteArray);
	api_register_static_func(ctx, NULL, "Delay", js_Delay);
	api_register_static_func(ctx, NULL, "DoesFileExist", js_DoesFileExist);
	api_register_static_func(ctx, NULL, "EvaluateScript", js_EvaluateScript);
	api_register_static_func(ctx, NULL, "EvaluateSystemScript", js_EvaluateSystemScript);
	api_register_static_func(ctx, NULL, "Exit", js_Exit);
	api_register_static_func(ctx, NULL, "ExecuteGame", js_ExecuteGame);
	api_register_static_func(ctx, NULL, "FlipScreen", js_FlipScreen);
	api_register_static_func(ctx, NULL, "GarbageCollect", js_GarbageCollect);
	api_register_static_func(ctx, NULL, "GrabImage", js_GrabImage);
	api_register_static_func(ctx, NULL, "GrabSurface", js_GrabSurface);
	api_register_static_func(ctx, NULL, "GradientCircle", js_GradientCircle);
	api_register_static_func(ctx, NULL, "GradientRectangle", js_GradientRectangle);
	api_register_static_func(ctx, NULL, "HashByteArray", js_HashByteArray);
	api_register_static_func(ctx, NULL, "HashFromFile", js_HashFromFile);
	api_register_static_func(ctx, NULL, "InflateByteArray", js_InflateByteArray);
	api_register_static_func(ctx, NULL, "Line", js_Line);
	api_register_static_func(ctx, NULL, "LineSeries", js_LineSeries);
	api_register_static_func(ctx, NULL, "ListenOnPort", js_ListenOnPort);
	api_register_static_func(ctx, NULL, "LoadAnimation", js_LoadAnimation);
	api_register_static_func(ctx, NULL, "LoadFont", js_LoadFont);
	api_register_static_func(ctx, NULL, "LoadImage", js_LoadImage);
	api_register_static_func(ctx, NULL, "LoadSound", js_LoadSound);
	api_register_static_func(ctx, NULL, "LoadSpriteset", js_LoadSpriteset);
	api_register_static_func(ctx, NULL, "LoadSurface", js_LoadSurface);
	api_register_static_func(ctx, NULL, "LoadWindowStyle", js_LoadWindowStyle);
	api_register_static_func(ctx, NULL, "OpenAddress", js_OpenAddress);
	api_register_static_func(ctx, NULL, "OpenFile", js_OpenFile);
	api_register_static_func(ctx, NULL, "OpenLog", js_OpenLog);
	api_register_static_func(ctx, NULL, "OpenRawFile", js_OpenRawFile);
	api_register_static_func(ctx, NULL, "OutlinedCircle", js_OutlinedCircle);
	api_register_static_func(ctx, NULL, "OutlinedRectangle", js_OutlinedRectangle);
	api_register_static_func(ctx, NULL, "OutlinedRoundRectangle", js_OutlinedRoundRectangle);
	api_register_static_func(ctx, NULL, "Point", js_Point);
	api_register_static_func(ctx, NULL, "PointSeries", js_PointSeries);
	api_register_static_func(ctx, NULL, "Print", js_Print);
	api_register_static_func(ctx, NULL, "Rectangle", js_Rectangle);
	api_register_static_func(ctx, NULL, "RemoveDirectory", js_RemoveDirectory);
	api_register_static_func(ctx, NULL, "RemoveFile", js_RemoveFile);
	api_register_static_func(ctx, NULL, "Rename", js_Rename);
	api_register_static_func(ctx, NULL, "RequireScript", js_RequireScript);
	api_register_static_func(ctx, NULL, "RequireSystemScript", js_RequireSystemScript);
	api_register_static_func(ctx, NULL, "RestartGame", js_RestartGame);
	api_register_static_func(ctx, NULL, "RoundRectangle", js_RoundRectangle);
	api_register_static_func(ctx, NULL, "Triangle", js_Triangle);
	api_register_static_func(ctx, NULL, "UnbindJoystickButton", js_UnbindJoystickButton);
	api_register_static_func(ctx, NULL, "UnbindKey", js_UnbindKey);

	api_register_type(ctx, "ssAnimation", js_Animation_finalize);
	api_register_prop(ctx, "ssAnimation", "width", js_Animation_get_width, NULL);
	api_register_prop(ctx, "ssAnimation", "height", js_Animation_get_height, NULL);
	api_register_method(ctx, "ssAnimation", "getDelay", js_Animation_getDelay);
	api_register_method(ctx, "ssAnimation", "getNumFrames", js_Animation_getNumFrames);
	api_register_method(ctx, "ssAnimation", "drawFrame", js_Animation_drawFrame);
	api_register_method(ctx, "ssAnimation", "drawZoomedFrame", js_Animation_drawZoomedFrame);
	api_register_method(ctx, "ssAnimation", "readNextFrame", js_Animation_readNextFrame);

	api_register_type(ctx, "ssByteArray", js_ByteArray_finalize);
	api_register_prop(ctx, "ssByteArray", "length", js_ByteArray_get_length, NULL);
	api_register_method(ctx, "ssByteArray", "concat", js_ByteArray_concat);
	api_register_method(ctx, "ssByteArray", "slice", js_ByteArray_slice);
	api_register_method(ctx, "ssByteArray", "toString", js_ByteArray_toString);

	api_register_type(ctx, "ssColor", NULL);
	api_register_method(ctx, "ssColor", "toString", js_Color_toString);

	api_register_type(ctx, "ssColorMatrix", NULL);
	api_register_method(ctx, "ssColorMatrix", "toString", js_ColorMatrix_toString);

	api_register_type(ctx, "ssFile", js_File_finalize);
	api_register_method(ctx, "ssFile", "close", js_File_close);
	api_register_method(ctx, "ssFile", "flush", js_File_flush);
	api_register_method(ctx, "ssFile", "getKey", js_File_getKey);
	api_register_method(ctx, "ssFile", "getNumKeys", js_File_getNumKeys);
	api_register_method(ctx, "ssFile", "read", js_File_read);
	api_register_method(ctx, "ssFile", "toString", js_File_toString);
	api_register_method(ctx, "ssFile", "write", js_File_write);

	api_register_type(ctx, "ssFont", js_Font_finalize);
	api_register_method(ctx, "ssFont", "clone", js_Font_clone);
	api_register_method(ctx, "ssFont", "drawText", js_Font_drawText);
	api_register_method(ctx, "ssFont", "drawTextBox", js_Font_drawTextBox);
	api_register_method(ctx, "ssFont", "drawZoomedText", js_Font_drawZoomedText);
	api_register_method(ctx, "ssFont", "getCharacterImage", js_Font_getCharacterImage);
	api_register_method(ctx, "ssFont", "getColorMask", js_Font_getColorMask);
	api_register_method(ctx, "ssFont", "getHeight", js_Font_getHeight);
	api_register_method(ctx, "ssFont", "getStringHeight", js_Font_getStringHeight);
	api_register_method(ctx, "ssFont", "getStringWidth", js_Font_getStringWidth);
	api_register_method(ctx, "ssFont", "setCharacterImage", js_Font_setCharacterImage);
	api_register_method(ctx, "ssFont", "setColorMask", js_Font_setColorMask);
	api_register_method(ctx, "ssFont", "toString", js_Font_toString);
	api_register_method(ctx, "ssFont", "wordWrapString", js_Font_wordWrapString);

	api_register_type(ctx, "ssImage", js_Image_finalize);
	api_register_prop(ctx, "ssImage", "height", js_Image_get_height, NULL);
	api_register_prop(ctx, "ssImage", "width", js_Image_get_width, NULL);
	api_register_method(ctx, "ssImage", "blit", js_Image_blit);
	api_register_method(ctx, "ssImage", "blitMask", js_Image_blitMask);
	api_register_method(ctx, "ssImage", "createSurface", js_Image_createSurface);
	api_register_method(ctx, "ssImage", "rotateBlit", js_Image_rotateBlit);
	api_register_method(ctx, "ssImage", "rotateBlitMask", js_Image_rotateBlitMask);
	api_register_method(ctx, "ssImage", "toString", js_Image_toString);
	api_register_method(ctx, "ssImage", "transformBlit", js_Image_transformBlit);
	api_register_method(ctx, "ssImage", "transformBlitMask", js_Image_transformBlitMask);
	api_register_method(ctx, "ssImage", "zoomBlit", js_Image_zoomBlit);
	api_register_method(ctx, "ssImage", "zoomBlitMask", js_Image_zoomBlitMask);

	api_register_type(ctx, "ssLogger", js_Logger_finalize);
	api_register_method(ctx, "ssLogger", "toString", js_Logger_toString);
	api_register_method(ctx, "ssLogger", "beginBlock", js_Logger_beginBlock);
	api_register_method(ctx, "ssLogger", "endBlock", js_Logger_endBlock);
	api_register_method(ctx, "ssLogger", "write", js_Logger_write);

	api_register_type(ctx, "ssRawFile", js_RawFile_finalize);
	api_register_method(ctx, "ssRawFile", "close", js_RawFile_close);
	api_register_method(ctx, "ssRawFile", "getPosition", js_RawFile_getPosition);
	api_register_method(ctx, "ssRawFile", "getSize", js_RawFile_getSize);
	api_register_method(ctx, "ssRawFile", "read", js_RawFile_read);
	api_register_method(ctx, "ssRawFile", "setPosition", js_RawFile_setPosition);
	api_register_method(ctx, "ssRawFile", "toString", js_RawFile_toString);
	api_register_method(ctx, "ssRawFile", "write", js_RawFile_write);

	api_register_type(ctx, "ssSocket", js_Socket_finalize);
	api_register_method(ctx, "ssSocket", "close", js_Socket_close);
	api_register_method(ctx, "ssSocket", "getPendingReadSize", js_Socket_getPendingReadSize);
	api_register_method(ctx, "ssSocket", "isConnected", js_Socket_isConnected);
	api_register_method(ctx, "ssSocket", "read", js_Socket_read);
	api_register_method(ctx, "ssSocket", "toString", js_Socket_toString);
	api_register_method(ctx, "ssSocket", "write", js_Socket_write);

	api_register_type(ctx, "ssSound", js_Sound_finalize);
	api_register_method(ctx, "ssSound", "getLength", js_Sound_getLength);
	api_register_method(ctx, "ssSound", "getPan", js_Sound_getPan);
	api_register_method(ctx, "ssSound", "getPitch", js_Sound_getPitch);
	api_register_method(ctx, "ssSound", "getPosition", js_Sound_getPosition);
	api_register_method(ctx, "ssSound", "getRepeat", js_Sound_getRepeat);
	api_register_method(ctx, "ssSound", "getVolume", js_Sound_getVolume);
	api_register_method(ctx, "ssSound", "isPlaying", js_Sound_isPlaying);
	api_register_method(ctx, "ssSound", "isSeekable", js_Sound_isSeekable);
	api_register_method(ctx, "ssSound", "pause", js_Sound_pause);
	api_register_method(ctx, "ssSound", "play", js_Sound_play);
	api_register_method(ctx, "ssSound", "reset", js_Sound_reset);
	api_register_method(ctx, "ssSound", "setPan", js_Sound_setPan);
	api_register_method(ctx, "ssSound", "setPitch", js_Sound_setPitch);
	api_register_method(ctx, "ssSound", "setPosition", js_Sound_setPosition);
	api_register_method(ctx, "ssSound", "setRepeat", js_Sound_setRepeat);
	api_register_method(ctx, "ssSound", "setVolume", js_Sound_setVolume);
	api_register_method(ctx, "ssSound", "stop", js_Sound_stop);
	api_register_method(ctx, "ssSound", "toString", js_Sound_toString);

	api_register_type(ctx, "ssSpriteset", js_Spriteset_finalize);
	api_register_prop(ctx, "ssSpriteset", "filename", js_Spriteset_get_filename, NULL);
	api_register_method(ctx, "ssSpriteset", "clone", js_Spriteset_clone);
	api_register_method(ctx, "ssSpriteset", "toString", js_Spriteset_toString);

	api_register_type(ctx, "ssSurface", js_Surface_finalize);
	api_register_prop(ctx, "ssSurface", "height", js_Surface_get_height, NULL);
	api_register_prop(ctx, "ssSurface", "width", js_Surface_get_width, NULL);
	api_register_method(ctx, "ssSurface", "applyColorFX", js_Surface_applyColorFX);
	api_register_method(ctx, "ssSurface", "applyColorFX4", js_Surface_applyColorFX4);
	api_register_method(ctx, "ssSurface", "applyLookup", js_Surface_applyLookup);
	api_register_method(ctx, "ssSurface", "blit", js_Surface_blit);
	api_register_method(ctx, "ssSurface", "blitMaskSurface", js_Surface_blitMaskSurface);
	api_register_method(ctx, "ssSurface", "blitSurface", js_Surface_blitSurface);
	api_register_method(ctx, "ssSurface", "clone", js_Surface_clone);
	api_register_method(ctx, "ssSurface", "cloneSection", js_Surface_cloneSection);
	api_register_method(ctx, "ssSurface", "createImage", js_Surface_createImage);
	api_register_method(ctx, "ssSurface", "drawText", js_Surface_drawText);
	api_register_method(ctx, "ssSurface", "filledCircle", js_Surface_filledCircle);
	api_register_method(ctx, "ssSurface", "flipHorizontally", js_Surface_flipHorizontally);
	api_register_method(ctx, "ssSurface", "flipVertically", js_Surface_flipVertically);
	api_register_method(ctx, "ssSurface", "getPixel", js_Surface_getPixel);
	api_register_method(ctx, "ssSurface", "gradientCircle", js_Surface_gradientCircle);
	api_register_method(ctx, "ssSurface", "gradientRectangle", js_Surface_gradientRectangle);
	api_register_method(ctx, "ssSurface", "line", js_Surface_line);
	api_register_method(ctx, "ssSurface", "outlinedCircle", js_Surface_outlinedCircle);
	api_register_method(ctx, "ssSurface", "outlinedRectangle", js_Surface_outlinedRectangle);
	api_register_method(ctx, "ssSurface", "pointSeries", js_Surface_pointSeries);
	api_register_method(ctx, "ssSurface", "rotate", js_Surface_rotate);
	api_register_method(ctx, "ssSurface", "rectangle", js_Surface_rectangle);
	api_register_method(ctx, "ssSurface", "replaceColor", js_Surface_replaceColor);
	api_register_method(ctx, "ssSurface", "rescale", js_Surface_rescale);
	api_register_method(ctx, "ssSurface", "save", js_Surface_save);
	api_register_method(ctx, "ssSurface", "setAlpha", js_Surface_setAlpha);
	api_register_method(ctx, "ssSurface", "setBlendMode", js_Surface_setBlendMode);
	api_register_method(ctx, "ssSurface", "setPixel", js_Surface_setPixel);
	api_register_method(ctx, "ssSurface", "toString", js_Surface_toString);

	api_register_type(ctx, "ssWindowStyle", js_WindowStyle_finalize);
	api_register_method(ctx, "ssWindowStyle", "drawWindow", js_WindowStyle_drawWindow);
	api_register_method(ctx, "ssWindowStyle", "getColorMask", js_WindowStyle_getColorMask);
	api_register_method(ctx, "ssWindowStyle", "setColorMask", js_WindowStyle_setColorMask);
	api_register_method(ctx, "ssWindowStyle", "toString", js_WindowStyle_toString);

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
	
	// joystick axes
	api_register_const(ctx, NULL, "JOYSTICK_AXIS_X", 0);
	api_register_const(ctx, NULL, "JOYSTICK_AXIS_Y", 1);
	api_register_const(ctx, NULL, "JOYSTICK_AXIS_Z", 2);
	api_register_const(ctx, NULL, "JOYSTICK_AXIS_R", 3);
	api_register_const(ctx, NULL, "JOYSTICK_AXIS_U", 4);
	api_register_const(ctx, NULL, "JOYSTICK_AXIS_V", 5);
	
	// LineSeries() modes
	api_register_const(ctx, NULL, "LINE_MULTIPLE", LINE_MULTIPLE);
	api_register_const(ctx, NULL, "LINE_STRIP", LINE_STRIP);
	api_register_const(ctx, NULL, "LINE_LOOP", LINE_LOOP);

	// keyboard keys
	api_register_const(ctx, NULL, "KEY_NONE", 0);
	api_register_const(ctx, NULL, "KEY_SHIFT", ALLEGRO_KEY_LSHIFT);
	api_register_const(ctx, NULL, "KEY_CTRL", ALLEGRO_KEY_LCTRL);
	api_register_const(ctx, NULL, "KEY_ALT", ALLEGRO_KEY_ALT);
	api_register_const(ctx, NULL, "KEY_UP", ALLEGRO_KEY_UP);
	api_register_const(ctx, NULL, "KEY_DOWN", ALLEGRO_KEY_DOWN);
	api_register_const(ctx, NULL, "KEY_LEFT", ALLEGRO_KEY_LEFT);
	api_register_const(ctx, NULL, "KEY_RIGHT", ALLEGRO_KEY_RIGHT);
	api_register_const(ctx, NULL, "KEY_APOSTROPHE", ALLEGRO_KEY_QUOTE);
	api_register_const(ctx, NULL, "KEY_BACKSLASH", ALLEGRO_KEY_BACKSLASH);
	api_register_const(ctx, NULL, "KEY_BACKSPACE", ALLEGRO_KEY_BACKSPACE);
	api_register_const(ctx, NULL, "KEY_CLOSEBRACE", ALLEGRO_KEY_CLOSEBRACE);
	api_register_const(ctx, NULL, "KEY_CAPSLOCK", ALLEGRO_KEY_CAPSLOCK);
	api_register_const(ctx, NULL, "KEY_COMMA", ALLEGRO_KEY_COMMA);
	api_register_const(ctx, NULL, "KEY_DELETE", ALLEGRO_KEY_DELETE);
	api_register_const(ctx, NULL, "KEY_END", ALLEGRO_KEY_END);
	api_register_const(ctx, NULL, "KEY_ENTER", ALLEGRO_KEY_ENTER);
	api_register_const(ctx, NULL, "KEY_EQUALS", ALLEGRO_KEY_EQUALS);
	api_register_const(ctx, NULL, "KEY_ESCAPE", ALLEGRO_KEY_ESCAPE);
	api_register_const(ctx, NULL, "KEY_HOME", ALLEGRO_KEY_HOME);
	api_register_const(ctx, NULL, "KEY_INSERT", ALLEGRO_KEY_INSERT);
	api_register_const(ctx, NULL, "KEY_MINUS", ALLEGRO_KEY_MINUS);
	api_register_const(ctx, NULL, "KEY_NUMLOCK", ALLEGRO_KEY_NUMLOCK);
	api_register_const(ctx, NULL, "KEY_OPENBRACE", ALLEGRO_KEY_OPENBRACE);
	api_register_const(ctx, NULL, "KEY_PAGEDOWN", ALLEGRO_KEY_PGDN);
	api_register_const(ctx, NULL, "KEY_PAGEUP", ALLEGRO_KEY_PGUP);
	api_register_const(ctx, NULL, "KEY_PERIOD", ALLEGRO_KEY_FULLSTOP);
	api_register_const(ctx, NULL, "KEY_SCROLLOCK", ALLEGRO_KEY_SCROLLLOCK);
	api_register_const(ctx, NULL, "KEY_SCROLLLOCK", ALLEGRO_KEY_SCROLLLOCK);
	api_register_const(ctx, NULL, "KEY_SEMICOLON", ALLEGRO_KEY_SEMICOLON);
	api_register_const(ctx, NULL, "KEY_SPACE", ALLEGRO_KEY_SPACE);
	api_register_const(ctx, NULL, "KEY_SLASH", ALLEGRO_KEY_SLASH);
	api_register_const(ctx, NULL, "KEY_TAB", ALLEGRO_KEY_TAB);
	api_register_const(ctx, NULL, "KEY_TILDE", ALLEGRO_KEY_TILDE);
	api_register_const(ctx, NULL, "KEY_F1", ALLEGRO_KEY_F1);
	api_register_const(ctx, NULL, "KEY_F2", ALLEGRO_KEY_F2);
	api_register_const(ctx, NULL, "KEY_F3", ALLEGRO_KEY_F3);
	api_register_const(ctx, NULL, "KEY_F4", ALLEGRO_KEY_F4);
	api_register_const(ctx, NULL, "KEY_F5", ALLEGRO_KEY_F5);
	api_register_const(ctx, NULL, "KEY_F6", ALLEGRO_KEY_F6);
	api_register_const(ctx, NULL, "KEY_F7", ALLEGRO_KEY_F7);
	api_register_const(ctx, NULL, "KEY_F8", ALLEGRO_KEY_F8);
	api_register_const(ctx, NULL, "KEY_F9", ALLEGRO_KEY_F9);
	api_register_const(ctx, NULL, "KEY_F10", ALLEGRO_KEY_F10);
	api_register_const(ctx, NULL, "KEY_F11", ALLEGRO_KEY_F11);
	api_register_const(ctx, NULL, "KEY_F12", ALLEGRO_KEY_F12);
	api_register_const(ctx, NULL, "KEY_A", ALLEGRO_KEY_A);
	api_register_const(ctx, NULL, "KEY_B", ALLEGRO_KEY_B);
	api_register_const(ctx, NULL, "KEY_C", ALLEGRO_KEY_C);
	api_register_const(ctx, NULL, "KEY_D", ALLEGRO_KEY_D);
	api_register_const(ctx, NULL, "KEY_E", ALLEGRO_KEY_E);
	api_register_const(ctx, NULL, "KEY_F", ALLEGRO_KEY_F);
	api_register_const(ctx, NULL, "KEY_G", ALLEGRO_KEY_G);
	api_register_const(ctx, NULL, "KEY_H", ALLEGRO_KEY_H);
	api_register_const(ctx, NULL, "KEY_I", ALLEGRO_KEY_I);
	api_register_const(ctx, NULL, "KEY_J", ALLEGRO_KEY_J);
	api_register_const(ctx, NULL, "KEY_K", ALLEGRO_KEY_K);
	api_register_const(ctx, NULL, "KEY_L", ALLEGRO_KEY_L);
	api_register_const(ctx, NULL, "KEY_M", ALLEGRO_KEY_M);
	api_register_const(ctx, NULL, "KEY_N", ALLEGRO_KEY_N);
	api_register_const(ctx, NULL, "KEY_O", ALLEGRO_KEY_O);
	api_register_const(ctx, NULL, "KEY_P", ALLEGRO_KEY_P);
	api_register_const(ctx, NULL, "KEY_Q", ALLEGRO_KEY_Q);
	api_register_const(ctx, NULL, "KEY_R", ALLEGRO_KEY_R);
	api_register_const(ctx, NULL, "KEY_S", ALLEGRO_KEY_S);
	api_register_const(ctx, NULL, "KEY_T", ALLEGRO_KEY_T);
	api_register_const(ctx, NULL, "KEY_U", ALLEGRO_KEY_U);
	api_register_const(ctx, NULL, "KEY_V", ALLEGRO_KEY_V);
	api_register_const(ctx, NULL, "KEY_W", ALLEGRO_KEY_W);
	api_register_const(ctx, NULL, "KEY_X", ALLEGRO_KEY_X);
	api_register_const(ctx, NULL, "KEY_Y", ALLEGRO_KEY_Y);
	api_register_const(ctx, NULL, "KEY_Z", ALLEGRO_KEY_Z);
	api_register_const(ctx, NULL, "KEY_1", ALLEGRO_KEY_1);
	api_register_const(ctx, NULL, "KEY_2", ALLEGRO_KEY_2);
	api_register_const(ctx, NULL, "KEY_3", ALLEGRO_KEY_3);
	api_register_const(ctx, NULL, "KEY_4", ALLEGRO_KEY_4);
	api_register_const(ctx, NULL, "KEY_5", ALLEGRO_KEY_5);
	api_register_const(ctx, NULL, "KEY_6", ALLEGRO_KEY_6);
	api_register_const(ctx, NULL, "KEY_7", ALLEGRO_KEY_7);
	api_register_const(ctx, NULL, "KEY_8", ALLEGRO_KEY_8);
	api_register_const(ctx, NULL, "KEY_9", ALLEGRO_KEY_9);
	api_register_const(ctx, NULL, "KEY_0", ALLEGRO_KEY_0);
	api_register_const(ctx, NULL, "KEY_NUM_1", ALLEGRO_KEY_PAD_1);
	api_register_const(ctx, NULL, "KEY_NUM_2", ALLEGRO_KEY_PAD_2);
	api_register_const(ctx, NULL, "KEY_NUM_3", ALLEGRO_KEY_PAD_3);
	api_register_const(ctx, NULL, "KEY_NUM_4", ALLEGRO_KEY_PAD_4);
	api_register_const(ctx, NULL, "KEY_NUM_5", ALLEGRO_KEY_PAD_5);
	api_register_const(ctx, NULL, "KEY_NUM_6", ALLEGRO_KEY_PAD_6);
	api_register_const(ctx, NULL, "KEY_NUM_7", ALLEGRO_KEY_PAD_7);
	api_register_const(ctx, NULL, "KEY_NUM_8", ALLEGRO_KEY_PAD_8);
	api_register_const(ctx, NULL, "KEY_NUM_9", ALLEGRO_KEY_PAD_9);
	api_register_const(ctx, NULL, "KEY_NUM_0", ALLEGRO_KEY_PAD_0);

	api_register_const(ctx, NULL, "MOUSE_LEFT", MOUSE_BUTTON_LEFT);
	api_register_const(ctx, NULL, "MOUSE_MIDDLE", MOUSE_BUTTON_MIDDLE);
	api_register_const(ctx, NULL, "MOUSE_RIGHT", MOUSE_BUTTON_RIGHT);
	api_register_const(ctx, NULL, "MOUSE_WHEEL_UP", MOUSE_WHEEL_UP);
	api_register_const(ctx, NULL, "MOUSE_WHEEL_DOWN", MOUSE_WHEEL_DOWN);
	
	api_register_const(ctx, NULL, "PLAYER_1", 0);
	api_register_const(ctx, NULL, "PLAYER_2", 1);
	api_register_const(ctx, NULL, "PLAYER_3", 2);
	api_register_const(ctx, NULL, "PLAYER_4", 3);

	api_register_const(ctx, NULL, "PLAYER_KEY_MENU", PLAYER_KEY_MENU);
	api_register_const(ctx, NULL, "PLAYER_KEY_UP", PLAYER_KEY_UP);
	api_register_const(ctx, NULL, "PLAYER_KEY_DOWN", PLAYER_KEY_DOWN);
	api_register_const(ctx, NULL, "PLAYER_KEY_LEFT", PLAYER_KEY_LEFT);
	api_register_const(ctx, NULL, "PLAYER_KEY_RIGHT", PLAYER_KEY_RIGHT);
	api_register_const(ctx, NULL, "PLAYER_KEY_A", PLAYER_KEY_A);
	api_register_const(ctx, NULL, "PLAYER_KEY_B", PLAYER_KEY_B);
	api_register_const(ctx, NULL, "PLAYER_KEY_X", PLAYER_KEY_X);
	api_register_const(ctx, NULL, "PLAYER_KEY_Y", PLAYER_KEY_Y);
}

void
duk_push_sphere_bytearray(duk_context* ctx, bytearray_t* array)
{
	duk_idx_t obj_index;

	duk_push_sphere_obj(ctx, "ssByteArray", bytearray_ref(array));
	obj_index = duk_normalize_index(ctx, -1);

	// return proxy object so we can catch array accesses
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Proxy");
	duk_dup(ctx, obj_index);
	duk_push_object(ctx);
	duk_push_c_function(ctx, js_ByteArray_proxy_get, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "get");
	duk_push_c_function(ctx, js_ByteArray_proxy_set, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, "set");
	duk_new(ctx, 2);
	duk_get_prototype(ctx, obj_index);
	duk_set_prototype(ctx, -2);
	duk_remove(ctx, -2);
	duk_remove(ctx, -2);
}

void
duk_push_sphere_color(duk_context* ctx, color_t color)
{
	duk_get_global_string(ctx, "CreateColor");
	duk_push_number(ctx, color.r);
	duk_push_number(ctx, color.g);
	duk_push_number(ctx, color.b);
	duk_push_number(ctx, color.a);
	duk_call(ctx, 4);
}

void
duk_push_sphere_font(duk_context* ctx, font_t* font)
{
	duk_push_sphere_obj(ctx, "ssFont", font_ref(font));
	duk_push_sphere_color(ctx, color_new(255, 255, 255, 255));
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
}

void
duk_push_sphere_spriteset(duk_context* ctx, spriteset_t* spriteset)
{
	char prop_name[20];

	int i, j;

	duk_push_sphere_obj(ctx, "ssSpriteset", ref_spriteset(spriteset));

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

color_t
duk_require_sphere_color(duk_context* ctx, duk_idx_t index)
{
	int r, g, b;
	int a;

	duk_require_sphere_obj(ctx, index, "ssColor");
	duk_get_prop_string(ctx, index, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	a = a < 0 ? 0 : a > 255 ? 255 : a;
	return color_new(r, g, b, a);
}

colormatrix_t
duk_require_sphere_colormatrix(duk_context* ctx, duk_idx_t index)
{
	colormatrix_t matrix;

	duk_require_sphere_obj(ctx, index, "ssColorMatrix");
	duk_get_prop_string(ctx, index, "rn"); matrix.rn = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rr"); matrix.rr = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rg"); matrix.rg = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rb"); matrix.rb = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gn"); matrix.gn = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gr"); matrix.gr = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gg"); matrix.gg = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gb"); matrix.gb = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bn"); matrix.bn = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "br"); matrix.br = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bg"); matrix.bg = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bb"); matrix.bb = duk_get_int(ctx, -1); duk_pop(ctx);
	return matrix;
}

static void
duk_push_sphere_windowstyle(duk_context* ctx, windowstyle_t* winstyle)
{
	duk_push_sphere_obj(ctx, "ssWindowStyle", ref_windowstyle(winstyle));
	duk_push_sphere_color(ctx, color_new(255, 255, 255, 255));
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
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
js_AreKeysLeft(duk_context* ctx)
{
	update_input();
	duk_push_boolean(ctx, kb_queue_len() > 0);
	return 1;
}

static duk_ret_t
js_IsAnyKeyPressed(duk_context* ctx)
{
	duk_push_boolean(ctx, kb_is_any_key_down());
	return 1;
}

static duk_ret_t
js_IsJoystickButtonPressed(duk_context* ctx)
{
	int joy_index = duk_require_int(ctx, 0);
	int button = duk_require_int(ctx, 1);

	duk_push_boolean(ctx, joy_is_button_down(joy_index, button));
	return 1;
}

static duk_ret_t
js_IsKeyPressed(duk_context* ctx)
{
	int keycode = duk_require_int(ctx, 0);

	duk_push_boolean(ctx, kb_is_key_down(keycode));
	return 1;
}

static duk_ret_t
js_IsMouseButtonPressed(duk_context* ctx)
{
	int                 button;
	int                 button_id;
	ALLEGRO_DISPLAY*    display;
	ALLEGRO_MOUSE_STATE mouse_state;

	button = duk_require_int(ctx, 0);
	button_id = button == MOUSE_BUTTON_RIGHT ? 2
		: button == MOUSE_BUTTON_MIDDLE ? 3
		: 1;
	al_get_mouse_state(&mouse_state);
	display = screen_display(g_screen);
	duk_push_boolean(ctx, mouse_state.display == display && al_mouse_button_down(&mouse_state, button_id));
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
js_GetDirectoryList(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* dirname = n_args >= 1
		? duk_require_path(ctx, 0, NULL, true)
		: "";

	vector_t*  list;
	lstring_t* *p_filename;

	iter_t iter;

	list = fs_list_dir(g_fs, dirname, NULL, true);
	duk_push_array(ctx);
	iter = vector_enum(list);
	while (p_filename = vector_next(&iter)) {
		duk_push_string(ctx, lstr_cstr(*p_filename));
		duk_put_prop_index(ctx, -2, (duk_uarridx_t)iter.index);
		lstr_free(*p_filename);
	}
	vector_free(list);
	return 1;
}

static duk_ret_t
js_GetFileList(duk_context* ctx)
{
	const char* directory_name;
	vector_t*   list;
	int         num_args;

	iter_t iter;
	lstring_t* *p_filename;

	num_args = duk_get_top(ctx);
	directory_name = num_args >= 1
		? duk_require_path(ctx, 0, NULL, true)
		: "save";

	list = fs_list_dir(g_fs, directory_name, NULL, false);
	duk_push_array(ctx);
	iter = vector_enum(list);
	while (p_filename = vector_next(&iter)) {
		duk_push_string(ctx, lstr_cstr(*p_filename));
		duk_put_prop_index(ctx, -2, (duk_uarridx_t)iter.index);
		lstr_free(*p_filename);
	}
	vector_free(list);
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
				if (sandbox = fs_new(path_cstr(path))) {
					duk_push_lstring_t(ctx, fs_manifest(sandbox));
					duk_json_decode(ctx, -1);
					duk_push_string(ctx, path_cstr(path));
					duk_put_prop_string(ctx, -2, "directory");
					duk_put_prop_index(ctx, -2, j++);
					fs_free(sandbox);
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
js_GetJoystickAxis(duk_context* ctx)
{
	int joy_index = duk_require_int(ctx, 0);
	int axis_index = duk_require_int(ctx, 1);

	duk_push_number(ctx, joy_position(joy_index, axis_index));
	return 1;
}

static duk_ret_t
js_GetKey(duk_context* ctx)
{
	while (kb_queue_len() == 0)
		do_events();
	duk_push_int(ctx, kb_get_key());
	return 1;
}

static duk_ret_t
js_GetKeyString(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int keycode = duk_require_int(ctx, 0);
	bool shift = n_args >= 2 ? duk_require_boolean(ctx, 1) : false;

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
	case ALLEGRO_KEY_1: duk_push_string(ctx, shift ? "!" : "1"); break;
	case ALLEGRO_KEY_2: duk_push_string(ctx, shift ? "@" : "2"); break;
	case ALLEGRO_KEY_3: duk_push_string(ctx, shift ? "#" : "3"); break;
	case ALLEGRO_KEY_4: duk_push_string(ctx, shift ? "$" : "4"); break;
	case ALLEGRO_KEY_5: duk_push_string(ctx, shift ? "%" : "5"); break;
	case ALLEGRO_KEY_6: duk_push_string(ctx, shift ? "^" : "6"); break;
	case ALLEGRO_KEY_7: duk_push_string(ctx, shift ? "&" : "7"); break;
	case ALLEGRO_KEY_8: duk_push_string(ctx, shift ? "*" : "8"); break;
	case ALLEGRO_KEY_9: duk_push_string(ctx, shift ? "(" : "9"); break;
	case ALLEGRO_KEY_0: duk_push_string(ctx, shift ? ")" : "0"); break;
	case ALLEGRO_KEY_BACKSLASH: duk_push_string(ctx, shift ? "|" : "\\"); break;
	case ALLEGRO_KEY_FULLSTOP: duk_push_string(ctx, shift ? ">" : "."); break;
	case ALLEGRO_KEY_CLOSEBRACE: duk_push_string(ctx, shift ? "}" : "]"); break;
	case ALLEGRO_KEY_COMMA: duk_push_string(ctx, shift ? "<" : ","); break;
	case ALLEGRO_KEY_EQUALS: duk_push_string(ctx, shift ? "+" : "="); break;
	case ALLEGRO_KEY_MINUS: duk_push_string(ctx, shift ? "_" : "-"); break;
	case ALLEGRO_KEY_QUOTE: duk_push_string(ctx, shift ? "\"" : "'"); break;
	case ALLEGRO_KEY_OPENBRACE: duk_push_string(ctx, shift ? "{" : "["); break;
	case ALLEGRO_KEY_SEMICOLON: duk_push_string(ctx, shift ? ":" : ";"); break;
	case ALLEGRO_KEY_SLASH: duk_push_string(ctx, shift ? "?" : "/"); break;
	case ALLEGRO_KEY_SPACE: duk_push_string(ctx, " "); break;
	case ALLEGRO_KEY_TAB: duk_push_string(ctx, "\t"); break;
	case ALLEGRO_KEY_TILDE: duk_push_string(ctx, shift ? "~" : "`"); break;
	default:
		duk_push_string(ctx, "");
	}
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
js_GetMouseWheelEvent(duk_context* ctx)
{
	while (mouse_queue_len() == 0)
		do_events();
	duk_push_int(ctx, mouse_get_key());
	return 1;
}

static duk_ret_t
js_GetMouseX(duk_context* ctx)
{
	int x;
	int y;

	screen_get_mouse_xy(g_screen, &x, &y);
	duk_push_int(ctx, x);
	return 1;
}

static duk_ret_t
js_GetMouseY(duk_context* ctx)
{
	int x;
	int y;

	screen_get_mouse_xy(g_screen, &x, &y);
	duk_push_int(ctx, y);
	return 1;
}

static duk_ret_t
js_GetNumJoysticks(duk_context* ctx)
{
	duk_push_int(ctx, joy_num_devices());
	return 1;
}

static duk_ret_t
js_GetNumJoystickAxes(duk_context* ctx)
{
	int joy_index;
	
	joy_index = duk_require_int(ctx, 0);

	duk_push_int(ctx, joy_num_axes(joy_index));
	return 1;
}

static duk_ret_t
js_GetNumJoystickButtons(duk_context* ctx)
{
	int joy_index;
	
	joy_index = duk_require_int(ctx, 0);

	duk_push_int(ctx, joy_num_buttons(joy_index));
	return 1;
}

static duk_ret_t
js_GetNumMouseWheelEvents(duk_context* ctx)
{
	duk_push_int(ctx, mouse_queue_len());
	return 1;
}

static duk_ret_t
js_GetPlayerKey(duk_context* ctx)
{
	int player;
	int key_type;

	player = duk_require_int(ctx, 0);
	key_type = duk_require_int(ctx, 1);

	if (player < 0 || player >= 4)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "player index out of range");
	if (key_type < 0 || key_type >= PLAYER_KEY_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid key type constant");
	duk_push_int(ctx, get_player_key(player, key_type));
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
js_GetSystemArrow(duk_context* ctx)
{
	if (s_sys_arrow == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "missing system arrow image");
	duk_push_sphere_obj(ctx, "ssImage", image_ref(s_sys_arrow));
	return 1;
}

static duk_ret_t
js_GetSystemDownArrow(duk_context* ctx)
{
	if (s_sys_dn_arrow == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "missing system down arrow image");
	duk_push_sphere_obj(ctx, "ssImage", image_ref(s_sys_dn_arrow));
	return 1;
}

static duk_ret_t
js_GetSystemFont(duk_context* ctx)
{
	duk_push_sphere_font(ctx, g_sys_font);
	return 1;
}

static duk_ret_t
js_GetSystemUpArrow(duk_context* ctx)
{
	if (s_sys_up_arrow == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "missing system up arrow image");
	duk_push_sphere_obj(ctx, "ssImage", image_ref(s_sys_up_arrow));
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
js_GetToggleState(duk_context* ctx)
{
	int keycode;
	
	keycode = duk_require_int(ctx, 0);

	if (keycode != ALLEGRO_KEY_CAPSLOCK
		&& keycode != ALLEGRO_KEY_NUMLOCK
		&& keycode != ALLEGRO_KEY_SCROLLLOCK)
	{
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "invalid toggle key constant");
	}

	duk_push_boolean(ctx, kb_is_toggled(keycode));
	return 1;
}

static duk_ret_t
js_GetVersion(duk_context* ctx)
{
	duk_push_number(ctx, API_VERSION);
	return 1;
}

static duk_ret_t
js_GetVersionString(duk_context* ctx)
{
	duk_push_string(ctx, API_VERSION_STRING);
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
js_SetMousePosition(duk_context* ctx)
{
	int x;
	int y;

	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);
	screen_set_mouse_xy(g_screen, x, y);
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
js_BindJoystickButton(duk_context* ctx)
{
	int joy_index = duk_require_int(ctx, 0);
	int button = duk_require_int(ctx, 1);
	script_t* on_down_script = duk_require_sphere_script(ctx, 2, "[button-down script]");
	script_t* on_up_script = duk_require_sphere_script(ctx, 3, "[button-up script]");

	if (joy_index < 0 || joy_index >= MAX_JOYSTICKS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BindJoystickButton(): joystick index `%d` out of range", joy_index);
	if (button < 0 || button >= MAX_JOY_BUTTONS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BindJoystickButton(): button index `%d` out of range", button);
	joy_bind_button(joy_index, button, on_down_script, on_up_script);
	return 0;
}

static duk_ret_t
js_BindKey(duk_context* ctx)
{
	int keycode = duk_require_int(ctx, 0);
	script_t* on_down_script = duk_require_sphere_script(ctx, 1, "[key-down script]");
	script_t* on_up_script = duk_require_sphere_script(ctx, 2, "[key-up script]");

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BindKey(): invalid key constant");
	kb_bind_key(keycode, on_down_script, on_up_script);
	return 0;
}

static duk_ret_t
js_BlendColors(duk_context* ctx)
{
	color_t color1;
	color_t color2;
	int     num_args;
	float   w1 = 1.0;
	float   w2 = 1.0;

	num_args = duk_get_top(ctx);
	color1 = duk_require_sphere_color(ctx, 0);
	color2 = duk_require_sphere_color(ctx, 1);
	if (num_args > 2) {
		w1 = duk_require_number(ctx, 2);
		w2 = duk_require_number(ctx, 3);
	}

	if (w1 < 0.0 || w2 < 0.0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "weights cannot be negative", w1, w2);

	duk_push_sphere_color(ctx, color_mix(color1, color2, w1, w2));
	return 1;
}

static duk_ret_t
js_CreateByteArray(duk_context* ctx)
{
	bytearray_t* array;
	int          size;

	size = duk_require_int(ctx, 0);

	if (size < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "size cannot be negative");
	if (!(array = bytearray_new(size)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create byte array");
	duk_push_sphere_bytearray(ctx, array);
	bytearray_free(array);
	return 1;
}

static duk_ret_t
js_CreateByteArrayFromString(duk_context* ctx)
{
	bytearray_t* array;
	lstring_t*   string;

	string = duk_require_lstring_t(ctx, 0);

	if (!(array = bytearray_from_lstring(string)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create byte array");
	lstr_free(string);
	duk_push_sphere_bytearray(ctx, array);
	bytearray_free(array);
	return 1;
}

static duk_ret_t
js_CreateColor(duk_context* ctx)
{
	int num_args = duk_get_top(ctx);
	int r = duk_require_int(ctx, 0);
	int g = duk_require_int(ctx, 1);
	int b = duk_require_int(ctx, 2);
	int a = num_args >= 4 ? duk_require_int(ctx, 3) : 255;

	// clamp components to 8-bit [0-255]
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	a = a < 0 ? 0 : a > 255 ? 255 : a;

	// construct a Color object
	duk_push_sphere_obj(ctx, "ssColor", NULL);
	duk_push_int(ctx, r); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, g); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, b); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, a); duk_put_prop_string(ctx, -2, "alpha");
	return 1;
}

static duk_ret_t
js_CreateColorMatrix(duk_context* ctx)
{
	int rn = duk_require_int(ctx, 0);
	int rr = duk_require_int(ctx, 1);
	int rg = duk_require_int(ctx, 2);
	int rb = duk_require_int(ctx, 3);
	int gn = duk_require_int(ctx, 4);
	int gr = duk_require_int(ctx, 5);
	int gg = duk_require_int(ctx, 6);
	int gb = duk_require_int(ctx, 7);
	int bn = duk_require_int(ctx, 8);
	int br = duk_require_int(ctx, 9);
	int bg = duk_require_int(ctx, 10);
	int bb = duk_require_int(ctx, 11);

	// construct a ColorMatrix object
	duk_push_sphere_obj(ctx, "ssColorMatrix", NULL);
	duk_push_int(ctx, rn); duk_put_prop_string(ctx, -2, "rn");
	duk_push_int(ctx, rr); duk_put_prop_string(ctx, -2, "rr");
	duk_push_int(ctx, rg); duk_put_prop_string(ctx, -2, "rg");
	duk_push_int(ctx, rb); duk_put_prop_string(ctx, -2, "rb");
	duk_push_int(ctx, gn); duk_put_prop_string(ctx, -2, "gn");
	duk_push_int(ctx, gr); duk_put_prop_string(ctx, -2, "gr");
	duk_push_int(ctx, gg); duk_put_prop_string(ctx, -2, "gg");
	duk_push_int(ctx, gb); duk_put_prop_string(ctx, -2, "gb");
	duk_push_int(ctx, bn); duk_put_prop_string(ctx, -2, "bn");
	duk_push_int(ctx, br); duk_put_prop_string(ctx, -2, "br");
	duk_push_int(ctx, bg); duk_put_prop_string(ctx, -2, "bg");
	duk_push_int(ctx, bb); duk_put_prop_string(ctx, -2, "bb");
	return 1;
}

static duk_ret_t
js_CreateStringFromByteArray(duk_context* ctx)
{
	bytearray_t* array;
	uint8_t*     buffer;
	size_t       size;

	array = duk_require_sphere_obj(ctx, 0, "ssByteArray");

	buffer = bytearray_buffer(array);
	size = bytearray_len(array);
	duk_push_lstring(ctx, (char*)buffer, size);
	return 1;
}

static duk_ret_t
js_CreateDirectory(duk_context* ctx)
{
	const char* name;

	name = duk_require_path(ctx, 0, "save", true);
	if (!sfs_mkdir(g_fs, name, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateDirectory(): unable to create directory `%s`", name);
	return 0;
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
	duk_push_sphere_obj(ctx, "ssSurface", image);
	return 1;
}

static duk_ret_t
js_DeflateByteArray(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	bytearray_t* array = duk_require_sphere_obj(ctx, 0, "ssByteArray");
	int level = n_args >= 2 ? duk_require_int(ctx, 1) : -1;

	bytearray_t* new_array;

	if ((level < 0 || level > 9) && n_args >= 2)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "DeflateByteArray(): compression level must be [0-9] (got: %i)", level);
	if (!(new_array = bytearray_deflate(array, level)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "DeflateByteArray(): unable to deflate source ByteArray");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_Delay(duk_context* ctx)
{
	double millisecs = floor(duk_require_number(ctx, 0));

	if (millisecs < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "time cannot be negative", millisecs);
	delay(millisecs / 1000);
	return 0;
}

static duk_ret_t
js_DoesFileExist(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, true);
	duk_push_boolean(ctx, sfs_fexist(g_fs, filename, NULL));
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
	g_last_game_path = path_dup(fs_path(g_fs));

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
js_GrabImage(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);

	image_t* image;

	if (!(image = screen_grab(g_screen, x, y, w, h)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GrabImage(): unable to grab backbuffer image");
	duk_push_sphere_obj(ctx, "ssImage", image);
	return 1;
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
	duk_push_sphere_obj(ctx, "ssSurface", image);
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
js_HashByteArray(duk_context* ctx)
{
	duk_require_sphere_obj(ctx, 0, "ssByteArray");

	// TODO: implement byte array hashing
	duk_error_ni(ctx, -1, DUK_ERR_ERROR, "function is not implemented");
}

static duk_ret_t
js_HashFromFile(duk_context* ctx)
{
	sfs_file_t* file;
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, true);
	file = sfs_fopen(g_fs, filename, "other", "rb");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to open `%s` for reading");
	sfs_fclose(file);
	// TODO: implement raw file hashing
	duk_error_ni(ctx, -1, DUK_ERR_ERROR, "function is not yet implemented");
}

static duk_ret_t
js_InflateByteArray(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	bytearray_t* array = duk_require_sphere_obj(ctx, 0, "ssByteArray");
	int max_size = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	bytearray_t* new_array;

	if (max_size < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "InflateByteArray(): buffer size must not be negative (got: %d)", max_size);
	if (!(new_array = bytearray_inflate(array, max_size)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "InflateByteArray(): unable to inflate source ByteArray");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
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
		duk_push_sphere_obj(ctx, "ssSocket", socket);
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
	duk_push_sphere_obj(ctx, "ssAnimation", anim);
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
js_LoadImage(duk_context* ctx)
{
	const char* filename;
	image_t*    image;

	filename = duk_require_path(ctx, 0, "images", true);
	if (!(image = image_load(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image(): unable to load image file `%s`", filename);
	duk_push_sphere_obj(ctx, "ssImage", image);
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
	duk_push_sphere_obj(ctx, "ssSound", sound);
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
	duk_push_sphere_obj(ctx, "ssSurface", image);
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
		duk_push_sphere_obj(ctx, "ssSocket", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_OpenFile(duk_context* ctx)
{
	kevfile_t*  file;
	const char* filename;

	filename = duk_require_path(ctx, 0, "save", true);

	if (!(file = kev_open(g_fs, filename, true)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to open file `%s`", filename);
	duk_push_sphere_obj(ctx, "ssFile", file);
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
	duk_push_sphere_obj(ctx, "ssLogger", logger);
	return 1;
}

static duk_ret_t
js_OpenRawFile(duk_context* ctx)
{
	sfs_file_t* file;
	const char* filename;
	int         num_args;
	bool        writable;

	num_args = duk_get_top(ctx);
	filename = duk_require_path(ctx, 0, "other", true);
	writable = num_args >= 2
		? duk_require_boolean(ctx, 1)
		: false;

	file = sfs_fopen(g_fs, filename, NULL, writable ? "w+b" : "rb");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenRawFile(): unable to open file `%s` for %s",
			filename, writable ? "writing" : "reading");
	duk_push_sphere_obj(ctx, "ssRawFile", file);
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

js_Print(duk_context* ctx)
{
	int   num_items;
	char* text;

	num_items = duk_get_top(ctx);

	// separate printed values with a space
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	text = strnewf("%s", duk_get_string(ctx, -1));
	debug_print(text, PRINT_NORMAL);
	printf("print: %s\n", text);
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
js_RemoveDirectory(duk_context* ctx)
{
	const char* name;

	name = duk_require_path(ctx, 0, "save", true);
	if (!sfs_rmdir(g_fs, name, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CreateDirectory(): unable to remove directory `%s`", name);
	return 0;
}

static duk_ret_t
js_RemoveFile(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, "save", true);
	if (!sfs_unlink(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RemoveFile(): unable to delete file `%s`", filename);
	return 0;
}

static duk_ret_t
js_Rename(duk_context* ctx)
{
	const char* name1;
	const char* name2;

	name1 = duk_require_path(ctx, 0, "save", true);
	name2 = duk_require_path(ctx, 1, "save", true);
	if (!sfs_rename(g_fs, name1, name2, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Rename(): unable to rename file `%s` to `%s`", name1, name2);
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
js_UnbindJoystickButton(duk_context* ctx)
{
	int joy_index = duk_require_int(ctx, 0);
	int button = duk_require_int(ctx, 1);

	if (joy_index < 0 || joy_index >= MAX_JOYSTICKS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BindJoystickButton(): joystick index `%d` out of range", joy_index);
	if (button < 0 || button >= MAX_JOY_BUTTONS)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BindJoystickButton(): button index `%d` out of range", button);
	joy_bind_button(joy_index, button, NULL, NULL);
	return 0;
}

static duk_ret_t
js_UnbindKey(duk_context* ctx)
{
	int keycode = duk_require_int(ctx, 0);

	if (keycode < 0 || keycode >= ALLEGRO_KEY_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "UnbindKey(): invalid key constant");
	kb_bind_key(keycode, NULL, NULL);
	return 0;
}

static duk_ret_t
js_Animation_finalize(duk_context* ctx)
{
	animation_t* anim;

	anim = duk_require_sphere_obj(ctx, 0, "ssAnimation");
	animation_free(anim);
	return 0;
}

static duk_ret_t
js_Animation_get_height(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "ssAnimation");

	duk_push_int(ctx, animation_height(anim));
	return 1;
}

static duk_ret_t
js_Animation_get_width(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "ssAnimation");

	duk_push_int(ctx, animation_width(anim));
	return 1;
}

static duk_ret_t
js_Animation_drawFrame(duk_context* ctx)
{
	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);

	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "ssAnimation");

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
	anim = duk_require_sphere_obj(ctx, -1, "ssAnimation");
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
js_Animation_getDelay(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "ssAnimation");
	duk_pop(ctx);
	duk_push_int(ctx, animation_delay(anim));
	return 1;
}

static duk_ret_t
js_Animation_getNumFrames(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "ssAnimation");

	duk_push_int(ctx, animation_num_frames(anim));
	return 1;
}

static duk_ret_t
js_Animation_readNextFrame(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "ssAnimation");
	duk_pop(ctx);
	animation_update(anim);
	return 0;
}

static duk_ret_t
js_ByteArray_finalize(duk_context* ctx)
{
	bytearray_t* array;

	array = duk_require_sphere_obj(ctx, 0, "ssByteArray");
	bytearray_free(array);
	return 0;
}

static duk_ret_t
js_ByteArray_proxy_get(duk_context* ctx)
{
	bytearray_t* array;
	int          index;
	int          size;

	array = duk_require_sphere_obj(ctx, 0, "ssByteArray");
	if (duk_is_number(ctx, 1)) {
		index = duk_to_int(ctx, 1);
		size = bytearray_len(array);
		if (index < 0 || index >= size)
			duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "byte index `%d` out of bounds", index);
		duk_push_uint(ctx, bytearray_get(array, index));
		return 1;
	}
	else {
		duk_dup(ctx, 1);
		duk_get_prop(ctx, 0);
		return 1;
	}
}

static duk_ret_t
js_ByteArray_proxy_set(duk_context* ctx)
{
	bytearray_t* array;
	int          index;
	int          size;

	array = duk_require_sphere_obj(ctx, 0, "ssByteArray");
	if (duk_is_number(ctx, 1)) {
		index = duk_to_int(ctx, 1);
		size = bytearray_len(array);
		if (index < 0 || index >= size)
			duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "byte index `%d` out of bounds", index);
		bytearray_set(array, index, duk_require_uint(ctx, 2));
		return 0;
	}
	else {
		duk_dup(ctx, 1);
		duk_dup(ctx, 2);
		duk_put_prop(ctx, 0);
		return 0;
	}
}

static duk_ret_t
js_ByteArray_get_length(duk_context* ctx)
{
	bytearray_t* array;

	duk_push_this(ctx);
	array = duk_require_sphere_obj(ctx, -1, "ssByteArray");
	duk_pop(ctx);
	duk_push_int(ctx, bytearray_len(array));
	return 1;
}

static duk_ret_t
js_ByteArray_concat(duk_context* ctx)
{
	bytearray_t* array[2];
	bytearray_t* new_array;

	duk_push_this(ctx);
	array[0] = duk_require_sphere_obj(ctx, -1, "ssByteArray");
	array[1] = duk_require_sphere_obj(ctx, 0, "ssByteArray");
	
	if (!(new_array = bytearray_concat(array[0], array[1])))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to concatenate byte arrays");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_ByteArray_slice(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int start = duk_require_int(ctx, 0);
	int end = (n_args >= 2) ? duk_require_int(ctx, 1) : INT_MAX;

	bytearray_t* array;
	int          end_norm;
	bytearray_t* new_array;
	int          size;

	duk_push_this(ctx);
	array = duk_require_sphere_obj(ctx, -1, "ssByteArray");

	size = bytearray_len(array);
	end_norm = fmin(end >= 0 ? end : size + end, size);
	if (end_norm < start || end_norm > size)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "start and/or end is out of bounds");
	if (!(new_array = bytearray_slice(array, start, end_norm - start)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to slice byte array");
	duk_push_sphere_bytearray(ctx, new_array);
	return 1;
}

static duk_ret_t
js_ByteArray_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object byte_array]");
	return 1;
}

static duk_ret_t
js_Color_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object color]");
	return 1;
}

static duk_ret_t
js_ColorMatrix_apply(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 0);

	colormatrix_t matrix;

	duk_push_this(ctx);
	matrix = duk_require_sphere_colormatrix(ctx, -1);
	duk_pop(ctx);
	duk_push_sphere_color(ctx, color_transform(color, matrix));
	return 1;
}

static duk_ret_t
js_ColorMatrix_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object colormatrix]");
	return 1;
}

static duk_ret_t
js_File_finalize(duk_context* ctx)
{
	kevfile_t* file;

	file = duk_require_sphere_obj(ctx, 0, "ssFile");
	kev_close(file);
	return 0;
}

static duk_ret_t
js_File_close(duk_context* ctx)
{
	kevfile_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	kev_close(file);
	duk_push_this(ctx);
	duk_push_pointer(ctx, NULL); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_File_flush(duk_context* ctx)
{
	kevfile_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:flush(): file was closed");
	kev_save(file);
	return 0;
}

static duk_ret_t
js_File_getKey(duk_context* ctx)
{
	int index = duk_require_int(ctx, 0);

	kevfile_t*  file;
	const char* key;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getKey(): file was closed");
	if (key = kev_get_key(file, index))
		duk_push_string(ctx, key);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_File_getNumKeys(duk_context* ctx)
{
	kevfile_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:getNumKeys(): file was closed");
	duk_push_int(ctx, kev_num_keys(file));
	return 1;
}

static duk_ret_t
js_File_read(duk_context* ctx)
{
	const char* key = duk_to_string(ctx, 0);

	bool        def_bool;
	double      def_num;
	const char* def_string;
	kevfile_t*  file;
	const char* value;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:read(): file was closed");
	switch (duk_get_type(ctx, 1)) {
	case DUK_TYPE_BOOLEAN:
		def_bool = duk_get_boolean(ctx, 1);
		duk_push_boolean(ctx, kev_read_bool(file, key, def_bool));
		break;
	case DUK_TYPE_NUMBER:
		def_num = duk_get_number(ctx, 1);
		duk_push_number(ctx, kev_read_float(file, key, def_num));
		break;
	default:
		def_string = duk_to_string(ctx, 1);
		value = kev_read_string(file, key, def_string);
		duk_push_string(ctx, value);
		break;
	}
	return 1;
}

static duk_ret_t
js_File_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object file]");
	return 1;
}

static duk_ret_t
js_File_write(duk_context* ctx)
{
	const char* key = duk_to_string(ctx, 0);

	kevfile_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "File:write(): file was closed");
	kev_write_string(file, key, duk_to_string(ctx, 1));
	return 0;
}

static duk_ret_t
js_Font_finalize(duk_context* ctx)
{
	font_t* font;

	font = duk_require_sphere_obj(ctx, 0, "ssFont");
	font_free(font);
	return 0;
}

static duk_ret_t
js_Font_clone(duk_context* ctx)
{
	font_t* dolly_font;
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "ssFont");
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
	font = duk_require_sphere_obj(ctx, -1, "ssFont");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen))
		font_draw_text(font, mask, x, y, TEXT_ALIGN_LEFT, text);
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
	font = duk_require_sphere_obj(ctx, -1, "ssFont");
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
	font = duk_require_sphere_obj(ctx, -1, "ssFont");
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
js_Font_getCharacterImage(duk_context* ctx)
{
	uint32_t cp;
	font_t*  font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "ssFont");
	cp = duk_require_uint(ctx, 0);

	duk_push_sphere_obj(ctx, "ssImage", image_ref(font_glyph(font, cp)));
	return 1;
}

static duk_ret_t
js_Font_getColorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_Font_getHeight(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "ssFont");
	duk_pop(ctx);
	duk_push_int(ctx, font_height(font));
	return 1;
}

static duk_ret_t
js_Font_getStringHeight(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	int width = duk_require_int(ctx, 1);

	font_t* font;
	int     num_lines;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "ssFont");
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
	font = duk_require_sphere_obj(ctx, -1, "ssFont");
	duk_pop(ctx);
	duk_push_int(ctx, font_get_width(font, text));
	return 1;
}

static duk_ret_t
js_Font_setCharacterImage(duk_context* ctx)
{
	int cp = duk_require_int(ctx, 0);
	image_t* image = duk_require_sphere_obj(ctx, 1, "ssImage");

	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "ssFont");

	font_set_glyph(font, cp, image);
	return 0;
}

static duk_ret_t
js_Font_setColorMask(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "ssFont");
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "color_mask"); duk_pop(ctx);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Font_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object font]");
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
	font = duk_require_sphere_obj(ctx, -1, "ssFont");
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
js_Image_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_sphere_obj(ctx, 0, "ssImage");
	image_free(image);
	return 0;
}

static duk_ret_t
js_Image_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	duk_pop(ctx);
	duk_push_int(ctx, image_height(image));
	return 1;
}

static duk_ret_t
js_Image_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	duk_pop(ctx);
	duk_push_int(ctx, image_width(image));
	return 1;
}

static duk_ret_t
js_Image_blit(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen)) al_draw_bitmap(image_bitmap(image), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Image_blitMask(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	color_t mask = duk_require_sphere_color(ctx, 2);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen)) al_draw_tinted_bitmap(image_bitmap(image), al_map_rgba(mask.r, mask.g, mask.b, mask.a), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Image_createSurface(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	duk_pop(ctx);
	if ((new_image = image_clone(image)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image:createSurface(): unable to create new surface image");
	duk_push_sphere_obj(ctx, "ssSurface", new_image);
	return 1;
}

static duk_ret_t
js_Image_rotateBlit(duk_context* ctx)
{
	float    angle;
	int      height;
	image_t* image;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);
	angle = duk_require_number(ctx, 2);

	if (!screen_is_skipframe(g_screen)) {
		width = image_width(image);
		height = image_height(image);
		al_draw_rotated_bitmap(image_bitmap(image), width / 2, height / 2,
			x + width / 2, y + height / 2, angle, 0x0);
	}
	return 0;
}

static duk_ret_t
js_Image_rotateBlitMask(duk_context* ctx)
{
	int      height;
	image_t* image;
	int      width;
	int      x;
	int      y;
	float    angle;
	color_t  mask;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);
	angle = duk_require_number(ctx, 2);
	mask = duk_require_sphere_color(ctx, 3);

	if (!screen_is_skipframe(g_screen)) {
		width = image_width(image);
		height = image_height(image);
		al_draw_tinted_rotated_bitmap(image_bitmap(image), al_map_rgba(mask.r, mask.g, mask.b, mask.a),
			width / 2, height / 2, x + width / 2, y + height / 2, angle, 0x0);
	}
	return 0;
}

static duk_ret_t
js_Image_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object image]");
	return 1;
}

static duk_ret_t
js_Image_transformBlit(duk_context* ctx)
{
	int           height;
	image_t*      image;
	ALLEGRO_COLOR mask;
	int           width;
	int           x1, y1;
	int           x2, y2;
	int           x3, y3;
	int           x4, y4;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	x1 = duk_require_int(ctx, 0);
	y1 = duk_require_int(ctx, 1);
	x2 = duk_require_int(ctx, 2);
	y2 = duk_require_int(ctx, 3);
	x3 = duk_require_int(ctx, 4);
	y3 = duk_require_int(ctx, 5);
	x4 = duk_require_int(ctx, 6);
	y4 = duk_require_int(ctx, 7);

	width = image_width(image);
	height = image_height(image);
	mask = al_map_rgba(255, 255, 255, 255);
	ALLEGRO_VERTEX v[] = {
		{ x1 + 0.5, y1 + 0.5, 0, 0, 0, mask },
		{ x2 + 0.5, y2 + 0.5, 0, width, 0, mask },
		{ x4 + 0.5, y4 + 0.5, 0, 0, height, mask },
		{ x3 + 0.5, y3 + 0.5, 0, width, height, mask }
	};
	if (!screen_is_skipframe(g_screen))
		al_draw_prim(v, NULL, image_bitmap(image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return 0;
}

static duk_ret_t
js_Image_transformBlitMask(duk_context* ctx)
{
	int      height;
	image_t* image;
	color_t  mask;
	int      width;
	int      x1, y1;
	int      x2, y2;
	int      x3, y3;
	int      x4, y4;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	x1 = duk_require_int(ctx, 0);
	y1 = duk_require_int(ctx, 1);
	x2 = duk_require_int(ctx, 2);
	y2 = duk_require_int(ctx, 3);
	x3 = duk_require_int(ctx, 4);
	y3 = duk_require_int(ctx, 5);
	x4 = duk_require_int(ctx, 6);
	y4 = duk_require_int(ctx, 7);
	mask = duk_require_sphere_color(ctx, 8);

	width = image_width(image);
	height = image_height(image);
	ALLEGRO_VERTEX v[] = {
		{ x1 + 0.5, y1 + 0.5, 0, 0, 0, nativecolor(mask) },
		{ x2 + 0.5, y2 + 0.5, 0, width, 0, nativecolor(mask) },
		{ x4 + 0.5, y4 + 0.5, 0, 0, height, nativecolor(mask) },
		{ x3 + 0.5, y3 + 0.5, 0, width, height, nativecolor(mask) }
	};
	if (!screen_is_skipframe(g_screen))
		al_draw_prim(v, NULL, image_bitmap(image), 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return 0;
}

static duk_ret_t
js_Image_zoomBlit(duk_context* ctx)
{
	int      height;
	image_t* image;
	float    scale;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);
	scale = duk_require_number(ctx, 2);

	if (!screen_is_skipframe(g_screen)) {
		width = image_width(image);
		height = image_height(image);
		al_draw_scaled_bitmap(image_bitmap(image), 0, 0, width, height, 
			x, y, width * scale, height * scale, 0x0);
	}
	return 0;
}

static duk_ret_t
js_Image_zoomBlitMask(duk_context* ctx)
{
	int      height;
	image_t* image;
	color_t  mask;
	float    scale;
	int      width;
	int      x;
	int      y;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssImage");
	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);
	scale = duk_require_number(ctx, 2);
	mask = duk_require_sphere_color(ctx, 3);

	if (!screen_is_skipframe(g_screen)) {
		width = image_width(image);
		height = image_height(image);
		al_draw_tinted_scaled_bitmap(image_bitmap(image), nativecolor(mask),
			0, 0, width, height, x, y, width * scale, height * scale, 0x0);
	}
	return 0;
}

static duk_ret_t
js_Logger_finalize(duk_context* ctx)
{
	logger_t* logger;

	logger = duk_require_sphere_obj(ctx, 0, "ssLogger");
	log_close(logger);
	return 0;
}

static duk_ret_t
js_Logger_beginBlock(duk_context* ctx)
{
	const char* title = duk_to_string(ctx, 0);

	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "ssLogger");
	if (!log_begin_block(logger, title))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Log:beginBlock(): unable to create new log block");
	return 0;
}

static duk_ret_t
js_Logger_endBlock(duk_context* ctx)
{
	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "ssLogger");
	log_end_block(logger);
	return 0;
}

static duk_ret_t
js_Logger_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object log]");
	return 1;
}

static duk_ret_t
js_Logger_write(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);

	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "ssLogger");
	log_write(logger, NULL, text);
	return 0;
}

static duk_ret_t
js_RawFile_finalize(duk_context* ctx)
{
	sfs_file_t* file;

	file = duk_require_sphere_obj(ctx, 0, "ssRawFile");
	if (file != NULL) sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_RawFile_close(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssRawFile");
	duk_push_pointer(ctx, NULL);
	duk_put_prop_string(ctx, -2, "\xFF" "udata");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:close(): file was closed");
	sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_RawFile_getPosition(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssRawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position: file was closed");
	duk_push_int(ctx, sfs_ftell(file));
	return 1;
}

static duk_ret_t
js_RawFile_getSize(duk_context* ctx)
{
	sfs_file_t* file;
	long  file_pos;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssRawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:size: file was closed");
	file_pos = sfs_ftell(file);
	sfs_fseek(file, 0, SEEK_END);
	duk_push_int(ctx, sfs_ftell(file));
	sfs_fseek(file, file_pos, SEEK_SET);
	return 1;
}

static duk_ret_t
js_RawFile_read(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	long num_bytes = n_args >= 1 ? duk_require_int(ctx, 0) : 0;

	bytearray_t* array;
	sfs_file_t*        file;
	long         pos;
	void*        read_buffer;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssRawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): file was closed");
	if (n_args < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes <= 0 || num_bytes > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RawFile:read(): read size out of range (%u)", num_bytes);
	if (!(read_buffer = malloc(num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): unable to allocate buffer for file read");
	num_bytes = (long)sfs_fread(read_buffer, 1, num_bytes, file);
	if (n_args < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	if (!(array = bytearray_from_buffer(read_buffer, (int)num_bytes)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:read(): unable to create byte array");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_RawFile_setPosition(duk_context* ctx)
{
	int new_pos = duk_require_int(ctx, 0);

	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssRawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position: file was closed");
	if (!sfs_fseek(file, new_pos, SEEK_SET))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:position: Failed to set read/write position");
	return 0;
}

static duk_ret_t
js_RawFile_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object rawfile]");
	return 1;
}

static duk_ret_t
js_RawFile_write(duk_context* ctx)
{
	bytearray_t* array;
	const void*  data;
	sfs_file_t*  file;
	duk_size_t   write_size;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "ssRawFile");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:write(): file was closed");
	if (duk_is_string(ctx, 0))
		data = duk_get_lstring(ctx, 0, &write_size);
	else if (duk_is_sphere_obj(ctx, 0, "ssByteArray")) {
		array = duk_require_sphere_obj(ctx, 0, "ssByteArray");
		data = bytearray_buffer(array);
		write_size = bytearray_len(array);
	}
	else {
		data = duk_require_buffer_data(ctx, 0, &write_size);
	}
	if (sfs_fwrite(data, 1, write_size, file) != write_size)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RawFile:write(): error writing to file");
	return 0;
}

static duk_ret_t
js_Socket_finalize(duk_context* ctx)
{
	socket_t* socket;

	socket = duk_require_sphere_obj(ctx, 0, "ssSocket");
	free_socket(socket);
	return 1;
}

static duk_ret_t
js_Socket_close(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "ssSocket");
	duk_push_null(ctx); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	if (socket != NULL)
		free_socket(socket);
	return 1;
}

static duk_ret_t
js_Socket_getPendingReadSize(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "ssSocket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getPendingReadSize(): socket has been closed");
	duk_push_uint(ctx, (duk_uint_t)peek_socket(socket));
	return 1;
}

static duk_ret_t
js_Socket_isConnected(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "ssSocket");
	duk_pop(ctx);
	if (socket != NULL)
		duk_push_boolean(ctx, is_socket_live(socket));
	else
		duk_push_false(ctx);
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
	socket = duk_require_sphere_obj(ctx, -1, "ssSocket");
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
js_Socket_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object socket]");
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
	socket = duk_require_sphere_obj(ctx, -1, "ssSocket");
	duk_pop(ctx);
	if (duk_is_string(ctx, 0))
		payload = (uint8_t*)duk_get_lstring(ctx, 0, &write_size);
	else {
		array = duk_require_sphere_obj(ctx, 0, "ssByteArray");
		payload = bytearray_buffer(array);
		write_size = bytearray_len(array);
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

	sound = duk_require_sphere_obj(ctx, 0, "ssSound");
	sound_free(sound);
	return 0;
}

static duk_ret_t
js_Sound_getLength(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	duk_push_number(ctx, floor(sound_len(sound) * 1000000));
	return 1;
}

static duk_ret_t
js_Sound_getPan(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	duk_push_int(ctx, sound_pan(sound) * 255);
	return 1;
}

static duk_ret_t
js_Sound_getPitch(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	duk_push_number(ctx, sound_speed(sound));
	return 1;
}

static duk_ret_t
js_Sound_getPosition(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	duk_push_number(ctx, floor(sound_tell(sound) * 1000000));
	return 1;
}

static duk_ret_t
js_Sound_getRepeat(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	duk_push_boolean(ctx, sound_repeat(sound));
	return 1;
}

static duk_ret_t
js_Sound_getVolume(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	duk_push_int(ctx, sound_gain(sound) * 255);
	return 1;
}

static duk_ret_t
js_Sound_isPlaying(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	duk_push_boolean(ctx, sound_playing(sound));
	return 1;
}

static duk_ret_t
js_Sound_isSeekable(duk_context* ctx)
{
	duk_push_true(ctx);
	return 1;
}

static duk_ret_t
js_Sound_pause(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	sound_pause(sound, true);
	return 0;
}

static duk_ret_t
js_Sound_play(duk_context* ctx)
{
	int      num_args;
	sound_t* sound;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	if (num_args >= 1) {
		sound_set_repeat(sound, duk_require_boolean(ctx, 0));
		sound_play(sound, s_sound_mixer);
	}
	else {
		sound_pause(sound, false);
		if (!sound_playing(sound))
			sound_play(sound, s_sound_mixer);
	}

	return 0;
}

static duk_ret_t
js_Sound_reset(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	sound_seek(sound, 0.0);
	return 0;
}

static duk_ret_t
js_Sound_setPan(duk_context* ctx)
{
	int      new_pan;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");
	new_pan = duk_require_int(ctx, 0);

	sound_set_pan(sound, (float)new_pan / 255);
	return 0;
}

static duk_ret_t
js_Sound_setPitch(duk_context* ctx)
{
	float    new_pitch;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");
	new_pitch = duk_require_number(ctx, 0);

	sound_set_speed(sound, new_pitch);
	return 0;
}

static duk_ret_t
js_Sound_setPosition(duk_context* ctx)
{
	double   new_pos;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");
	new_pos = duk_require_number(ctx, 0);

	sound_seek(sound, floor(new_pos) / 1000000);
	return 0;
}

static duk_ret_t
js_Sound_setRepeat(duk_context* ctx)
{
	bool     is_looped;
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");
	is_looped = duk_require_boolean(ctx, 0);

	sound_set_repeat(sound, is_looped);
	return 0;
}

static duk_ret_t
js_Sound_setVolume(duk_context* ctx)
{
	int volume;

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");
	volume = duk_require_int(ctx, 0);

	volume = volume < 0 ? 0 : volume > 255 ? 255 : volume;
	sound_set_gain(sound, (float)volume / 255);
	return 0;
}

static duk_ret_t
js_Sound_stop(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "ssSound");

	sound_stop(sound);
	return 0;
}

static duk_ret_t
js_Sound_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object sound]");
	return 1;
}

static duk_ret_t
js_Spriteset_finalize(duk_context* ctx)
{
	spriteset_t* spriteset;

	spriteset = duk_require_sphere_obj(ctx, 0, "ssSpriteset");
	free_spriteset(spriteset);
	return 0;
}

static duk_ret_t
js_Spriteset_get_filename(duk_context* ctx)
{
	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "ssSpriteset");
	duk_pop(ctx);
	duk_push_string(ctx, spriteset->filename);
	return 1;
}

static duk_ret_t
js_Spriteset_get_image(duk_context* ctx)
{
	duk_uarridx_t index;
	spriteset_t*  spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "ssSpriteset");
	index = duk_to_int(ctx, 0);

	duk_push_sphere_obj(ctx, "ssImage", image_ref(get_spriteset_image(spriteset, index)));
	return 1;
}

static duk_ret_t
js_Spriteset_set_image(duk_context* ctx)
{
	image_t*      image;
	duk_uarridx_t index;
	spriteset_t*  spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "ssSpriteset");
	image = duk_require_sphere_obj(ctx, 0, "ssImage");
	index = duk_to_int(ctx, 1);

	set_spriteset_image(spriteset, index, image);
	return 0;
}

static duk_ret_t
js_Spriteset_clone(duk_context* ctx)
{
	spriteset_t* new_spriteset;
	spriteset_t* spriteset;

	duk_push_this(ctx);
	spriteset = duk_require_sphere_obj(ctx, -1, "ssSpriteset");

	if ((new_spriteset = clone_spriteset(spriteset)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to clone spriteset");
	duk_push_sphere_spriteset(ctx, new_spriteset);
	free_spriteset(new_spriteset);
	return 1;
}

static duk_ret_t
js_Spriteset_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object spriteset]");
	return 1;
}

static duk_ret_t
js_Surface_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_sphere_obj(ctx, 0, "ssSurface");
	image_free(image);
	return 0;
}

static duk_ret_t
js_Surface_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");

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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");

	if (image != NULL)
		duk_push_int(ctx, image_width(image));
	else
		duk_push_int(ctx, g_res_x);
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");

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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");

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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");

	if (!screen_is_skipframe(g_screen))
		al_draw_bitmap(image_bitmap(image), x, y, 0x0);
	return 0;
}

static duk_ret_t
js_Surface_blitMaskSurface(duk_context* ctx)
{
	image_t* src_image = duk_require_sphere_obj(ctx, 0, "ssSurface");
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);
	color_t mask = duk_require_sphere_color(ctx, 3);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image_t* src_image = duk_require_sphere_obj(ctx, 0, "ssSurface");
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	int      blend_mode;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");

	if ((new_image = image_clone(image)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:clone() - Unable to create new surface image");
	duk_push_sphere_obj(ctx, "ssSurface", new_image);
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);
	width = duk_require_int(ctx, 2);
	height = duk_require_int(ctx, 3);

	if ((new_image = image_new(width, height)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:cloneSection(): unable to create surface");
	al_set_target_bitmap(image_bitmap(new_image));
	al_draw_bitmap_region(image_bitmap(image), x, y, width, height, 0, 0, 0x0);
	al_set_target_backbuffer(screen_display(g_screen));
	duk_push_sphere_obj(ctx, "ssSurface", new_image);
	return 1;
}

static duk_ret_t
js_Surface_createImage(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");

	if ((new_image = image_clone(image)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:createImage(): unable to create image");
	duk_push_sphere_obj(ctx, "ssImage", new_image);
	return 1;
}

static duk_ret_t
js_Surface_drawText(duk_context* ctx)
{
	font_t* font = duk_require_sphere_obj(ctx, 0, "ssFont");
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);
	const char* text = duk_to_string(ctx, 3);

	int      blend_mode;
	color_t  color;
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
	duk_pop(ctx);
	image_flip(image, true, false);
	return 0;
}

static duk_ret_t
js_Surface_flipVertically(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
	duk_pop(ctx);
	image_flip(image, false, true);
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");

	w = new_w = image_width(image);
	h = new_h = image_height(image);
	if (want_resize) {
		// TODO: implement in-place resizing for Surface:rotate()
	}
	if ((new_image = image_new(new_w, new_h)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "failed to create new surface bitmap");
	al_set_target_bitmap(image_bitmap(new_image));
	al_draw_rotated_bitmap(image_bitmap(image), (float)w / 2, (float)h / 2, (float)new_w / 2, (float)new_h / 2, angle, 0x0);
	al_set_target_backbuffer(screen_display(g_screen));

	// free old image and replace internal image pointer
	// at one time this was an acceptable thing to do; now it's just a hack
	image_free(image);
	duk_push_this(ctx);
	duk_push_pointer(ctx, new_image);
	duk_put_prop_string(ctx, -2, "\xFF" "udata");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
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
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
	duk_pop(ctx);
	filename = duk_require_path(ctx, 0, "images", true);
	image_save(image, filename);
	return 1;
}

static duk_ret_t
js_Surface_setAlpha(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int a = duk_require_int(ctx, 0);
	bool want_all = n_args >= 2 ? duk_require_boolean(ctx, 1) : true;

	image_t*      image;
	image_lock_t* lock;
	color_t*      pixel;
	int           w, h;

	int i_x, i_y;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");
	duk_pop(ctx);
	if (!(lock = image_lock(image)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Surface:setAlpha(): unable to lock surface");
	w = image_width(image);
	h = image_height(image);
	a = a < 0 ? 0 : a > 255 ? 255 : a;
	for (i_y = h - 1; i_y >= 0; --i_y) for (i_x = w - 1; i_x >= 0; --i_x) {
		pixel = &lock->pixels[i_x + i_y * lock->pitch];
		pixel->a = want_all || pixel->a != 0
			? a : pixel->a;
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
js_Surface_setPixel(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	color_t color = duk_require_sphere_color(ctx, 2);

	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "ssSurface");

	image_set_pixel(image, x, y, color);
	return 0;
}

static duk_ret_t
js_Surface_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object surface]");
	return 1;
}

static duk_ret_t
js_WindowStyle_finalize(duk_context* ctx)
{
	windowstyle_t* winstyle;

	winstyle = duk_require_sphere_obj(ctx, 0, "ssWindowStyle");
	free_windowstyle(winstyle);
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
	winstyle = duk_require_sphere_obj(ctx, -1, "ssWindowStyle");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	mask = duk_require_sphere_color(ctx, -1);
	duk_pop(ctx);
	duk_pop(ctx);
	draw_window(winstyle, mask, x, y, w, h);
	return 0;
}

static duk_ret_t
js_WindowStyle_getColorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_require_sphere_obj(ctx, -1, "ssWindowStyle");
	duk_get_prop_string(ctx, -2, "\xFF" "color_mask");
	duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_WindowStyle_setColorMask(duk_context* ctx)
{
	color_t mask = duk_require_sphere_color(ctx, 0);

	duk_push_this(ctx);
	duk_require_sphere_obj(ctx, -1, "ssWindowStyle");
	duk_push_sphere_color(ctx, mask);
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_WindowStyle_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object windowstyle]");
	return 1;
}
