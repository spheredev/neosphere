#include "minisphere.h"
#include "pegasus.h"

#include "api.h"
#include "async.h"
#include "audio.h"
#include "color.h"
#include "commonjs.h"
#include "console.h"
#include "debugger.h"
#include "font.h"
#include "galileo.h"
#include "image.h"
#include "input.h"
#include "rng.h"
#include "shader.h"
#include "sockets.h"

#define SPHERE_API_VERSION 2
#define SPHERE_API_LEVEL   1

static const char* const EXTENSIONS[] =
{
	"sphere_fs_system_alias",
};

static duk_ret_t js_console_assert             (duk_context* ctx);
static duk_ret_t js_console_debug              (duk_context* ctx);
static duk_ret_t js_console_error              (duk_context* ctx);
static duk_ret_t js_console_info               (duk_context* ctx);
static duk_ret_t js_console_log                (duk_context* ctx);
static duk_ret_t js_console_trace              (duk_context* ctx);
static duk_ret_t js_console_warn               (duk_context* ctx);
static duk_ret_t js_engine_get_apiLevel        (duk_context* ctx);
static duk_ret_t js_engine_get_apiVersion      (duk_context* ctx);
static duk_ret_t js_engine_get_extensions      (duk_context* ctx);
static duk_ret_t js_engine_get_game            (duk_context* ctx);
static duk_ret_t js_engine_get_name            (duk_context* ctx);
static duk_ret_t js_engine_get_time            (duk_context* ctx);
static duk_ret_t js_engine_get_version         (duk_context* ctx);
static duk_ret_t js_engine_dispatch            (duk_context* ctx);
static duk_ret_t js_engine_doEvents            (duk_context* ctx);
static duk_ret_t js_engine_exit                (duk_context* ctx);
static duk_ret_t js_engine_restart             (duk_context* ctx);
static duk_ret_t js_engine_sleep               (duk_context* ctx);
static duk_ret_t js_fs_exists                  (duk_context* ctx);
static duk_ret_t js_fs_mkdir                   (duk_context* ctx);
static duk_ret_t js_fs_open                    (duk_context* ctx);
static duk_ret_t js_fs_rename                  (duk_context* ctx);
static duk_ret_t js_fs_rmdir                   (duk_context* ctx);
static duk_ret_t js_fs_unlink                  (duk_context* ctx);
static duk_ret_t js_keyboard_get_capsLock      (duk_context* ctx);
static duk_ret_t js_keyboard_get_numLock       (duk_context* ctx);
static duk_ret_t js_keyboard_get_scrollLock    (duk_context* ctx);
static duk_ret_t js_keyboard_clearQueue        (duk_context* ctx);
static duk_ret_t js_keyboard_getKey            (duk_context* ctx);
static duk_ret_t js_keyboard_isDown            (duk_context* ctx);
static duk_ret_t js_keyboard_keyChar           (duk_context* ctx);
static duk_ret_t js_random_chance              (duk_context* ctx);
static duk_ret_t js_random_normal              (duk_context* ctx);
static duk_ret_t js_random_random              (duk_context* ctx);
static duk_ret_t js_random_range               (duk_context* ctx);
static duk_ret_t js_random_reseed              (duk_context* ctx);
static duk_ret_t js_random_sample              (duk_context* ctx);
static duk_ret_t js_random_string              (duk_context* ctx);
static duk_ret_t js_random_uniform             (duk_context* ctx);
static duk_ret_t js_screen_get_frameRate       (duk_context* ctx);
static duk_ret_t js_screen_set_frameRate       (duk_context* ctx);
static duk_ret_t js_screen_clipTo              (duk_context* ctx);
static duk_ret_t js_screen_flip                (duk_context* ctx);
static duk_ret_t js_screen_resize              (duk_context* ctx);
static duk_ret_t js_abort                      (duk_context* ctx);
static duk_ret_t js_alert                      (duk_context* ctx);
static duk_ret_t js_assert                     (duk_context* ctx);
static duk_ret_t js_FileStream_finalize        (duk_context* ctx);
static duk_ret_t js_FileStream_get_position    (duk_context* ctx);
static duk_ret_t js_FileStream_set_position    (duk_context* ctx);
static duk_ret_t js_FileStream_get_length      (duk_context* ctx);
static duk_ret_t js_FileStream_close           (duk_context* ctx);
static duk_ret_t js_FileStream_read            (duk_context* ctx);
static duk_ret_t js_FileStream_readDouble      (duk_context* ctx);
static duk_ret_t js_FileStream_readFloat       (duk_context* ctx);
static duk_ret_t js_FileStream_readInt         (duk_context* ctx);
static duk_ret_t js_FileStream_readPString     (duk_context* ctx);
static duk_ret_t js_FileStream_readString      (duk_context* ctx);
static duk_ret_t js_FileStream_readUInt        (duk_context* ctx);
static duk_ret_t js_FileStream_write           (duk_context* ctx);
static duk_ret_t js_FileStream_writeDouble     (duk_context* ctx);
static duk_ret_t js_FileStream_writeFloat      (duk_context* ctx);
static duk_ret_t js_FileStream_writeInt        (duk_context* ctx);
static duk_ret_t js_FileStream_writePString    (duk_context* ctx);
static duk_ret_t js_FileStream_writeString     (duk_context* ctx);
static duk_ret_t js_FileStream_writeUInt       (duk_context* ctx);
static duk_ret_t js_Font_get_Default           (duk_context* ctx);
static duk_ret_t js_new_Font                   (duk_context* ctx);
static duk_ret_t js_Font_finalize              (duk_context* ctx);
static duk_ret_t js_Font_get_height            (duk_context* ctx);
static duk_ret_t js_Font_drawText              (duk_context* ctx);
static duk_ret_t js_Font_getStringHeight       (duk_context* ctx);
static duk_ret_t js_Font_getStringWidth        (duk_context* ctx);
static duk_ret_t js_Font_wordWrap              (duk_context* ctx);
static duk_ret_t js_new_Group                  (duk_context* ctx);
static duk_ret_t js_Group_finalize             (duk_context* ctx);
static duk_ret_t js_Group_get_shader           (duk_context* ctx);
static duk_ret_t js_Group_get_transform        (duk_context* ctx);
static duk_ret_t js_Group_set_shader           (duk_context* ctx);
static duk_ret_t js_Group_set_transform        (duk_context* ctx);
static duk_ret_t js_Group_draw                 (duk_context* ctx);
static duk_ret_t js_Group_setFloat             (duk_context* ctx);
static duk_ret_t js_Group_setInt               (duk_context* ctx);
static duk_ret_t js_Group_setMatrix            (duk_context* ctx);
static duk_ret_t js_new_Image                  (duk_context* ctx);
static duk_ret_t js_Image_finalize             (duk_context* ctx);
static duk_ret_t js_Image_get_height           (duk_context* ctx);
static duk_ret_t js_Image_get_width            (duk_context* ctx);
static duk_ret_t js_Mixer_get_Default          (duk_context* ctx);
static duk_ret_t js_new_Mixer                  (duk_context* ctx);
static duk_ret_t js_Mixer_finalize             (duk_context* ctx);
static duk_ret_t js_Mixer_get_volume           (duk_context* ctx);
static duk_ret_t js_Mixer_set_volume           (duk_context* ctx);
static duk_ret_t js_new_Server                 (duk_context* ctx);
static duk_ret_t js_Server_finalize            (duk_context* ctx);
static duk_ret_t js_Server_close               (duk_context* ctx);
static duk_ret_t js_Server_accept              (duk_context* ctx);
static duk_ret_t js_ShaderProgram_get_Default  (duk_context* ctx);
static duk_ret_t js_new_ShaderProgram          (duk_context* ctx);
static duk_ret_t js_ShaderProgram_finalize     (duk_context* ctx);
static duk_ret_t js_new_Shape                  (duk_context* ctx);
static duk_ret_t js_Shape_finalize             (duk_context* ctx);
static duk_ret_t js_Shape_get_texture          (duk_context* ctx);
static duk_ret_t js_Shape_set_texture          (duk_context* ctx);
static duk_ret_t js_Shape_draw                 (duk_context* ctx);
static duk_ret_t js_new_Socket                 (duk_context* ctx);
static duk_ret_t js_Socket_finalize            (duk_context* ctx);
static duk_ret_t js_Socket_get_bytesPending    (duk_context* ctx);
static duk_ret_t js_Socket_get_connected       (duk_context* ctx);
static duk_ret_t js_Socket_get_remoteAddress   (duk_context* ctx);
static duk_ret_t js_Socket_get_remotePort      (duk_context* ctx);
static duk_ret_t js_Socket_close               (duk_context* ctx);
static duk_ret_t js_Socket_read                (duk_context* ctx);
static duk_ret_t js_Socket_write               (duk_context* ctx);
static duk_ret_t js_new_Sound                  (duk_context* ctx);
static duk_ret_t js_Sound_finalize             (duk_context* ctx);
static duk_ret_t js_Sound_get_length           (duk_context* ctx);
static duk_ret_t js_Sound_get_pan              (duk_context* ctx);
static duk_ret_t js_Sound_get_playing          (duk_context* ctx);
static duk_ret_t js_Sound_get_position         (duk_context* ctx);
static duk_ret_t js_Sound_get_repeat           (duk_context* ctx);
static duk_ret_t js_Sound_get_speed            (duk_context* ctx);
static duk_ret_t js_Sound_get_volume           (duk_context* ctx);
static duk_ret_t js_Sound_set_pan              (duk_context* ctx);
static duk_ret_t js_Sound_set_position         (duk_context* ctx);
static duk_ret_t js_Sound_set_repeat           (duk_context* ctx);
static duk_ret_t js_Sound_set_speed            (duk_context* ctx);
static duk_ret_t js_Sound_set_volume           (duk_context* ctx);
static duk_ret_t js_Sound_pause                (duk_context* ctx);
static duk_ret_t js_Sound_play                 (duk_context* ctx);
static duk_ret_t js_Sound_stop                 (duk_context* ctx);
static duk_ret_t js_new_SoundStream            (duk_context* ctx);
static duk_ret_t js_SoundStream_finalize       (duk_context* ctx);
static duk_ret_t js_SoundStream_get_bufferSize (duk_context* ctx);
static duk_ret_t js_SoundStream_buffer         (duk_context* ctx);
static duk_ret_t js_SoundStream_play           (duk_context* ctx);
static duk_ret_t js_SoundStream_pause          (duk_context* ctx);
static duk_ret_t js_SoundStream_stop           (duk_context* ctx);
static duk_ret_t js_new_Surface                (duk_context* ctx);
static duk_ret_t js_Surface_finalize           (duk_context* ctx);
static duk_ret_t js_Surface_get_height         (duk_context* ctx);
static duk_ret_t js_Surface_get_width          (duk_context* ctx);
static duk_ret_t js_Surface_toImage            (duk_context* ctx);
static duk_ret_t js_new_Transform              (duk_context* ctx);
static duk_ret_t js_Transform_finalize         (duk_context* ctx);
static duk_ret_t js_Transform_compose          (duk_context* ctx);
static duk_ret_t js_Transform_identity         (duk_context* ctx);
static duk_ret_t js_Transform_rotate           (duk_context* ctx);
static duk_ret_t js_Transform_scale            (duk_context* ctx);
static duk_ret_t js_Transform_translate        (duk_context* ctx);

