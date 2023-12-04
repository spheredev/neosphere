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

#ifndef SPHERE__GAME_H__INCLUDED
#define SPHERE__GAME_H__INCLUDED

#include "font.h"
#include "geometry.h"
#include "image.h"
#include "jsal.h"
#include "windowstyle.h"

typedef struct directory directory_t;
typedef struct file      file_t;
typedef struct game      game_t;

typedef
enum fs_safety
{
	FS_SAFETY_NONE,
	FS_SAFETY_RELAXED,
	FS_SAFETY_FULL,
} fs_safety_t;

typedef
enum whence
{
	WHENCE_SET,
	WHENCE_CUR,
	WHENCE_END,
} whence_t;

game_t*         game_open                (const char* game_path);
game_t*         game_ref                 (game_t* it);
void            game_unref               (game_t* it);
int             game_api_level           (const game_t* it);
const char*     game_author              (const game_t* it);
const char*     game_compiler            (const game_t* it);
image_t*        game_default_arrow       (const game_t* it);
image_t*        game_default_arrow_down  (const game_t* it);
image_t*        game_default_arrow_up    (const game_t* it);
font_t*         game_default_font        (const game_t* it);
windowstyle_t*  game_default_windowstyle (const game_t* it);
bool            game_dir_exists          (const game_t* it, const char* dirname);
bool            game_file_exists         (const game_t* it, const char* filename);
path_t*         game_full_path           (const game_t* it, const char* filename, const char* base_dir_name, bool v1_mode);
bool            game_fullscreen          (const game_t* it);
const js_ref_t* game_manifest            (const game_t* it);
const char*     game_name                (const game_t* it);
const path_t*   game_path                (const game_t* it);
path_t*         game_relative_path       (const game_t* it, const char* filename, const char* base_dir_name);
size2_t         game_resolution          (const game_t* it);
const char*     game_save_id             (const game_t* it);
const path_t*   game_script_path         (const game_t* it);
const char*     game_summary             (const game_t* it);
int             game_version             (const game_t* it);
bool            game_is_prefix_path      (const game_t* it, const char* pathname);
bool            game_is_writable         (const game_t* it, const char* pathname, bool v1_mode);
bool            game_mkdir               (game_t* it, const char* dirname);
void*           game_read_file           (game_t* it, const char* filename, size_t *out_size);
bool            game_rename              (game_t* it, const char* old_pathname, const char* new_pathname);
bool            game_rmdir               (game_t* it, const char* dirname);
bool            game_unlink              (game_t* it, const char* filename);
bool            game_write_file          (game_t* it, const char* filename, const void* buf, size_t size);
directory_t*    directory_open           (game_t* game, const char* dirname, bool recursive);
void            directory_close          (directory_t* it);
int             directory_num_files      (directory_t* it);
const path_t*   directory_path           (const directory_t* it);
const char*     directory_pathname       (const directory_t* it);
int             directory_position       (const directory_t* it);
const path_t*   directory_next           (directory_t* it);
void            directory_rewind         (directory_t* it);
bool            directory_seek           (directory_t* it, int position);
file_t*         file_open                (game_t* game, const char* filename, const char* mode);
void            file_close               (file_t* it);
const char*     file_pathname            (const file_t* it);
long long       file_position            (const file_t* it);
int             file_puts                (file_t* it, const char* string);
size_t          file_read                (file_t* it, void* buf, size_t count, size_t size);
bool            file_seek                (file_t* it, long long offset, whence_t whence);
size_t          file_write               (file_t* it, const void* buf, size_t count, size_t size);

#endif // SPHERE__GAME_H__INCLUDED
