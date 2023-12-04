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

#ifndef SPHERE__VANILLA_H__INCLUDED
#define SPHERE__VANILLA_H__INCLUDED

#include "byte_array.h"
#include "jsal.h"
#include "script.h"
#include "spriteset.h"

enum vanilla_class
{
	SV1_ANIMATION = 100,
	SV1_BYTE_ARRAY,
	SV1_COLOR,
	SV1_COLOR_MATRIX,
	SV1_FILE,
	SV1_FONT,
	SV1_IMAGE,
	SV1_LOGGER,
	SV1_RAW_FILE,
	SV1_SOCKET,
	SV1_SOUND,
	SV1_SOUND_EFFECT,
	SV1_SPRITESET,
	SV1_SURFACE,
	SV1_WINDOW_STYLE,
};

void vanilla_init   (void);
void vanilla_uninit (void);

void         jsal_push_sphere_bytearray    (bytearray_t* array);
void         jsal_push_sphere_color        (color_t color);
void         jsal_push_sphere_font         (font_t* font);
void         jsal_push_sphere_spriteset    (spriteset_t* spriteset);
color_t      jsal_require_sphere_color     (int index);
color_fx_t   jsal_require_sphere_color_fx  (int index);
script_t*    jsal_require_sphere_script    (int index, const char* name);
spriteset_t* jsal_require_sphere_spriteset (int index);

#endif // SPHERE__VANILLA_H__INCLUDED
