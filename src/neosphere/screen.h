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

#ifndef SPHERE__DISPLAY_H__INCLUDED
#define SPHERE__DISPLAY_H__INCLUDED

#include "font.h"
#include "geometry.h"
#include "image.h"

typedef struct screen screen_t;

screen_t*        screen_new               (const char* title, image_t* icon, size2_t resolution, int frameskip, font_t* font);
void             screen_free              (screen_t* it);
image_t*         screen_backbuffer        (const screen_t* it);
rect_t           screen_bounds            (const screen_t* it);
ALLEGRO_DISPLAY* screen_display           (const screen_t* it);
size2_t          screen_size              (const screen_t* it);
bool             screen_skipping_frame    (const screen_t* it);
int              screen_get_frameskip     (const screen_t* it);
bool             screen_get_fullscreen    (const screen_t* it);
void             screen_get_mouse_xy      (const screen_t* it, int* o_x, int* o_y);
void             screen_set_frameskip     (screen_t* it, int max_skips);
void             screen_set_fullscreen    (screen_t* it, bool fullscreen);
void             screen_set_mouse_xy      (screen_t* it, int x, int y);
void             screen_draw_status       (screen_t* it, const char* text, color_t color);
void             screen_fix_mouse_xy      (const screen_t* it, int *inout_x, int *inout_y);
void             screen_flip              (screen_t* it, int framerate, bool need_clear);
image_t*         screen_grab              (screen_t* it, int x, int y, int width, int height);
void             screen_queue_screenshot  (screen_t* it);
void             screen_resize            (screen_t* it, int x_size, int y_size);
void             screen_show_mouse        (screen_t* it, bool visible);
void             screen_toggle_fps        (screen_t* it);
void             screen_toggle_fullscreen (screen_t* it);
void             screen_unskip_frame      (screen_t* it);

#endif // SPHERE__DISPLAY_H__INCLUDED