static mixer_t* s_def_mixer;
static int      s_framerate = 60;

void
initialize_pegasus(duk_context* ctx)
{
	console_log(1, "initializing Pegasus (v%d Lv.%d)", SPHERE_API_VERSION, SPHERE_API_LEVEL);

	s_def_mixer = mixer_new(44100, 16, 2);

	api_register_type(ctx, "FileStream", js_FileStream_finalize);
	api_register_prop(ctx, "FileStream", "length", js_FileStream_get_length, NULL);
	api_register_prop(ctx, "FileStream", "position", js_FileStream_get_position, js_FileStream_set_position);
	api_register_prop(ctx, "FileStream", "size", js_FileStream_get_length, NULL);
	api_register_method(ctx, "FileStream", "close", js_FileStream_close);
	api_register_method(ctx, "FileStream", "read", js_FileStream_read);
	api_register_method(ctx, "FileStream", "readDouble", js_FileStream_readDouble);
	api_register_method(ctx, "FileStream", "readFloat", js_FileStream_readFloat);
	api_register_method(ctx, "FileStream", "readInt", js_FileStream_readInt);
	api_register_method(ctx, "FileStream", "readPString", js_FileStream_readPString);
	api_register_method(ctx, "FileStream", "readString", js_FileStream_readString);
	api_register_method(ctx, "FileStream", "readUInt", js_FileStream_readUInt);
	api_register_method(ctx, "FileStream", "write", js_FileStream_write);
	api_register_method(ctx, "FileStream", "writeDouble", js_FileStream_writeDouble);
	api_register_method(ctx, "FileStream", "writeFloat", js_FileStream_writeFloat);
	api_register_method(ctx, "FileStream", "writeInt", js_FileStream_writeInt);
	api_register_method(ctx, "FileStream", "writePString", js_FileStream_writePString);
	api_register_method(ctx, "FileStream", "writeString", js_FileStream_writeString);
	api_register_method(ctx, "FileStream", "writeUInt", js_FileStream_writeUInt);
	api_register_ctor(ctx, "Font", js_new_Font, js_Font_finalize);
	api_register_static_prop(ctx, "Font", "Default", js_Font_get_Default, NULL);
	api_register_prop(ctx, "Font", "height", js_Font_get_height, NULL);
	api_register_method(ctx, "Font", "drawText", js_Font_drawText);
	api_register_method(ctx, "Font", "getStringHeight", js_Font_getStringHeight);
	api_register_method(ctx, "Font", "getStringWidth", js_Font_getStringWidth);
	api_register_method(ctx, "Font", "wordWrap", js_Font_wordWrap);
	api_register_ctor(ctx, "Group", js_new_Group, js_Group_finalize);
	api_register_prop(ctx, "Group", "shader", js_Group_get_shader, js_Group_set_shader);
	api_register_prop(ctx, "Group", "transform", js_Group_get_transform, js_Group_set_transform);
	api_register_method(ctx, "Group", "draw", js_Group_draw);
	api_register_method(ctx, "Group", "setFloat", js_Group_setFloat);
	api_register_method(ctx, "Group", "setInt", js_Group_setInt);
	api_register_method(ctx, "Group", "setMatrix", js_Group_setMatrix);
	api_register_ctor(ctx, "Image", js_new_Image, js_Image_finalize);
	api_register_prop(ctx, "Image", "height", js_Image_get_height, NULL);
	api_register_prop(ctx, "Image", "width", js_Image_get_width, NULL);
	api_register_ctor(ctx, "Mixer", js_new_Mixer, js_Mixer_finalize);
	api_register_static_prop(ctx, "Mixer", "Default", js_Mixer_get_Default, NULL);
	api_register_prop(ctx, "Mixer", "volume", js_Mixer_get_volume, js_Mixer_set_volume);
	api_register_ctor(ctx, "Server", js_new_Server, js_Server_finalize);
	api_register_method(ctx, "Server", "close", js_Server_close);
	api_register_method(ctx, "Server", "accept", js_Server_accept);
	api_register_ctor(ctx, "ShaderProgram", js_new_ShaderProgram, js_ShaderProgram_finalize);
	api_register_static_prop(ctx, "ShaderProgram", "Default", js_ShaderProgram_get_Default, NULL);
	api_register_ctor(ctx, "Shape", js_new_Shape, js_Shape_finalize);
	api_register_prop(ctx, "Shape", "texture", js_Shape_get_texture, js_Shape_set_texture);
	api_register_method(ctx, "Shape", "draw", js_Shape_draw);
	api_register_ctor(ctx, "Socket", js_new_Socket, js_Socket_finalize);
	api_register_prop(ctx, "Socket", "bytesPending", js_Socket_get_bytesPending, NULL);
	api_register_prop(ctx, "Socket", "connected", js_Socket_get_connected, NULL);
	api_register_prop(ctx, "Socket", "remoteAddress", js_Socket_get_remoteAddress, NULL);
	api_register_prop(ctx, "Socket", "remotePort", js_Socket_get_remotePort, NULL);
	api_register_method(ctx, "Socket", "close", js_Socket_close);
	api_register_method(ctx, "Socket", "read", js_Socket_read);
	api_register_method(ctx, "Socket", "write", js_Socket_write);
	api_register_ctor(ctx, "SoundStream", js_new_SoundStream, js_SoundStream_finalize);
	api_register_prop(ctx, "SoundStream", "bufferSize", js_SoundStream_get_bufferSize, NULL);
	api_register_method(ctx, "SoundStream", "buffer", js_SoundStream_buffer);
	api_register_method(ctx, "SoundStream", "pause", js_SoundStream_pause);
	api_register_method(ctx, "SoundStream", "play", js_SoundStream_play);
	api_register_method(ctx, "SoundStream", "stop", js_SoundStream_stop);
	api_register_ctor(ctx, "Sound", js_new_Sound, js_Sound_finalize);
	api_register_prop(ctx, "Sound", "length", js_Sound_get_length, NULL);
	api_register_prop(ctx, "Sound", "pan", js_Sound_get_pan, js_Sound_set_pan);
	api_register_prop(ctx, "Sound", "playing", js_Sound_get_playing, NULL);
	api_register_prop(ctx, "Sound", "position", js_Sound_get_position, js_Sound_set_position);
	api_register_prop(ctx, "Sound", "repeat", js_Sound_get_repeat, js_Sound_set_repeat);
	api_register_prop(ctx, "Sound", "speed", js_Sound_get_speed, js_Sound_set_speed);
	api_register_prop(ctx, "Sound", "volume", js_Sound_get_volume, js_Sound_set_volume);
	api_register_method(ctx, "Sound", "pause", js_Sound_pause);
	api_register_method(ctx, "Sound", "play", js_Sound_play);
	api_register_method(ctx, "Sound", "stop", js_Sound_stop);
	api_register_ctor(ctx, "Surface", js_new_Surface, js_Surface_finalize);
	api_register_prop(ctx, "Surface", "height", js_Surface_get_height, NULL);
	api_register_prop(ctx, "Surface", "width", js_Surface_get_width, NULL);
	api_register_method(ctx, "Surface", "toImage", js_Surface_toImage);
	api_register_ctor(ctx, "Transform", js_new_Transform, js_Transform_finalize);
	api_register_method(ctx, "Transform", "compose", js_Transform_compose);
	api_register_method(ctx, "Transform", "identity", js_Transform_identity);
	api_register_method(ctx, "Transform", "rotate", js_Transform_rotate);
	api_register_method(ctx, "Transform", "scale", js_Transform_scale);
	api_register_method(ctx, "Transform", "translate", js_Transform_translate);

	api_register_static_func(ctx, NULL, "abort", js_abort);
	api_register_static_func(ctx, NULL, "alert", js_alert);
	api_register_static_func(ctx, NULL, "assert", js_assert);

	api_register_static_func(ctx, "console", "assert", js_console_assert);
	api_register_static_func(ctx, "console", "debug", js_console_debug);
	api_register_static_func(ctx, "console", "error", js_console_error);
	api_register_static_func(ctx, "console", "info", js_console_info);
	api_register_static_func(ctx, "console", "log", js_console_log);
	api_register_static_func(ctx, "console", "trace", js_console_trace);
	api_register_static_func(ctx, "console", "warn", js_console_warn);
	api_register_static_prop(ctx, "engine", "apiLevel", js_engine_get_apiLevel, NULL);
	api_register_static_prop(ctx, "engine", "apiVersion", js_engine_get_apiVersion, NULL);
	api_register_static_prop(ctx, "engine", "extensions", js_engine_get_extensions, NULL);
	api_register_static_prop(ctx, "engine", "game", js_engine_get_game, NULL);
	api_register_static_prop(ctx, "engine", "name", js_engine_get_name, NULL);
	api_register_static_prop(ctx, "engine", "time", js_engine_get_time, NULL);
	api_register_static_prop(ctx, "engine", "version", js_engine_get_version, NULL);
	api_register_static_func(ctx, "engine", "dispatch", js_engine_dispatch);
	api_register_static_func(ctx, "engine", "doEvents", js_engine_doEvents);
	api_register_static_func(ctx, "engine", "exit", js_engine_exit);
	api_register_static_func(ctx, "engine", "restart", js_engine_restart);
	api_register_static_func(ctx, "engine", "sleep", js_engine_sleep);
	api_register_static_func(ctx, "fs", "exists", js_fs_exists);
	api_register_static_func(ctx, "fs", "open", js_fs_open);
	api_register_static_func(ctx, "fs", "mkdir", js_fs_mkdir);
	api_register_static_func(ctx, "fs", "rename", js_fs_rename);
	api_register_static_func(ctx, "fs", "rmdir", js_fs_rmdir);
	api_register_static_func(ctx, "fs", "unlink", js_fs_unlink);
	api_register_static_prop(ctx, "keyboard", "capsLock", js_keyboard_get_capsLock, NULL);
	api_register_static_prop(ctx, "keyboard", "numLock", js_keyboard_get_numLock, NULL);
	api_register_static_prop(ctx, "keyboard", "scrollLock", js_keyboard_get_scrollLock, NULL);
	api_register_static_func(ctx, "keyboard", "clearQueue", js_keyboard_clearQueue);
	api_register_static_func(ctx, "keyboard", "getKey", js_keyboard_getKey);
	api_register_static_func(ctx, "keyboard", "isDown", js_keyboard_isDown);
	api_register_static_func(ctx, "keyboard", "keyChar", js_keyboard_keyChar);
	api_register_static_func(ctx, "random", "chance", js_random_chance);
	api_register_static_func(ctx, "random", "normal", js_random_normal);
	api_register_static_func(ctx, "random", "random", js_random_random);
	api_register_static_func(ctx, "random", "range", js_random_range);
	api_register_static_func(ctx, "random", "reseed", js_random_reseed);
	api_register_static_func(ctx, "random", "sample", js_random_sample);
	api_register_static_func(ctx, "random", "string", js_random_string);
	api_register_static_func(ctx, "random", "uniform", js_random_uniform);
	api_register_static_obj(ctx, NULL, "screen", "Surface", NULL);
	api_register_static_prop(ctx, "screen", "frameRate", js_screen_get_frameRate, js_screen_set_frameRate);
	api_register_static_func(ctx, "screen", "clipTo", js_screen_clipTo);
	api_register_static_func(ctx, "screen", "flip", js_screen_flip);
	api_register_static_func(ctx, "screen", "resize", js_screen_resize);

	api_register_const(ctx, "Key", "None", 0);
	api_register_const(ctx, "Key", "Alt", ALLEGRO_KEY_ALT);
	api_register_const(ctx, "Key", "AltGr", ALLEGRO_KEY_ALTGR);
	api_register_const(ctx, "Key", "Apostrophe", ALLEGRO_KEY_QUOTE);
	api_register_const(ctx, "Key", "Backslash", ALLEGRO_KEY_BACKSLASH);
	api_register_const(ctx, "Key", "Backspace", ALLEGRO_KEY_BACKSPACE);
	api_register_const(ctx, "Key", "CloseBrace", ALLEGRO_KEY_CLOSEBRACE);
	api_register_const(ctx, "Key", "CapsLock", ALLEGRO_KEY_CAPSLOCK);
	api_register_const(ctx, "Key", "Comma", ALLEGRO_KEY_COMMA);
	api_register_const(ctx, "Key", "LCtrl", ALLEGRO_KEY_LCTRL);
	api_register_const(ctx, "Key", "RCtrl", ALLEGRO_KEY_RCTRL);
	api_register_const(ctx, "Key", "Delete", ALLEGRO_KEY_DELETE);
	api_register_const(ctx, "Key", "Down", ALLEGRO_KEY_DOWN);
	api_register_const(ctx, "Key", "End", ALLEGRO_KEY_END);
	api_register_const(ctx, "Key", "Enter", ALLEGRO_KEY_ENTER);
	api_register_const(ctx, "Key", "Equals", ALLEGRO_KEY_EQUALS);
	api_register_const(ctx, "Key", "Escape", ALLEGRO_KEY_ESCAPE);
	api_register_const(ctx, "Key", "Home", ALLEGRO_KEY_HOME);
	api_register_const(ctx, "Key", "Hyphen", ALLEGRO_KEY_MINUS);
	api_register_const(ctx, "Key", "Insert", ALLEGRO_KEY_INSERT);
	api_register_const(ctx, "Key", "Left", ALLEGRO_KEY_LEFT);
	api_register_const(ctx, "Key", "NumLock", ALLEGRO_KEY_NUMLOCK);
	api_register_const(ctx, "Key", "OpenBrace", ALLEGRO_KEY_OPENBRACE);
	api_register_const(ctx, "Key", "PageDown", ALLEGRO_KEY_PGDN);
	api_register_const(ctx, "Key", "PageUp", ALLEGRO_KEY_PGUP);
	api_register_const(ctx, "Key", "Period", ALLEGRO_KEY_FULLSTOP);
	api_register_const(ctx, "Key", "Right", ALLEGRO_KEY_RIGHT);
	api_register_const(ctx, "Key", "ScrollLock", ALLEGRO_KEY_SCROLLLOCK);
	api_register_const(ctx, "Key", "Semicolon", ALLEGRO_KEY_SEMICOLON);
	api_register_const(ctx, "Key", "LShift", ALLEGRO_KEY_LSHIFT);
	api_register_const(ctx, "Key", "RShift", ALLEGRO_KEY_RSHIFT);
	api_register_const(ctx, "Key", "Slash", ALLEGRO_KEY_SLASH);
	api_register_const(ctx, "Key", "Space", ALLEGRO_KEY_SPACE);
	api_register_const(ctx, "Key", "Tab", ALLEGRO_KEY_TAB);
	api_register_const(ctx, "Key", "Tilde", ALLEGRO_KEY_TILDE);
	api_register_const(ctx, "Key", "Up", ALLEGRO_KEY_UP);
	api_register_const(ctx, "Key", "F1", ALLEGRO_KEY_F1);
	api_register_const(ctx, "Key", "F2", ALLEGRO_KEY_F2);
	api_register_const(ctx, "Key", "F3", ALLEGRO_KEY_F3);
	api_register_const(ctx, "Key", "F4", ALLEGRO_KEY_F4);
	api_register_const(ctx, "Key", "F5", ALLEGRO_KEY_F5);
	api_register_const(ctx, "Key", "F6", ALLEGRO_KEY_F6);
	api_register_const(ctx, "Key", "F7", ALLEGRO_KEY_F7);
	api_register_const(ctx, "Key", "F8", ALLEGRO_KEY_F8);
	api_register_const(ctx, "Key", "F9", ALLEGRO_KEY_F9);
	api_register_const(ctx, "Key", "F10", ALLEGRO_KEY_F10);
	api_register_const(ctx, "Key", "F11", ALLEGRO_KEY_F11);
	api_register_const(ctx, "Key", "F12", ALLEGRO_KEY_F12);
	api_register_const(ctx, "Key", "A", ALLEGRO_KEY_A);
	api_register_const(ctx, "Key", "B", ALLEGRO_KEY_B);
	api_register_const(ctx, "Key", "C", ALLEGRO_KEY_C);
	api_register_const(ctx, "Key", "D", ALLEGRO_KEY_D);
	api_register_const(ctx, "Key", "E", ALLEGRO_KEY_E);
	api_register_const(ctx, "Key", "F", ALLEGRO_KEY_F);
	api_register_const(ctx, "Key", "G", ALLEGRO_KEY_G);
	api_register_const(ctx, "Key", "H", ALLEGRO_KEY_H);
	api_register_const(ctx, "Key", "I", ALLEGRO_KEY_I);
	api_register_const(ctx, "Key", "J", ALLEGRO_KEY_J);
	api_register_const(ctx, "Key", "K", ALLEGRO_KEY_K);
	api_register_const(ctx, "Key", "L", ALLEGRO_KEY_L);
	api_register_const(ctx, "Key", "M", ALLEGRO_KEY_M);
	api_register_const(ctx, "Key", "N", ALLEGRO_KEY_N);
	api_register_const(ctx, "Key", "O", ALLEGRO_KEY_O);
	api_register_const(ctx, "Key", "P", ALLEGRO_KEY_P);
	api_register_const(ctx, "Key", "Q", ALLEGRO_KEY_Q);
	api_register_const(ctx, "Key", "R", ALLEGRO_KEY_R);
	api_register_const(ctx, "Key", "S", ALLEGRO_KEY_S);
	api_register_const(ctx, "Key", "T", ALLEGRO_KEY_T);
	api_register_const(ctx, "Key", "U", ALLEGRO_KEY_U);
	api_register_const(ctx, "Key", "V", ALLEGRO_KEY_V);
	api_register_const(ctx, "Key", "W", ALLEGRO_KEY_W);
	api_register_const(ctx, "Key", "X", ALLEGRO_KEY_X);
	api_register_const(ctx, "Key", "Y", ALLEGRO_KEY_Y);
	api_register_const(ctx, "Key", "Z", ALLEGRO_KEY_Z);
	api_register_const(ctx, "Key", "D1", ALLEGRO_KEY_1);
	api_register_const(ctx, "Key", "D2", ALLEGRO_KEY_2);
	api_register_const(ctx, "Key", "D3", ALLEGRO_KEY_3);
	api_register_const(ctx, "Key", "D4", ALLEGRO_KEY_4);
	api_register_const(ctx, "Key", "D5", ALLEGRO_KEY_5);
	api_register_const(ctx, "Key", "D6", ALLEGRO_KEY_6);
	api_register_const(ctx, "Key", "D7", ALLEGRO_KEY_7);
	api_register_const(ctx, "Key", "D8", ALLEGRO_KEY_8);
	api_register_const(ctx, "Key", "D9", ALLEGRO_KEY_9);
	api_register_const(ctx, "Key", "D0", ALLEGRO_KEY_0);
	api_register_const(ctx, "Key", "NumPad1", ALLEGRO_KEY_PAD_1);
	api_register_const(ctx, "Key", "NumPad2", ALLEGRO_KEY_PAD_2);
	api_register_const(ctx, "Key", "NumPad3", ALLEGRO_KEY_PAD_3);
	api_register_const(ctx, "Key", "NumPad4", ALLEGRO_KEY_PAD_4);
	api_register_const(ctx, "Key", "NumPad5", ALLEGRO_KEY_PAD_5);
	api_register_const(ctx, "Key", "NumPad6", ALLEGRO_KEY_PAD_6);
	api_register_const(ctx, "Key", "NumPad7", ALLEGRO_KEY_PAD_7);
	api_register_const(ctx, "Key", "NumPad8", ALLEGRO_KEY_PAD_8);
	api_register_const(ctx, "Key", "NumPad9", ALLEGRO_KEY_PAD_9);
	api_register_const(ctx, "Key", "NumPad0", ALLEGRO_KEY_PAD_0);
	api_register_const(ctx, "Key", "NumPadEnter", ALLEGRO_KEY_PAD_ENTER);
	api_register_const(ctx, "Key", "Add", ALLEGRO_KEY_PAD_PLUS);
	api_register_const(ctx, "Key", "Decimal", ALLEGRO_KEY_PAD_DELETE);
	api_register_const(ctx, "Key", "Divide", ALLEGRO_KEY_PAD_SLASH);
	api_register_const(ctx, "Key", "Multiply", ALLEGRO_KEY_PAD_ASTERISK);
	api_register_const(ctx, "Key", "Subtract", ALLEGRO_KEY_PAD_MINUS);
	api_register_const(ctx, "ShapeType", "Auto", SHAPE_AUTO);
	api_register_const(ctx, "ShapeType", "Fan", SHAPE_TRI_FAN);
	api_register_const(ctx, "ShapeType", "Lines", SHAPE_LINES);
	api_register_const(ctx, "ShapeType", "LineLoop", SHAPE_LINE_LOOP);
	api_register_const(ctx, "ShapeType", "LineStrip", SHAPE_LINE_STRIP);
	api_register_const(ctx, "ShapeType", "Points", SHAPE_POINTS);
	api_register_const(ctx, "ShapeType", "Triangles", SHAPE_TRIANGLES);
	api_register_const(ctx, "ShapeType", "TriStrip", SHAPE_TRI_STRIP);

	// `console` is a Proxy so that unimplemented methods do not throw
	duk_eval_string_noresult(g_duk,
		"global.console = new Proxy(global.console, {\n"
		"    get: function(t, name) {\n"
		"        return name in t ? t[name] : function() {};\n"
		"    }\n"
		"});"
	);
}

