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

#ifndef SPHERE__SPRITESET_H__INCLUDED
#define SPHERE__SPRITESET_H__INCLUDED

#include "image.h"

typedef struct spriteset spriteset_t;

void         spritesets_init             (void);
void         spritesets_uninit           (void);
spriteset_t* spriteset_new               (void);
spriteset_t* spriteset_load              (const char* filename);
spriteset_t* spriteset_clone             (const spriteset_t* it);
spriteset_t* spriteset_ref               (spriteset_t* it);
void         spriteset_unref             (spriteset_t* it);
int          spriteset_frame_delay       (const spriteset_t* it, const char* pose_name, int frame_index);
int          spriteset_frame_image_index (const spriteset_t* it, const char* pose_name, int frame_index);
int          spriteset_height            (const spriteset_t* it);
image_t*     spriteset_image             (const spriteset_t* it, int index);
int          spriteset_num_frames        (const spriteset_t* it, const char* pose_name);
int          spriteset_num_images        (const spriteset_t* it);
int          spriteset_num_poses         (const spriteset_t* it);
const char*  spriteset_pathname          (const spriteset_t* it);
const char*  spriteset_pose_name         (const spriteset_t* it, int index);
int          spriteset_width             (const spriteset_t* it);
rect_t       spriteset_get_base          (const spriteset_t* it);
void         spriteset_set_base          (spriteset_t* it, rect_t new_base);
void         spriteset_add_frame         (spriteset_t* it, const char* pose_name, int image_idx, int delay);
void         spriteset_add_image         (spriteset_t* it, image_t* image);
void         spriteset_add_pose          (spriteset_t* it, const char* name);
void         spriteset_draw              (const spriteset_t* it, color_t mask, bool is_flipped, double theta, double scale_x, double scale_y, const char* pose_name, float x, float y, int frame_index);
bool         spriteset_save              (const spriteset_t* it, const char* filename);

#endif // SPHERE__SPRITESET_H__INCLUDED
