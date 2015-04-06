typedef struct sound sound_t;

extern void     initialize_sound  (void);
extern void     shutdown_sound    (void);
extern void     update_sounds     (void);
extern sound_t* load_sound        (const char* path, bool streaming);
extern sound_t* ref_sound         (sound_t* sound);
extern void     free_sound        (sound_t* sound);
extern bool     is_sound_looping  (sound_t* sound);
extern bool     is_sound_playing  (sound_t* sound);
extern float    get_sound_gain    (sound_t* sound);
extern int      get_sound_length  (sound_t* sound);
extern float    get_sound_pan     (sound_t* sound);
extern float    get_sound_pitch   (sound_t* sound);
extern int      get_sound_seek    (sound_t* sound);
extern void     set_sound_gain    (sound_t* sound, float gain);
extern void     set_sound_looping (sound_t* sound, bool is_looping);
extern void     set_sound_pan     (sound_t* sound, float pan);
extern void     set_sound_pitch   (sound_t* sound, float pitch);
extern void     play_sound        (sound_t* sound);
extern bool     reload_sound      (sound_t* sound);
extern void     seek_sound        (sound_t* sound, int position);
extern void     stop_sound        (sound_t* sound, bool rewind);

extern void init_sound_api        (void);
extern void duk_push_sphere_sound (duk_context* ctx, sound_t* sound);
