/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2022, Fat Cerberus
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
#include "loader.h"

struct texture
{
	unsigned int refcount;
	char*        error_text;
	char*        filename;
	image_t*     image;
};

texture_t*
texture_from_file(const char* filename)
{
	texture_t* texture;

	if (!(texture = calloc(1, sizeof(texture_t))))
		return NULL;
	texture->filename = strdup(filename);
	return texture_ref(texture);
}

texture_t*
texture_from_image(image_t* image)
{
	texture_t* texture;

	if (!(texture = calloc(1, sizeof(texture_t))))
		return NULL;
	texture->image = image_ref(image);
	return texture_ref(texture);
}

texture_t*
texture_ref(texture_t* it)
{
	if (it != NULL)
		++it->refcount;
	return it;
}

void
texture_unref(texture_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	image_unref(it->image);
	free(it->error_text);
	free(it->filename);
	free(it);
}

const char*
texture_error(const texture_t* it)
{
	return it->error_text;
}

const char*
texture_filename(const texture_t* it)
{
	return it->filename;
}

image_t*
texture_image(const texture_t* it)
{
	return it->image;
}

bool
texture_load(texture_t* it)
{
	image_t* image;
	
	if (it->image != NULL)
		return true;
	if (it->error_text != NULL)
		return false;
	if ((image = image_load(it->filename))) {
		it->image = image;
		return true;
	}
	else {
		it->error_text = strnewf("Unable to load an image from '%s'.", it->filename);
		return false;
	}
}
