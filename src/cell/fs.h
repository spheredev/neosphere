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

#ifndef SPHERE__FS_H__INCLUDED
#define SPHERE__FS_H__INCLUDED

typedef struct directory directory_t;
typedef struct fs        fs_t;

fs_t*         fs_new              (const char* origin_dir, const char* game_dir, const char* home_dir);
void          fs_free             (fs_t* fs);
bool          fs_dir_exists       (const fs_t* fs, const char* dirname);
path_t*       fs_full_path        (const char* filename, const char* base_dir_name);
path_t*       fs_relative_path    (const char* filename, const char* base_dir_name);
int           fs_fcopy            (const fs_t* fs, const char* destination, const char* source, int overwrite);
bool          fs_fexist           (const fs_t* fs, const char* filename);
FILE*         fs_fopen            (const fs_t* fs, const char* filename, const char* mode);
void*         fs_fslurp           (const fs_t* fs, const char* filename, size_t* out_size);
bool          fs_fspew            (const fs_t* fs, const char* filename, const void* data, size_t size);
bool          fs_is_game_dir      (const fs_t* fs, const char* dirname);
bool          fs_is_prefix_path   (const fs_t* it, const char* pathname);
vector_t*     fs_list_dir         (const fs_t* fs, const char* dirname, bool recursive);
int           fs_mkdir            (const fs_t* fs, const char* dirname);
int           fs_rename           (const fs_t* fs, const char* old_name, const char* new_name);
int           fs_rmdir            (const fs_t* fs, const char* dirname);
int           fs_stat             (const fs_t* fs, const char* filename, struct stat* p_stat);
int           fs_unlink           (const fs_t* fs, const char* filename);
int           fs_utime            (const fs_t* fs, const char* filename, struct utimbuf* in_times);
directory_t*  directory_open      (fs_t* fs, const char* dirname, bool recursive);
void          directory_close     (directory_t* it);
int           directory_num_files (directory_t* it);
const path_t* directory_path      (const directory_t* it);
const char*   directory_pathname  (const directory_t* it);
int           directory_position  (const directory_t* it);
const path_t* directory_next      (directory_t* it);
void          directory_rewind    (directory_t* it);
bool          directory_seek      (directory_t* it, int position);

#endif // SPHERE__FS_H__INCLUDED
