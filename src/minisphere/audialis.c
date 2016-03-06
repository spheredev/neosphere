#include "minisphere.h"
#include "api.h"
#include "bytearray.h"

#include "audialis.h"

static duk_ret_t js_GetDefaultMixer            (duk_context* ctx);
static duk_ret_t js_new_Mixer                  (duk_context* ctx);
static duk_ret_t js_Mixer_finalize             (duk_context* ctx);
static duk_ret_t js_Mixer_get_volume           (duk_context* ctx);
static duk_ret_t js_Mixer_set_volume           (duk_context* ctx);
static duk_ret_t js_LoadSound                  (duk_context* ctx);
static duk_ret_t js_new_Sound                  (duk_context* ctx);
static duk_ret_t js_Sound_finalize             (duk_context* ctx);
static duk_ret_t js_Sound_toString             (duk_context* ctx);
static duk_ret_t js_Sound_getVolume            (duk_context* ctx);
static duk_ret_t js_Sound_setVolume            (duk_context* ctx);
static duk_ret_t js_Sound_get_length           (duk_context* ctx);
static duk_ret_t js_Sound_get_mixer            (duk_context* ctx);
static duk_ret_t js_Sound_set_mixer            (duk_context* ctx);
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
static duk_ret_t js_Sound_clone                (duk_context* ctx);
static duk_ret_t js_Sound_pause                (duk_context* ctx);
static duk_ret_t js_Sound_play                 (duk_context* ctx);
static duk_ret_t js_Sound_reset                (duk_context* ctx);
static duk_ret_t js_Sound_stop                 (duk_context* ctx);
static duk_ret_t js_new_SoundStream            (duk_context* ctx);
static duk_ret_t js_SoundStream_finalize       (duk_context* ctx);
static duk_ret_t js_SoundStream_get_bufferSize (duk_context* ctx);
static duk_ret_t js_SoundStream_get_mixer      (duk_context* ctx);
static duk_ret_t js_SoundStream_set_mixer      (duk_context* ctx);
static duk_ret_t js_SoundStream_play           (duk_context* ctx);
static duk_ret_t js_SoundStream_pause          (duk_context* ctx);
static duk_ret_t js_SoundStream_stop           (duk_context* ctx);
static duk_ret_t js_SoundStream_write          (duk_context* ctx);

static void update_stream (stream_t* stream);

struct mixer
{
	unsigned int   refcount;
	unsigned int   id;
	ALLEGRO_MIXER* ptr;
	ALLEGRO_VOICE* voice;
	float          gain;
};

struct stream
{
	unsigned int          refcount;
	unsigned int          id;
	ALLEGRO_AUDIO_STREAM* ptr;
	unsigned char*        buffer;
	size_t                buffer_size;
	size_t                feed_size;
	size_t                fragment_size;
	mixer_t*              mixer;
};

struct sound
{
	unsigned int          refcount;
	unsigned int          id;
	void*                 file_data;
	size_t                file_size;
	float                 gain;
	bool                  is_looping;
	mixer_t*              mixer;
	char*                 path;
	float                 pan;
	float                 pitch;
	ALLEGRO_AUDIO_STREAM* stream;
};

static bool                 s_have_sound;
static ALLEGRO_AUDIO_DEPTH  s_bit_depth;
static ALLEGRO_CHANNEL_CONF s_channel_conf;
static mixer_t*             s_def_mixer;
static unsigned int         s_next_mixer_id = 0;
static unsigned int         s_next_sound_id = 0;
static unsigned int         s_next_stream_id = 0;
static vector_t*            s_streams;

void
initialize_audialis(void)
{
	console_log(1, "initializing Audialis");
	
	s_have_sound = true;
	if (!al_install_audio() || !(s_def_mixer = create_mixer(44100, 16, 2))) {
		s_have_sound = false;
		console_log(1, "  no audio is available");
		return;
	}
	al_init_acodec_addon();
	al_reserve_samples(10);
	s_streams = vector_new(sizeof(stream_t*));
}

void
shutdown_audialis(void)
{
	console_log(1, "shutting down Audialis");
	vector_free(s_streams);
	free_mixer(s_def_mixer);
	if (s_have_sound)
		al_uninstall_audio();
}

