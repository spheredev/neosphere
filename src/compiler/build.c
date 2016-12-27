#include "cell.h"
#include "build.h"

#include "spk_writer.h"

struct build
{
	vector_t* installs;
	path_t*   in_path;
	path_t*   out_path;
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
	build->installs = vector_new(sizeof(struct install));
	build->in_path = path_dup(in_path);
	build->out_path = path_dup(out_path);
	return build;
}

void
build_free(build_t* build)
{
	iter_t iter;
	struct install *p;
	
	if (build == NULL)
		return;
	iter = vector_enum(build->installs);
	while (p = vector_next(&iter)) {
		target_free(p->target);
		path_free(p->path);
	}
	vector_free(build->installs);
	free(build);
}

const path_t*
build_in_path(const build_t* build)
{
	return build->in_path;
}

const path_t*
build_out_path(const build_t* build)
{
	return build->out_path;
}

void
build_run(build_t* build)
{
	path_t*       asset_path;
	path_t*       install_path;
	const path_t* name;
	spk_writer_t* spk = NULL;

	iter_t iter;
	struct install *p;

	if (path_is_file(build->out_path))
		spk = spk_create(path_cstr(build->out_path));

	iter = vector_enum(build->installs);
	while (p = vector_next(&iter)) {
		target_build(p->target, build->out_path);
		name = target_name(p->target);
		asset_path = path_rebase(path_dup(target_path(p->target)), build->in_path);
		install_path = path_rebase(path_dup(name), p->path);
		printf("%s => %s\n", path_cstr(name), path_cstr(install_path));
		if (spk != NULL) {
			spk_add_file(spk,
				path_cstr(asset_path),
				path_cstr(install_path));
		}
		else {
			path_rebase(install_path, build->out_path);
			// TODO: install built targets
		}
		path_free(install_path);
	}

	spk_close(spk);
}

void
build_install(build_t* build, target_t* target, const path_t* path)
{
	struct install install;
	
	install.target = target_ref(target);
	install.path = path_dup(path);
	vector_push(build->installs, &install);
}
