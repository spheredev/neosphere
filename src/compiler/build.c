#include "cell.h"
#include "build.h"

#include "fs.h"
#include "spk_writer.h"
#include "visor.h"

struct build
{
	fs_t*        fs;
	duk_context* js;
	vector_t*    targets;
	visor_t*     visor;
};

build_t*
build_new(const path_t* in_path, const path_t* out_path)
{
	build_t*     build = NULL;
	duk_context* js;
	path_t*      tmp_path;
	visor_t*     visor;

	// note: Duktape is initialized without a fatal error handler.  if a JavaScript
	//       exception is thrown and not caught, Cell will crash.
	js = duk_create_heap_default();
	visor = visor_new();

	// start with an empty game descriptor.  this gets JSON encoded
	// and written to game.json as the final step of a build.
	duk_push_global_stash(js);
	duk_push_object(js);
	duk_put_prop_string(js, -2, "descriptor");
	duk_pop(js);

	tmp_path = path_rebase(path_new(".cell_staging/"), in_path);
	build = calloc(1, sizeof(build_t));
	build->fs = fs_new(path_cstr(in_path), path_cstr(out_path), path_cstr(tmp_path));
	build->targets = vector_new(sizeof(target_t*));
	build->js = js;
	build->visor = visor;
	path_free(tmp_path);
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
	duk_destroy_heap(build->js);
	visor_free(build->visor);
	free(build);
}

fs_t*
build_fs(const build_t* build)
{
	return build->fs;
}

duk_context*
build_js_realm(const build_t* build)
{
	return build->js;
}

visor_t*
build_visor(const build_t* build)
{
	return build->visor;
}

void
build_add_target(build_t* build, target_t* target)
{
	target_ref(target);
	vector_push(build->targets, &target);
}

bool
build_run(build_t* build, bool rebuilding)
{
	const char*   json;
	size_t        json_size;
	int           num_built = 0;
	int           num_errors;
	int           num_warns;
	const path_t* path;
	
	iter_t iter;
	target_t* *p;

	iter = vector_enum(build->targets);
	while (p = vector_next(&iter)) {
		path = target_path(*p);
		if (path_num_hops(path) == 0 || !path_hop_cmp(path, 0, "@"))
			continue;
		target_build(*p, build->visor, rebuilding);
	}

	num_errors = visor_num_errors(build->visor);
	num_warns = visor_num_warns(build->visor);
	
	// only generate a JSON manifest if the build finished with no errors.
	// warnings are fine (for now).
	if (num_errors == 0) {
		printf("generating game manifest...\n");
		duk_push_global_stash(build->js);
		duk_get_prop_string(build->js, -1, "descriptor");
		duk_json_encode(build->js, -1);
		json = duk_get_lstring(build->js, -1, &json_size);
		fs_fspew(build->fs, "@/game.json", json, json_size);
		duk_pop_2(build->js);
	}
	else {
		// delete any existing game manifest to ensure we don't accidentally
		// generate a functional but broken distribution.
		fs_unlink(build->fs, "@/game.json");
	}

	printf("%d error(s), %d warning(s).\n", num_errors, num_warns);
	return num_errors == 0;
}
