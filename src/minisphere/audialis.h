#ifndef MINISPHERE__AUDIALIS_H__INCLUDED
#define MINISPHERE__AUDIALIS_H__INCLUDED

typedef struct mixer  mixer_t;
typedef struct sound  sound_t;
typedef struct stream stream_t;

void      initialize_audialis (void);
void      shutdown_audialis   (void);
void      update_audialis     (void);
mixer_t*  get_default_mixer   (void);

mixer_t*  mixer_new         (int frequency, int bits, int channels);
mixer_t*  mixer_ref         (mixer_t* mixer);
void      mixer_free        (mixer_t* mixer);
float     mixer_get_gain    (mixer_t* mixer);
void      mixer_set_gain    (mixer_t* mixer, float gain);
sound_t*  sound_load        (const char* path, mixer_t* mixer);
sound_t*  sound_ref         (sound_t* sound);
void      sound_free        (sound_t* sound);
double    sound_len         (sound_t* sound);
bool      sound_playing     (sound_t* sound);
double    sound_tell        (sound_t* sound);
float     sound_get_gain    (sound_t* sound);
bool      sound_get_looping (sound_t* sound);
mixer_t*  sound_get_mixer   (sound_t* sound);
float     sound_get_pan     (sound_t* sound);
float     sound_get_pitch   (sound_t* sound);
void      sound_set_gain    (sound_t* sound, float gain);
void      sound_set_looping (sound_t* sound, bool is_looping);
void      sound_set_mixer   (sound_t* sound, mixer_t* mixer);
void      sound_set_pan     (sound_t* sound, float pan);
void      sound_set_pitch   (sound_t* sound, float pitch);
void      sound_play        (sound_t* sound);
bool      sound_reload      (sound_t* sound);
void      sound_seek        (sound_t* sound, double position);
void      sound_stop        (sound_t* sound, bool rewind);
stream_t* stream_new        (int frequency, int bits, int channels);
stream_t* stream_ref        (stream_t* stream);
void      stream_free       (stream_t* stream);
bool      stream_playing    (const stream_t* stream);
mixer_t*  stream_get_mixer  (stream_t* stream);
void      stream_set_mixer  (stream_t* stream, mixer_t* mixer);
void      stream_buffer     (stream_t* stream, const void* data, size_t size);
void      stream_pause      (stream_t* stream);
void      stream_play       (stream_t* stream);
void      stream_stop       (stream_t* stream);

void init_audialis_api (void);

#endif // MINISPHERE__AUDIALIS_H__INCLUDED
