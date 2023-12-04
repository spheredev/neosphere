/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "neosphere.h"
#include "audio.h"

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
	bool                  suspended;
	ALLEGRO_AUDIO_STREAM* stream;
};

struct sample
{
	unsigned int    refcount;
	unsigned int    id;
	float           gain;
	float           pan;
	char*           path;
	bool            polyphonic;
	float           speed;
	ALLEGRO_SAMPLE* ptr;
};

struct sample_instance
{
	mixer_t*                 mixer;
	ALLEGRO_SAMPLE_INSTANCE* ptr;
	sample_t*                sample;
};

static bool reload_sound  (sound_t* sound);
static void update_stream (stream_t* stream);

static vector_t*            s_active_samples;
static vector_t*            s_active_sounds;
static vector_t*            s_active_streams;
static bool                 s_have_sound;
static unsigned int         s_next_mixer_id = 1;
static unsigned int         s_next_sample_id = 1;
static unsigned int         s_next_sound_id = 1;
static unsigned int         s_next_stream_id = 1;
static unsigned int         s_num_refs = 0;

void
audio_init(void)
{
	if (++s_num_refs > 1)
		return;

	console_log(1, "initializing audio subsystem");

	s_have_sound = true;
	if (!al_install_audio()) {
		s_have_sound = false;
		console_log(1, "  audio is not available");
		return;
	}
	al_init_acodec_addon();
	s_active_samples = vector_new(sizeof(struct sample_instance));
	s_active_sounds = vector_new(sizeof(sound_t*));
	s_active_streams = vector_new(sizeof(stream_t*));
}

void
audio_uninit(void)
{
	struct sample_instance* sample_instance;
	sound_t**               sound_ptr;

	iter_t iter;

	if (--s_num_refs > 0)
		return;

	console_log(1, "shutting down audio subsystem");

	iter = vector_enum(s_active_sounds);
	while ((sound_ptr = iter_next(&iter)))
		sound_unref(*sound_ptr);
	vector_free(s_active_sounds);
	iter = vector_enum(s_active_samples);
	while ((sample_instance = iter_next(&iter))) {
		al_destroy_sample_instance(sample_instance->ptr);
		sample_unref(sample_instance->sample);
		mixer_unref(sample_instance->mixer);
	}
	vector_free(s_active_samples);
	vector_free(s_active_streams);
	if (s_have_sound)
		al_uninstall_audio();
}

void
audio_resume(void)
{
	sound_t** sound_ptr;

	iter_t iter;

	iter = vector_enum(s_active_sounds);
	while ((sound_ptr = iter_next(&iter))) {
		if ((*sound_ptr)->suspended)
			sound_pause(*sound_ptr, false);
	}
}

void
audio_suspend(void)
{
	sound_t** sound_ptr;

	iter_t iter;

	iter = vector_enum(s_active_sounds);
	while ((sound_ptr = iter_next(&iter))) {
		(*sound_ptr)->suspended = sound_playing(*sound_ptr);
		sound_pause(*sound_ptr, true);
	}
}

void
audio_update(void)
{
	sound_t*                sound;
	struct sample_instance* sample_instance;
	stream_t**              stream_ptr;

	iter_t iter;

	if (s_num_refs == 0)
		return;

	iter = vector_enum(s_active_streams);
	while ((stream_ptr = iter_next(&iter)))
		update_stream(*stream_ptr);

	iter = vector_enum(s_active_samples);
	while ((sample_instance = iter_next(&iter))) {
		if (al_get_sample_instance_playing(sample_instance->ptr))
			continue;
		al_destroy_sample_instance(sample_instance->ptr);
		sample_unref(sample_instance->sample);
		mixer_unref(sample_instance->mixer);
		iter_remove(&iter);
	}

	iter = vector_enum(s_active_sounds);
	while (iter_next(&iter)) {
		sound = *(sound_t**)iter.ptr;
		if (sound_playing(sound) || sound->suspended)
			continue;
		sound_unref(sound);
		iter_remove(&iter);
	}
}

mixer_t*
mixer_new(int frequency, int bits, int channels)
{
	ALLEGRO_CHANNEL_CONF conf;
	ALLEGRO_AUDIO_DEPTH  depth;
	mixer_t*             mixer;

	console_log(2, "creating new mixer #%u at %d kHz", s_next_mixer_id, frequency / 1000);
	console_log(3, "    format: %dch %d Hz, %d-bit", channels, frequency, bits);

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

	if (!(mixer = calloc(1, sizeof(mixer_t))))
		goto on_error;
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
	return mixer_ref(mixer);

on_error:
	console_log(2, "failed to create mixer #%u", s_next_mixer_id++);
	if (mixer != NULL) {
		if (mixer->ptr != NULL)
			al_destroy_mixer(mixer->ptr);
		if (mixer->voice != NULL)
			al_destroy_voice(mixer->voice);
		free(mixer);
	}
	return NULL;
}

