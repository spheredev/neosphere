#ifndef MINISPHERE__MAP_ENGINE_H__INCLUDED
#define MINISPHERE__MAP_ENGINE_H__INCLUDED

#include "obsmap.h"
#include "tileset.h"

typedef struct map map_t;

extern void             init_map_engine_api     (duk_context* ctx);
extern rect_t           get_map_bounds          (void);
extern const obsmap_t*  get_map_layer_obsmap    (int layer);
extern point3_t         get_map_origin          (void);
extern int              get_map_tile            (int x, int y, int layer);
extern const tileset_t* get_map_tileset         (void);
extern void             normalize_map_entity_xy (double* inout_x, double* inout_y, int layer);

extern bool g_map_running;

#endif // MINISPHERE__MAP_ENGINE_H__INCLUDED
