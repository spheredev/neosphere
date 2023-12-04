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

#ifndef SPHERE__PATH_H__INCLUDED
#define SPHERE__PATH_H__INCLUDED

#include <stdbool.h>
#include <stddef.h>

typedef struct path path_t;

path_t*     path_new          (const char* pathname);
path_t*     path_new_dir      (const char* pathname);
path_t*     path_new_self     (void);
path_t*     path_dup          (const path_t* path);
void        path_free         (path_t* path);
const char* path_cstr         (const path_t* path);
const char* path_extension    (const path_t* path);
const char* path_filename     (const path_t* path);
const char* path_hop          (const path_t* path, int idx);
path_t*     path_insert_hop   (path_t* path, int idx, const char* name);
bool        path_extension_is (const path_t* path, const char* extension);
bool        path_is_file      (const path_t* path);
bool        path_rooted       (const path_t* path);
int         path_num_hops     (const path_t* path);
bool        path_filename_is  (const path_t* path, const char* name);
bool        path_hop_is       (const path_t* path, int idx, const char* name);
path_t*     path_append       (path_t* path, const char* pathname);
path_t*     path_append_dir   (path_t* path, const char* pathname);
path_t*     path_cat          (path_t* path, const path_t* tail);
path_t*     path_change_name  (path_t* path, const char* filename);
bool        path_is           (const path_t* path1, const path_t* path2);
path_t*     path_collapse     (path_t* path, bool collapse_uplevel);
bool        path_mkdir        (const path_t* path);
path_t*     path_rebase       (path_t* path, const path_t* root);
path_t*     path_relativize   (path_t* path, const path_t* origin);
path_t*     path_remove_hop   (path_t* path, int idx);
path_t*     path_resolve      (path_t* path, const path_t* relative_to);
path_t*     path_set          (path_t* path, const char* pathname);
path_t*     path_set_dir      (path_t* path, const char* pathname);
path_t*     path_strip        (path_t* path);
path_t*     path_to_dir       (path_t* path);

#endif // SPHERE__PATH_H__INCLUDED