static duk_ret_t
js_console_assert(duk_context* ctx)
{
	const char* message;
	bool        result;

	result = duk_to_boolean(ctx, 0);
	message = duk_safe_to_string(ctx, 1);

	if (!result)
		debug_print(message, PRINT_ASSERT);
	return 0;
}

static duk_ret_t
js_console_debug(duk_context* ctx)
{
	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_DEBUG);
	return 0;
}

static duk_ret_t
js_console_error(duk_context* ctx)
{
	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_ERROR);
	return 0;
}

static duk_ret_t
js_console_info(duk_context* ctx)
{
	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_INFO);
	return 0;
}

static duk_ret_t
js_console_log(duk_context* ctx)
{
	// note: console.log() does not currently support format specifiers.
	//       this may change in a future implementation.

	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_NORMAL);
	return 0;
}

static duk_ret_t
js_console_trace(duk_context* ctx)
{
	// note: console.log() does not currently support format specifiers.
	//       this may change in a future implementation.

	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_TRACE);
	return 0;
}

static duk_ret_t
js_console_warn(duk_context* ctx)
{
	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_WARN);
	return 0;
}

static duk_ret_t
js_engine_get_apiLevel(duk_context* ctx)
{
	duk_push_int(ctx, SPHERE_API_LEVEL);
	return 1;
}

