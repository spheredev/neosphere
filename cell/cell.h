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
#include "lstring.h"
#include "path.h"
#include "spk_writer.h"
#include "vector.h"

#include "posix.h"

#define CELL_VERSION "v2.0-WIP"

extern duk_context*  g_duk;
extern path_t*       g_in_path;
extern bool          g_is_verbose;
extern path_t*       g_out_path;
extern spk_writer_t* g_spk_writer;
extern bool          g_want_dry_run;
extern bool          g_want_source_map;

extern bool mkdir_r (const char* pathname);
extern void print_v (const char* fmt, ...);

#endif // CELL__CELL_H__INCLUDED
