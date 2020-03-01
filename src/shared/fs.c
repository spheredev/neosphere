/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2020, Fat Cerberus
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

#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "fs.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "vector.h"

struct fs
{
	vector_t*          aliases;
	fs_fslurp_impl_t   fslurp_impl;
	char*              prefixes;
	fs_stat_impl_t     stat_impl;
	void*              user_ptr;
};

struct alias
{
	char    prefix;
	path_t* path;
};

fs_t*
fs_new(const char* prefixes, void* userdata)
{
	fs_t* fs;

	if (!(fs = calloc(1, sizeof(fs_t))))
		return NULL;
	fs->user_ptr = userdata;
	fs->prefixes = strdup(prefixes);
	fs->aliases = vector_new(sizeof(struct alias));
	return fs;
}

void
fs_free(fs_t* it)
{
	struct alias* alias;

	iter_t iter;

	iter = vector_enum(it->aliases);
	while (alias = iter_next(&iter))
		path_free(alias->path);
	vector_free(it->aliases);
	free(it->prefixes);
	free(it);
}

void
fs_define_alias(fs_t* it, char prefix, const char* base_dir)
{
	struct alias alias;

	alias.prefix = prefix;
	alias.path = fs_pathname(it, base_dir, NULL);
	vector_push(it->aliases, &alias);
}

void
fs_on_fslurp(fs_t* it, fs_fslurp_impl_t impl)
{
	it->fslurp_impl = impl;
}

void
fs_on_stat(fs_t* it, fs_stat_impl_t impl)
{
	it->stat_impl = impl;
}

void*
fs_user_ptr(const fs_t* it)
{
	return it->user_ptr;
}

bool
fs_dir_exists(fs_t* it, const char* filename)
{
	struct stat stat;

	if (fs_stat(it, filename, &stat) != 0)
		return false;
	return (stat.st_mode & S_IFDIR) == S_IFDIR;
}

bool
fs_file_exists(fs_t* it, const char* filename)
{
	struct stat stat;
	
	if (fs_stat(it, filename, &stat) != 0)
		return false;
	return (stat.st_mode & S_IFREG) == S_IFREG;
}

void*
fs_fslurp(fs_t* it, const char* filename, size_t* out_size)
{
	return it->fslurp_impl(it, filename, out_size);
}

int
fs_stat(fs_t* it, const char* filename, struct stat* out_stat)
{
	return it->stat_impl(it, filename, out_stat);
}

path_t*
fs_pathname(fs_t* it, const char* filename, const char* base_dir)
{
	// note: '../' path hops are collapsed unconditionally, per SphereFS specification.
	//       this ensures the game can't subvert its sandbox by navigating outside through
	//       a symbolic link.

	struct alias* alias;
	path_t*       base_path = NULL;
	const char*   first_hop;
	bool          has_prefix = false;
	path_t*       path;
	char          prefix[2];

	iter_t iter;

	path = path_new(filename);

	// don't touch absolute paths
	if (path_rooted(path))
		return path;

	// first canonicalize the provided base directory path
	if (base_dir != NULL) {
		base_path = fs_pathname(it, base_dir, NULL);
		path_to_dir(base_path);
	}

	// if there's no prefix, the path is relative and we need to rebase it
	if (path_num_hops(path) > 0) {
		first_hop = path_hop(path, 0);
		if (strlen(first_hop) == 1 && strpbrk(first_hop, it->prefixes)) {
			strcpy(prefix, first_hop);
			has_prefix = true;
		}
	}
	if (!has_prefix) {
		if (base_path != NULL)
			path_rebase(path, base_path);
		else
			path_insert_hop(path, 0, "@");
		strcpy(prefix, path_hop(path, 0));
	}

	// resolve aliases to their canonical representation
	iter = vector_enum(it->aliases);
	while (alias = iter_next(&iter)) {
		if (prefix[0] == alias->prefix) {
			path_remove_hop(path, 0);
			path_rebase(path, alias->path);
			strcpy(prefix, path_hop(path, 0));
		}
	}

	path_remove_hop(path, 0);
	path_collapse(path, true);
	path_insert_hop(path, 0, prefix);
	path_free(base_path);

	if (path_is_file(path) && fs_dir_exists(it, path_cstr(path)))
		path_to_dir(path);
	return path;
}
