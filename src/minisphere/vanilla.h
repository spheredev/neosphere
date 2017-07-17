#ifndef MINISPHERE__VANILLA_H__INCLUDED
#define MINISPHERE__VANILLA_H__INCLUDED

#include "bytearray.h"
#include "spriteset.h"

void initialize_vanilla_api (duk_context* ctx);

void          duk_push_sphere_bytearray      (duk_context* ctx, bytearray_t* array);
void          duk_push_sphere_color          (duk_context* ctx, color_t color);
void          duk_push_sphere_font           (duk_context* ctx, font_t* font);
void          duk_push_sphere_spriteset      (duk_context* ctx, spriteset_t* spriteset);
color_t       duk_require_sphere_color       (duk_context* ctx, duk_idx_t index);
colormatrix_t duk_require_sphere_colormatrix (duk_context* ctx, duk_idx_t index);
script_t*     duk_require_sphere_script      (duk_context* ctx, duk_idx_t index, const char* name);
spriteset_t*  duk_require_sphere_spriteset   (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__VANILLA_H__INCLUDED