void
update_audialis(void)
{
	stream_t* *p_stream;
	
	iter_t iter;

	iter = vector_enum(s_streams);
	while (p_stream = vector_next(&iter))
		update_stream(*p_stream);
}

mixer_t*
get_default_mixer(void)
{
	return s_def_mixer;
}

mixer_t*
create_mixer(int frequency, int bits, int channels)
{
	ALLEGRO_CHANNEL_CONF conf;
	ALLEGRO_AUDIO_DEPTH  depth;
	mixer_t*             mixer;

	console_log(2, "creating new mixer #%u at %i kHz", s_next_mixer_id, frequency / 1000);
	console_log(3, "    format: %ich %i Hz, %i-bit", channels, frequency, bits);

	conf = channels == 2 ? ALLEGRO_CHANNEL_CONF_2
		: channels == 3 ? ALLEGRO_CHANNEL_CONF_3
		: channels == 4 ? ALLEGRO_CHANNEL_CONF_4
		: channels == 5 ? ALLEGRO_CHANNEL_CONF_5_1
		: channels == 6 ? ALLEGRO_CHANNEL_CONF_6_1
		: channels == 7 ? ALLEGRO_CHANNEL_CONF_7_1
		: ALLEGRO_CHANNEL_CONF_1;
	depth = bits == 16 ? ALLEGRO_AUDIO_DEPTH_INT16
		: bits == 24 ? ALLEGRO_AUDIO_DEPTH_INT24
		: bits == 32 ? ALLEGRO_AUDIO_DEPTH_FLOAT32
		: ALLEGRO_AUDIO_DEPTH_UINT8;
	
	mixer = calloc(1, sizeof(mixer_t));
	if (!(mixer->voice = al_create_voice(frequency, depth, conf)))
		goto on_error;
	if (!(mixer->ptr = al_create_mixer(frequency, ALLEGRO_AUDIO_DEPTH_FLOAT32, conf)))
		goto on_error;
	al_attach_mixer_to_voice(mixer->ptr, mixer->voice);
	al_set_mixer_gain(mixer->ptr, 1.0);
	al_set_voice_playing(mixer->voice, true);
	al_set_mixer_playing(mixer->ptr, true);

	mixer->gain = al_get_mixer_gain(mixer->ptr);
	mixer->id = s_next_mixer_id++;
	return ref_mixer(mixer);

on_error:
	console_log(2, "failed to create mixer #%u", s_next_mixer_id++);
	if (mixer->ptr != NULL)
		al_destroy_mixer(mixer->ptr);
	if (mixer->voice != NULL)
		al_destroy_voice(mixer->voice);
	free(mixer);
	return NULL;
}

mixer_t*
ref_mixer(mixer_t* mixer)
{
	++mixer->refcount;
	return mixer;
}

void
free_mixer(mixer_t* mixer)
{
	if (mixer == NULL || --mixer->refcount > 0)
		return;
	
	console_log(3, "disposing mixer #%u no longer in use", mixer->id);
	al_destroy_mixer(mixer->ptr);
	free(mixer);
}

float
get_mixer_gain(mixer_t* mixer)
{
	return mixer->gain;
}

void
set_mixer_gain(mixer_t* mixer, float gain)
{
	al_set_mixer_gain(mixer->ptr, gain);
	mixer->gain = gain;
}

sound_t*
load_sound(const char* path, mixer_t* mixer)
{
	sound_t* sound;

	console_log(2, "loading sound #%u as '%s'", s_next_sound_id, path);
	
	sound = calloc(1, sizeof(sound_t));
	sound->path = strdup(path);
	
	if (!(sound->file_data = sfs_fslurp(g_fs, sound->path, NULL, &sound->file_size)))
		goto on_error;
	sound->mixer = ref_mixer(mixer);
	sound->gain = 1.0;
	sound->pan = 0.0;
	sound->pitch = 1.0;
	if (!reload_sound(sound))
		goto on_error;
	sound->id = s_next_sound_id++;
	return ref_sound(sound);

on_error:
	console_log(2, "    failed to load sound #%u", s_next_sound_id);
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
	
	console_log(3, "disposing sound #%u no longer in use", sound->id);
	free(sound->file_data);
	if (sound->stream != NULL)
		al_destroy_audio_stream(sound->stream);
	free_mixer(sound->mixer);
	free(sound->path);
	free(sound);
}

