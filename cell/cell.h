#ifndef CELL__CELL_H__INCLUDED
#define CELL__CELL_H__INCLUDED

#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "duktape.h"
#include "tinydir.h"

#define CELL_VERSION "v2.0-WIP"

#include "engine.h"

extern duk_context* g_duk;
extern bool         g_is_verbose;
extern char*        g_out_path;
extern bool         g_want_source_map;

extern void print_verbose (const char* fmt, ...);

#endif // CELL__CELL_H__INCLUDED
