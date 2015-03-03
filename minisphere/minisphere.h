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

static const char* SPHERE_API_VER = "v1.5";
static const char* ENGINE_VER = "v0.0";

struct key_queue
{
	int num_keys;
	int keys[255];
};

struct lstring
{
	size_t length;
	char*  cstr;
};

struct point3
{
	int x;
	int y;
	int z;
};

struct rect
{
	int x1;
	int y1;
	int x2;
	int y2;
};

typedef struct key_queue key_queue_t;
typedef struct lstring   lstring_t;
typedef struct point3    point3_t;
typedef struct rect      rect_t;

extern ALLEGRO_DISPLAY*     g_display;
extern ALLEGRO_EVENT_QUEUE* g_events;
extern duk_context*         g_duktape;
extern int                  g_fps;
extern key_queue_t          g_key_queue;
extern int                  g_render_scale;
extern bool                 g_skip_frame;
extern ALLEGRO_CONFIG*      g_sys_conf;
extern ALLEGRO_FONT*        g_sys_font;
extern int                  g_res_x, g_res_y;

extern void            al_draw_tiled_bitmap  (ALLEGRO_BITMAP* bitmap, float x, float y, float width, float height);
extern ALLEGRO_BITMAP* al_fread_bitmap       (ALLEGRO_FILE* file, int width, int height);
extern lstring_t*      al_fread_lstring      (ALLEGRO_FILE* file);
extern lstring_t*      duk_require_lstring_t (duk_context* ctx, duk_idx_t index);

extern bool       begin_frame        (int framerate);
extern bool       do_events          (void);
extern lstring_t* new_lstring        (size_t length, const char* buffer);
extern void       free_lstring       (lstring_t* string);
extern char*      get_asset_path     (const char* path, const char* base_dir, bool allow_mkdir);
extern char*      get_sys_asset_path (const char* path, const char* base_dir);
