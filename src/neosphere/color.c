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

#include "neosphere.h"
#include "color.h"

ALLEGRO_COLOR
nativecolor(color_t color)
{
	return al_map_rgba(color.r, color.g, color.b, color.a);
}

color_t
mk_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	color_t color;

	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
	return color;
}

color_t
color_mix(color_t color, color_t other, float w1, float w2)
{
	color_t blend;
	float   sigma;

	sigma = w1 + w2;
	blend.r = (color.r * w1 + other.r * w2) / sigma;
	blend.g = (color.g * w1 + other.g * w2) / sigma;
	blend.b = (color.b * w1 + other.b * w2) / sigma;
	blend.a = (color.a * w1 + other.a * w2) / sigma;
	return blend;
}

color_t
color_transform(color_t color, color_fx_t mat)
{
	int r, g, b;

	r = mat.rn + (mat.rr * color.r + mat.rg * color.g + mat.rb * color.b) / 255;
	g = mat.gn + (mat.gr * color.r + mat.gg * color.g + mat.gb * color.b) / 255;
	b = mat.bn + (mat.br * color.r + mat.bg * color.g + mat.bb * color.b) / 255;
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	return mk_color(r, g, b, color.a);
}

color_fx_t
mk_color_fx(int rn, int rr, int rg, int rb, int gn, int gr, int gg, int gb, int bn, int br, int bg, int bb)
{
	color_fx_t matrix = {
		rn, rr, rg, rb,
		gn, gr, gg, gb,
		bn, br, bg, bb,
	};
	return matrix;
}

color_fx_t
color_fx_mix(color_fx_t mat, color_fx_t other, int w1, int w2)
{
	color_fx_t output;
	int        sigma;

	sigma = w1 + w2;
	output.rn = (mat.rn * w1 + other.rn * w2) / sigma;
	output.rr = (mat.rr * w1 + other.rr * w2) / sigma;
	output.rg = (mat.rg * w1 + other.rg * w2) / sigma;
	output.rb = (mat.rb * w1 + other.rb * w2) / sigma;
	output.gn = (mat.gn * w1 + other.gn * w2) / sigma;
	output.gr = (mat.gr * w1 + other.gr * w2) / sigma;
	output.gg = (mat.gg * w1 + other.gg * w2) / sigma;
	output.gb = (mat.gb * w1 + other.gb * w2) / sigma;
	output.bn = (mat.bn * w1 + other.bn * w2) / sigma;
	output.br = (mat.br * w1 + other.br * w2) / sigma;
	output.bg = (mat.bg * w1 + other.bg * w2) / sigma;
	output.bb = (mat.bb * w1 + other.bb * w2) / sigma;
	return output;
}
