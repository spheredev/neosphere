#ifndef MINISPHERE__MAP_ENGINE_H__INCLUDED
#define MINISPHERE__MAP_ENGINE_H__INCLUDED

typedef struct map map_t;

extern void     init_map_engine_api (duk_context* ctx);
extern point3_t get_map_origin      (void);

#endif // MINISPHERE__MAP_ENGINE_H__INCLUDED
