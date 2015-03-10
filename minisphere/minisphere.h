#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define strcasecmp stricmp
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include "duktape.h"
#include "font.h"
#include "geometry.h"
#include "lstring.h"
#include "script.h"

static double      const SPHERE_API_VERSION        = 1.5;
static const char* const SPHERE_API_VERSION_STRING = "v1.5 (compatible; minisphere v1.0-b1)";

typedef struct key_queue key_queue_t;
struct key_queue
{
	int num_keys;
	int keys[255];
};

extern ALLEGRO_DISPLAY*     g_display;
extern ALLEGRO_EVENT_QUEUE* g_events;
extern duk_context*         g_duktape;
extern int                  g_fps;
extern key_queue_t          g_key_queue;
extern float                g_scale_x, g_scale_y;
extern bool                 g_skip_frame;
extern ALLEGRO_CONFIG*      g_sys_conf;
extern font_t*              g_sys_font;
extern int                  g_res_x, g_res_y;

extern void            al_draw_tiled_bitmap  (ALLEGRO_BITMAP* bitmap, float x, float y, float width, float height);
extern ALLEGRO_BITMAP* al_fread_bitmap       (ALLEGRO_FILE* file, int width, int height);

extern bool       begin_frame        (int framerate);
extern bool       do_events          (void);
extern char*      get_asset_path     (const char* path, const char* base_dir, bool allow_mkdir);
extern char*      get_sys_asset_path (const char* path, const char* base_dir);
