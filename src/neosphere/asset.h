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

#ifndef SPHERE__ASSET_H__INCLUDED
#define SPHERE__ASSET_H__INCLUDED

#include "audio.h"
#include "image.h"

typedef enum asset_type
{
	ASSET_SOUND,
	ASSET_TEXTURE,
} asset_type_t;

typedef struct asset asset_t;

asset_t*     asset_from_file  (const char* filename, asset_type_t type);
asset_t*     asset_from_image (image_t* image);
asset_t*     asset_from_sound (sound_t* sound);
asset_t*     asset_ref        (asset_t* it);
void         asset_unref      (asset_t* it);
const char*  asset_error      (const asset_t* it);
const char*  asset_filename   (const asset_t* it);
image_t*     asset_image      (const asset_t* it);
void*        asset_resource   (const asset_t* it);
sound_t*     asset_sound      (const asset_t* it);
asset_type_t asset_type       (const asset_t* it);
bool         asset_load       (asset_t* it);

#endif // SPHERE__ASSET_H__INCLUDED
