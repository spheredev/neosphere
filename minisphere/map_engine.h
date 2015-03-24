#ifndef MINISPHERE__MAP_ENGINE_H__INCLUDED
#define MINISPHERE__MAP_ENGINE_H__INCLUDED

#include "obsmap.h"
#include "tileset.h"

typedef struct map map_t;

extern void             init_map_engine         (void);
extern void             shutdown_map_engine     (void);
extern bool             is_map_engine_running   (void);
extern rect_t           get_map_bounds          (void);
extern const obsmap_t*  get_map_layer_obsmap    (int layer);
extern point3_t         get_map_origin          (void);
extern int              get_map_tile            (int x, int y, int layer);
extern const tileset_t* get_map_tileset         (void);
extern void             normalize_map_entity_xy (double* inout_x, double* inout_y, int layer);

extern void             init_map_engine_api   (duk_context* ctx);
extern int              duk_require_map_layer (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__MAP_ENGINE_H__INCLUDED
