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

#include "cell.h"
#include "image.h"

#include <png.h>
#include "fs.h"

struct image
{
	int       height;
	uint32_t* pixels;
	int       width;
};

image_t*
image_load(const fs_t* fs, const char* pathname)
{
	png_byte    bit_depth;
	png_byte    color_type;
	FILE*       file = NULL;
	int         height;
	image_t*    image;
	uint32_t*   pixels;
	png_structp png;
	png_infop   png_info;
	png_bytep*  row_ptrs;
	int         width;

	int i;

	if (!(image = calloc(1, sizeof(image_t))))
		goto on_error;
	
	if (!(file = fs_fopen(fs, pathname, "rb")))
		goto on_error;
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_info = png_create_info_struct(png);
	if (setjmp(png_jmpbuf(png)) == 0) {
		png_init_io(png, file);
		png_read_info(png, png_info);
		width = (int)png_get_image_width(png, png_info);
		height = (int)png_get_image_height(png, png_info);
		color_type = png_get_color_type(png, png_info);
		bit_depth = png_get_bit_depth(png, png_info);

		if (bit_depth == 16)
			png_set_strip_16(png);
		if (color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png);
		if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
			png_set_expand_gray_1_2_4_to_8(png);
		if (png_get_valid(png, png_info, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png);
		if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
		if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png);
		png_read_update_info(png, png_info);

		pixels = malloc(width * height * sizeof(uint32_t));
		row_ptrs = alloca(height * sizeof(png_bytep));
		for (i = 0; i < height; ++i)
			row_ptrs[i] = (png_bytep)(pixels + i * width);
		png_read_image(png, row_ptrs);
	}
	else {
		png_destroy_read_struct(&png, &png_info, NULL);
		goto on_error;
	}

	png_destroy_read_struct(&png, &png_info, NULL);
	fclose(file);

	image->width = width;
	image->height = height;
	image->pixels = pixels;
	return image;

on_error:
	if (file != NULL)
		fclose(file);
	free(image);
	return NULL;
}

void
image_free(image_t* image)
{
	if (image == NULL)
		return;
	free(image->pixels);
	free(image);
}

uint32_t*
image_bitmap(const image_t* image, size_t *out_size)
{
	if (out_size != NULL)
		*out_size = image->width * image->height * sizeof(uint32_t);
	return image->pixels;
}

int
image_height(const image_t* image)
{
	return image->height;
}

int
image_width(const image_t* image)
{
	return image->width;
}

bool
image_save(const image_t* it, const fs_t* fs, const char* pathname)
{
	FILE*       file;
	png_structp png;
	png_infop   png_info;
	png_bytep*  row_ptrs;

	int i;

	if (!(file = fs_fopen(fs, pathname, "wb")))
		return false;

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_info = png_create_info_struct(png);

	if (setjmp(png_jmpbuf(png)) == 0) {
		row_ptrs = alloca(it->height * sizeof(png_bytep));
		for (i = 0; i < it->height; ++i)
			row_ptrs[i] = (png_bytep)(it->pixels + i * it->width);
		png_init_io(png, file);
		png_set_IHDR(png, png_info,
			it->width, it->height, 8,
			PNG_COLOR_TYPE_RGBA,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png, png_info);
		png_write_image(png, row_ptrs);
		png_write_end(png, png_info);
		png_destroy_write_struct(&png, &png_info);
		fclose(file);
		return true;
	}
	else {
		fclose(file);
		png_destroy_write_struct(&png, &png_info);
		return false;
	}
}

image_t*
image_slice(const image_t* image, int x, int y, int width, int height)
{
	uint32_t* in;
	uint32_t* out;
	image_t*  slice;
	size_t    stride;

	int i;

	if (!(slice = calloc(1, sizeof(image_t))))
		goto on_error;
	if (!(slice->pixels = malloc(width * height * sizeof(uint32_t))))
		goto on_error;
	slice->width = width;
	slice->height = height;
	stride = width * sizeof(uint32_t);
	for (i = 0; i < height; ++i) {
		in = image->pixels + (y + i) * image->width + x;
		out = slice->pixels + i * width;
		memcpy(out, in, stride);
	}
	return slice;

on_error:
	free(slice);
	return NULL;
}