static duk_ret_t
js_engine_get_apiVersion(duk_context* ctx)
{
	duk_push_int(ctx, SPHERE_API_VERSION);
	return 1;
}

static duk_ret_t
js_engine_get_extensions(duk_context* ctx)
{
	int i;

	duk_push_array(ctx);
	for (i = 0; i < sizeof EXTENSIONS / sizeof *EXTENSIONS; ++i) {
		duk_push_string(ctx, EXTENSIONS[i]);
		duk_put_prop_index(ctx, -2, i++);
	}

	duk_push_this(ctx);
	duk_push_string(ctx, "extensions");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_engine_get_game(duk_context* ctx)
{
	duk_push_lstring_t(ctx, get_game_manifest(g_fs));
	duk_json_decode(ctx, -1);

	duk_push_this(ctx);
	duk_push_string(ctx, "game");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_engine_get_name(duk_context* ctx)
{
	duk_push_string(ctx, PRODUCT_NAME);
	return 1;
}

static duk_ret_t
js_engine_get_time(duk_context* ctx)
{
	duk_push_number(ctx, al_get_time());
	return 1;
}

static duk_ret_t
js_engine_get_version(duk_context* ctx)
{
	duk_push_string(ctx, VERSION_NAME);
	return 1;
}

static duk_ret_t
js_engine_dispatch(duk_context* ctx)
{
	script_t* script;

	script = duk_require_sphere_script(ctx, 0, "synth:async.js");

	if (!queue_async_script(script))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to dispatch async script");
	return 0;
}

static duk_ret_t
js_engine_doEvents(duk_context* ctx)
{
	do_events();
	duk_push_boolean(ctx, true);
	return 1;
}

static duk_ret_t
js_engine_exit(duk_context* ctx)
{
	exit_game(false);
}

static duk_ret_t
js_engine_restart(duk_context* ctx)
{
	restart_engine();
}

static duk_ret_t
js_engine_sleep(duk_context* ctx)
{
	double timeout;

	timeout = duk_require_number(ctx, 0);

	delay(timeout);
	return 0;
}

static duk_ret_t
js_fs_exists(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, false);
	duk_push_boolean(ctx, sfs_fexist(g_fs, filename, NULL));
	return 1;
}

static duk_ret_t
js_fs_mkdir(duk_context* ctx)
{
	const char* name;

	name = duk_require_path(ctx, 0, NULL, false);
	if (!sfs_mkdir(g_fs, name, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to make directory `%s`", name);
	return 0;
}

static duk_ret_t
js_fs_open(duk_context* ctx)
{
	sfs_file_t* file;
	const char* filename;
	const char* mode;

	filename = duk_require_path(ctx, 0, NULL, false);
	mode = duk_require_string(ctx, 1);
	file = sfs_fopen(g_fs, filename, NULL, mode);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to open `%s` in mode `%s`",
			filename, mode);
	duk_push_sphere_obj(ctx, "FileStream", file);
	return 1;
}

static duk_ret_t
js_fs_rename(duk_context* ctx)
{
	const char* name1;
	const char* name2;

	name1 = duk_require_path(ctx, 0, NULL, false);
	name2 = duk_require_path(ctx, 1, NULL, false);
	if (!sfs_rename(g_fs, name1, name2, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to rename `%s` to `%s`", name1, name2);
	return 0;
}

static duk_ret_t
js_fs_rmdir(duk_context* ctx)
{
	const char* name;

	name = duk_require_path(ctx, 0, NULL, false);
	if (!sfs_rmdir(g_fs, name, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to remove directory `%s`", name);
	return 0;
}

static duk_ret_t
js_fs_unlink(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, NULL, false);
	if (!sfs_unlink(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to unlink `%s`", filename);
	return 0;
}

static duk_ret_t
js_keyboard_isDown(duk_context* ctx)
{
	int keycode;

	keycode = duk_require_int(ctx, 0);

	duk_push_boolean(ctx, kb_is_key_down(keycode));
	return 1;
}

static duk_ret_t
js_keyboard_keyChar(duk_context* ctx)
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
js_keyboard_get_capsLock(duk_context* ctx)
{
	duk_push_boolean(ctx, kb_is_toggled(ALLEGRO_KEY_CAPSLOCK));
	return 1;
}

static duk_ret_t
js_keyboard_get_numLock(duk_context* ctx)
{
	duk_push_boolean(ctx, kb_is_toggled(ALLEGRO_KEY_NUMLOCK));
	return 1;
}

static duk_ret_t
js_keyboard_get_scrollLock(duk_context* ctx)
{
	duk_push_boolean(ctx, kb_is_toggled(ALLEGRO_KEY_SCROLLLOCK));
	return 1;
}

static duk_ret_t
js_keyboard_clearQueue(duk_context* ctx)
{
	kb_clear_queue();
	return 0;
}

static duk_ret_t
js_keyboard_getKey(duk_context* ctx)
{
	duk_push_int(ctx, kb_get_key());
	return 1;
}

static duk_ret_t
js_random_chance(duk_context* ctx)
{
	double odds;

	odds = duk_require_number(ctx, 0);
	duk_push_boolean(ctx, rng_chance(odds));
	return 1;
}

static duk_ret_t
js_random_normal(duk_context* ctx)
{
	double mean;
	double sigma;

	mean = duk_require_number(ctx, 0);
	sigma = duk_require_number(ctx, 1);
	duk_push_number(ctx, rng_normal(mean, sigma));
	return 1;
}

static duk_ret_t
js_random_random(duk_context* ctx)
{
	duk_push_number(ctx, rng_random());
	return 1;
}

static duk_ret_t
js_random_range(duk_context* ctx)
{
	long lower;
	long upper;

	lower = duk_require_number(ctx, 0);
	upper = duk_require_number(ctx, 1);
	duk_push_number(ctx, rng_int(lower, upper));
	return 1;
}

static duk_ret_t
js_random_reseed(duk_context* ctx)
{
	uint64_t new_seed;

	new_seed = duk_require_number(ctx, 0);
	seed_rng(new_seed);
	return 0;
}

static duk_ret_t
js_random_sample(duk_context* ctx)
{
	duk_uarridx_t index;
	long          length;

	duk_require_object_coercible(ctx, 0);
	length = (long)duk_get_length(ctx, 0);
	index = rng_int(0, length - 1);
	duk_get_prop_index(ctx, 0, index);
	return 1;
}

static duk_ret_t
js_random_string(duk_context* ctx)
{
	int num_args;
	int length;

	num_args = duk_get_top(ctx);
	length = num_args >= 1 ? duk_require_number(ctx, 0)
		: 10;
	if (length < 1 || length > 255)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "RNG.string(): length must be [1-255] (got: %d)", length);

	duk_push_string(ctx, rng_string(length));
	return 1;
}

static duk_ret_t
js_random_uniform(duk_context* ctx)
{
	double mean;
	double variance;

	mean = duk_require_number(ctx, 0);
	variance = duk_require_number(ctx, 1);
	duk_push_number(ctx, rng_uniform(mean, variance));
	return 1;
}

static duk_ret_t
js_screen_get_frameRate(duk_context* ctx)
{
	duk_push_int(ctx, s_framerate);
	return 1;
}

static duk_ret_t
js_screen_set_frameRate(duk_context* ctx)
{
	int framerate;

	framerate = duk_require_int(ctx, 0);

	if (framerate < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "frameRate cannot be negative");
	s_framerate = framerate;
	return 0;
}

static duk_ret_t
js_screen_clipTo(duk_context* ctx)
{
	int x;
	int y;
	int width;
	int height;

	x = duk_require_int(ctx, 0);
	y = duk_require_int(ctx, 1);
	width = duk_require_int(ctx, 2);
	height = duk_require_int(ctx, 3);
	screen_set_clipping(g_screen, new_rect(x, y, x + width, y + height));
	return 0;
}

static duk_ret_t
js_screen_flip(duk_context* ctx)
{
	screen_flip(g_screen, s_framerate);
	return 0;
}

static duk_ret_t
js_screen_resize(duk_context* ctx)
{
	int  res_width;
	int  res_height;

	res_width = duk_require_int(ctx, 0);
	res_height = duk_require_int(ctx, 1);

	if (res_width < 0 || res_height < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "illegal screen resolution");
	screen_resize(g_screen, res_width, res_height);
	return 0;
}

static duk_ret_t
js_abort(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* message = n_args >= 1 ? duk_to_string(ctx, 0) : "Some type of weird pig just ate your game!\n\n\n\n\n\n\n\n...and you*munch*";
	int stack_offset = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Abort(): stack offset must be negative");
	duk_error_ni(ctx, -1 + stack_offset, DUK_ERR_ERROR, "%s", message);
}

static duk_ret_t
js_alert(duk_context* ctx)
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
js_assert(duk_context* ctx)
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
js_FileStream_finalize(duk_context* ctx)
{
	sfs_file_t* file;

	file = duk_require_sphere_obj(ctx, 0, "FileStream");
	if (file != NULL) sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_FileStream_get_position(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_push_number(ctx, sfs_ftell(file));
	return 1;
}

static duk_ret_t
js_FileStream_set_position(duk_context* ctx)
{
	sfs_file_t* file;
	long long   new_pos;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	new_pos = duk_require_number(ctx, 0);
	sfs_fseek(file, new_pos, SFS_SEEK_SET);
	return 0;
}

static duk_ret_t
js_FileStream_get_length(duk_context* ctx)
{
	sfs_file_t* file;
	long        file_pos;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	file_pos = sfs_ftell(file);
	sfs_fseek(file, 0, SEEK_END);
	duk_push_number(ctx, sfs_ftell(file));
	sfs_fseek(file, file_pos, SEEK_SET);
	return 1;
}

static duk_ret_t
js_FileStream_close(duk_context* ctx)
{
	sfs_file_t* file;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_push_pointer(ctx, NULL);
	duk_put_prop_string(ctx, -2, "\xFF" "udata");
	sfs_fclose(file);
	return 0;
}

static duk_ret_t
js_FileStream_read(duk_context* ctx)
{
	// FileStream:read([numBytes]);
	// Reads data from the stream, returning it in an ArrayBuffer.
	// Arguments:
	//     numBytes: Optional. The number of bytes to read. If not provided, the
	//               entire file is read.

	int          argc;
	void*        buffer;
	sfs_file_t*  file;
	int          num_bytes;
	long         pos;

	argc = duk_get_top(ctx);
	num_bytes = argc >= 1 ? duk_require_int(ctx, 0) : 0;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (argc < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "string length must be zero or greater");
	buffer = duk_push_fixed_buffer(ctx, num_bytes);
	num_bytes = (int)sfs_fread(buffer, 1, num_bytes, file);
	if (argc < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	duk_push_buffer_object(ctx, -1, 0, num_bytes, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_FileStream_readDouble(duk_context* ctx)
{
	int         argc;
	uint8_t     data[8];
	sfs_file_t* file;
	bool        little_endian;
	double      value;

	int i;

	argc = duk_get_top(ctx);
	little_endian = argc >= 1 ? duk_require_boolean(ctx, 0) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (sfs_fread(data, 1, 8, file) != 8)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read double from file");
	if (little_endian == is_cpu_little_endian())
		memcpy(&value, data, 8);
	else {
		for (i = 0; i < 8; ++i)
			((uint8_t*)&value)[i] = data[7 - i];
	}
	duk_push_number(ctx, value);
	return 1;
}

static duk_ret_t
js_FileStream_readFloat(duk_context* ctx)
{
	int         argc;
	uint8_t     data[8];
	sfs_file_t* file;
	bool        little_endian;
	float       value;

	int i;

	argc = duk_get_top(ctx);
	little_endian = argc >= 1 ? duk_require_boolean(ctx, 0) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (sfs_fread(data, 1, 4, file) != 4)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read float from file");
	if (little_endian == is_cpu_little_endian())
		memcpy(&value, data, 4);
	else {
		for (i = 0; i < 4; ++i)
			((uint8_t*)&value)[i] = data[3 - i];
	}
	duk_push_number(ctx, value);
	return 1;
}

static duk_ret_t
js_FileStream_readInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	int         num_bytes;
	intmax_t    value;

	argc = duk_get_top(ctx);
	num_bytes = duk_require_int(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "int byte size must be in [1-6] range");
	if (!sfs_read_int(file, &value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read int from file");
	duk_push_number(ctx, (double)value);
	return 1;
}

static duk_ret_t
js_FileStream_readPString(duk_context* ctx)
{
	int          argc;
	void*        buffer;
	sfs_file_t*  file;
	intmax_t     length;
	bool         little_endian;
	int          uint_size;

	argc = duk_get_top(ctx);
	uint_size = duk_require_int(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (uint_size < 1 || uint_size > 4)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "length bytes must be in [1-4] range (got: %d)", uint_size);
	if (!sfs_read_uint(file, &length, uint_size, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read pstring from file");
	buffer = malloc((size_t)length);
	if (sfs_fread(buffer, 1, (size_t)length, file) != (size_t)length)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read pstring from file");
	duk_push_lstring(ctx, buffer, (size_t)length);
	free(buffer);
	return 1;
}

static duk_ret_t
js_FileStream_readString(duk_context* ctx)
{
	// FileStream:readString([numBytes]);
	// Reads data from the stream, returning it in an ArrayBuffer.
	// Arguments:
	//     numBytes: Optional. The number of bytes to read. If not provided, the
	//               entire file is read.

	int          argc;
	void*        buffer;
	sfs_file_t*  file;
	int          num_bytes;
	long         pos;

	argc = duk_get_top(ctx);
	num_bytes = argc >= 1 ? duk_require_int(ctx, 0) : 0;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (argc < 1) {  // if no arguments, read entire file back to front
		pos = sfs_ftell(file);
		num_bytes = (sfs_fseek(file, 0, SEEK_END), sfs_ftell(file));
		sfs_fseek(file, 0, SEEK_SET);
	}
	if (num_bytes < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "string length must be zero or greater");
	buffer = malloc(num_bytes);
	num_bytes = (int)sfs_fread(buffer, 1, num_bytes, file);
	if (argc < 1)  // reset file position after whole-file read
		sfs_fseek(file, pos, SEEK_SET);
	duk_push_lstring(ctx, buffer, num_bytes);
	free(buffer);
	return 1;
}

static duk_ret_t
js_FileStream_readUInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	int         num_bytes;
	intmax_t     value;

	argc = duk_get_top(ctx);
	num_bytes = duk_require_int(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "uint byte size must be in [1-6] range");
	if (!sfs_read_uint(file, &value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to read uint from file");
	duk_push_number(ctx, (double)value);
	return 1;
}

static duk_ret_t
js_FileStream_write(duk_context* ctx)
{
	const void* data;
	sfs_file_t* file;
	duk_size_t  num_bytes;

	duk_require_stack_top(ctx, 1);
	data = duk_require_buffer_data(ctx, 0, &num_bytes);

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (sfs_fwrite(data, 1, num_bytes, file) != num_bytes)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write data to file");
	return 0;
}

static duk_ret_t
js_FileStream_writeDouble(duk_context* ctx)
{
	int         argc;
	uint8_t     data[8];
	sfs_file_t* file;
	bool        little_endian;
	double      value;

	int i;

	argc = duk_get_top(ctx);
	value = duk_require_number(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (little_endian == is_cpu_little_endian())
		memcpy(data, &value, 8);
	else {
		for (i = 0; i < 8; ++i)
			data[i] = ((uint8_t*)&value)[7 - i];
	}
	if (sfs_fwrite(data, 1, 8, file) != 8)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write double to file");
	return 0;
}

static duk_ret_t
js_FileStream_writeFloat(duk_context* ctx)
{
	int         argc;
	uint8_t     data[4];
	sfs_file_t* file;
	bool        little_endian;
	float       value;

	int i;

	argc = duk_get_top(ctx);
	value = duk_require_number(ctx, 0);
	little_endian = argc >= 2 ? duk_require_boolean(ctx, 1) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (little_endian == is_cpu_little_endian())
		memcpy(data, &value, 4);
	else {
		for (i = 0; i < 4; ++i)
			data[i] = ((uint8_t*)&value)[3 - i];
	}
	if (sfs_fwrite(data, 1, 4, file) != 4)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write float to file");
	return 0;
}

static duk_ret_t
js_FileStream_writeInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	intmax_t    min_value;
	intmax_t    max_value;
	int         num_bytes;
	intmax_t    value;

	argc = duk_get_top(ctx);
	value = (intmax_t)duk_require_number(ctx, 0);
	num_bytes = duk_require_int(ctx, 1);
	little_endian = argc >= 3 ? duk_require_boolean(ctx, 2) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "int byte size must be in [1-6] range");
	min_value = pow(-2, num_bytes * 8 - 1);
	max_value = pow(2, num_bytes * 8 - 1) - 1;
	if (value < min_value || value > max_value)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "value is unrepresentable in `%d` bytes", num_bytes);
	if (!sfs_write_int(file, value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write int to file");
	return 0;
}

static duk_ret_t
js_FileStream_writePString(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	intmax_t    max_len;
	intmax_t    num_bytes;
	const char* string;
	duk_size_t  string_len;
	int         uint_size;

	argc = duk_get_top(ctx);
	string = duk_require_lstring(ctx, 0, &string_len);
	uint_size = duk_require_int(ctx, 1);
	little_endian = argc >= 3 ? duk_require_boolean(ctx, 2) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (uint_size < 1 || uint_size > 4)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "length bytes must be in [1-4] range", uint_size);
	max_len = pow(2, uint_size * 8) - 1;
	num_bytes = (intmax_t)string_len;
	if (num_bytes > max_len)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "string is too long for `%d`-byte length", uint_size);
	if (!sfs_write_uint(file, num_bytes, uint_size, little_endian)
		|| sfs_fwrite(string, 1, string_len, file) != string_len)
	{
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write pstring to file");
	}
	return 0;
}

static duk_ret_t
js_FileStream_writeString(duk_context* ctx)
{
	const void* data;
	sfs_file_t* file;
	duk_size_t  num_bytes;

	duk_require_stack_top(ctx, 1);
	data = duk_get_lstring(ctx, 0, &num_bytes);

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (sfs_fwrite(data, 1, num_bytes, file) != num_bytes)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write string to file");
	return 0;
}

static duk_ret_t
js_FileStream_writeUInt(duk_context* ctx)
{
	int         argc;
	sfs_file_t* file;
	bool        little_endian;
	intmax_t    max_value;
	int         num_bytes;
	intmax_t    value;

	argc = duk_get_top(ctx);
	value = (intmax_t)duk_require_number(ctx, 0);
	num_bytes = duk_require_int(ctx, 1);
	little_endian = argc >= 3 ? duk_require_boolean(ctx, 2) : false;

	duk_push_this(ctx);
	file = duk_require_sphere_obj(ctx, -1, "FileStream");
	duk_pop(ctx);
	if (file == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FileStream was closed");
	if (num_bytes < 1 || num_bytes > 6)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "uint byte size must be in [1-6] range");
	max_value = pow(2, num_bytes * 8) - 1;
	if (value < 0 || value > max_value)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "value is unrepresentable in `%d` bytes", num_bytes);
	if (!sfs_write_uint(file, value, num_bytes, little_endian))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to write int to file");
	return 0;
}

static duk_ret_t
js_Font_get_Default(duk_context* ctx)
{
	duk_push_sphere_obj(ctx, "Font", g_sys_font);

	duk_push_this(ctx);
	duk_push_string(ctx, "Default");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_new_Font(duk_context* ctx)
{
	const char* filename;
	font_t*     font;

	filename = duk_require_path(ctx, 0, NULL, false);
	font = font_load(filename);
	if (font == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to load font `%s`", filename);
	duk_push_sphere_obj(ctx, "Font", font);
	font_free(font);
	return 1;
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
js_Font_drawText(duk_context* ctx)
{
	color_t     color;
	font_t*     font;
	int         height;
	int         num_args;
	image_t*    surface;
	const char* text;
	int         width;
	wraptext_t* wraptext;
	int         x;
	int         y;

	int i;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	surface = duk_require_sphere_obj(ctx, 0, "Surface");
	x = duk_require_int(ctx, 1);
	y = duk_require_int(ctx, 2);
	text = duk_to_string(ctx, 3);
	color = num_args >= 5 ? duk_require_sphere_color(ctx, 4)
		: color_new(255, 255, 255, 255);
	width = num_args >= 6 ? duk_require_int(ctx, 5) : 0;

	if (surface == NULL && screen_is_skipframe(g_screen))
		return 0;
	else {
		if (surface != NULL)
			al_set_target_bitmap(image_bitmap(surface));
		if (num_args < 6)
			font_draw_text(font, color, x, y, TEXT_ALIGN_LEFT, text);
		else {
			wraptext = wraptext_new(text, font, width);
			height = font_height(font);
			for (i = 0; i < wraptext_len(wraptext); ++i)
				font_draw_text(font, color, x, y + i * height, TEXT_ALIGN_LEFT, wraptext_line(wraptext, i));
			wraptext_free(wraptext);
		}
		if (surface != NULL)
			al_set_target_backbuffer(screen_display(g_screen));
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
	duk_push_c_function(ctx, js_Font_wordWrap, DUK_VARARGS);
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
js_Font_wordWrap(duk_context* ctx)
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
js_new_Group(duk_context* ctx)
{
	group_t*  group;
	int       num_args;
	size_t    num_shapes;
	shader_t* shader;
	shape_t*  shape;

	duk_uarridx_t i;

	num_args = duk_get_top(ctx);
	duk_require_object_coercible(ctx, 0);
	shader = num_args >= 2
		? duk_require_sphere_obj(ctx, 1, "ShaderProgram")
		: get_default_shader();

	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "argument 1 to Group() must be an array");
	if (!(group = group_new(shader)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create Galileo group");
	num_shapes = duk_get_length(ctx, 0);
	for (i = 0; i < num_shapes; ++i) {
		duk_get_prop_index(ctx, 0, i);
		shape = duk_require_sphere_obj(ctx, -1, "Shape");
		group_add_shape(group, shape);
	}
	duk_push_sphere_obj(ctx, "Group", group);
	return 1;
}

static duk_ret_t
js_Group_finalize(duk_context* ctx)
{
	group_t* group;

	group = duk_require_sphere_obj(ctx, 0, "Group");
	group_free(group);
	return 0;
}

static duk_ret_t
js_Group_get_shader(duk_context* ctx)
{
	group_t*  group;
	shader_t* shader;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");

	shader = group_get_shader(group);
	duk_push_sphere_obj(ctx, "ShaderProgram", shader_ref(shader));
	return 1;
}

static duk_ret_t
js_Group_get_transform(duk_context* ctx)
{
	group_t*  group;
	matrix_t* matrix;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");

	matrix = group_get_transform(group);
	duk_push_sphere_obj(ctx, "Transform", matrix_ref(matrix));
	return 1;
}

static duk_ret_t
js_Group_set_shader(duk_context* ctx)
{
	group_t*  group;
	shader_t* shader;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	shader = duk_require_sphere_obj(ctx, 0, "ShaderProgram");

	group_set_shader(group, shader);
	return 0;
}

static duk_ret_t
js_Group_set_transform(duk_context* ctx)
{
	group_t*  group;
	matrix_t* transform;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	transform = duk_require_sphere_obj(ctx, 0, "Transform");

	group_set_transform(group, transform);
	return 0;
}

static duk_ret_t
js_Group_draw(duk_context* ctx)
{
	group_t* group;
	int      num_args;
	image_t* surface;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	surface = num_args >= 1 ? duk_require_sphere_obj(ctx, 0, "Surface")
		: NULL;

	if (!screen_is_skipframe(g_screen))
		group_draw(group, surface);
	return 0;
}

static duk_ret_t
js_Group_setFloat(duk_context* ctx)
{
	group_t*    group;
	const char* name;
	float       value;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	name = duk_require_string(ctx, 0);
	value = duk_require_number(ctx, 1);

	group_put_float(group, name, value);
	return 1;
}

static duk_ret_t
js_Group_setInt(duk_context* ctx)
{
	group_t*    group;
	const char* name;
	int         value;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	name = duk_require_string(ctx, 0);
	value = duk_require_int(ctx, 1);

	group_put_int(group, name, value);
	return 1;
}

static duk_ret_t
js_Group_setMatrix(duk_context* ctx)
{
	group_t*    group;
	matrix_t*   matrix;
	const char* name;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	name = duk_require_string(ctx, 0);
	matrix = duk_require_sphere_obj(ctx, 1, "Transform");

	group_put_matrix(group, name, matrix);
	return 1;
}

static duk_ret_t
js_new_Image(duk_context* ctx)
{
	const color_t* buffer;
	size_t         buffer_size;
	const char*    filename;
	color_t        fill_color;
	int            height;
	image_t*       image;
	image_lock_t*  lock;
	int            num_args;
	color_t*       p_line;
	image_t*       src_image;
	int            width;

	int y;

	num_args = duk_get_top(ctx);
	if (num_args >= 3 && duk_is_sphere_obj(ctx, 2, "Color")) {
		// create an Image filled with a single pixel value
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		fill_color = duk_require_sphere_color(ctx, 2);
		if (!(image = image_new(width, height)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image(): unable to create new image");
		image_fill(image, fill_color);
	}
	else if (num_args >= 3 && (buffer = duk_get_buffer_data(ctx, 2, &buffer_size))) {
		// create an Image from an ArrayBuffer or similar object
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		if (buffer_size < width * height * sizeof(color_t))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "buffer is too small to describe a %dx%d image", width, height);
		if (!(image = image_new(width, height)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create image");
		if (!(lock = image_lock(image))) {
			image_free(image);
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to lock pixels for writing");
		}
		p_line = lock->pixels;
		for (y = 0; y < height; ++y) {
			memcpy(p_line, buffer + y * width, width * sizeof(color_t));
			p_line += lock->pitch;
		}
		image_unlock(image, lock);
	}
	else if (duk_is_sphere_obj(ctx, 0, "Surface")) {
		// create an Image from a Surface
		src_image = duk_require_sphere_obj(ctx, 0, "Surface");
		if (!(image = image_clone(src_image)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image(): unable to create image from surface");
	}
	else {
		// create an Image by loading an image file
		filename = duk_require_path(ctx, 0, NULL, false);
		image = image_load(filename);
		if (image == NULL)
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Image(): unable to load image file `%s`", filename);
	}
	duk_push_sphere_obj(ctx, "Image", image);
	return 1;
}

static duk_ret_t
js_Image_finalize(duk_context* ctx)
{
	image_t* image;

	image = duk_require_sphere_obj(ctx, 0, "Image");
	image_free(image);
	return 0;
}

static duk_ret_t
js_Image_get_height(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Image");

	duk_push_int(ctx, image_height(image));
	return 1;
}

static duk_ret_t
js_Image_get_width(duk_context* ctx)
{
	image_t* image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Image");

	duk_push_int(ctx, image_width(image));
	return 1;
}

static duk_ret_t
js_Mixer_get_Default(duk_context* ctx)
{
	duk_push_sphere_obj(ctx, "Mixer", mixer_ref(s_def_mixer));

	duk_push_this(ctx);
	duk_push_string(ctx, "Default");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_new_Mixer(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int freq = duk_require_int(ctx, 0);
	int bits = duk_require_int(ctx, 1);
	int channels = n_args >= 3 ? duk_require_int(ctx, 2) : 2;

	mixer_t* mixer;

	if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Mixer(): invalid bit depth for mixer (%i)", bits);
	if (channels < 1 || channels > 7)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Mixer(): invalid channel count for mixer (%i)", channels);
	if (!(mixer = mixer_new(freq, bits, channels)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Mixer(): unable to create %i-bit %ich voice", bits, channels);
	duk_push_sphere_obj(ctx, "Mixer", mixer);
	return 1;
}

static duk_ret_t
js_Mixer_finalize(duk_context* ctx)
{
	mixer_t* mixer;

	mixer = duk_require_sphere_obj(ctx, 0, "Mixer");
	mixer_free(mixer);
	return 0;
}

static duk_ret_t
js_Mixer_get_volume(duk_context* ctx)
{
	mixer_t* mixer;

	duk_push_this(ctx);
	mixer = duk_require_sphere_obj(ctx, -1, "Mixer");

	duk_push_number(ctx, mixer_get_gain(mixer));
	return 1;
}

static duk_ret_t
js_Mixer_set_volume(duk_context* ctx)
{
	float volume = duk_require_number(ctx, 0);

	mixer_t* mixer;

	duk_push_this(ctx);
	mixer = duk_require_sphere_obj(ctx, -1, "Mixer");

	mixer_set_gain(mixer, volume);
	return 0;
}

static duk_ret_t
js_new_Server(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int port = duk_require_int(ctx, 0);
	int max_backlog = n_args >= 2 ? duk_require_int(ctx, 1) : 16;

	socket_t* socket;

	if (max_backlog <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "max_backlog cannot be <= 0", max_backlog);
	if (socket = listen_on_port(NULL, port, 1024, max_backlog))
		duk_push_sphere_obj(ctx, "Server", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Server_finalize(duk_context* ctx)
{
	socket_t*   socket;

	socket = duk_require_sphere_obj(ctx, 0, "Server");
	free_socket(socket);
	return 0;
}

static duk_ret_t
js_Server_accept(duk_context* ctx)
{
	socket_t* new_socket;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Server");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Server:accept(): socket has been closed");
	new_socket = accept_next_socket(socket);
	if (new_socket)
		duk_push_sphere_obj(ctx, "Socket", new_socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Server_close(duk_context* ctx)
{
	socket_t*   socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Server");
	duk_push_null(ctx); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	if (socket != NULL)
		free_socket(socket);
	return 0;
}

static duk_ret_t
js_ShaderProgram_get_Default(duk_context* ctx)
{
	shader_t* shader;

	if (!(shader = get_default_shader()))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to build default shader program");
	duk_push_sphere_obj(ctx, "ShaderProgram", shader_ref(shader));

	duk_push_this(ctx);
	duk_push_string(ctx, "Default");
	duk_dup(ctx, -3);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_CLEAR_WRITABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(ctx);

	return 1;
}

static duk_ret_t
js_new_ShaderProgram(duk_context* ctx)
{
	const char* fs_filename;
	const char* vs_filename;
	shader_t*   shader;

	if (!duk_is_object(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "ShaderProgram(): JS object expected as argument");
	if (duk_get_prop_string(ctx, 0, "vertex"), !duk_is_string(ctx, -1))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "ShaderProgram(): 'vertex' property, string required");
	if (duk_get_prop_string(ctx, 0, "fragment"), !duk_is_string(ctx, -1))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "ShaderProgram(): 'fragment' property, string required");
	duk_pop_2(ctx);

	if (!are_shaders_active())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ShaderProgram(): shaders not supported on this system");

	duk_get_prop_string(ctx, 0, "vertex");
	duk_get_prop_string(ctx, 0, "fragment");
	vs_filename = duk_require_path(ctx, -2, NULL, false);
	fs_filename = duk_require_path(ctx, -1, NULL, false);
	duk_pop_2(ctx);
	if (!(shader = shader_new(vs_filename, fs_filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ShaderProgram(): failed to build shader from `%s`, `%s`", vs_filename, fs_filename);
	duk_push_sphere_obj(ctx, "ShaderProgram", shader);
	return 1;
}

static duk_ret_t
js_ShaderProgram_finalize(duk_context* ctx)
{
	shader_t* shader = duk_require_sphere_obj(ctx, 0, "ShaderProgram");

	shader_free(shader);
	return 0;
}

static duk_ret_t
js_new_Shape(duk_context* ctx)
{
	bool         is_missing_uv = false;
	int          num_args;
	size_t       num_vertices;
	shape_t*     shape;
	duk_idx_t    stack_idx;
	image_t*     texture;
	shape_type_t type;
	vertex_t     vertex;

	duk_uarridx_t i;

	num_args = duk_get_top(ctx);
	duk_require_object_coercible(ctx, 0);
	texture = !duk_is_null(ctx, 1) ? duk_require_sphere_obj(ctx, 1, "Image") : NULL;
	type = num_args >= 3 ? duk_require_int(ctx, 2) : SHAPE_AUTO;

	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "Shape(): first argument must be an array");
	if (type < 0 || type >= SHAPE_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Shape(): invalid shape type constant");
	if (!(shape = shape_new(type, texture)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Shape(): unable to create shape object");
	num_vertices = duk_get_length(ctx, 0);
	for (i = 0; i < num_vertices; ++i) {
		duk_get_prop_index(ctx, 0, i);
		stack_idx = duk_normalize_index(ctx, -1);
		vertex.x = duk_get_prop_string(ctx, stack_idx, "x") ? duk_require_number(ctx, -1) : 0.0;
		vertex.y = duk_get_prop_string(ctx, stack_idx, "y") ? duk_require_number(ctx, -1) : 0.0;
		vertex.z = duk_get_prop_string(ctx, stack_idx, "z") ? duk_require_number(ctx, -1) : 0.0;
		if (duk_get_prop_string(ctx, stack_idx, "u"))
			vertex.u = duk_require_number(ctx, -1);
		else
			is_missing_uv = true;
		if (duk_get_prop_string(ctx, stack_idx, "v"))
			vertex.v = duk_require_number(ctx, -1);
		else
			is_missing_uv = true;
		vertex.color = duk_get_prop_string(ctx, stack_idx, "color")
			? duk_require_sphere_color(ctx, -1)
			: color_new(255, 255, 255, 255);
		duk_pop_n(ctx, 6);
		shape_add_vertex(shape, vertex);
	}
	if (is_missing_uv)
		shape_calculate_uv(shape);
	shape_upload(shape);
	duk_push_sphere_obj(ctx, "Shape", shape);
	return 1;
}

static duk_ret_t
js_Shape_finalize(duk_context* ctx)
{
	shape_t* shape;

	shape = duk_require_sphere_obj(ctx, 0, "Shape");
	shape_free(shape);
	return 0;
}

static duk_ret_t
js_Shape_get_texture(duk_context* ctx)
{
	shape_t* shape;

	duk_push_this(ctx);
	shape = duk_require_sphere_obj(ctx, -1, "Shape");

	duk_push_sphere_obj(ctx, "Image", image_ref(shape_texture(shape)));
	return 1;
}

static duk_ret_t
js_Shape_set_texture(duk_context* ctx)
{
	shape_t* shape;
	image_t* texture;

	duk_push_this(ctx);
	shape = duk_require_sphere_obj(ctx, -1, "Shape");
	texture = duk_require_sphere_obj(ctx, 0, "Image");

	shape_set_texture(shape, texture);
	return 0;
}

static duk_ret_t
js_Shape_draw(duk_context* ctx)
{
	int       num_args;
	shape_t*  shape;
	image_t*  surface = NULL;
	matrix_t* transform = NULL;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	shape = duk_require_sphere_obj(ctx, -1, "Shape");
	if (num_args >= 1)
		surface = duk_require_sphere_obj(ctx, 0, "Surface");
	if (num_args >= 2)
		transform = duk_require_sphere_obj(ctx, 1, "Transform");

	shader_use(get_default_shader());
	shape_draw(shape, transform, surface);
	shader_use(NULL);
	return 0;
}

static duk_ret_t
js_new_Socket(duk_context* ctx)
{
	const char* hostname = duk_require_string(ctx, 0);
	int port = duk_require_int(ctx, 1);

	socket_t*   socket;

	if ((socket = connect_to_host(hostname, port, 1024)) != NULL)
		duk_push_sphere_obj(ctx, "Socket", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Socket_finalize(duk_context* ctx)
{
	socket_t* socket;

	socket = duk_require_sphere_obj(ctx, 0, "Socket");
	free_socket(socket);
	return 0;
}

static duk_ret_t
js_Socket_get_bytesPending(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:bytesPending: Socket has been closed");
	duk_push_uint(ctx, (duk_uint_t)get_socket_read_size(socket));
	return 1;
}

static duk_ret_t
js_Socket_get_connected(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");

	if (socket != NULL)
		duk_push_boolean(ctx, is_socket_live(socket));
	else
		duk_push_false(ctx);
	return 1;
}

static duk_ret_t
js_Socket_get_remoteAddress(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");

	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:remoteAddress - Socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:remoteAddress - Socket is not connected");
	duk_push_string(ctx, get_socket_host(socket));
	return 1;
}

static duk_ret_t
js_Socket_get_remotePort(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:remotePort - Socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:remotePort - Socket is not connected");
	duk_push_int(ctx, get_socket_port(socket));
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
	return 0;
}

static duk_ret_t
js_Socket_read(duk_context* ctx)
{
	void*      buffer;
	size_t     bytes_read;
	duk_size_t num_bytes;
	socket_t*  socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	num_bytes = duk_require_uint(ctx, 0);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "socket is closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "socket is disconnected");
	buffer = duk_push_fixed_buffer(ctx, num_bytes);
	bytes_read = read_socket(socket, buffer, num_bytes);
	duk_push_buffer_object(ctx, -1, 0, bytes_read, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_Socket_write(duk_context* ctx)
{
	const uint8_t* payload;
	socket_t*      socket;
	duk_size_t     write_size;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	payload = duk_require_buffer_data(ctx, 0, &write_size);

	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "socket is closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "socket is disconnected");
	write_socket(socket, payload, write_size);
	return 0;
}

static duk_ret_t
js_new_Sound(duk_context* ctx)
{
	const char* filename;
	duk_int_t   num_args;
	sound_t*    sound;

	num_args = duk_get_top(ctx);
	filename = duk_require_path(ctx, 0, NULL, false);

	if (!(sound = sound_new(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to load sound `%s`", filename);
	duk_push_sphere_obj(ctx, "Sound", sound);
	return 1;
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
js_Sound_get_length(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	duk_push_number(ctx, sound_len(sound));
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
js_Sound_get_speed(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	duk_push_number(ctx, sound_speed(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_speed(duk_context* ctx)
{
	float new_speed = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");

	sound_set_speed(sound, new_speed);
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

	duk_push_number(ctx, sound_tell(sound));
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

	sound_seek(sound, new_pos);
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

	if (num_args < 1)
		sound_pause(sound, false);
	else {
		mixer = duk_require_sphere_obj(ctx, 0, "Mixer");
		sound_play(sound, mixer);
	}
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
js_new_SoundStream(duk_context* ctx)
{
	// new SoundStream(frequency[, bits[, channels]]);
	// Arguments:
	//     frequency: Audio frequency in Hz. (default: 22050)
	//     bits:      Bit depth. (default: 8)
	//     channels:  Number of independent channels. (default: 1)

	stream_t* stream;
	int       argc;
	int       frequency;
	int       bits;
	int       channels;

	argc = duk_get_top(ctx);
	frequency = argc >= 1 ? duk_require_int(ctx, 0) : 22050;
	bits = argc >= 2 ? duk_require_int(ctx, 1) : 8;
	channels = argc >= 3 ? duk_require_int(ctx, 1) : 1;
	if (bits != 8 && bits != 16 && bits != 24 && bits != 32)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SoundStream(): invalid bit depth (%i)", bits);
	if (!(stream = stream_new(frequency, bits, channels)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SoundStream(): stream creation failed");
	duk_push_sphere_obj(ctx, "SoundStream", stream);
	return 1;
}

static duk_ret_t
js_SoundStream_finalize(duk_context* ctx)
{
	stream_t* stream;

	stream = duk_require_sphere_obj(ctx, 0, "SoundStream");
	stream_free(stream);
	return 0;
}

static duk_ret_t
js_SoundStream_get_bufferSize(duk_context* ctx)
{
	stream_t*    stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");

	duk_push_number(ctx, stream_bytes_left(stream));
	return 1;
}

static duk_ret_t
js_SoundStream_buffer(duk_context* ctx)
{
	// SoundStream:buffer(data);
	// Arguments:
	//     data: An ArrayBuffer or TypedArray containing the audio data
	//           to feed into the stream buffer.

	const void* data;
	duk_size_t  size;
	stream_t*   stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");

	data = duk_require_buffer_data(ctx, 0, &size);
	stream_buffer(stream, data, size);
	return 0;
}

static duk_ret_t
js_SoundStream_pause(duk_context* ctx)
{
	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");

	stream_pause(stream, true);
	return 0;
}

static duk_ret_t
js_SoundStream_play(duk_context* ctx)
{
	mixer_t*  mixer;
	int       num_args;
	stream_t* stream;

	num_args = duk_get_top(ctx);
	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");

	if (num_args < 1)
		stream_pause(stream, false);
	else {
		mixer = duk_require_sphere_obj(ctx, 0, "Mixer");
		stream_play(stream, mixer);
	}
	return 0;
}

static duk_ret_t
js_SoundStream_stop(duk_context* ctx)
{
	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");

	stream_stop(stream);
	return 0;
}

static duk_ret_t
js_new_Surface(duk_context* ctx)
{
	int         n_args;
	const char* filename;
	color_t     fill_color;
	image_t*    image;
	image_t*    src_image;
	int         width, height;

	n_args = duk_get_top(ctx);
	if (n_args >= 2) {
		width = duk_require_int(ctx, 0);
		height = duk_require_int(ctx, 1);
		fill_color = n_args >= 3 ? duk_require_sphere_color(ctx, 2) : color_new(0, 0, 0, 0);
		if (!(image = image_new(width, height)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create surface");
		image_fill(image, fill_color);
	}
	else if (duk_is_sphere_obj(ctx, 0, "Image")) {
		src_image = duk_require_sphere_obj(ctx, 0, "Image");
		if (!(image = image_clone(src_image)))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create surface from image");
	}
	else {
		filename = duk_require_path(ctx, 0, NULL, false);
		image = image_load(filename);
		if (image == NULL)
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to load image `%s`", filename);
	}
	duk_push_sphere_obj(ctx, "Surface", image);
	return 1;
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
js_Surface_toImage(duk_context* ctx)
{
	image_t* image;
	image_t* new_image;

	duk_push_this(ctx);
	image = duk_require_sphere_obj(ctx, -1, "Surface");

	if ((new_image = image_clone(image)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create image");
	duk_push_sphere_obj(ctx, "Image", new_image);
	return 1;
}

static duk_ret_t
js_new_Transform(duk_context* ctx)
{
	matrix_t* matrix;

	matrix = matrix_new();
	duk_push_sphere_obj(ctx, "Transform", matrix);
	return 1;
}

static duk_ret_t
js_Transform_finalize(duk_context* ctx)
{
	matrix_t* matrix;

	matrix = duk_require_sphere_obj(ctx, 0, "Transform");

	matrix_free(matrix);
	return 0;
}

static duk_ret_t
js_Transform_compose(duk_context* ctx)
{
	matrix_t* matrix;
	matrix_t* other;

	duk_push_this(ctx);
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	other = duk_require_sphere_obj(ctx, 0, "Transform");

	matrix_compose(matrix, other);
	return 1;
}

static duk_ret_t
js_Transform_identity(duk_context* ctx)
{
	matrix_t* matrix;

	duk_push_this(ctx);
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");

	matrix_identity(matrix);
	return 1;
}

static duk_ret_t
js_Transform_rotate(duk_context* ctx)
{
	matrix_t* matrix;
	int       num_args;
	float     theta;
	float     vx = 0.0;
	float     vy = 0.0;
	float     vz = 1.0;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	theta = duk_require_number(ctx, 0);
	if (num_args >= 2) {
		vx = duk_require_number(ctx, 1);
		vy = duk_require_number(ctx, 2);
		vz = duk_require_number(ctx, 3);
	}

	matrix_rotate(matrix, theta, vx, vy, vz);
	return 1;
}

static duk_ret_t
js_Transform_scale(duk_context* ctx)
{
	matrix_t* matrix;
	int       num_args;
	float     sx;
	float     sy;
	float     sz = 1.0;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	sx = duk_require_number(ctx, 0);
	sy = duk_require_number(ctx, 1);
	if (num_args >= 3)
		sz = duk_require_number(ctx, 2);

	matrix_scale(matrix, sx, sy, sz);
	return 1;
}

static duk_ret_t
js_Transform_translate(duk_context* ctx)
{
	matrix_t* matrix;
	int       num_args;
	float     dx;
	float     dy;
	float     dz = 0.0;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	dx = duk_require_number(ctx, 0);
	dy = duk_require_number(ctx, 1);
	if (num_args >= 3)
		dz = duk_require_number(ctx, 2);

	matrix_translate(matrix, dx, dy, dz);
	return 1;
}