bool
is_sound_looping(sound_t* sound)
{
	return sound->is_looping;
}

bool
is_sound_playing(sound_t* sound)
{
	if (sound->stream != NULL)
		return al_get_audio_stream_playing(sound->stream);
	else
		return false;
}

float
get_sound_gain(sound_t* sound)
{
	return sound->gain;
}

long long
get_sound_length(sound_t* sound)
{
	if (sound->stream != NULL)
		return al_get_audio_stream_length_secs(sound->stream) * 1000000;
	else
		return 0.0;
}

mixer_t*
get_sound_mixer(sound_t* sound)
{
	return sound->mixer;
}

float
get_sound_pan(sound_t* sound)
{
	return sound->pan;
}

float
get_sound_pitch(sound_t* sound)
{
	return sound->pitch;
}

long long
get_sound_seek(sound_t* sound)
{
	if (sound->stream != NULL)
		return al_get_audio_stream_position_secs(sound->stream) * 1000000;
	else
		return 0.0;
}

void
set_sound_gain(sound_t* sound, float gain)
{
	if (sound->stream != NULL)
		al_set_audio_stream_gain(sound->stream, gain);
	sound->gain = gain;
}

void
set_sound_looping(sound_t* sound, bool is_looping)
{
	int play_mode;

	play_mode = is_looping ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE;
	if (sound->stream != NULL)
		al_set_audio_stream_playmode(sound->stream, play_mode);
	sound->is_looping = is_looping;
}

void
set_sound_mixer(sound_t* sound, mixer_t* mixer)
{
	mixer_t* old_mixer;

	old_mixer = sound->mixer;
	sound->mixer = ref_mixer(mixer);
	al_attach_audio_stream_to_mixer(sound->stream, sound->mixer->ptr);
	free_mixer(old_mixer);
}

void
set_sound_pan(sound_t* sound, float pan)
{
	if (sound->stream != NULL)
		al_set_audio_stream_pan(sound->stream, pan);
	sound->pan = pan;
}

void
set_sound_pitch(sound_t* sound, float pitch)
{
	if (sound->stream != NULL)
		al_set_audio_stream_speed(sound->stream, pitch);
	sound->pitch = pitch;
}

void
play_sound(sound_t* sound)
{
	console_log(2, "playing sound #%u on mixer #%u", sound->id, sound->mixer->id);
	if (sound->stream != NULL)
		al_set_audio_stream_playing(sound->stream, true);
}

bool
reload_sound(sound_t* sound)
{
	ALLEGRO_FILE*         memfile;
	ALLEGRO_AUDIO_STREAM* new_stream = NULL;
	int                   play_mode;

	console_log(4, "reloading sound #%u", sound->id);

	new_stream = NULL;
	if (s_have_sound) {
		memfile = al_open_memfile(sound->file_data, sound->file_size, "rb");
		if (!(new_stream = al_load_audio_stream_f(memfile, strrchr(sound->path, '.'), 4, 1024)))
			goto on_error;
	}
	if (s_have_sound && new_stream == NULL)
		return false;
	if (sound->stream != NULL)
		al_destroy_audio_stream(sound->stream);
	if ((sound->stream = new_stream) != NULL) {
		play_mode = sound->is_looping ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE;
		al_set_audio_stream_gain(sound->stream, sound->gain);
		al_set_audio_stream_pan(sound->stream, sound->pan);
		al_set_audio_stream_speed(sound->stream, sound->pitch);
		al_set_audio_stream_playmode(sound->stream, play_mode);
		al_attach_audio_stream_to_mixer(sound->stream, sound->mixer->ptr);
		al_set_audio_stream_playing(sound->stream, false);
	}
	return true;

on_error:
	return false;
}

void
seek_sound(sound_t* sound, long long position)
{
	if (sound->stream != NULL)
		al_seek_audio_stream_secs(sound->stream, (double)position / 1000000);
}

void
stop_sound(sound_t* sound, bool rewind)
{
	console_log(3, "stopping sound #%u %s", sound->id, rewind ? "and rewinding" : "");
	if (sound->stream == NULL)
		return;
	al_set_audio_stream_playing(sound->stream, false);
	if (rewind)
		al_rewind_audio_stream(sound->stream);
}

