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
#include "asset.h"

#include "audio.h"
#include "image.h"

struct asset
{
	unsigned int refcount;
	asset_type_t type;
	char*        error_text;
	char*        filename;
	void*        resource;
};

asset_t*
asset_from_file(const char* filename, asset_type_t type)
{
	asset_t* asset;

	if (!(asset = calloc(1, sizeof(asset_t))))
		return NULL;
	asset->type = type;
	asset->filename = strdup(filename);
	return asset_ref(asset);
}

asset_t*
asset_from_image(image_t* image)
{
	asset_t* asset;

	if (!(asset = calloc(1, sizeof(asset_t))))
		return NULL;
	asset->type = ASSET_TEXTURE;
	asset->resource = image_ref(image);
	return asset_ref(asset);
}

asset_t*
asset_from_sound(sound_t* sound)
{
	asset_t* asset;

	if (!(asset = calloc(1, sizeof(asset_t))))
		return NULL;
	asset->type = ASSET_SOUND;
	asset->resource = sound_ref(sound);
	return asset_ref(asset);
}

asset_t*
asset_ref(asset_t* it)
{
	if (it != NULL)
		++it->refcount;
	return it;
}

void
asset_unref(asset_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	switch (it->type) {
	case ASSET_TEXTURE:
		image_unref(it->resource);
		break;
	}
	free(it->error_text);
	free(it->filename);
	free(it);
}

const char*
asset_error(const asset_t* it)
{
	return it->error_text;
}

const char*
asset_filename(const asset_t* it)
{
	return it->filename;
}

image_t*
asset_image(const asset_t* it)
{
	return it->type == ASSET_TEXTURE ? it->resource : NULL;
}

void*
asset_resource(const asset_t* it)
{
	return it->resource;
}

sound_t*
asset_sound(const asset_t* it)
{
	return it->type == ASSET_SOUND ? it->resource : NULL;
}

asset_type_t
asset_type(const asset_t* it)
{
	return it->type;
}

bool
asset_load(asset_t* it)
{
	image_t* image;
	sound_t* sound;

	if (it->resource != NULL)
		return true;
	if (it->error_text != NULL)
		return false;
	switch (it->type) {
	case ASSET_SOUND:
		if ((sound = sound_new(it->filename))) {
			it->resource = sound;
			return true;
		}
		else {
			it->error_text = strnewf("Unable to load audio from '%s'.", it->filename);
			return false;
		}
		break;
	case ASSET_TEXTURE:
		if ((image = image_load(it->filename))) {
			it->resource = image;
			return true;
		}
		else {
			it->error_text = strnewf("Unable to load image from '%s'.", it->filename);
			return false;
		}
		break;
	default:
		return false;
	}
}
