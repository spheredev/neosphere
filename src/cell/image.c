/**
 *  Cell, the Sphere packaging compiler
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

#include "image.h"

#include "cell.h"

struct image
{
	uint32_t*  pixels;
	size_t     pitch;
	png_image* png;
};

image_t*
image_open(const path_t* path)
{
	image_t*   image;
	uint32_t*  pixelbuf = NULL;
	png_image* png;

	image = calloc(1, sizeof(image_t));

	png = calloc(1, sizeof(png_image));
	png->version = PNG_IMAGE_VERSION;
	png->format = PNG_FORMAT_RGBA;
	if (!png_image_begin_read_from_file(png, path_cstr(path)))
		goto on_error;
	pixelbuf = malloc(PNG_IMAGE_SIZE(*png));
	if (!png_image_finish_read(png, NULL, pixelbuf, 0, NULL))
		goto on_error;

	image->png = png;
	image->pixels = pixelbuf;
	image->pitch = PNG_IMAGE_ROW_STRIDE(*png) / 4;
	return image;

on_error:
	free(pixelbuf);
	png_image_free(png);
	return NULL;
}

void
image_close(image_t* image)
{
	if (image == NULL)
		return;
	png_image_free(image->png);
	free(image->pixels);
	free(image);
}

const ptrdiff_t
image_get_pitch(const image_t* image)
{
	return image->pitch;
}

const uint32_t*
image_get_pixelbuf(const image_t* image)
{
	return image->pixels;
}

int
image_get_height(const image_t* image)
{
	return image->png->height;
}

int
image_get_width(const image_t* image)
{
	return image->png->width;
}
