#include "minisphere.h"
#include "api.h"

#include "sound.h"

#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

static duk_ret_t js_LoadSound            (duk_context* ctx);
static duk_ret_t js_new_Sound            (duk_context* ctx);
static duk_ret_t js_Sound_finalize       (duk_context* ctx);
static duk_ret_t js_Sound_toString       (duk_context* ctx);
static duk_ret_t js_Sound_get_isPlaying  (duk_context* ctx);
static duk_ret_t js_Sound_get_isSeekable (duk_context* ctx);
static duk_ret_t js_Sound_get_Length     (duk_context* ctx);
static duk_ret_t js_Sound_get_Pan        (duk_context* ctx);
static duk_ret_t js_Sound_get_Pitch      (duk_context* ctx);
static duk_ret_t js_Sound_get_Position   (duk_context* ctx);
static duk_ret_t js_Sound_get_Repeat     (duk_context* ctx);
static duk_ret_t js_Sound_get_Volume     (duk_context* ctx);
static duk_ret_t js_Sound_set_Pan        (duk_context* ctx);
static duk_ret_t js_Sound_set_Pitch      (duk_context* ctx);
static duk_ret_t js_Sound_set_Position   (duk_context* ctx);
static duk_ret_t js_Sound_set_Repeat     (duk_context* ctx);
static duk_ret_t js_Sound_set_Volume     (duk_context* ctx);
static duk_ret_t js_Sound_clone          (duk_context* ctx);
static duk_ret_t js_Sound_pause          (duk_context* ctx);
static duk_ret_t js_Sound_play           (duk_context* ctx);
static duk_ret_t js_Sound_reset          (duk_context* ctx);
static duk_ret_t js_Sound_stop           (duk_context* ctx);

struct sound
{
	int                   refcount;
	char*                 path;
	ALLEGRO_AUDIO_STREAM* stream;
};

void
initialize_sound(void)
{
	al_install_audio();
	al_init_acodec_addon();
	al_reserve_samples(10);
	al_set_mixer_gain(al_get_default_mixer(), 1.0);
}

void
shutdown_sound(void)
{
	al_uninstall_audio();
}

void
update_sounds(void)
{
}

sound_t*
load_sound(const char* path, bool streaming)
{
	sound_t* sound;

	if (!(sound = calloc(1, sizeof(sound_t)))) goto on_error;
	if (!(sound->path = strdup(path))) goto on_error;
	if (!reload_sound(sound))
		goto on_error;
	return ref_sound(sound);

on_error:
	if (sound != NULL) {
		free(sound->path);
		free(sound);
	}
	return NULL;
}

sound_t*
ref_sound(sound_t* sound)
{
	++sound->refcount;
	return sound;
}

void
free_sound(sound_t* sound)
{
	if (sound == NULL || --sound->refcount > 0)
		return;
	al_destroy_audio_stream(sound->stream);
	free(sound->path);
}

bool
is_sound_looping(sound_t* sound)
{
	return al_get_audio_stream_playmode(sound->stream) == ALLEGRO_PLAYMODE_LOOP;
}

bool
is_sound_playing(sound_t* sound)
{
	return al_get_audio_stream_playing(sound->stream);
}

float
get_sound_gain(sound_t* sound)
{
	return al_get_audio_stream_gain(sound->stream);
}

int
get_sound_length(sound_t* sound)
{
	return al_get_audio_stream_length_secs(sound->stream) * 1000;
}

float
get_sound_pan(sound_t* sound)
{
	return al_get_audio_stream_pan(sound->stream);
}

float
get_sound_pitch(sound_t* sound)
{
	return al_get_audio_stream_speed(sound->stream);
}

int
get_sound_seek(sound_t* sound)
{
	return al_get_audio_stream_position_secs(sound->stream) * 1000;
}

void
set_sound_gain(sound_t* sound, float gain)
{
	al_set_audio_stream_gain(sound->stream, gain);
}

void
set_sound_looping(sound_t* sound, bool is_looping)
{
	al_set_audio_stream_playmode(sound->stream,
		is_looping ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE);
}

