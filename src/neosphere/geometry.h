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

#ifndef SPHERE__GEOMETRY_H__INCLUDED
#define SPHERE__GEOMETRY_H__INCLUDED

typedef
struct point2
{
	int x;
	int y;
	int z;
} point2_t;

typedef
struct point3
{
	int x;
	int y;
	int z;
} point3_t;

typedef
struct rect
{
	int x1;
	int y1;
	int x2;
	int y2;
} rect_t;

typedef
struct rectf
{
	float x1;
	float y1;
	float x2;
	float y2;
} rectf_t;

typedef
struct size2
{
	int width;
	int height;
} size2_t;

bool     do_lines_overlap   (rect_t a, rect_t b);
bool     do_rects_overlap   (rect_t a, rect_t b);
bool     is_point_in_rect   (int x, int y, rect_t bounds);

point2_t mk_point2          (int x, int y);
point3_t mk_point3          (int x, int y, int z);
rect_t   mk_rect            (int x1, int y1, int x2, int y2);
rect_t   rect_intersect     (rect_t rect1, rect_t rect2);
void     rect_normalize     (rect_t* inout_rect);
rect_t   rect_translate     (rect_t rect, int x_offset, int y_offset);
rect_t   rect_zoom          (rect_t rect, double scale_x, double scale_y);
rectf_t  mk_rectf           (float x1, float y1, float x2, float y2);
rectf_t  rectf_scale        (rectf_t rect, float x_scale, float y_scale);
rectf_t  rectf_translate    (rectf_t rect, float x_offset, float y_offset);
size2_t  mk_size2           (int width, int height);

#endif // SPHERE__GEOMETRY_H__INCLUDED
