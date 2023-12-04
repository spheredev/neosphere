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

#ifndef SPHERE__PACKAGE_H__INCLUDED
#define SPHERE__PACKAGE_H__INCLUDED

typedef struct asset    asset_t;
typedef struct package  package_t;

typedef
enum spk_seek_origin
{
	SPK_SEEK_SET,
	SPK_SEEK_CUR,
	SPK_SEEK_END
} spk_seek_origin_t;

package_t*  package_open        (const char* filename);
package_t*  package_ref         (package_t* it);
void        package_unref       (package_t* it);
bool        package_dir_exists  (const package_t* it, const char* dirname);
bool        package_file_exists (const package_t* it, const char* filename);
vector_t*   package_list_dir    (package_t* it, const char* dirname, bool want_dirs, bool recursive);
asset_t*    asset_fopen         (package_t* package, const char* path, const char* mode);
void        asset_fclose        (asset_t* file);
int         asset_fputc         (int ch, asset_t* file);
int         asset_fputs         (const char* string, asset_t* file);
size_t      asset_fread         (void* buf, size_t size, size_t count, asset_t* file);
bool        asset_fseek         (asset_t* file, long long offset, spk_seek_origin_t origin);
void*       asset_fslurp        (package_t* it, const char* path, size_t *out_size);
long long   asset_ftell         (asset_t* file);
size_t      asset_fwrite        (const void* buf, size_t size, size_t count, asset_t* file);

#endif // SPHERE__PACKAGE_H__INCLUDED
