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

#ifndef SPHERE__COLOR_H__INCLUDED
#define SPHERE__COLOR_H__INCLUDED

typedef
struct color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} color_t;

typedef
struct color_fx
{
	int rn, rr, rg, rb;
	int gn, gr, gg, gb;
	int bn, br, bg, bb;
} color_fx_t;

ALLEGRO_COLOR nativecolor     (color_t color);
color_t       mk_color        (uint8_t r, uint8_t g, uint8_t b, uint8_t a);
color_t       color_mix       (color_t color, color_t other, float w1, float w2);
color_t       color_transform (color_t color, color_fx_t matrix);
color_fx_t    mk_color_fx     (int rn, int rr, int rg, int rb, int gn, int gr, int gg, int gb, int bn, int br, int bg, int bb);
color_fx_t    color_fx_mix    (color_fx_t mat1, color_fx_t mat2, int w1, int w2);

#endif // SPHERE__COLOR_H__INCLUDED
