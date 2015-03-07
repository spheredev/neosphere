#ifndef MINISPHERE__MAP_ENGINE_H__INCLUDED
#define MINISPHERE__MAP_ENGINE_H__INCLUDED

#include "obsmap.h"

typedef struct map map_t;

extern void      init_map_engine_api  (duk_context* ctx);
extern rect_t    get_map_bounds       (void);
extern obsmap_t* get_map_layer_obsmap (int layer);
extern point3_t  get_map_origin       (void);

extern bool g_map_running;

#endif // MINISPHERE__MAP_ENGINE_H__INCLUDED
