#ifndef SSJ__SSJ_H__INCLUDED
#define SSJ__SSJ_H__INCLUDED

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif

#include "posix.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#if !defined(_WIN32)
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#else
#include <Shlwapi.h>
#endif

#include "path.h"
#include "vector.h"

#include "version.h"

bool  launch_minisphere (path_t* game_path);
char* strnewf           (const char* fmt, ...);

#endif // SSJ__SSJ_H__INCLUDED
