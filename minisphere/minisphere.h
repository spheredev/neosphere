#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include "duktape.h"

static const char* SPHERE_API_VER = "v1.5";
static const char* ENGINE_VER = "v0.0";

extern ALLEGRO_DISPLAY*     g_display;
extern ALLEGRO_EVENT_QUEUE* g_events;
extern duk_context*         g_duktape;

extern char* normalize_path (const char* path, const char* base_dir);