stream_t*
create_stream(int frequency, int bits, int channels)
{
	ALLEGRO_CHANNEL_CONF conf;
	ALLEGRO_AUDIO_DEPTH  depth_flag;
	size_t               sample_size;
	stream_t*            stream;

	console_log(2, "creating new stream #%u at %i kHz", s_next_stream_id, frequency / 1000);
	console_log(3, "    format: %ich %i Hz, %i-bit", channels, frequency, bits);
	
	stream = calloc(1, sizeof(stream_t));
	
	// create the underlying Allegro stream
	depth_flag = bits == 8 ? ALLEGRO_AUDIO_DEPTH_UINT8
		: bits == 24 ? ALLEGRO_AUDIO_DEPTH_INT24
		: bits == 32 ? ALLEGRO_AUDIO_DEPTH_FLOAT32
		: ALLEGRO_AUDIO_DEPTH_INT16;
	conf = channels == 2 ? ALLEGRO_CHANNEL_CONF_2
		: channels == 3 ? ALLEGRO_CHANNEL_CONF_3
		: channels == 4 ? ALLEGRO_CHANNEL_CONF_4
		: channels == 5 ? ALLEGRO_CHANNEL_CONF_5_1
		: channels == 6 ? ALLEGRO_CHANNEL_CONF_6_1
		: channels == 7 ? ALLEGRO_CHANNEL_CONF_7_1
		: ALLEGRO_CHANNEL_CONF_1;
	if (!(stream->ptr = al_create_audio_stream(4, 1024, frequency, depth_flag, conf)))
		goto on_error;
	stream->mixer = ref_mixer(s_def_mixer);
	al_set_audio_stream_playing(stream->ptr, false);
	al_attach_audio_stream_to_mixer(stream->ptr, stream->mixer->ptr);

	// allocate an initial stream buffer
	sample_size = bits == 8 ? 1
		: bits == 16 ? 2
		: bits == 24 ? 3
		: bits == 32 ? 4
		: 0;
	stream->fragment_size = 1024 * sample_size;
	stream->buffer_size = frequency * sample_size;  // 1 second
	stream->buffer = malloc(stream->buffer_size);
	
	stream->id = s_next_stream_id++;
	vector_push(s_streams, &stream);
	return ref_stream(stream);

on_error:
	console_log(2, "failed to create stream #%u", s_next_stream_id);
	free(stream);
	return NULL;
}

stream_t*
ref_stream(stream_t* stream)
{
	++stream->refcount;
	return stream;
}

void
free_stream(stream_t* stream)
{
	stream_t* *p_stream;
	
	iter_t iter;
	
	if (stream == NULL || --stream->refcount > 0)
		return;
	
	console_log(3, "disposing stream #%u no longer in use", stream->id);
	al_drain_audio_stream(stream->ptr);
	al_destroy_audio_stream(stream->ptr);
	free_mixer(stream->mixer);
	free(stream->buffer);
	free(stream);
	iter = vector_enum(s_streams);
	while (p_stream = vector_next(&iter)) {
		if (*p_stream == stream) {
			vector_remove(s_streams, iter.index);
			break;
		}
	}
}

mixer_t*
get_stream_mixer(stream_t* stream)
{
	return stream->mixer;
}

void
set_stream_mixer(stream_t* stream, mixer_t* mixer)
{
	mixer_t* old_mixer;

	old_mixer = stream->mixer;
	stream->mixer = ref_mixer(mixer);
	al_attach_audio_stream_to_mixer(stream->ptr, stream->mixer->ptr);
	free_mixer(old_mixer);
}

void
feed_stream(stream_t* stream, const void* data, size_t size)
{
	size_t needed_size;
	
	console_log(4, "buffering %zu bytes into stream #%u", size, stream->id);
	
	needed_size = stream->feed_size + size;
	if (needed_size > stream->buffer_size) {
		// buffer is too small, double size until large enough
		while (needed_size > stream->buffer_size)
			stream->buffer_size *= 2;
		stream->buffer = realloc(stream->buffer, stream->buffer_size);
	}
	memcpy(stream->buffer + stream->feed_size, data, size);
	stream->feed_size += size;
}

