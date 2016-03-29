#ifndef MINISPHERE__MAP_ENGINE_H__INCLUDED
#define MINISPHERE__MAP_ENGINE_H__INCLUDED

#include "obsmap.h"
#include "persons.h"
#include "tileset.h"

extern void             initialize_map_engine   (void);
extern void             shutdown_map_engine     (void);
extern bool             is_map_engine_running   (void);
extern rect_t           get_map_bounds          (void);
extern const obsmap_t*  get_map_layer_obsmap    (int layer);
extern const char*      get_map_name            (void);
extern point3_t         get_map_origin          (void);
extern int              get_map_tile            (int x, int y, int layer);
extern const tileset_t* get_map_tileset         (void);
extern rect_t           get_zone_bounds         (int zone_index);
extern int              get_zone_layer          (int zone_index);
extern int              get_zone_steps          (int zone_index);
extern void             set_zone_bounds         (int zone_index, rect_t bounds);
extern void             set_zone_script         (int zone_index, script_t* script);
extern void             set_zone_steps          (int zone_index, int steps);
extern bool             add_zone                (rect_t bounds, int layer, script_t* script, int steps);
extern void             detach_person           (const person_t* person);
extern void             normalize_map_entity_xy (double* inout_x, double* inout_y, int layer);
extern void             remove_zone             (int zone_index);
extern bool             resize_map_layer        (int layer, int x_size, int y_size);

extern void             init_map_engine_api   (duk_context* ctx);
extern int              duk_require_map_layer (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__MAP_ENGINE_H__INCLUDED
