#ifndef CELL__CELL_H__INCLUDED
#define CELL__CELL_H__INCLUDED

#include "posix.h"

#include <zlib.h>
#include "duktape.h"
#include "duk_rubber.h"
#include "lstring.h"
#include "path.h"
#include "utility.h"
#include "vector.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <sys/utime.h>
#else
#include <utime.h>
#endif

#include "version.h"

#if defined(__GNUC__)
#define no_return __attribute__((no_return)) void
#elif defined(__clang__)
#define no_return __attribute__((no_return)) void
#elif defined(_MSC_VER)
#define no_return __declspec(noreturn) void
#else
#define no_return void
#endif

#endif // CELL__CELL_H__INCLUDED