void
pause_stream(stream_t* stream)
{
	al_set_audio_stream_playing(stream->ptr, false);
}

void
play_stream(stream_t* stream)
{
	al_set_audio_stream_playing(stream->ptr, true);
}

void
stop_stream(stream_t* stream)
{
	al_drain_audio_stream(stream->ptr);
	free(stream->buffer); stream->buffer = NULL;
	stream->feed_size = 0;
}

static void
update_stream(stream_t* stream)
{
	void*  buffer;

	if (stream->feed_size <= stream->fragment_size) return;
	if (!(buffer = al_get_audio_stream_fragment(stream->ptr)))
		return;
	memcpy(buffer, stream->buffer, stream->fragment_size);
	stream->feed_size -= stream->fragment_size;
	memmove(stream->buffer, stream->buffer + stream->fragment_size, stream->feed_size);
	al_set_audio_stream_fragment(stream->ptr, buffer);
}

void
init_audialis_api(void)
{
	// core Audialis API functions
	register_api_method(g_duk, NULL, "GetDefaultMixer", js_GetDefaultMixer);

	// Mixer object
	register_api_ctor(g_duk, "Mixer", js_new_Mixer, js_Mixer_finalize);
	register_api_prop(g_duk, "Mixer", "volume", js_Mixer_get_volume, js_Mixer_set_volume);

	// SoundStream object
	register_api_ctor(g_duk, "SoundStream", js_new_SoundStream, js_SoundStream_finalize);
	register_api_prop(g_duk, "SoundStream", "bufferSize", js_SoundStream_get_bufferSize, NULL);
	register_api_prop(g_duk, "SoundStream", "mixer", js_SoundStream_get_mixer, js_SoundStream_set_mixer);
	register_api_method(g_duk, "SoundStream", "pause", js_SoundStream_pause);
	register_api_method(g_duk, "SoundStream", "play", js_SoundStream_play);
	register_api_method(g_duk, "SoundStream", "stop", js_SoundStream_stop);
	register_api_method(g_duk, "SoundStream", "write", js_SoundStream_write);

	// Sound object
	register_api_method(g_duk, NULL, "LoadSound", js_LoadSound);
	register_api_ctor(g_duk, "Sound", js_new_Sound, js_Sound_finalize);
	register_api_method(g_duk, "Sound", "toString", js_Sound_toString);
	register_api_prop(g_duk, "Sound", "length", js_Sound_get_length, NULL);
	register_api_prop(g_duk, "Sound", "mixer", js_Sound_get_mixer, js_Sound_set_mixer);
	register_api_prop(g_duk, "Sound", "pan", js_Sound_get_pan, js_Sound_set_pan);
	register_api_prop(g_duk, "Sound", "pitch", js_Sound_get_pitch, js_Sound_set_pitch);
	register_api_prop(g_duk, "Sound", "playing", js_Sound_get_playing, NULL);
	register_api_prop(g_duk, "Sound", "position", js_Sound_get_position, js_Sound_set_position);
	register_api_prop(g_duk, "Sound", "repeat", js_Sound_get_repeat, js_Sound_set_repeat);
	register_api_prop(g_duk, "Sound", "seekable", js_Sound_get_seekable, NULL);
	register_api_prop(g_duk, "Sound", "volume", js_Sound_get_volume, js_Sound_set_volume);
	register_api_method(g_duk, "Sound", "isPlaying", js_Sound_get_playing);
	register_api_method(g_duk, "Sound", "isSeekable", js_Sound_get_seekable);
	register_api_method(g_duk, "Sound", "getLength", js_Sound_get_length);
	register_api_method(g_duk, "Sound", "getPan", js_Sound_get_pan);
	register_api_method(g_duk, "Sound", "getPitch", js_Sound_get_pitch);
	register_api_method(g_duk, "Sound", "getPosition", js_Sound_get_position);
	register_api_method(g_duk, "Sound", "getRepeat", js_Sound_get_repeat);
	register_api_method(g_duk, "Sound", "getVolume", js_Sound_getVolume);
	register_api_method(g_duk, "Sound", "setPan", js_Sound_set_pan);
	register_api_method(g_duk, "Sound", "setPitch", js_Sound_set_pitch);
	register_api_method(g_duk, "Sound", "setPosition", js_Sound_set_position);
	register_api_method(g_duk, "Sound", "setRepeat", js_Sound_set_repeat);
	register_api_method(g_duk, "Sound", "setVolume", js_Sound_setVolume);
	register_api_method(g_duk, "Sound", "pause", js_Sound_pause);
	register_api_method(g_duk, "Sound", "play", js_Sound_play);
	register_api_method(g_duk, "Sound", "reset", js_Sound_reset);
	register_api_method(g_duk, "Sound", "stop", js_Sound_stop);
}

