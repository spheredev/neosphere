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

#ifndef SPHERE__AUDIO_H__INCLUDED
#define SPHERE__AUDIO_H__INCLUDED

typedef struct mixer  mixer_t;
typedef struct sample sample_t;
typedef struct sound  sound_t;
typedef struct stream stream_t;

void        audio_init        (void);
void        audio_uninit      (void);
void        audio_resume      (void);
void        audio_suspend     (void);
void        audio_update      (void);
mixer_t*    mixer_new         (int frequency, int bits, int channels);
mixer_t*    mixer_ref         (mixer_t* mixer);
void        mixer_unref       (mixer_t* mixer);
float       mixer_get_gain    (mixer_t* mixer);
void        mixer_set_gain    (mixer_t* mixer, float gain);
sample_t*   sample_new        (const char* path, bool polyphonic);
sample_t*   sample_ref        (sample_t* sample);
void        sample_unref      (sample_t* sample);
const char* sample_path       (const sample_t* sample);
float       sample_get_gain   (const sample_t* sample);
float       sample_get_pan    (const sample_t* sample);
float       sample_get_speed  (const sample_t* sample);
void        sample_set_gain   (sample_t* sample, float gain);
void        sample_set_pan    (sample_t* sample, float pan);
void        sample_set_speed  (sample_t* sample, float speed);
void        sample_play       (sample_t* sample, mixer_t* mixer);
void        sample_stop_all   (sample_t* sample);
sound_t*    sound_new         (const char* path);
sound_t*    sound_ref         (sound_t* sound);
void        sound_unref       (sound_t* sound);
float       sound_gain        (sound_t* sound);
double      sound_len         (sound_t* sound);
mixer_t*    sound_mixer       (sound_t* sound);
float       sound_pan         (sound_t* sound);
const char* sound_path        (const sound_t* sound);
bool        sound_playing     (sound_t* sound);
bool        sound_repeat      (sound_t* sound);
float       sound_speed       (sound_t* sound);
void        sound_set_gain    (sound_t* sound, float gain);
void        sound_set_pan     (sound_t* sound, float pan);
void        sound_set_repeat  (sound_t* sound, bool repeat);
void        sound_set_speed   (sound_t* sound, float pitch);
void        sound_pause       (sound_t* sound, bool paused);
void        sound_play        (sound_t* sound, mixer_t* mixer);
void        sound_seek        (sound_t* sound, double position);
void        sound_stop        (sound_t* sound);
double      sound_tell        (sound_t* sound);
stream_t*   stream_new        (int frequency, int bits, int channels);
stream_t*   stream_ref        (stream_t* stream);
void        stream_unref      (stream_t* stream);
double      stream_length     (const stream_t* stream);
mixer_t*    stream_mixer      (const stream_t* stream);
bool        stream_playing    (const stream_t* stream);
void        stream_buffer     (stream_t* stream, const void* data, size_t size);
void        stream_pause      (stream_t* stream, bool paused);
void        stream_play       (stream_t* stream, mixer_t* mixer);
void        stream_stop       (stream_t* stream);

#endif // SPHERE__AUDIO_H__INCLUDED
