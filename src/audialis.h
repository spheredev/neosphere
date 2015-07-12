#ifndef MINISPHERE__AUDIALIS_H__INCLUDED
#define MINISPHERE__AUDIALIS_H__INCLUDED

typedef struct mixer  mixer_t;
typedef struct sound  sound_t;
typedef struct stream stream_t;

void      initialize_audialis (void);
void      shutdown_audialis   (void);
void      update_audialis     (void);
mixer_t*  get_default_mixer   (void);

extern mixer_t* create_mixer (int frequency, int bits, int channels);
extern mixer_t* ref_mixer    (mixer_t* mixer);
extern void     free_mixer   (mixer_t* mixer);

extern stream_t* create_stream (int frequency, int bits);
extern stream_t* ref_stream    (stream_t* stream);
extern void      free_stream   (stream_t* stream);
extern void      feed_stream   (stream_t* stream, const void* data, size_t size);
extern void      pause_stream  (stream_t* stream);
extern void      play_stream   (stream_t* stream);
extern void      stop_stream   (stream_t* stream);

extern sound_t*  load_sound        (const char* path, bool streaming);
extern sound_t*  ref_sound         (sound_t* sound);
extern void      free_sound        (sound_t* sound);
extern bool      is_sound_looping  (sound_t* sound);
extern bool      is_sound_playing  (sound_t* sound);
extern float     get_sound_gain    (sound_t* sound);
extern long long get_sound_length  (sound_t* sound);
extern float     get_sound_pan     (sound_t* sound);
extern float     get_sound_pitch   (sound_t* sound);
extern long long get_sound_seek    (sound_t* sound);
extern void      set_sound_gain    (sound_t* sound, float gain);
extern void      set_sound_looping (sound_t* sound, bool is_looping);
extern void      set_sound_pan     (sound_t* sound, float pan);
extern void      set_sound_pitch   (sound_t* sound, float pitch);
extern void      play_sound        (sound_t* sound);
extern bool      reload_sound      (sound_t* sound);
extern void      seek_sound        (sound_t* sound, long long position);
extern void      stop_sound        (sound_t* sound, bool rewind);

extern void init_audialis_api     (void);
extern void duk_push_sphere_sound (duk_context* ctx, sound_t* sound);

#endif // MINISPHERE__AUDIALIS_H__INCLUDED
