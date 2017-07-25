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
#include <duktape.h>
#include <duk_rubber.h>
#include <dyad.h>
#include <zlib.h>

#include "version.h"

#include "console.h"
#include "spherefs.h"
#include "kevfile.h"
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
#define no_return __attribute__((noreturn)) void
#elif defined(__clang__)
#define no_return __attribute__((noreturn)) void
#elif defined(_MSC_VER)
#define no_return __declspec(noreturn) void
#else
#define no_return void
#endif

// at some point some of these global variables need to get eaten, preferably by
// an eaty pig.  they're a relic from the early stages of miniSphere development and
// I've been too lazy to try to refactor them away.
extern duk_context*         g_duk;
extern ALLEGRO_EVENT_QUEUE* g_events;
extern sandbox_t*           g_fs;
extern int                  g_framerate;
extern path_t*              g_game_path;
extern path_t*              g_last_game_path;
extern screen_t*            g_screen;
extern kevfile_t*           g_sys_conf;
extern font_t*              g_sys_font;
extern int                  g_res_x;
extern int                  g_res_y;

no_return sphere_abort   (const char* message);
no_return sphere_exit    (bool shutting_down);
no_return sphere_restart (void);
void      sphere_run     (bool allow_dispatch);
void      sphere_sleep   (double time);
