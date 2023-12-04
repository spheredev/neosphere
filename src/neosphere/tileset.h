/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
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
 *  * Neither the name of Spherical nor the names of its contributors may be
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

#ifndef SPHERE__TILESET_H__INCLUDED
#define SPHERE__TILESET_H__INCLUDED

#include "atlas.h"
#include "image.h"
#include "obstruction.h"

typedef struct tileset tileset_t;

tileset_t*       tileset_new       (const char* filename);
tileset_t*       tileset_read      (file_t* file);
void             tileset_free      (tileset_t* tileset);
int              tileset_len       (const tileset_t* tileset);
const obsmap_t*  tileset_obsmap    (const tileset_t* tileset, int tile_index);
int              tileset_get_delay (const tileset_t* tileset, int tile_index);
image_t*         tileset_get_image (const tileset_t* tileset, int tile_index);
const lstring_t* tileset_get_name  (const tileset_t* tileset, int tile_index);
int              tileset_get_next  (const tileset_t* tileset, int tile_index);
void             tileset_get_size  (const tileset_t* tileset, int* out_w, int* out_h);
void             tileset_set_delay (tileset_t* tileset, int tile_index, int delay);
void             tileset_set_image (tileset_t* tileset, int tile_index, image_t* image);
void             tileset_set_next  (tileset_t* tileset, int tile_index, int next_index);
bool             tileset_set_name  (tileset_t* tileset, int tile_index, const lstring_t* name);
void             tileset_draw      (const tileset_t* tileset, color_t mask, float x, float y, int tile_index);
void             tileset_update    (tileset_t* tileset);

#endif // SPHERE__TILESET_H__INCLUDED