mixer_t*
mixer_ref(mixer_t* mixer)
{
	++mixer->refcount;
	return mixer;
}

void
mixer_unref(mixer_t* mixer)
{
	if (mixer == NULL || --mixer->refcount > 0)
		return;

	console_log(3, "disposing mixer #%u no longer in use", mixer->id);
	al_destroy_mixer(mixer->ptr);
	free(mixer);
}

float
mixer_get_gain(mixer_t* mixer)
{
	return mixer->gain;
}

void
mixer_set_gain(mixer_t* mixer, float gain)
{
	al_set_mixer_gain(mixer->ptr, gain);
	mixer->gain = gain;
}

sample_t*
sample_new(const char* path, bool polyphonic)
{
	ALLEGRO_SAMPLE* al_sample;
	ALLEGRO_FILE*   file;
	void*           file_data;
	size_t          file_size;
	sample_t*       sample = NULL;

	console_log(2, "loading sample #%u from '%s'", s_next_sample_id, path);

	if (!(file_data = game_read_file(g_game, path, &file_size)))
		goto on_error;
	file = al_open_memfile(file_data, file_size, "rb");
	al_sample = al_load_sample_f(file, strrchr(path, '.'));
	al_fclose(file);

	if (!(sample = calloc(1, sizeof(sample_t))))
		goto on_error;
	sample->id = s_next_sample_id++;
	sample->path = strdup(path);
	sample->ptr = al_sample;
	sample->polyphonic = polyphonic;
	sample->gain = 1.0;
	sample->pan = 0.0;
	sample->speed = 1.0;
	return sample_ref(sample);

on_error:
	console_log(2, "    failed to load sample #%u", s_next_sample_id);
	free(sample);
	return NULL;
}

sample_t*
sample_ref(sample_t* sample)
{
	++sample->refcount;
	return sample;
}

void
sample_unref(sample_t* sample)
{
	if (sample == NULL || --sample->refcount > 0)
		return;

	console_log(3, "disposing sample #%u no longer in use", sample->id);
	al_destroy_sample(sample->ptr);
	free(sample);
}

const char*
sample_path(const sample_t* sample)
{
	return sample->path;
}

float
sample_get_gain(const sample_t* sample)
{
	return sample->gain;
}

float
sample_get_pan(const sample_t* sample)
{
	return sample->pan;
}

float
sample_get_speed(const sample_t* sample)
{
	return sample->speed;
}

void
sample_set_gain(sample_t* sample, float gain)
{
	sample->gain = gain;
}

void
sample_set_pan(sample_t* sample, float pan)
{
	sample->pan = pan;
}

void
sample_set_speed(sample_t* sample, float speed)
{
	sample->speed = speed;
}

void
sample_play(sample_t* sample, mixer_t* mixer)
{
	struct sample_instance   instance;
	ALLEGRO_SAMPLE_INSTANCE* stream_ptr;

	console_log(2, "playing sample #%u on mixer #%u", sample->id, mixer->id);

	if (!sample->polyphonic)
		sample_stop_all(sample);
	stream_ptr = al_create_sample_instance(sample->ptr);
	al_set_sample_instance_gain(stream_ptr, sample->gain);
	al_set_sample_instance_speed(stream_ptr, sample->speed);
	al_set_sample_instance_pan(stream_ptr, sample->pan);
	al_attach_sample_instance_to_mixer(stream_ptr, mixer->ptr);
	al_play_sample_instance(stream_ptr);

	instance.sample = sample_ref(sample);
	instance.mixer = mixer_ref(mixer);
	instance.ptr = stream_ptr;
	vector_push(s_active_samples, &instance);
}

void
sample_stop_all(sample_t* sample)
{
	struct sample_instance* instance;

	iter_t iter;

	console_log(2, "stopping all instances of sample #%u", sample->id);
	iter = vector_enum(s_active_samples);
	while ((instance = iter_next(&iter))) {
		if (instance->sample != sample)
			continue;
		al_destroy_sample_instance(instance->ptr);
		sample_unref(instance->sample);
		mixer_unref(instance->mixer);
		iter_remove(&iter);
	}
}