void
set_sound_pan(sound_t* sound, float pan)
{
	al_set_audio_stream_pan(sound->stream, pan);
}

void
set_sound_pitch(sound_t* sound, float pitch)
{
	al_set_audio_stream_speed(sound->stream, pitch);
}

void
play_sound(sound_t* sound)
{
	al_set_audio_stream_playing(sound->stream, true);
}

bool
reload_sound(sound_t* sound)
{
	ALLEGRO_AUDIO_STREAM* new_stream;

	if (!(new_stream = al_load_audio_stream(sound->path, 4, 1024)))
		return false;
	if (sound->stream != NULL) {
		al_destroy_audio_stream(sound->stream);
	}
	sound->stream = new_stream;
	al_set_audio_stream_gain(sound->stream, 1.0);
	al_attach_audio_stream_to_mixer(sound->stream, al_get_default_mixer());
	al_set_audio_stream_playing(sound->stream, false);
	return true;
}

void
seek_sound(sound_t* sound, int position)
{
	al_seek_audio_stream_secs(sound->stream, (double)position / 1000);
}

void
stop_sound(sound_t* sound, bool rewind)
{
	al_set_audio_stream_playing(sound->stream, false);
	if (rewind)
		al_rewind_audio_stream(sound->stream);
}

void
init_sound_api()
{
	// register Sound object constructor and methods
	register_api_func(g_duktape, NULL, "LoadSound", js_LoadSound);
	register_api_func(g_duktape, NULL, "Sound", js_new_Sound);
	register_api_func(g_duktape, "Sound", "toString", js_Sound_toString);
	register_api_func(g_duktape, "Sound", "isPlaying", js_Sound_get_isPlaying);
	register_api_func(g_duktape, "Sound", "isSeekable", js_Sound_get_isSeekable);
	register_api_func(g_duktape, "Sound", "getLength", js_Sound_get_Length);
	register_api_func(g_duktape, "Sound", "getPan", js_Sound_get_Pan);
	register_api_func(g_duktape, "Sound", "getPitch", js_Sound_get_Pitch);
	register_api_func(g_duktape, "Sound", "getPosition", js_Sound_get_Position);
	register_api_func(g_duktape, "Sound", "getRepeat", js_Sound_get_Repeat);
	register_api_func(g_duktape, "Sound", "getVolume", js_Sound_get_Volume);
	register_api_func(g_duktape, "Sound", "setPan", js_Sound_set_Pan);
	register_api_func(g_duktape, "Sound", "setPitch", js_Sound_set_Pitch);
	register_api_func(g_duktape, "Sound", "setPosition", js_Sound_set_Position);
	register_api_func(g_duktape, "Sound", "setRepeat", js_Sound_set_Repeat);
	register_api_func(g_duktape, "Sound", "setVolume", js_Sound_set_Volume);
	register_api_func(g_duktape, "Sound", "pause", js_Sound_pause);
	register_api_func(g_duktape, "Sound", "play", js_Sound_play);
	register_api_func(g_duktape, "Sound", "reset", js_Sound_reset);
	register_api_func(g_duktape, "Sound", "stop", js_Sound_stop);
	
	// register Sound object properties
	register_api_prop(g_duktape, "Sound", "isPlaying", js_Sound_get_isPlaying, NULL);
	register_api_prop(g_duktape, "Sound", "isSeekable", js_Sound_get_isSeekable, NULL);
	register_api_prop(g_duktape, "Sound", "length", js_Sound_get_Length, NULL);
	register_api_prop(g_duktape, "Sound", "pan", js_Sound_get_Pan, js_Sound_set_Pan);
	register_api_prop(g_duktape, "Sound", "pitch", js_Sound_get_Pitch, js_Sound_set_Pitch);
	register_api_prop(g_duktape, "Sound", "position", js_Sound_get_Position, js_Sound_set_Position);
	register_api_prop(g_duktape, "Sound", "repeat", js_Sound_get_Repeat, js_Sound_set_Repeat);
	register_api_prop(g_duktape, "Sound", "volume", js_Sound_get_Volume, js_Sound_set_Volume);
}

