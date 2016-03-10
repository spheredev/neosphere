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
#include "screen.h"
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

#if ALLEGRO_VERSION >= 5 && ALLEGRO_SUB_VERSION >= 1
#define MINISPHERE_USE_CLIPBOARD
#define MINISPHERE_USE_SHADERS
#define MINISPHERE_USE_VERTEX_BUF
#endif

extern screen_t*            g_screen;
extern duk_context*         g_duk;
extern ALLEGRO_EVENT_QUEUE* g_events;
extern int                  g_fps;
extern sandbox_t*           g_fs;
extern path_t*              g_game_path;
extern path_t*              g_last_game_path;
extern kev_file_t*          g_sys_conf;
extern font_t*              g_sys_font;
extern int                  g_res_x, g_res_y;

bool     is_skipped_frame   (void);
int      get_max_frameskip  (void);
void     set_max_frameskip  (int frames);
void     delay              (double time);
void     do_events          (void);
noreturn exit_game          (bool force_shutdown);
void     flip_screen        (int framerate);
noreturn restart_engine     (void);
void     take_screenshot    (void);
void     toggle_fps_display (void);
void     unskip_frame       (void);
