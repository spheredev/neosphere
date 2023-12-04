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
#include "obstruction.h"

struct obsmap
{
	unsigned int id;
	rect_t*      lines;
	int          max_lines;
	int          num_lines;
};

static unsigned int s_next_obsmap_id = 0;

obsmap_t*
obsmap_new(void)
{
	obsmap_t* obsmap = NULL;

	console_log(4, "creating new obstruction map #%u", s_next_obsmap_id);

	if (!(obsmap = calloc(1, sizeof(obsmap_t))))
		return NULL;
	obsmap->max_lines = 0;
	obsmap->num_lines = 0;

	obsmap->id = s_next_obsmap_id++;
	return obsmap;
}

void
obsmap_free(obsmap_t* obsmap)
{
	if (obsmap == NULL)
		return;
	console_log(4, "disposing obstruction map #%u no longer in use", obsmap->id);
	free(obsmap->lines);
	free(obsmap);
}

bool
obsmap_add_line(obsmap_t* obsmap, rect_t line)
{
	int    new_size;
	rect_t *line_list;

	console_log(4, "adding line segment (%d,%d)-(%d,%d) to obstruction map #%u",
		line.x1, line.y1, line.x2, line.y2, obsmap->id);

	if (obsmap->num_lines + 1 > obsmap->max_lines) {
		new_size = (obsmap->num_lines + 1) * 2;
		if ((line_list = realloc(obsmap->lines, new_size * sizeof(rect_t))) == NULL)
			return false;
		obsmap->max_lines = new_size;
		obsmap->lines = line_list;
	}
	obsmap->lines[obsmap->num_lines] = line;
	++obsmap->num_lines;
	return true;
}

bool
obsmap_test_line(const obsmap_t* obsmap, rect_t line)
{
	int i;

	for (i = 0; i < obsmap->num_lines; ++i) {
		if (do_lines_overlap(line, obsmap->lines[i]))
			return true;
	}
	return false;
}

bool
obsmap_test_rect(const obsmap_t* obsmap, rect_t rectangle)
{
	// this treats 'rect' as hollow, which differs from the usual treatment of rectangles
	// in the engine but matches the behavior of Sphere 1.x.
	return obsmap_test_line(obsmap, mk_rect(rectangle.x1, rectangle.y1, rectangle.x2, rectangle.y1))
		|| obsmap_test_line(obsmap, mk_rect(rectangle.x2, rectangle.y1, rectangle.x2, rectangle.y2))
		|| obsmap_test_line(obsmap, mk_rect(rectangle.x1, rectangle.y2, rectangle.x2, rectangle.y2))
		|| obsmap_test_line(obsmap, mk_rect(rectangle.x1, rectangle.y1, rectangle.x1, rectangle.y2));
}
