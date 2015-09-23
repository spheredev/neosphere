#ifndef CELL__CELL_H__INCLUDED
#define CELL__CELL_H__INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "duktape.h"
#include "lstring.h"
#include "path.h"
#include "tinydir.h"
#include "vector.h"

#include "posix.h"

#define CELL_VERSION "v2.0-WIP"

extern duk_context* g_duk;
extern path_t*      g_in_path;
extern bool         g_is_verbose;
extern path_t*      g_out_path;
extern bool         g_want_dry_run;
extern bool         g_want_package;
extern bool         g_want_source_map;

extern void print_verbose (const char* fmt, ...);

#endif // CELL__CELL_H__INCLUDED
