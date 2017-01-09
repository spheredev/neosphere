#include "cell.h"
#include "build.h"

#include "fs.h"
#include "spk_writer.h"

struct build
{
	fs_t*     fs;
	vector_t* targets;
};

struct install
{
	path_t*   path;
	target_t* target;
};

build_t*
build_new(const path_t* in_path, const path_t* out_path)
{
	build_t* build = NULL;

	build = calloc(1, sizeof(build_t));
	build->fs = fs_new(path_cstr(in_path), path_cstr(out_path), NULL);
	build->targets = vector_new(sizeof(target_t*));
	return build;
}

void
build_free(build_t* build)
{
	iter_t iter;
	target_t* *p;
	
	if (build == NULL)
		return;
	iter = vector_enum(build->targets);
	while (p = vector_next(&iter)) {
		target_free(*p);
	}
	vector_free(build->targets);
	free(build);
}

fs_t*
build_fs(const build_t* build)
{
	return build->fs;
}

void
build_add_target(build_t* build, target_t* target)
{
	target_ref(target);
	vector_push(build->targets, &target);
}

void
build_run(build_t* build)
{
	int           num_built = 0;
	const path_t* path;
	
	iter_t iter;
	target_t* *p;

	printf("processing %zu targets...\n", vector_len(build->targets));
	iter = vector_enum(build->targets);
	while (p = vector_next(&iter)) {
		path = target_path(*p);
		if (path_num_hops(path) == 0 || !path_hop_cmp(path, 0, "@"))
			continue;
		target_build(*p, build->fs);
	}

	printf("writing game.json... ");
	printf("OK.\n");
}
