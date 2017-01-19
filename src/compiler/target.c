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
};

target_t*
target_new(const path_t* name, fs_t* fs, const path_t* path, tool_t* tool)
{
	target_t* target;

	target = calloc(1, sizeof(target_t));
	target->name = path_dup(name);
	target->fs = fs;
	target->path = path_dup(path);
	target->sources = vector_new(sizeof(target_t*));
	target->tool = tool_ref(tool);
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

void
target_add_source(target_t* target, target_t* source)
{
	target_ref(source);
	vector_push(target->sources, &source);
}

bool
target_build(target_t* target, visor_t* visor, bool forced)
{
	vector_t* in_paths = NULL;
	path_t*   path;
	bool      status;

	iter_t iter;
	path_t*   *p_path;
	target_t* *p_target;

	// build dependencies and add them to the source list
	in_paths = vector_new(sizeof(path_t*));
	iter = vector_enum(target->sources);
	while (p_target = vector_next(&iter)) {
		target_build(*p_target, visor, forced);
		path = path_dup(target_path(*p_target));
		vector_push(in_paths, &path);
	}

	// now build the target
	status = tool_run(target->tool, visor, target->fs, target->path, in_paths, forced);
	iter = vector_enum(in_paths);
	while (p_path = vector_next(&iter))
		path_free(*p_path);
	vector_free(in_paths);
	return status;
}
