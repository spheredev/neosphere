#ifndef MINISPHERE__MAP_ENGINE_H__INCLUDED
#define MINISPHERE__MAP_ENGINE_H__INCLUDED

#include "obsmap.h"
#include "persons.h"
#include "tileset.h"

void             initialize_map_engine   (void);
void             shutdown_map_engine     (void);
bool             is_map_engine_running   (void);
rect_t           get_map_bounds          (void);
const obsmap_t*  get_map_layer_obsmap    (int layer);
const char*      get_map_name            (void);
point3_t         get_map_origin          (void);
int              get_map_tile            (int x, int y, int layer);
const tileset_t* get_map_tileset         (void);
rect_t           get_zone_bounds         (int zone_index);
int              get_zone_layer          (int zone_index);
int              get_zone_steps          (int zone_index);
void             set_zone_bounds         (int zone_index, rect_t bounds);
void             set_zone_script         (int zone_index, script_t* script);
void             set_zone_steps          (int zone_index, int steps);
bool             add_zone                (rect_t bounds, int layer, script_t* script, int steps);
void             detach_person           (const person_t* person);
void             normalize_map_entity_xy (double* inout_x, double* inout_y, int layer);
void             remove_zone             (int zone_index);
bool             resize_map_layer        (int layer, int x_size, int y_size);

void             init_map_engine_api   (duk_context* ctx);
int              duk_require_map_layer (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__MAP_ENGINE_H__INCLUDED
