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
	tool_t*      tool;
	bool         tracked;
};

target_t*
target_new(const path_t* name, fs_t* fs, const path_t* path, tool_t* tool, bool tracked)
{
	target_t* target;

	target = calloc(1, sizeof(target_t));
	target->name = path_dup(name);
	target->fs = fs;
	target->path = path_dup(path);
	target->sources = vector_new(sizeof(target_t*));
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
	while (p = vector_next(&iter))
		target_free(*p);
	vector_free(target->sources);
	tool_free(target->tool);
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
	bool        status;
	struct stat sb;

	iter_t iter;
	path_t*   *p_path;
	target_t* *p_target;

	// build dependencies and add them to the source list
	in_paths = vector_new(sizeof(path_t*));
	iter = vector_enum(target->sources);
	while (p_target = vector_next(&iter)) {
		target_build(*p_target, visor, force_build);
		path = path_dup(target_path(*p_target));
		vector_push(in_paths, &path);
	}

	if (target->tracked)
		visor_add_file(visor, path_cstr(target->path));

	// check whether the output file is out of date with respect to its sources.
	// this is a simple timestamp comparison for now; it might be good to eventually
	// switch to a hash-based solution like in SCons.
	if (force_build)
		is_outdated = true;
	else {
		if (fs_stat(target->fs, path_cstr(target->path), &sb) == 0)
			last_time = sb.st_mtime;
		iter = vector_enum(in_paths);
		while (p_path = vector_next(&iter)) {
			fs_stat(target->fs, path_cstr(*p_path), &sb);
			if (is_outdated = sb.st_mtime > last_time)
				break;  // short circuit
		}
	}

	// build the target if it's out of date
	if (is_outdated) {
		status = tool_run(target->tool, visor, target->fs, target->path, in_paths);
		iter = vector_enum(in_paths);
		while (p_path = vector_next(&iter))
			path_free(*p_path);
		vector_free(in_paths);
		return status;
	}
	else {
		return true;
	}
}
