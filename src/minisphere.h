#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <time.h>
#include <sys/stat.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_memfile.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <zlib.h>

#include "version.h"

#include "duktape.h"
#include "dyad.h"
#include "console.h"
#include "spherefs.h"
#include "file.h"
#include "font.h"
#include "geometry.h"
#include "lstring.h"
#include "path.h"
#include "script.h"
#include "utility.h"
#include "vector.h"

#ifdef _MSC_VER
#define strcasecmp stricmp
#define snprintf _snprintf
#endif

#define SPHERE_PATH_MAX 1024

#if defined(__GNUC__)
#define noreturn __attribute__((noreturn)) void
#elif defined(__clang__)
#define noreturn __attribute__((noreturn)) void
#elif defined(_MSC_VER)
#define noreturn __declspec(noreturn) void
#else
#define noreturn void
#endif

extern ALLEGRO_DISPLAY*     g_display;
extern ALLEGRO_EVENT_QUEUE* g_events;
extern duk_context*         g_duk;
extern int                  g_fps;
extern sandbox_t*           g_fs;
extern ALLEGRO_PATH*        g_game_path;
extern char*                g_last_game_path;
extern float                g_scale_x, g_scale_y;
extern kev_file_t*           g_sys_conf;
extern font_t*              g_sys_font;
extern int                  g_res_x, g_res_y;

extern bool     is_skipped_frame   (void);
extern rect_t   get_clip_rectangle (void);
extern int      get_max_frameskip  (void);
extern void     set_clip_rectangle (rect_t clip_rect);
extern void     set_max_frameskip  (int frames);
extern void     delay              (double time);
extern void     do_events          (void);
extern noreturn exit_game          (bool force_shutdown);
extern void     flip_screen        (int framerate);
extern noreturn restart_engine     (void);
extern void     take_screenshot    (void);
extern void     toggle_fps_display (void);
extern void     toggle_fullscreen  (void);
extern void     unskip_frame       (void);
