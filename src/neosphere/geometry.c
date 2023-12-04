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
#include "geometry.h"

bool
do_lines_overlap(rect_t a, rect_t b)
{
	float d, q, r, s;

	q = (a.y1 - b.y1) * (b.x2 - b.x1) - (a.x1 - b.x1) * (b.y2 - b.y1);
	d = (a.x2 - a.x1) * (b.y2 - b.y1) - (a.y2 - a.y1) * (b.x2 - b.x1);
	if (d == 0)
		return false;
	r = q / d;
	q = (a.y1 - b.y1) * (a.x2 - a.x1) - (a.x1 - b.x1) * (a.y2 - a.y1);
	s = q / d;
	return !(r < 0 || r > 1 || s < 0 || s > 1);
}

bool
do_rects_overlap(rect_t a, rect_t b)
{
	return !(a.x1 > b.x2 || a.x2 < b.x1 || a.y1 > b.y2 || a.y2 < b.y1);
}

bool
is_point_in_rect(int x, int y, rect_t bounds)
{
	return x >= bounds.x1 && x < bounds.x2
		&& y >= bounds.y1 && y < bounds.y2;
}

point2_t
mk_point2(int x, int y)
{
	point2_t point;

	point.x = x;
	point.y = y;
	return point;
}

point3_t
mk_point3(int x, int y, int z)
{
	point3_t point;

	point.x = x;
	point.y = y;
	point.z = z;
	return point;
}

rect_t
mk_rect(int x1, int y1, int x2, int y2)
{
	rect_t rectangle;

	rectangle.x1 = x1;
	rectangle.y1 = y1;
	rectangle.x2 = x2;
	rectangle.y2 = y2;
	return rectangle;
}

rect_t
rect_intersect(rect_t rect1, rect_t rect2)
{
	int x1, x2;
	int y1, y2;

	x1 = rect1.x1 > rect2.x1 ? rect1.x1 : rect2.x1;
	y1 = rect1.y1 > rect2.y1 ? rect1.y1 : rect2.y1;
	x2 = rect1.x2 < rect2.x2 ? rect1.x2 : rect2.x2;
	y2 = rect1.y2 < rect2.y2 ? rect1.y2 : rect2.y2;
	return mk_rect(x1, y1, x2, y2);
}

void
rect_normalize(rect_t* inout_rect)
{
	int tmp;

	if (inout_rect->x1 > inout_rect->x2) {
		tmp = inout_rect->x1;
		inout_rect->x1 = inout_rect->x2;
		inout_rect->x2 = tmp;
	}
	if (inout_rect->y1 > inout_rect->y2) {
		tmp = inout_rect->y1;
		inout_rect->y1 = inout_rect->y2;
		inout_rect->y2 = tmp;
	}
}

rect_t
rect_translate(rect_t in_rect, int x_offset, int y_offset)
{
	return mk_rect(
		in_rect.x1 + x_offset, in_rect.y1 + y_offset,
		in_rect.x2 + x_offset, in_rect.y2 + y_offset);
}

rect_t
rect_zoom(rect_t in_rect, double scale_x, double scale_y)
{
	return mk_rect(
		in_rect.x1 * scale_x, in_rect.y1 * scale_y,
		in_rect.x2 * scale_x, in_rect.y2 * scale_y);
}

rectf_t
mk_rectf(float x1, float y1, float x2, float y2)
{
	rectf_t rectangle;

	rectangle.x1 = x1;
	rectangle.y1 = y1;
	rectangle.x2 = x2;
	rectangle.y2 = y2;
	return rectangle;
}

rectf_t
rectf_scale(rectf_t in_rect, float x_scale, float y_scale)
{
	return mk_rectf(
		in_rect.x1 * x_scale, in_rect.y1 * y_scale,
		in_rect.x2 * x_scale, in_rect.y2 * y_scale);
}

rectf_t
rectf_translate(rectf_t rect, float x_offset, float y_offset)
{
	return mk_rectf(
		rect.x1 + x_offset, rect.y1 + y_offset,
		rect.x2 + x_offset, rect.y2 + y_offset);
}

size2_t
mk_size2(int width, int height)
{
	size2_t size;

	size.width = width;
	size.height = height;
	return size;
}