sound_t*
sound_new(const char* path)
{
	sound_t* sound;

	console_log(2, "loading sound #%u from '%s'", s_next_sound_id, path);

	if (!(sound = calloc(1, sizeof(sound_t))))
		goto on_error;
	sound->path = strdup(path);

	if (!(sound->file_data = game_read_file(g_game, sound->path, &sound->file_size)))
		goto on_error;
	sound->gain = 1.0;
	sound->pan = 0.0;
	sound->pitch = 1.0;
	if (!reload_sound(sound))
		goto on_error;
	sound->id = s_next_sound_id++;
	return sound_ref(sound);

on_error:
	console_log(2, "    failed to load sound #%u", s_next_sound_id);
	if (sound != NULL) {
		free(sound->path);
		free(sound);
	}
	return NULL;
}

sound_t*
sound_ref(sound_t* sound)
{
	++sound->refcount;
	return sound;
}

void
sound_unref(sound_t* sound)
{
	if (sound == NULL || --sound->refcount > 0)
		return;

	console_log(3, "disposing sound #%u no longer in use", sound->id);
	free(sound->file_data);
	if (sound->stream != NULL)
		al_destroy_audio_stream(sound->stream);
	mixer_unref(sound->mixer);
	free(sound->path);
	free(sound);
}

float
sound_gain(sound_t* sound)
{
	return sound->gain;
}

double
sound_len(sound_t* sound)
{
	if (sound->stream != NULL)
		return al_get_audio_stream_length_secs(sound->stream);
	else
		return 0.0;
}

mixer_t*
sound_mixer(sound_t* sound)
{
	return sound->mixer;
}

const char*
sound_path(const sound_t* sound)
{
	return sound->path;
}

float
sound_pan(sound_t* sound)
{
	return sound->pan;
}

bool
sound_playing(sound_t* sound)
{
	if (sound->stream != NULL)
		return al_get_audio_stream_playing(sound->stream);
	else
		return false;
}

float
sound_speed(sound_t* sound)
{
	return sound->pitch;
}

double
sound_tell(sound_t* sound)
{
	if (sound->stream != NULL)
		return al_get_audio_stream_position_secs(sound->stream);
	else
		return 0.0;
}

bool
sound_repeat(sound_t* sound)
{
	return sound->is_looping;
}

void
sound_set_gain(sound_t* sound, float gain)
{
	if (sound->stream != NULL)
		al_set_audio_stream_gain(sound->stream, gain);
	sound->gain = gain;
}

void
sound_set_repeat(sound_t* sound, bool is_looping)
{
	int play_mode;

	play_mode = is_looping ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE;
	if (sound->stream != NULL)
		al_set_audio_stream_playmode(sound->stream, play_mode);
	sound->is_looping = is_looping;
}

void
sound_set_pan(sound_t* sound, float pan)
{
	if (sound->stream != NULL)
		al_set_audio_stream_pan(sound->stream, pan);
	sound->pan = pan;
}

void
sound_set_speed(sound_t* sound, float pitch)
{
	if (sound->stream != NULL)
		al_set_audio_stream_speed(sound->stream, pitch);
	sound->pitch = pitch;
}

void
sound_pause(sound_t* sound, bool paused)
{
	if (sound->stream != NULL && sound->mixer != NULL)
		al_set_audio_stream_playing(sound->stream, !paused);
}

void
sound_play(sound_t* sound, mixer_t* mixer)
{
	mixer_t* old_mixer;

	console_log(2, "playing sound #%u on mixer #%u", sound->id, mixer->id);
	if (sound->stream != NULL) {
		old_mixer = sound->mixer;
		sound->mixer = mixer_ref(mixer);
		mixer_unref(old_mixer);
		al_rewind_audio_stream(sound->stream);
		al_attach_audio_stream_to_mixer(sound->stream, sound->mixer->ptr);
		al_set_audio_stream_playing(sound->stream, true);
		sound_ref(sound);
		vector_push(s_active_sounds, &sound);
	}
}

void
sound_seek(sound_t* sound, double position)
{
	if (sound->stream != NULL)
		al_seek_audio_stream_secs(sound->stream, position);
}

void
sound_stop(sound_t* sound)
{
	console_log(3, "stopping playback of sound #%u", sound->id);
	if (sound->stream == NULL)
		return;
	al_set_audio_stream_playing(sound->stream, false);
	al_rewind_audio_stream(sound->stream);
	mixer_unref(sound->mixer);
	sound->mixer = NULL;
}

