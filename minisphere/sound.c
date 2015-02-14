#include "minisphere.h"
#include "api.h"

static duk_ret_t _js_LoadSound(duk_context* ctx);
static duk_ret_t _js_Sound_finalize(duk_context* ctx);
static duk_ret_t _js_Sound_isPlaying(duk_context* ctx);
static duk_ret_t _js_Sound_isSeekable(duk_context* ctx);
static duk_ret_t _js_Sound_getLength(duk_context* ctx);
static duk_ret_t _js_Sound_getPan(duk_context* ctx);
static duk_ret_t _js_Sound_getPitch(duk_context* ctx);
static duk_ret_t _js_Sound_getPosition(duk_context* ctx);
static duk_ret_t _js_Sound_getRepeat(duk_context* ctx);
static duk_ret_t _js_Sound_getVolume(duk_context* ctx);
static duk_ret_t _js_Sound_setPan(duk_context* ctx);
static duk_ret_t _js_Sound_setPitch(duk_context* ctx);
static duk_ret_t _js_Sound_setPosition(duk_context* ctx);
static duk_ret_t _js_Sound_setRepeat(duk_context* ctx);
static duk_ret_t _js_Sound_setVolume(duk_context* ctx);
static duk_ret_t _js_Sound_clone(duk_context* ctx);
static duk_ret_t _js_Sound_pause(duk_context* ctx);
static duk_ret_t _js_Sound_play(duk_context* ctx);
static duk_ret_t _js_Sound_reset(duk_context* ctx);
static duk_ret_t _js_Sound_stop(duk_context* ctx);

static void _duk_push_sphere_Sound (duk_context* ctx, ALLEGRO_AUDIO_STREAM* stream);

void
init_sound_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "LoadSound", &_js_LoadSound);
}

static void
_duk_push_sphere_Sound(duk_context* ctx, ALLEGRO_AUDIO_STREAM* stream)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, stream); duk_put_prop_string(ctx, -2, "\xFF" "stream_ptr");
	duk_push_c_function(ctx, &_js_Sound_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &_js_Sound_isPlaying, DUK_VARARGS); duk_put_prop_string(ctx, -2, "isPlaying");
	duk_push_c_function(ctx, &_js_Sound_isSeekable, DUK_VARARGS); duk_put_prop_string(ctx, -2, "isSeekable");
	duk_push_c_function(ctx, &_js_Sound_getRepeat, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getRepeat");
	duk_push_c_function(ctx, &_js_Sound_getVolume, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getVolume");
	duk_push_c_function(ctx, &_js_Sound_setRepeat, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setRepeat");
	duk_push_c_function(ctx, &_js_Sound_setVolume, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setVolume");
	duk_push_c_function(ctx, &_js_Sound_pause, DUK_VARARGS); duk_put_prop_string(ctx, -2, "pause");
	duk_push_c_function(ctx, &_js_Sound_play, DUK_VARARGS); duk_put_prop_string(ctx, -2, "play");
	duk_push_c_function(ctx, &_js_Sound_reset, DUK_VARARGS); duk_put_prop_string(ctx, -2, "reset");
	duk_push_c_function(ctx, &_js_Sound_stop, DUK_VARARGS); duk_put_prop_string(ctx, -2, "stop");
}

static duk_ret_t
_js_LoadSound(duk_context* ctx)
{
	duk_int_t n_args = duk_get_top(ctx);
	const char* filename = duk_get_string(ctx, 0);
	char* sound_path = get_asset_path(filename, "sounds", false);
	duk_bool_t is_stream = n_args >= 2 ? duk_get_boolean(ctx, 1) : true;
	ALLEGRO_AUDIO_STREAM* stream = al_load_audio_stream(sound_path, 4, 2048);
	free(sound_path);
	if (stream != NULL) {
		al_set_audio_stream_playing(stream, false);
		al_attach_audio_stream_to_mixer(stream, al_get_default_mixer());
		al_set_audio_stream_gain(stream, 1.0);
		_duk_push_sphere_Sound(ctx, stream);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "LoadSound(): Unable to load sound file '%s'", filename);
	}
}

static duk_ret_t
_js_Sound_finalize(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_get_prop_string(ctx, 0, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_set_audio_stream_playing(stream, false);
	al_detach_audio_stream(stream);
	al_destroy_audio_stream(stream);
	return 0;
}

static duk_ret_t
_js_Sound_isPlaying(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_boolean(ctx, al_get_audio_stream_playing(stream));
	return 1;
}

static duk_ret_t
_js_Sound_isSeekable(duk_context* ctx)
{
	duk_push_true(ctx);
	return 1;
}

static duk_ret_t
_js_Sound_getRepeat(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_boolean(ctx, al_get_audio_stream_playmode(stream) == ALLEGRO_PLAYMODE_LOOP);
	return 1;
}

static duk_ret_t
_js_Sound_getVolume(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, (int)(255 * al_get_audio_stream_gain(stream)));
	return 1;
}

static duk_ret_t
_js_Sound_setRepeat(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_bool_t is_looped = duk_get_boolean(ctx, 0);
	ALLEGRO_PLAYMODE play_mode = is_looped ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE;
	al_set_audio_stream_playmode(stream, play_mode);
	return 0;
}

static duk_ret_t
_js_Sound_setVolume(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	float new_vol = duk_get_number(ctx, 0) / 255;
	al_set_audio_stream_gain(stream, new_vol);
	return 0;
}

static duk_ret_t
_js_Sound_pause(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	al_set_audio_stream_playing(stream, false);
	return 0;
}

static duk_ret_t
_js_Sound_play(duk_context* ctx)
{
	duk_idx_t n_args = duk_get_top(ctx);
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (n_args >= 1) {
		ALLEGRO_PLAYMODE play_mode = duk_get_boolean(ctx, 0)
			? ALLEGRO_PLAYMODE_LOOP
			: ALLEGRO_PLAYMODE_ONCE;
		al_seek_audio_stream_secs(stream, 0.0);
		al_set_audio_stream_playmode(stream, play_mode);
	}
	al_set_audio_stream_playing(stream, true);
	return 0;
}

static duk_ret_t
_js_Sound_reset(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	al_seek_audio_stream_secs(stream, 0.0);
	al_set_audio_stream_playing(stream, true);
	return 0;
}

static duk_ret_t
_js_Sound_stop(duk_context* ctx)
{
	ALLEGRO_AUDIO_STREAM* stream;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "stream_ptr"); stream = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	al_set_audio_stream_playing(stream, false);
	al_seek_audio_stream_secs(stream, 0.0);
	return 0;
}
