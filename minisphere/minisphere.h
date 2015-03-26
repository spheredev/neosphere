#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define strcasecmp stricmp
#define snprintf _snprintf
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <setjmp.h>
#include <time.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include "duktape.h"
#include "dyad.h"
#include "font.h"
#include "geometry.h"
#include "lstring.h"
#include "script.h"

static const char* const ENGINE_VERSION_NAME       = "minisphere 1.0b4";
static double      const SPHERE_API_VERSION        = 1.5;
static const char* const SPHERE_API_VERSION_STRING = "v1.5 (compatible; minisphere 1.0b4)";

extern ALLEGRO_DISPLAY*     g_display;
extern ALLEGRO_EVENT_QUEUE* g_events;
extern duk_context*         g_duktape;
extern int                  g_fps;
extern ALLEGRO_PATH*        g_game_path;
extern char*                g_last_game_path;
extern float                g_scale_x, g_scale_y;
extern ALLEGRO_CONFIG*      g_sys_conf;
extern font_t*              g_sys_font;
extern int                  g_res_x, g_res_y;

extern bool   is_skipped_frame   (void);
extern char*  get_asset_path     (const char* path, const char* base_dir, bool allow_mkdir);
extern rect_t get_clip_rectangle (void);
extern char*  get_sys_asset_path (const char* path, const char* base_dir);
extern void   set_clip_rectangle (rect_t clip_rect);
extern void   do_events          (void);
extern void   exit_game          (bool is_shutdown);
extern void   flip_screen        (int framerate);
extern void   restart_engine     (void);
extern void   take_screenshot    (void);
extern void   toggle_fps_display (void);
extern void   toggle_fullscreen  (void);
extern void   unskip_frame       (void);
