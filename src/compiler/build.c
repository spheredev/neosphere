#include "cell.h"
#include "build.h"

#include "fs.h"
#include "spk_writer.h"

struct build
{
	fs_t*        fs;
	duk_context* js;
	int          num_errors;
	int          num_warns;
	vector_t*    targets;
};

build_t*
build_new(const path_t* in_path, const path_t* out_path)
{
	build_t*     build = NULL;
	duk_context* js;

	// note: Duktape is initialized without a fatal error handler.  if a JavaScript
	//       exception is thrown and not caught, Cell will crash.
	js = duk_create_heap_default();

	// start with an empty game descriptor.  this gets JSON encoded
	// and written to game.json as the final step of a build.
	duk_push_global_stash(js);
	duk_push_object(js);
	duk_put_prop_string(js, -2, "descriptor");
	duk_pop(js);

	build = calloc(1, sizeof(build_t));
	build->fs = fs_new(path_cstr(in_path), path_cstr(out_path), NULL);
	build->targets = vector_new(sizeof(target_t*));
	build->js = js;
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

void
build_add_target(build_t* build, target_t* target)
{
	target_ref(target);
	vector_push(build->targets, &target);
}

void
build_emit_error(build_t* build, const char* fmt, ...)
{
	va_list ap;

	++build->num_errors;
	va_start(ap, fmt);
	printf("    ERROR: ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

void
build_emit_warning(build_t* build, const char* fmt, ...)
{
	va_list ap;

	++build->num_warns;
	va_start(ap, fmt);
	printf("    warning: ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

bool
build_run(build_t* build, bool rebuilding)
{
	const char*   json;
	size_t        json_size;
	int           num_built = 0;
	const path_t* path;
	
	iter_t iter;
	target_t* *p;

	build->num_errors = 0;
	build->num_warns = 0;

	iter = vector_enum(build->targets);
	while (p = vector_next(&iter)) {
		path = target_path(*p);
		if (path_num_hops(path) == 0 || !path_hop_cmp(path, 0, "@"))
			continue;
		if (!target_build(*p, build->fs, rebuilding)) {
			build_emit_error(build, "cannot build '%s'", path_cstr(path));
		}
	}

	// only generate a JSON manifest if the build finished with no errors.
	// warnings are fine (for now).
	if (build->num_errors == 0) {
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

	printf("%d error(s), %d warning(s).\n", build->num_errors, build->num_warns);
	return build->num_errors == 0;
}