static duk_ret_t
js_GetDefaultMixer(duk_context* ctx)
{
	duk_push_sphere_obj(ctx, "Mixer", ref_mixer(s_def_mixer));
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
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Mixer(): Invalid bit depth for mixer (%i)", bits);
	if (channels < 1 || channels > 7)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Mixer(): Invalid channel count for mixer (%i)", channels);
	if (!(mixer = create_mixer(freq, bits, channels)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Mixer(): Unable to create %i-bit %ich voice", bits, channels);
	duk_push_sphere_obj(ctx, "Mixer", mixer);
	return 1;
}

static duk_ret_t
js_Mixer_finalize(duk_context* ctx)
{
	mixer_t* mixer;

	mixer = duk_require_sphere_obj(ctx, 0, "Mixer");
	free_mixer(mixer);
	return 0;
}

static duk_ret_t
js_Mixer_get_volume(duk_context* ctx)
{
	mixer_t* mixer;

	duk_push_this(ctx);
	mixer = duk_require_sphere_obj(ctx, -1, "Mixer");
	duk_pop(ctx);
	duk_push_number(ctx, get_mixer_gain(mixer));
	return 1;
}

static duk_ret_t
js_Mixer_set_volume(duk_context* ctx)
{
	float volume = duk_require_number(ctx, 0);
	
	mixer_t* mixer;

	duk_push_this(ctx);
	mixer = duk_require_sphere_obj(ctx, -1, "Mixer");
	duk_pop(ctx);
	set_mixer_gain(mixer, volume);
	return 0;
}

static duk_ret_t
js_LoadSound(duk_context* ctx)
{
	const char* filename;
	sound_t*    sound;

	filename = duk_require_path(ctx, 0, "sounds");
	if (!(sound = load_sound(filename, get_default_mixer())))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LoadSound(): Failed to load sound file '%s'", filename);
	duk_push_sphere_obj(ctx, "Sound", sound);
	return 1;
}

static duk_ret_t
js_new_Sound(duk_context* ctx)
{
	duk_int_t   n_args;
	const char* filename;
	mixer_t*    mixer;
	sound_t*    sound;

	n_args = duk_get_top(ctx);
	filename = duk_require_path(ctx, 0, NULL);
	mixer = n_args >= 2 && !duk_is_boolean(ctx, 1)
		? duk_require_sphere_obj(ctx, 1, "Mixer")
		: get_default_mixer();
	sound = load_sound(filename, mixer);
	if (sound == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Sound(): Failed to load sound file '%s'", filename);
	duk_push_sphere_obj(ctx, "Sound", sound);
	return 1;
}

static duk_ret_t
js_Sound_finalize(duk_context* ctx)
{
	sound_t* sound;

	sound = duk_require_sphere_obj(ctx, 0, "Sound");
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
js_Sound_getVolume(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	duk_push_int(ctx, get_sound_gain(sound) * 255);
	return 1;
}

static duk_ret_t
js_Sound_setVolume(duk_context* ctx)
{
	int volume = duk_require_int(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	volume = volume < 0 ? 0 : volume > 255 ? 255 : volume;
	set_sound_gain(sound, (float)volume / 255);
	return 0;
}

static duk_ret_t
js_Sound_get_length(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	duk_push_number(ctx, get_sound_length(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_mixer(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	duk_push_sphere_obj(ctx, "Mixer", ref_mixer(get_sound_mixer(sound)));
	return 1;
}

static duk_ret_t
js_Sound_set_mixer(duk_context* ctx)
{
	mixer_t* mixer = duk_require_sphere_obj(ctx, 0, "Mixer");

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	set_sound_mixer(sound, mixer);
	return 1;
}

static duk_ret_t
js_Sound_get_pan(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	duk_push_int(ctx, get_sound_pan(sound) * 255);
	return 1;
}

static duk_ret_t
js_Sound_set_pan(duk_context* ctx)
{
	int new_pan = duk_require_int(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	set_sound_pan(sound, (float)new_pan / 255);
	return 0;
}

static duk_ret_t
js_Sound_get_pitch(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	duk_push_number(ctx, get_sound_pitch(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_pitch(duk_context* ctx)
{
	float new_pitch = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	set_sound_pitch(sound, new_pitch);
	return 0;
}

static duk_ret_t
js_Sound_get_playing(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	duk_push_boolean(ctx, is_sound_playing(sound));
	return 1;
}

static duk_ret_t
js_Sound_get_position(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	duk_push_number(ctx, get_sound_seek(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_position(duk_context* ctx)
{
	long long new_pos = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	seek_sound(sound, new_pos);
	return 0;
}

static duk_ret_t
js_Sound_get_repeat(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	duk_push_boolean(ctx, is_sound_looping(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_repeat(duk_context* ctx)
{
	bool is_looped = duk_require_boolean(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	set_sound_looping(sound, is_looped);
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
	duk_pop(ctx);
	duk_push_number(ctx, get_sound_gain(sound));
	return 1;
}

static duk_ret_t
js_Sound_set_volume(duk_context* ctx)
{
	float volume = duk_require_number(ctx, 0);

	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	set_sound_gain(sound, volume);
	return 0;
}

static duk_ret_t
js_Sound_pause(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
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
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
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
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	seek_sound(sound, 0);
	return 0;
}

static duk_ret_t
js_Sound_stop(duk_context* ctx)
{
	sound_t* sound;

	duk_push_this(ctx);
	sound = duk_require_sphere_obj(ctx, -1, "Sound");
	duk_pop(ctx);
	stop_sound(sound, true);
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
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SoundStream(): Invalid bit depth (%i)", bits);
	if (!(stream = create_stream(frequency, bits, channels)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SoundStream(): Stream creation failed");
	duk_push_sphere_obj(ctx, "SoundStream", stream);
	return 1;
}

static duk_ret_t
js_SoundStream_finalize(duk_context* ctx)
{
	stream_t* stream;

	stream = duk_require_sphere_obj(ctx, 0, "SoundStream");
	free_stream(stream);
	return 0;
}

static duk_ret_t
js_SoundStream_get_bufferSize(duk_context* ctx)
{
	stream_t*    stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_pop(ctx);
	duk_push_number(ctx, stream->feed_size);
	return 1;
}

static duk_ret_t
js_SoundStream_get_mixer(duk_context* ctx)
{
	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_pop(ctx);
	duk_push_sphere_obj(ctx, "Mixer", ref_mixer(get_stream_mixer(stream)));
	return 1;
}

static duk_ret_t
js_SoundStream_set_mixer(duk_context* ctx)
{
	mixer_t* mixer = duk_require_sphere_obj(ctx, 0, "Mixer");

	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_pop(ctx);
	set_stream_mixer(stream, mixer);
	return 1;
}

static duk_ret_t
js_SoundStream_pause(duk_context* ctx)
{
	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_pop(ctx);
	pause_stream(stream);
	return 0;
}

static duk_ret_t
js_SoundStream_play(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	mixer_t* mixer = n_args >= 1
		? duk_require_sphere_obj(ctx, 0, "Mixer")
		: get_default_mixer();

	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_pop(ctx);
	set_stream_mixer(stream, mixer);
	play_stream(stream);
	return 0;
}

static duk_ret_t
js_SoundStream_stop(duk_context* ctx)
{
	stream_t* stream;

	duk_push_this(ctx);
	stream = duk_require_sphere_obj(ctx, -1, "SoundStream");
	duk_pop(ctx);
	stop_stream(stream);
	return 0;
}

static duk_ret_t
js_SoundStream_write(duk_context* ctx)
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
	duk_pop(ctx);
	data = duk_require_buffer_data(ctx, 0, &size);
	feed_stream(stream, data, size);
	return 0;
}
