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
#include "target.h"

#include "fs.h"
#include "tool.h"
#include "visor.h"

struct target
{
	unsigned int refcount;
	path_t*      name;
	fs_t*        fs;
	path_t*      path;
	vector_t*    sources;
	time_t       timestamp;
	tool_t*      tool;
	bool         tracked;
};

target_t*
target_new(const path_t* name, fs_t* fs, const path_t* path, tool_t* tool, time_t timestamp, bool tracked)
{
	target_t* target;

	if (!(target = calloc(1, sizeof(target_t))))
		return NULL;
	if (name != NULL)
		target->name = path_dup(name);
	target->fs = fs;
	target->path = path_dup(path);
	target->sources = vector_new(sizeof(target_t*));
	target->timestamp = timestamp;
	target->tool = tool_ref(tool);
	target->tracked = tracked;
	return target_ref(target);
}

target_t*
target_ref(target_t* target)
{
	++target->refcount;
	return target;
}

void
target_free(target_t* target)
{
	iter_t iter;
	target_t* *p;

	if (--target->refcount > 0)
		return;

	iter = vector_enum(target->sources);
	while ((p = iter_next(&iter)))
		target_free(*p);
	vector_free(target->sources);
	tool_unref(target->tool);
	path_free(target->name);
	free(target);
}

const path_t*
target_name(const target_t* target)
{
	return target->name;
}

const path_t*
target_path(const target_t* target)
{
	return target->path;
}

const path_t*
target_source_path(const target_t* target)
{
	target_t* source;

	if (vector_len(target->sources) != 1)
		return NULL;
	source = *(target_t**)vector_get(target->sources, 0);
	if (source->tool != NULL)
		return NULL;
	return target_path(source);
}

void
target_add_source(target_t* target, target_t* source)
{
	target_ref(source);
	vector_push(target->sources, &source);
}

bool
target_build(target_t* target, visor_t* visor, bool force_build)
{
	vector_t*   in_paths = NULL;
	bool        is_outdated = false;
	time_t      last_time = 0;
	path_t*     path;
	path_t**    path_ptr;
	bool        status;
	struct stat sb;
	target_t**  target_ptr;

	iter_t iter;

	// build dependencies and add them to the source list
	in_paths = vector_new(sizeof(path_t*));
	iter = vector_enum(target->sources);
	while ((target_ptr = iter_next(&iter))) {
		target_build(*target_ptr, visor, force_build);
		path = path_dup(target_path(*target_ptr));
		vector_push(in_paths, &path);
	}

	if (target->tracked)
		visor_add_file(visor, path_cstr(target->path));

	if (target->tracked && vector_len(target->sources) == 0) {
		visor_warn(visor, "always up-to-date: '%s' (no sources)", path_cstr(target->path));
		return true;
	}

	// check whether the output file is out of date with respect to its sources.
	// this is a simple timestamp comparison for now; it might be good to eventually
	// switch to a hash-based solution like in SCons.
	if (fs_stat(target->fs, path_cstr(target->path), &sb) == 0)
		last_time = sb.st_mtime;
	if (force_build || (sb.st_mode & S_IFDIR) == S_IFDIR || target->timestamp > last_time) {
		is_outdated = true;
	}
	else {
		iter = vector_enum(in_paths);
		while ((path_ptr = iter_next(&iter))) {
			fs_stat(target->fs, path_cstr(*path_ptr), &sb);
			if ((is_outdated = sb.st_mtime > last_time))
				break;  // short circuit
		}
	}

	// build the target if it's out of date
	status = is_outdated
		? tool_run(target->tool, visor, target->fs, target->path, in_paths)
		: true;
	iter = vector_enum(in_paths);
	while ((path_ptr = iter_next(&iter)))
		path_free(*path_ptr);
	vector_free(in_paths);
	return status;
}
