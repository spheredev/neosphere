/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2017, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#ifndef MINISPHERE__VANILLA_H__INCLUDED
#define MINISPHERE__VANILLA_H__INCLUDED

#include "byte_array.h"
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
