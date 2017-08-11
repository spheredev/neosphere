/**
 *  miniSphere JavaScript game engine
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

#ifndef MINISPHERE__SPK_H__INCLUDED
#define MINISPHERE__SPK_H__INCLUDED

typedef struct spk      spk_t;
typedef struct spk_file spk_file_t;

typedef
enum spk_seek_origin
{
	SPK_SEEK_SET,
	SPK_SEEK_CUR,
	SPK_SEEK_END
} spk_seek_origin_t;

spk_t*      open_spk           (const char* path);
spk_t*      ref_spk            (spk_t* spk);
void        unref_spk          (spk_t* spk);
vector_t*   list_spk_filenames (spk_t* spk, const char* dirname, bool want_dirs);

bool        spk_dir_exists (const spk_t* spk, const char* dirname);
spk_file_t* spk_fopen  (spk_t* spk, const char* path, const char* mode);
void        spk_fclose (spk_file_t* file);
int         spk_fputc  (int ch, spk_file_t* file);
int         spk_fputs  (const char* string, spk_file_t* file);
size_t      spk_fread  (void* buf, size_t size, size_t count, spk_file_t* file);
bool        spk_fseek  (spk_file_t* file, long long offset, spk_seek_origin_t origin);
void*       spk_fslurp (spk_t* spk, const char* path, size_t *out_size);
long long   spk_ftell  (spk_file_t* file);
size_t      spk_fwrite (const void* buf, size_t size, size_t count, spk_file_t* file);

#endif // MINISPHERE__SPK_H__INCLUDED
