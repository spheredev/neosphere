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

#ifndef MINISPHERE__IMAGE_H__INCLUDED
#define MINISPHERE__IMAGE_H__INCLUDED

#include "geometry.h"
#include "transform.h"

typedef struct image image_t;

typedef
struct image_lock
{
	color_t*  pixels;
	ptrdiff_t pitch;
	int       num_lines;
} image_lock_t;

image_t*        image_new                (int width, int height);
image_t*        image_new_slice          (image_t* parent, int x, int y, int width, int height);
image_t*        image_clone              (const image_t* image);
image_t*        image_load               (const char* filename);
image_t*        image_read               (file_t* file, int width, int height);
image_t*        image_read_slice         (file_t* file, image_t* parent, int x, int y, int width, int height);
image_t*        image_ref                (image_t* image);
void            image_unref              (image_t* image);
ALLEGRO_BITMAP* image_bitmap             (image_t* image);
int             image_height             (const image_t* image);
const char*     image_path               (const image_t* image);
int             image_width              (const image_t* image);
rect_t          image_get_scissor        (const image_t* it);
transform_t*    image_get_transform      (const image_t* image);
void            image_set_scissor        (image_t* it, rect_t value);
void            image_set_transform      (image_t* image, transform_t* transform);
bool            image_apply_colormat     (image_t* image, colormatrix_t matrix, int x, int y, int width, int height);
bool            image_apply_colormat_4   (image_t* image, colormatrix_t ul_mat, colormatrix_t ur_mat, colormatrix_t ll_mat, colormatrix_t lr_mat, int x, int y, int width, int height);
bool            image_apply_lookup       (image_t* image, int x, int y, int width, int height, uint8_t red_lu[256], uint8_t green_lu[256], uint8_t blue_lu[256], uint8_t alpha_lu[256]);
void            image_blit               (image_t* image, image_t* target_image, int x, int y);
void            image_draw               (image_t* image, int x, int y);
void            image_draw_masked        (image_t* image, color_t mask, int x, int y);
void            image_draw_scaled        (image_t* image, int x, int y, int width, int height);
void            image_draw_scaled_masked (image_t* image, color_t mask, int x, int y, int width, int height);
void            image_draw_tiled         (image_t* image, int x, int y, int width, int height);
void            image_draw_tiled_masked  (image_t* image, color_t mask, int x, int y, int width, int height);
void            image_fill               (image_t* image, color_t color);
bool            image_flip               (image_t* image, bool is_h_flip, bool is_v_flip);
color_t         image_get_pixel          (image_t* image, int x, int y);
image_lock_t*   image_lock               (image_t* image);
void            image_render_to          (image_t* image, transform_t* transform);
bool            image_replace_color      (image_t* image, color_t color, color_t new_color);
bool            image_rescale            (image_t* image, int width, int height);
bool            image_save               (image_t* image, const char* filename);
void            image_set_pixel          (image_t* image, int x, int y, color_t color);
void            image_unlock             (image_t* image, image_lock_t* lock);
bool            image_write              (image_t* it, file_t* file);

#endif // MINISPHERE__IMAGE_H__INCLUDED
