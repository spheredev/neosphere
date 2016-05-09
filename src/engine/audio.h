#ifndef MINISPHERE__AUDIO_H__INCLUDED
#define MINISPHERE__AUDIO_H__INCLUDED

typedef struct mixer  mixer_t;
typedef struct sound  sound_t;
typedef struct stream stream_t;

void      initialize_audio  (void);
void      shutdown_audio    (void);
void      update_audio      (void);

mixer_t*  mixer_new         (int frequency, int bits, int channels);
mixer_t*  mixer_ref         (mixer_t* mixer);
void      mixer_free        (mixer_t* mixer);
float     mixer_get_gain    (mixer_t* mixer);
void      mixer_set_gain    (mixer_t* mixer, float gain);
sound_t*  sound_new         (const char* path);
sound_t*  sound_ref         (sound_t* sound);
void      sound_free        (sound_t* sound);
float     sound_gain        (sound_t* sound);
double    sound_len         (sound_t* sound);
mixer_t*  sound_mixer       (sound_t* sound);
float     sound_pan         (sound_t* sound);
float     sound_pitch       (sound_t* sound);
bool      sound_playing     (sound_t* sound);
bool      sound_repeat      (sound_t* sound);
void      sound_set_gain    (sound_t* sound, float gain);
void      sound_set_pan     (sound_t* sound, float pan);
void      sound_set_pitch   (sound_t* sound, float pitch);
void      sound_set_repeat  (sound_t* sound, bool repeat);
void      sound_pause       (sound_t* sound, bool paused);
void      sound_play        (sound_t* sound, mixer_t* mixer);
void      sound_seek        (sound_t* sound, double position);
void      sound_stop        (sound_t* sound);
double    sound_tell        (sound_t* sound);
stream_t* stream_new        (int frequency, int bits, int channels);
stream_t* stream_ref        (stream_t* stream);
void      stream_free       (stream_t* stream);
size_t    stream_bytes_left (const stream_t* stream);
mixer_t*  stream_mixer      (const stream_t* stream);
bool      stream_playing    (const stream_t* stream);
void      stream_buffer     (stream_t* stream, const void* data, size_t size);
void      stream_pause      (stream_t* stream, bool paused);
void      stream_play       (stream_t* stream, mixer_t* mixer);
void      stream_stop       (stream_t* stream);

void init_audio_api (void);

#endif // MINISPHERE__AUDIO_H__INCLUDED