static duk_ret_t
js_LoadSound(duk_context* ctx)
{
	duk_int_t n_args = duk_get_top(ctx);
	const char* filename = duk_require_string(ctx, 0);
	duk_bool_t is_stream = n_args >= 2 ? duk_require_boolean(ctx, 1) : true;

	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Sound");
	duk_push_string(ctx, filename);
	duk_push_boolean(ctx, is_stream);
	duk_new(ctx, 2);
	return 1;
}

static duk_ret_t
js_new_Sound(duk_context* ctx)
{
	duk_int_t n_args = duk_get_top(ctx);
	const char* filename = duk_require_string(ctx, 0);
	duk_bool_t is_stream = n_args >= 2 ? duk_require_boolean(ctx, 1) : true;

	sound_t* sound;
	char*    sound_path;

	if (!duk_is_constructor_call(ctx))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "Sound(): constructor must be used with 'new'");
	
	sound_path = get_asset_path(filename, "sounds", false);
	sound = load_sound(sound_path, is_stream);
	free(sound_path);
	if (sound == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Sound(): failed to load sound file '%s'", filename);
	
	duk_push_this(ctx);
	duk_push_pointer(ctx, sound); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_c_function(ctx, js_Sound_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	return 0;
}

static duk_ret_t
js_Sound_finalize(duk_context* ctx)
{
	sound_t* sound;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	free_sound(sound);
	return 0;
}

static duk_ret_t
js_Sound_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object sound]");
	return 1;
}

static duk_ret_t
js_Sound_get_isPlaying(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_boolean(ctx, is_sound_playing(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_isSeekable(duk_context* ctx)
{
	duk_push_true(ctx);
	return 1;
}

static duk_ret_t
js_Sound_get_Length(duk_context* ctx)
{
	sound_t* sound;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, get_sound_length(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_Pan(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, get_sound_pan(sound) * 255);
	return 1;
}

static duk_ret_t
js_Sound_get_Pitch(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_number(ctx, get_sound_pitch(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_Position(duk_context* ctx)
{
	sound_t* sound;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, get_sound_seek(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_Repeat(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_boolean(ctx, is_sound_looping(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_Volume(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, get_sound_gain(sound) * 255);
	return 1;
}

static duk_ret_t
js_Sound_set_Pan(duk_context* ctx)
{
	int new_pan = duk_require_int(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	set_sound_pan(sound, (float)new_pan / 255);
	return 0;
}

static duk_ret_t
js_Sound_set_Pitch(duk_context* ctx)
{
	float new_pitch = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	set_sound_pitch(sound, new_pitch);
	return 0;
}

static duk_ret_t
js_Sound_set_Position(duk_context* ctx)
{
	int new_pos = duk_require_int(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	seek_sound(sound, new_pos);
	return 0;
}

static duk_ret_t
js_Sound_set_Repeat(duk_context* ctx)
{
	bool is_looped = duk_require_boolean(ctx, 0);
	
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	set_sound_looping(sound, is_looped);
	return 0;
}

static duk_ret_t
js_Sound_set_Volume(duk_context* ctx)
{
	float new_gain = duk_require_int(ctx, 0);
	
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	set_sound_gain(sound, (float)new_gain / 255);
	return 0;
}

static duk_ret_t
js_Sound_pause(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	stop_sound(sound, false);
	return 0;
}

static duk_ret_t
js_Sound_play(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);

	bool     is_looping;
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (n_args >= 1) {
		reload_sound(sound);
		is_looping = duk_require_boolean(ctx, 0);
		set_sound_looping(sound, is_looping);
	}
	play_sound(sound);
	return 0;
}

static duk_ret_t
js_Sound_reset(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	seek_sound(sound, 0);
	play_sound(sound);
	return 0;
}

static duk_ret_t
js_Sound_stop(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); sound = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	stop_sound(sound, true);
	return 0;
}
