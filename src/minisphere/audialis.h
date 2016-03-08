#ifndef MINISPHERE__AUDIALIS_H__INCLUDED
#define MINISPHERE__AUDIALIS_H__INCLUDED

typedef struct mixer  mixer_t;
typedef struct sound  sound_t;
typedef struct stream stream_t;

void      initialize_audialis (void);
void      shutdown_audialis   (void);
void      update_audialis     (void);
mixer_t*  get_default_mixer   (void);

mixer_t* create_mixer   (int frequency, int bits, int channels);
mixer_t* ref_mixer      (mixer_t* mixer);
void     free_mixer     (mixer_t* mixer);
float    get_mixer_gain (mixer_t* mixer);
void     set_mixer_gain (mixer_t* mixer, float gain);

stream_t* create_stream     (int frequency, int bits, int channels);
stream_t* ref_stream        (stream_t* stream);
void      free_stream       (stream_t* stream);
bool      is_stream_playing (const stream_t* stream);
mixer_t*  get_stream_mixer  (stream_t* stream);
void      set_stream_mixer  (stream_t* stream, mixer_t* mixer);
void      feed_stream       (stream_t* stream, const void* data, size_t size);
void      pause_stream      (stream_t* stream);
void      play_stream       (stream_t* stream);
void      stop_stream       (stream_t* stream);

sound_t*  load_sound        (const char* path, mixer_t* mixer);
sound_t*  ref_sound         (sound_t* sound);
void      free_sound        (sound_t* sound);
bool      is_sound_looping  (sound_t* sound);
bool      is_sound_playing  (sound_t* sound);
float     get_sound_gain    (sound_t* sound);
long long get_sound_length  (sound_t* sound);
mixer_t*  get_sound_mixer   (sound_t* sound);
float     get_sound_pan     (sound_t* sound);
float     get_sound_pitch   (sound_t* sound);
long long get_sound_seek    (sound_t* sound);
void      set_sound_gain    (sound_t* sound, float gain);
void      set_sound_looping (sound_t* sound, bool is_looping);
void      set_sound_mixer   (sound_t* sound, mixer_t* mixer);
void      set_sound_pan     (sound_t* sound, float pan);
void      set_sound_pitch   (sound_t* sound, float pitch);
void      play_sound        (sound_t* sound);
bool      reload_sound      (sound_t* sound);
void      seek_sound        (sound_t* sound, long long position);
void      stop_sound        (sound_t* sound, bool rewind);

void init_audialis_api (void);

#endif // MINISPHERE__AUDIALIS_H__INCLUDED