stream_t*
stream_new(int frequency, int bits, int channels)
{
	ALLEGRO_CHANNEL_CONF conf;
	ALLEGRO_AUDIO_DEPTH  depth_flag;
	size_t               sample_size;
	stream_t*            stream;

	console_log(2, "creating new stream #%u at %d kHz", s_next_stream_id, frequency / 1000);
	console_log(3, "    format: %dch %d Hz, %d-bit", channels, frequency, bits);

	if (!(stream = calloc(1, sizeof(stream_t))))
		goto on_error;

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
	if (!(stream->ptr = al_create_audio_stream(8, 1024, frequency, depth_flag, conf)))
		goto on_error;
	al_set_audio_stream_playing(stream->ptr, false);

	// allocate an initial stream buffer
	sample_size = bits == 8 ? 1
		: bits == 16 ? 2
		: bits == 24 ? 3
		: bits == 32 ? 4
		: 0;
	stream->fragment_size = 1024 * sample_size * al_get_channel_count(conf);
	stream->buffer_size = frequency * sample_size;  // 1 second
	stream->buffer = malloc(stream->buffer_size);

	stream->id = s_next_stream_id++;
	vector_push(s_active_streams, &stream);
	return stream_ref(stream);

on_error:
	console_log(2, "couldn't create stream #%u", s_next_stream_id++);
	free(stream);
	return NULL;
}

stream_t*
stream_ref(stream_t* stream)
{
	++stream->refcount;
	return stream;
}

void
stream_unref(stream_t* stream)
{
	stream_t** stream_ptr;

	iter_t iter;

	if (stream == NULL || --stream->refcount > 0)
		return;

	console_log(3, "disposing stream #%u no longer in use", stream->id);

	iter = vector_enum(s_active_streams);
	while ((stream_ptr = iter_next(&iter))) {
		if (*stream_ptr == stream) {
			iter_remove(&iter);
			break;
		}
	}

	al_drain_audio_stream(stream->ptr);
	al_destroy_audio_stream(stream->ptr);
	mixer_unref(stream->mixer);
	free(stream->buffer);
	free(stream);
}

double
stream_length(const stream_t* stream)
{
	ALLEGRO_CHANNEL_CONF channel_conf;
	ALLEGRO_AUDIO_DEPTH  depth_conf;
	unsigned int         frequency;
	size_t               num_channels;
	size_t               sample_size;

	channel_conf = al_get_audio_stream_channels(stream->ptr);
	depth_conf = al_get_audio_stream_depth(stream->ptr);
	frequency = al_get_audio_stream_frequency(stream->ptr);
	num_channels = al_get_channel_count(channel_conf);
	sample_size = al_get_audio_depth_size(depth_conf);

	return (double)stream->feed_size / (frequency * num_channels * sample_size);
}

mixer_t*
stream_mixer(const stream_t* stream)
{
	return stream->mixer;
}

bool
stream_playing(const stream_t* stream)
{
	return al_get_audio_stream_playing(stream->ptr);
}

void
stream_buffer(stream_t* stream, const void* data, size_t size)
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
stream_pause(stream_t* stream, bool paused)
{
	if (stream->mixer == NULL)
		return;
	al_set_audio_stream_playing(stream->ptr, !paused);
}

void
stream_play(stream_t* stream, mixer_t* mixer)
{
	mixer_t* old_mixer;

	old_mixer = stream->mixer;
	stream->mixer = mixer_ref(mixer);
	mixer_unref(old_mixer);

	al_detach_audio_stream(stream->ptr);
	al_attach_audio_stream_to_mixer(stream->ptr, stream->mixer->ptr);
	al_set_audio_stream_playing(stream->ptr, true);
}

void
stream_stop(stream_t* stream)
{
	al_drain_audio_stream(stream->ptr);
	mixer_unref(stream->mixer);
	stream->mixer = NULL;
	free(stream->buffer); stream->buffer = NULL;
	stream->feed_size = 0;
}

static bool
reload_sound(sound_t* sound)
{
	// TODO: fold this back into sound_new() as it's not used anywhere else anymore.
	//       it was originally needed to work around a bug where Allegro wouldn't play a
	//       stream that had already played once, but that bug has since been fixed.

	ALLEGRO_FILE*         memfile;
	ALLEGRO_AUDIO_STREAM* new_stream = NULL;
	int                   play_mode;

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
		al_set_audio_stream_playing(sound->stream, false);
	}
	return true;

on_error:
	return false;
}

static void
update_stream(stream_t* stream)
{
	void*  buffer;

	if (stream->feed_size < stream->fragment_size)
		return;
	if (!(buffer = al_get_audio_stream_fragment(stream->ptr)))
		return;
	memcpy(buffer, stream->buffer, stream->fragment_size);
	stream->feed_size -= stream->fragment_size;
	memmove(stream->buffer, stream->buffer + stream->fragment_size, stream->feed_size);
	al_set_audio_stream_fragment(stream->ptr, buffer);
}
