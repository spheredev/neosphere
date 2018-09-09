/**
 *  Cell, the Sphere packaging compiler
 *  Copyright (c) 2015-2018, Fat Cerberus
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

#include "cell.h"
#include "build.h"

#include "api.h"
#include "compress.h"
#include "encoding.h"
#include "fs.h"
#include "image.h"
#include "spk_writer.h"
#include "target.h"
#include "tool.h"
#include "utility.h"
#include "visor.h"
#include "xoroshiro.h"

struct build
{
	vector_t*     artifacts;
	bool          crashed;
	fs_t*         fs;
	vector_t*     targets;
	time_t        timestamp;
	visor_t*      visor;
};

enum file_op
{
	FILE_OP_READ,
	FILE_OP_WRITE,
	FILE_OP_UPDATE,
	FILE_OP_MAX,
};

static bool js_require                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_error                         (int num_args, bool is_ctor, intptr_t magic);
static bool js_files                         (int num_args, bool is_ctor, intptr_t magic);
static bool js_install                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_warn                          (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sphere_get_Compiler           (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sphere_get_Game               (int num_args, bool is_ctor, intptr_t magic);
static bool js_Sphere_get_Version            (int num_args, bool is_ctor, intptr_t magic);
static bool js_new_DirectoryStream           (int num_args, bool is_ctor, intptr_t magic);
static bool js_DirectoryStream_get_fileCount (int num_args, bool is_ctor, intptr_t magic);
static bool js_DirectoryStream_get_fileName  (int num_args, bool is_ctor, intptr_t magic);
static bool js_DirectoryStream_get_position  (int num_args, bool is_ctor, intptr_t magic);
static bool js_DirectoryStream_set_position  (int num_args, bool is_ctor, intptr_t magic);
static bool js_DirectoryStream_iterator      (int num_args, bool is_ctor, intptr_t magic);
static bool js_DirectoryStream_dispose       (int num_args, bool is_ctor, intptr_t magic);
static bool js_DirectoryStream_next          (int num_args, bool is_ctor, intptr_t magic);
static bool js_DirectoryStream_rewind        (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_createDirectory            (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_deleteFile                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_directoryExists            (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_fileExists                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_fullPath                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_readFile                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_relativePath               (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_rename                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_removeDirectory            (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_writeFile                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_new_FileStream                (int num_args, bool is_ctor, intptr_t magic);
static bool js_FileStream_dispose            (int num_args, bool is_ctor, intptr_t magic);
static bool js_FileStream_get_fileSize       (int num_args, bool is_ctor, intptr_t magic);
static bool js_FileStream_get_position       (int num_args, bool is_ctor, intptr_t magic);
static bool js_FileStream_set_position       (int num_args, bool is_ctor, intptr_t magic);
static bool js_FileStream_read               (int num_args, bool is_ctor, intptr_t magic);
static bool js_FileStream_write              (int num_args, bool is_ctor, intptr_t magic);
static bool js_new_Image                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_get_bitmap              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_get_height              (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_get_width               (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_saveAs                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_Image_slice                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_RNG_fromSeed                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_RNG_fromState                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_new_RNG                       (int num_args, bool is_ctor, intptr_t magic);
static bool js_RNG_get_state                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_RNG_set_state                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_RNG_iterator                  (int num_args, bool is_ctor, intptr_t magic);
static bool js_RNG_next                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_Target_get_fileName           (int num_args, bool is_ctor, intptr_t magic);
static bool js_Target_get_name               (int num_args, bool is_ctor, intptr_t magic);
static bool js_new_TextDecoder               (int num_args, bool is_ctor, intptr_t magic);
static bool js_TextDecoder_get_encoding      (int num_args, bool is_ctor, intptr_t magic);
static bool js_TextDecoder_get_fatal         (int num_args, bool is_ctor, intptr_t magic);
static bool js_TextDecoder_get_ignoreBOM     (int num_args, bool is_ctor, intptr_t magic);
static bool js_TextDecoder_decode            (int num_args, bool is_ctor, intptr_t magic);
static bool js_new_TextEncoder               (int num_args, bool is_ctor, intptr_t magic);
static bool js_TextEncoder_get_encoding      (int num_args, bool is_ctor, intptr_t magic);
static bool js_TextEncoder_encode            (int num_args, bool is_ctor, intptr_t magic);
static bool js_new_Tool                      (int num_args, bool is_ctor, intptr_t magic);
static bool js_Tool_stage                    (int num_args, bool is_ctor, intptr_t magic);
static bool js_Z_deflate                     (int num_args, bool is_ctor, intptr_t magic);
static bool js_Z_inflate                     (int num_args, bool is_ctor, intptr_t magic);

static void js_DirectoryStream_finalize (void* host_ptr);
static void js_FileStream_finalize      (void* host_ptr);
static void js_Image_finalize           (void* host_ptr);
static void js_RNG_finalize             (void* host_ptr);
static void js_Target_finalize          (void* host_ptr);
static void js_TextDecoder_finalize     (void* host_ptr);
static void js_TextEncoder_finalize     (void* host_ptr);
static void js_Tool_finalize            (void* host_ptr);

static void    cache_value_to_this  (const char* key);
static void    clean_old_artifacts  (build_t* build, bool keep_targets);
static bool    eval_module_file     (fs_t* fs, const char* filename);
static path_t* find_module_file     (fs_t* fs, const char* id, const char* origin, const char* sys_origin);
static void    handle_module_import (void);
static bool    install_target       (int num_args, bool is_ctor, intptr_t magic);
static path_t* load_package_json    (const char* filename);
static void    make_file_targets    (fs_t* fs, const char* wildcard, const path_t* path, const path_t* subdir, vector_t* targets, bool recursive, time_t timestamp);
static void    package_dir          (build_t* build, spk_writer_t* spk, const char* from_dirname, const char* to_dirname);
static void    push_require         (const char* module_id);
static int     sort_targets_by_path (const void* p_a, const void* p_b);
static bool    write_manifests      (build_t* build);

static build_t*     s_build;
static unsigned int s_next_module_id = 1;

build_t*
build_new(const path_t* source_path, const path_t* out_path)
{
	vector_t*    artifacts;
	build_t*     build;
	char*        filename;
	fs_t*        fs;
	char*        json;
	size_t       json_size;
	int          num_artifacts;
	visor_t*     visor;

	int i;

	build = calloc(1, sizeof(build_t));
	visor = visor_new();
	fs = fs_new(path_cstr(source_path), path_cstr(out_path), NULL);

	visor_begin_op(visor, "setting up Cellscript environment");

	jsal_on_import_module(handle_module_import);

	// initialize the CommonJS cache and global require()
	jsal_push_hidden_stash();
	jsal_push_new_object();
	jsal_put_prop_string(-2, "moduleCache");
	jsal_pop(1);

	jsal_push_global_object();
	push_require(NULL);
	jsal_push_value_desc(true, false, true);
	jsal_def_prop_string(-2, "require");
	jsal_pop(1);

	// initialize the Cellscript API
	api_init();
	api_define_function(NULL, "error", js_error, 0);
	api_define_function(NULL, "files", js_files, 0);
	api_define_function(NULL, "install", js_install, 0);
	api_define_function(NULL, "warn", js_warn, 0);
	api_define_static_prop("Sphere", "Compiler", js_Sphere_get_Compiler, NULL);
	api_define_static_prop("Sphere", "Game", js_Sphere_get_Game, NULL);
	api_define_static_prop("Sphere", "Version", js_Sphere_get_Version, NULL);
	api_define_class("DirectoryStream", CELL_DIR_STREAM, js_new_DirectoryStream, js_DirectoryStream_finalize, 0);
	api_define_property("DirectoryStream", "fileCount", false, js_DirectoryStream_get_fileCount, NULL);
	api_define_property("DirectoryStream", "fileName", false, js_DirectoryStream_get_fileName, NULL);
	api_define_property("DirectoryStream", "position", false, js_DirectoryStream_get_position, js_DirectoryStream_set_position);
	api_define_method("DirectoryStream", "@@iterator", js_DirectoryStream_iterator, 0);
	api_define_method("DirectoryStream", "dispose", js_DirectoryStream_dispose, 0);
	api_define_method("DirectoryStream", "next", js_DirectoryStream_next, 0);
	api_define_method("DirectoryStream", "rewind", js_DirectoryStream_rewind, 0);
	api_define_function("FS", "createDirectory", js_FS_createDirectory, 0);
	api_define_function("FS", "deleteFile", js_FS_deleteFile, 0);
	api_define_function("FS", "directoryExists", js_FS_directoryExists, 0);
	api_define_function("FS", "fileExists", js_FS_fileExists, 0);
	api_define_function("FS", "fullPath", js_FS_fullPath, 0);
	api_define_function("FS", "readFile", js_FS_readFile, 0);
	api_define_function("FS", "relativePath", js_FS_relativePath, 0);
	api_define_function("FS", "removeDirectory", js_FS_removeDirectory, 0);
	api_define_function("FS", "rename", js_FS_rename, 0);
	api_define_function("FS", "writeFile", js_FS_writeFile, 0);
	api_define_class("FileStream", CELL_FILE_STREAM, js_new_FileStream, js_FileStream_finalize, 0);
	api_define_property("FileStream", "fileSize", false, js_FileStream_get_fileSize, NULL);
	api_define_property("FileStream", "position", false, js_FileStream_get_position, js_FileStream_set_position);
	api_define_method("FileStream", "dispose", js_FileStream_dispose, 0);
	api_define_method("FileStream", "read", js_FileStream_read, 0);
	api_define_method("FileStream", "write", js_FileStream_write, 0);
	api_define_class("Image", CELL_IMAGE, js_new_Image, js_Image_finalize, 0);
	api_define_property("Image", "bitmap", false, js_Image_get_bitmap, NULL);
	api_define_property("Image", "height", false, js_Image_get_height, NULL);
	api_define_property("Image", "width", false, js_Image_get_width, NULL);
	api_define_method("Image", "saveAs", js_Image_saveAs, 0);
	api_define_method("Image", "slice", js_Image_slice, 0);
	api_define_class("RNG", CELL_RNG, js_new_RNG, js_RNG_finalize, 0);
	api_define_function("RNG", "fromSeed", js_RNG_fromSeed, 0);
	api_define_function("RNG", "fromState", js_RNG_fromState, 0);
	api_define_property("RNG", "state", false, js_RNG_get_state, js_RNG_set_state);
	api_define_method("RNG", "@@iterator", js_RNG_iterator, 0);
	api_define_method("RNG", "next", js_RNG_next, 0);
	api_define_class("Target", CELL_TARGET, NULL, js_Target_finalize, 0);
	api_define_property("Target", "fileName", false, js_Target_get_fileName, NULL);
	api_define_property("Target", "name", false, js_Target_get_name, NULL);
	api_define_class("TextDecoder", CELL_TEXT_DEC, js_new_TextDecoder, js_TextDecoder_finalize, 0);
	api_define_property("TextDecoder", "encoding", false, js_TextDecoder_get_encoding, NULL);
	api_define_property("TextDecoder", "fatal", false, js_TextDecoder_get_fatal, NULL);
	api_define_property("TextDecoder", "ignoreBOM", false, js_TextDecoder_get_ignoreBOM, NULL);
	api_define_method("TextDecoder", "decode", js_TextDecoder_decode, 0);
	api_define_class("TextEncoder", CELL_TEXT_ENC, js_new_TextEncoder, js_TextEncoder_finalize, 0);
	api_define_property("TextEncoder", "encoding", false, js_TextEncoder_get_encoding, NULL);
	api_define_method("TextEncoder", "encode", js_TextEncoder_encode, 0);
	api_define_class("Tool", CELL_TOOL, js_new_Tool, js_Tool_finalize, 0);
	api_define_method("Tool", "stage", js_Tool_stage, 0);
	api_define_function("Z", "deflate", js_Z_deflate, 0);
	api_define_function("Z", "inflate", js_Z_inflate, 0);

	api_define_const("FileOp", "Read", FILE_OP_READ);
	api_define_const("FileOp", "Write", FILE_OP_WRITE);
	api_define_const("FileOp", "Update", FILE_OP_UPDATE);

	// game manifest (gets JSON encoded at end of build)
	jsal_push_hidden_stash();
	jsal_push_new_object();
	jsal_put_prop_string(-2, "manifest");
	jsal_pop(1);

	// create a Tool for the install() function to use
	jsal_push_hidden_stash();
	jsal_push_new_function(install_target, "doInstall", 0, 0);
	jsal_push_class_obj(CELL_TOOL, tool_new("installing"), false);
	jsal_put_prop_string(-2, "installTool");
	jsal_pop(1);

	// load artifacts from previous build
	artifacts = vector_new(sizeof(char*));
	if ((json = fs_fslurp(fs, "@/artifacts.json", &json_size))) {
		jsal_push_lstring(json, json_size);
		free(json);
		if (jsal_try_parse(-1) && jsal_is_array(-1)) {
			num_artifacts = jsal_get_length(-1);
			for (i = 0; i < num_artifacts; ++i) {
				jsal_get_prop_index(-1, i);
				filename = strdup(jsal_to_string(-1));
				vector_push(artifacts, &filename);
				jsal_pop(1);
			}
		}
		jsal_pop(1);
	}

	visor_end_op(visor);

	s_build = build;

	build->visor = visor;
	build->fs = fs;
	build->artifacts = artifacts;
	build->targets = vector_new(sizeof(target_t*));
	return build;
}

void
build_free(build_t* build)
{
	iter_t iter;

	if (build == NULL)
		return;

	if (!build->crashed) {
		printf("%d error(s), %d warning(s).\n",
			visor_num_errors(build->visor),
			visor_num_warns(build->visor));
	}

	iter = vector_enum(build->artifacts);
	while (iter_next(&iter))
		free(*(char**)iter.ptr);
	iter = vector_enum(build->targets);
	while (iter_next(&iter))
		target_free(*(target_t**)iter.ptr);

	fs_free(build->fs);
	visor_free(build->visor);
	free(build);
}

bool
build_eval(build_t* build, const char* filename)
{
	int         error_column;
	int         error_line = 0;
	char*       error_stack = NULL;
	char*       error_url = NULL;
	bool        is_ok = true;
	struct stat stats;

	if (fs_stat(build->fs, filename, &stats) != 0)
		return false;

	visor_begin_op(build->visor, "evaluating '%s'", filename);
	build->timestamp = stats.st_mtime;
	if (!eval_module_file(build->fs, filename)) {
		build->crashed = true;
		is_ok = false;
		if (jsal_is_error(-1)) {
			if (jsal_get_prop_string(-1, "stack"))
				error_stack = strdup(jsal_get_string(-1));
			if (jsal_get_prop_string(-2, "url"))
				error_url = strdup(jsal_get_string(-1));
			if (jsal_get_prop_string(-3, "line"))
				error_line = jsal_get_int(-1) + 1;
			if (jsal_get_prop_string(-4, "column"))
				error_column = jsal_get_int(-1) + 1;
			jsal_pop(4);
		}
		visor_error(build->visor, "error in JavaScript code detected");
		visor_end_op(build->visor);
		if (error_url != NULL && error_line > 0)
			printf("\nBUILD CRASH: error at '%s':%d:%d\n", error_url, error_line, error_column);
		else if (error_url != NULL)
			printf("\nBUILD CRASH: error in '%s'\n", error_url);
		else
			printf("\nBUILD CRASH: uncaught JavaScript exception\n");
		if (error_stack != NULL)
			printf("%s\n", error_stack);
		else
			printf("%s\n", jsal_to_string(-1));
		free(error_stack);
		free(error_url);
	}
	jsal_pop(1);
	if (is_ok)
		visor_end_op(build->visor);
	return is_ok;
}

bool
build_clean(build_t* build)
{
	clean_old_artifacts(build, false);
	fs_unlink(build->fs, "@/artifacts.json");
	return true;
}

bool
build_package(build_t* build, const char* filename)
{
	const path_t* in_path;
	path_t*       out_path;
	spk_writer_t* spk;
	target_t**    target_ptr;

	iter_t iter;

	visor_begin_op(build->visor, "packaging game to '%s'", filename);
	spk = spk_create(filename);
	spk_add_file(spk, build->fs, "@/game.json", "game.json");
	spk_add_file(spk, build->fs, "@/game.sgm", "game.sgm");
	package_dir(build, spk, "#/", "#/");
	iter = vector_enum(build->targets);
	while ((target_ptr = iter_next(&iter))) {
		in_path = target_path(*target_ptr);
		if (path_num_hops(in_path) == 0 || !path_hop_is(in_path, 0, "@"))
			continue;
		out_path = path_dup(target_path(*target_ptr));
		path_remove_hop(out_path, 0);
		visor_begin_op(build->visor, "packaging '%s'", path_cstr(out_path));
		spk_add_file(spk, build->fs,
			path_cstr(target_path(*target_ptr)),
			path_cstr(out_path));
		path_free(out_path);
		visor_end_op(build->visor);
	}
	spk_close(spk);
	visor_end_op(build->visor);
	return true;
}

bool
build_run(build_t* build, bool want_debug, bool rebuild_all)
{
	const char*   filename;
	vector_t*     filenames;
	const char*   json;
	size_t        json_size;
	const char*   last_filename = "";
	int           num_matches = 1;
	const path_t* path;
	const path_t* source_path;
	vector_t*     sorted_targets;
	target_t**    target_ptr;

	iter_t iter;

	// ensure there are no conflicting targets before building.  to simplify the check,
	// we sort the targets by filename first and then look for runs of identical filenames.
	// by doing this, we only have to walk the list once.
	visor_begin_op(build->visor, "building targets");
	sorted_targets = vector_dup(build->targets);
	vector_sort(sorted_targets, sort_targets_by_path);
	iter = vector_enum(sorted_targets);
	while (iter_next(&iter)) {
		filename = path_cstr(target_path(*(target_t**)iter.ptr));
		if (strcmp(filename, last_filename) == 0) {
			++num_matches;
		}
		else {
			if (num_matches > 1)
				visor_error(build->visor, "%d-way conflict '%s'", num_matches, filename);
			num_matches = 1;
		}
		last_filename = filename;
	}
	if (num_matches > 1)
		visor_error(build->visor, "%d-way conflict '%s'", num_matches, filename);
	vector_free(sorted_targets);
	if (visor_num_errors(build->visor) > 0) {
		visor_end_op(build->visor);
		goto finished;
	}

	// build all relevant targets
	iter = vector_enum(build->targets);
	while ((target_ptr = iter_next(&iter))) {
		path = target_path(*target_ptr);
		if (path_num_hops(path) == 0 || !path_hop_is(path, 0, "@"))
			continue;
		target_build(*target_ptr, build->visor, rebuild_all);
	}
	visor_end_op(build->visor);

	// add metadata
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "manifest");
	jsal_push_sprintf("%s %s", SPHERE_COMPILER_NAME, SPHERE_VERSION);
	jsal_put_prop_string(-2, "$COMPILER");
	jsal_pop(2);

	// generate the source map
	if (want_debug) {
		visor_begin_op(build->visor, "collecting debugging information");
		jsal_push_hidden_stash();
		jsal_get_prop_string(-1, "manifest");
		jsal_push_new_object();
		iter = vector_enum(build->targets);
		while ((target_ptr = iter_next(&iter))) {
			path = target_path(*target_ptr);
			if (path_num_hops(path) == 0 || !path_hop_is(path, 0, "@"))
				continue;
			if (!(source_path = target_source_path(*target_ptr)))
				continue;
			jsal_push_string(path_cstr(path));
			jsal_push_string(path_cstr(source_path));
			jsal_put_prop(-3);
		}
		jsal_put_prop_string(-2, "$SOURCES");
		jsal_pop(2);
		visor_end_op(build->visor);
	}

	// only generate a game manifest if the build finished with no errors.
	// warnings are fine.
	if (visor_num_errors(build->visor) == 0) {
		clean_old_artifacts(build, true);
		if (!write_manifests(build)) {
			fs_unlink(build->fs, "@/game.json");
			fs_unlink(build->fs, "@/game.sgm");
			goto finished;
		}
	}
	else {
		// delete any existing game manifest to ensure we don't accidentally
		// generate a functional but broken distribution.
		fs_unlink(build->fs, "@/game.json");
		fs_unlink(build->fs, "@/game.sgm");
		goto finished;
	}

	filenames = visor_filenames(build->visor);
	jsal_push_new_array();
	iter = vector_enum(filenames);
	while (iter_next(&iter)) {
		jsal_push_string(*(char**)iter.ptr);
		jsal_put_prop_index(-2, iter.index);
	}
	jsal_stringify(-1);
	json = jsal_get_lstring(-1, &json_size);
	fs_fspew(build->fs, "@/artifacts.json", json, json_size);

finished:
	return visor_num_errors(build->visor) == 0;
}

static void
cache_value_to_this(const char* key)
{
	jsal_push_this();
	jsal_dup(-2);
	jsal_push_value_desc(false, false, true);
	jsal_def_prop_string(-2, key);
	jsal_pop(1);
}

static void
clean_old_artifacts(build_t* build, bool keep_targets)
{
	vector_t* filenames;
	bool      keep_file;

	iter_t iter_i, iter_j;

	visor_begin_op(build->visor, "cleaning up outdated artifacts");
	filenames = visor_filenames(build->visor);
	iter_i = vector_enum(build->artifacts);
	while (iter_next(&iter_i)) {
		keep_file = false;
		if (keep_targets) {
			iter_j = vector_enum(filenames);
			while (iter_next(&iter_j)) {
				if (strcmp(*(char**)iter_j.ptr, *(char**)iter_i.ptr) == 0)
					keep_file = true;
			}
		}
		if (!keep_file) {
			visor_begin_op(build->visor, "removing '%s'", *(char**)iter_i.ptr);
			fs_unlink(build->fs, *(char**)iter_i.ptr);
			visor_end_op(build->visor);
		}
	}
	visor_end_op(build->visor);
}

static bool
eval_module_file(fs_t* fs, const char* filename)
{
	// HERE BE DRAGONS!
	// this function is horrendous.  Duktape's stack-based API is powerful, but gets
	// very messy very quickly when dealing with object properties.  I tried to add
	// comments to illuminate what's going on, but it's still likely to be confusing for
	// someone not familiar with Duktape code.  proceed with caution.

	// notes:
	//     - the final value of 'module.exports' is left on top of the Duktape value stack.
	//     - 'module.id' is set to the given filename.  in order to guarantee proper cache
	//       behavior, the filename should be in canonical form.
	//     - this is a protected call.  if the module being loaded throws, the error will be
	//       caught and left on top of the stack for the caller to deal with.

	lstring_t*  code_string;
	path_t*     dir_path;
	path_t*     file_path;
	bool        is_module_loaded;
	size_t      source_size;
	char*       source;

	file_path = path_new(filename);
	dir_path = path_strip(path_dup(file_path));

	// is the requested module already in the cache?
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	if (jsal_get_prop_string(-1, filename)) {
		jsal_remove(-2);
		jsal_remove(-2);
		goto have_module;
	}
	else {
		jsal_pop(3);
	}

	source = fs_fslurp(fs, filename, &source_size);
	code_string = lstr_from_utf8(source, source_size, true);
	free(source);

	// construct a module object for the new module
	jsal_push_new_object();  // module object
	jsal_push_new_object();
	jsal_put_prop_string(-2, "exports");  // module.exports = {}
	jsal_push_string(filename);
	jsal_put_prop_string(-2, "filename");  // module.filename
	jsal_push_string(filename);
	jsal_put_prop_string(-2, "id");  // module.id
	jsal_push_boolean(false);
	jsal_put_prop_string(-2, "loaded");  // module.loaded = false
	push_require(filename);
	jsal_put_prop_string(-2, "require");  // module.require

	// evaluate .mjs scripts as ES6 modules
	if (path_has_extension(file_path, ".mjs")) {
		jsal_push_lstring_t(code_string);
		is_module_loaded = jsal_try_eval_module(filename, NULL);
		lstr_free(code_string);
		if (!is_module_loaded)
			goto on_error;
		jsal_remove(-2);
		return true;
	}

	// cache the module object in advance
	// note: the reason this isn't done above is because we don't want mJS modules
	//       to go into the CommonJS cache.
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	jsal_dup(-3);
	jsal_put_prop_string(-2, filename);
	jsal_pop(2);

	if (path_has_extension(file_path, ".json")) {
		// JSON file, decode to JavaScript object
		jsal_push_lstring_t(code_string);
		lstr_free(code_string);
		if (!jsal_try_parse(-1))
			goto on_error;
		jsal_put_prop_string(-2, "exports");
	}
	else {
		// synthesize a function to wrap the module code.  this is the simplest way to
		// implement CommonJS semantics and matches the behavior of Node.js.
		jsal_push_sprintf("(function (exports, require, module, __filename, __dirname) {%s%s\n})",
			strncmp(lstr_cstr(code_string), "#!", 2) == 0 ? "//" : "",  // shebang?
			lstr_cstr(code_string));
		if (!jsal_try_compile(filename))
			goto on_error;
		jsal_call(0);
		jsal_push_new_object();
		jsal_push_string("main");
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-2, "name");
		lstr_free(code_string);

		// go, go, go!
		jsal_dup(-2);                           // this = module
		jsal_get_prop_string(-3, "exports");     // exports
		jsal_get_prop_string(-4, "require");     // require
		jsal_dup(-5);                           // module
		jsal_push_string(filename);             // __filename
		jsal_push_string(path_cstr(dir_path));  // __dirname
		if (!jsal_try_call_method(5))
			goto on_error;
		jsal_pop(1);
	}

	// module executed successfully, set 'module.loaded' to true
	jsal_push_boolean(true);
	jsal_put_prop_string(-2, "loaded");

have_module:
	// 'module' is on the stack, we need 'module.exports'
	jsal_get_prop_string(-1, "exports");
	jsal_remove(-2);
	return true;

on_error:
	// note: it's assumed that at this point, the only things left in our portion of the
	//       value stack are the module object and the thrown error.
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	jsal_del_prop_string(-1, filename);
	jsal_pop(2);
	jsal_remove(-2);  // leave the error on the stack
	return false;
}

static path_t*
find_module_file(fs_t* fs, const char* id, const char* origin, const char* sys_origin)
{
	const char* const filenames[] =
	{
		"%s",
		"%s.mjs",
		"%s.js",
		"%s.json",
		"%s/package.json",
		"%s/index.mjs",
		"%s/index.js",
		"%s/index.json",
	};

	path_t*     origin_path;
	char*       filename;
	path_t*     main_path;
	path_t*     path;

	int i;

	if (strncmp(id, "./", 2) == 0 || strncmp(id, "../", 3) == 0)
		// resolve module relative to calling module
		origin_path = path_new(origin != NULL ? origin : "./");
	else
		// resolve module from designated module repository
		origin_path = path_new_dir(sys_origin);

	for (i = 0; i < (int)(sizeof(filenames) / sizeof(filenames[0])); ++i) {
		filename = strnewf(filenames[i], id);
		if (strncmp(id, "@/", 2) == 0 || strncmp(id, "$/", 2) == 0 || strncmp(id, "~/", 2) == 0 || strncmp(id, "#/", 2) == 0) {
			path = fs_full_path(filename, NULL);
		}
		else {
			path = path_dup(origin_path);
			path_strip(path);
			path_append(path, filename);
			path_collapse(path, true);
		}
		free(filename);
		if (fs_fexist(fs, path_cstr(path))) {
			if (strcmp(path_filename(path), "package.json") != 0) {
				return path;
			}
			else {
				if (!(main_path = load_package_json(path_cstr(path))))
					goto next_filename;
				if (fs_fexist(fs, path_cstr(main_path))) {
					path_free(path);
					return main_path;
				}
			}
		}

	next_filename:
		path_free(path);
	}

	return NULL;
}

static void
handle_module_import(void)
{
	/* [ module_name parent_specifier ] -> [ ... specifier url source ] */

	const char* const PATHS[] =
	{
		"$/lib",
		"#/cell_modules",
		"#/runtime",
	};

	const char* caller_id = NULL;
	path_t*     path;
	char*       source;
	size_t      source_len;
	const char* specifier;

	int i;

	specifier = jsal_require_string(0);
	if (!jsal_is_null(1))
		caller_id = jsal_require_string(1);

	if (caller_id == NULL && (strncmp(specifier, "./", 2) == 0 || strncmp(specifier, "../", 3) == 0))
		jsal_error(JS_URI_ERROR, "relative import() outside of an mJS module");

	for (i = 0; i < sizeof PATHS / sizeof PATHS[0]; ++i) {
		if ((path = find_module_file(s_build->fs, specifier, caller_id, PATHS[i])))
			break;  // short-circuit
	}
	if (path == NULL) {
		jsal_push_new_error(JS_URI_ERROR, "Couldn't load JS module '%s'", specifier);
		if (caller_id != NULL) {
			jsal_push_string(caller_id);
			jsal_put_prop_string(-2, "url");
		}
		jsal_throw();
	}
	if (path_has_extension(path, ".mjs")) {
		source = fs_fslurp(s_build->fs, path_cstr(path), &source_len);
		jsal_push_string(path_cstr(path));
		jsal_dup(-1);
		jsal_push_lstring(source, source_len);
		free(source);
	}
	else {
		// ES module shim to allow 'import' of CommonJS modules
		jsal_push_sprintf("%%/moduleShim-%d.mjs", s_next_module_id++);
		jsal_dup(-1);
		jsal_push_sprintf("export default require(\"%s\");", path_cstr(path));
	}
}

static bool
install_target(int num_args, bool is_ctor, intptr_t magic)
{
	// note: install targets never have more than one source because an individual
	//       target is constructed for each file installed.

	int         result;
	const char* source_path;
	const char* target_path;

	target_path = jsal_require_string(0);
	jsal_get_prop_index(1, 0);
	source_path = jsal_require_string(-1);

	result = fs_fcopy(s_build->fs, target_path, source_path, true);
	if (result == 0)
		// touch file to prevent "target file unchanged" warning
		fs_utime(s_build->fs, target_path, NULL);
	jsal_push_boolean(result == 0);
	return true;
}

static path_t*
load_package_json(const char* filename)
{
	char*       json;
	size_t      json_size;
	path_t*     path;
	int         stack_top;

	stack_top = jsal_get_top();
	if (!(json = fs_fslurp(s_build->fs, filename, &json_size)))
		goto on_error;
	jsal_push_lstring(json, json_size);
	free(json);
	if (!jsal_try_parse(-1))
		goto on_error;
	if (!jsal_is_object_coercible(-1))
		goto on_error;
	jsal_get_prop_string(-1, "main");
	if (!jsal_is_string(-1))
		goto on_error;
	path = path_strip(path_new(filename));
	path_append(path, jsal_get_string(-1));
	path_collapse(path, true);
	if (!fs_fexist(s_build->fs, path_cstr(path)))
		goto on_error;
	return path;

on_error:
	jsal_set_top(stack_top);
	return NULL;
}

static void
make_file_targets(fs_t* fs, const char* wildcard, const path_t* path, const path_t* subdir_path, vector_t* targets, bool recursive, time_t timestamp)
{
	// note: 'targets' should be a vector_t initialized to sizeof(target_t*).

	path_t*      file_path;
	bool         ignore_dir;
	vector_t*    list;
	path_t*      name;
	path_t**     path_ptr;
	target_t*    target;

	iter_t iter;

	if (!(list = fs_list_dir(fs, path_cstr(path))))
		return;

	iter = vector_enum(list);
	while ((path_ptr = iter_next(&iter))) {
		ignore_dir = fs_is_game_dir(fs, path_cstr(*path_ptr))
			&& path_num_hops(path) > 0
			&& !path_hop_is(path, 0, "@");
		if (!path_is_file(*path_ptr) && !ignore_dir && recursive) {
			name = path_new_dir(path_hop(*path_ptr, path_num_hops(*path_ptr) - 1));
			file_path = path_dup(*path_ptr);
			if (subdir_path != NULL)
				path_rebase(name, subdir_path);
			make_file_targets(fs, wildcard, file_path, name, targets, true, timestamp);
			path_free(file_path);
			path_free(name);
		}
		else if (path_is_file(*path_ptr) && wildcmp(path_filename(*path_ptr), wildcard)) {
			name = path_new(path_filename(*path_ptr));
			file_path = path_dup(*path_ptr);
			if (subdir_path != NULL)
				path_rebase(name, subdir_path);
			target = target_new(name, fs, file_path, NULL, timestamp, false);
			vector_push(targets, &target);
			path_free(file_path);
			path_free(name);
		}
	}

	iter = vector_enum(list);
	while ((path_ptr = iter_next(&iter)))
		path_free(*path_ptr);
	vector_free(list);
}

static void
package_dir(build_t* build, spk_writer_t* spk, const char* from_dirname, const char* to_dirname)
{
	vector_t* file_list;
	path_t*   from_path;
	path_t*   from_dir_path;
	path_t*   to_dir_path;
	path_t*   to_path;

	iter_t iter;

	from_dir_path = path_new_dir(from_dirname);
	to_dir_path = path_new_dir(to_dirname);
	file_list = fs_list_dir(build->fs, path_cstr(from_dir_path));
	iter = vector_enum(file_list);
	while (iter_next(&iter)) {
		from_path = *(path_t**)iter.ptr;
		to_path = path_dup(from_path);
		path_relativize(to_path, from_dir_path);
		path_rebase(to_path, to_dir_path);
		if (path_is_file(from_path))
			spk_add_file(spk, build->fs, path_cstr(from_path), path_cstr(to_path));
		else
			package_dir(build, spk, path_cstr(from_path), path_cstr(to_path));
		path_free(to_path);
		path_free(from_path);
	}
}

static void
push_require(const char* module_id)
{
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "moduleCache");
	jsal_remove(-2);

	jsal_push_new_function(js_require, "require", 1, 0);
	jsal_push_new_object();
	jsal_pull(-3);
	jsal_put_prop_string(-2, "value");
	jsal_def_prop_string(-2, "cache");  // require.cache

	if (module_id != NULL) {
		jsal_push_new_object();
		jsal_push_string(module_id);
		jsal_put_prop_string(-2, "value");
		jsal_def_prop_string(-2, "id");  // require.id
	}
}

static int
sort_targets_by_path(const void* p_a, const void* p_b)
{
	const target_t* a;
	const target_t* b;

	a = *(const target_t**)p_a;
	b = *(const target_t**)p_b;
	return strcmp(path_cstr(target_path(a)), path_cstr(target_path(b)));
}

static bool
write_manifests(build_t* build)
{
	int         api_level;
	int         api_version = 2;
	FILE*       file;
	int         height;
	size_t      json_size;
	const char* json_text;
	path_t*     main_path;
	const char* sandbox_mode;
	path_t*     script_path;
	int         width;

	visor_begin_op(build->visor, "writing Sphere manifest files");

	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "manifest");

	// validate game descriptor before writing manifests
	jsal_get_prop_string(-1, "name");
	if (!jsal_is_string(-1)) {
		jsal_push_string("Untitled");
		jsal_remove(-2);
		visor_warn(build->visor, "'name': missing 'name' field");
	}

	jsal_get_prop_string(-2, "author");
	if (!jsal_is_string(-1)) {
		jsal_push_string("Author Unknown");
		jsal_remove(-2);
		visor_warn(build->visor, "'author': missing 'author' field");
	}

	jsal_get_prop_string(-3, "summary");
	if (!jsal_is_string(-1)) {
		jsal_push_string("No summary provided.");
		jsal_remove(-2);
	}

	if (jsal_get_prop_string(-4, "version")) {
		if (jsal_is_number(-1)) {
			api_version = jsal_get_int(-1);
			if (api_version < 1)
				visor_error(build->visor, "'version': must be greater than zero, found '%d'", api_version);
			else if (api_version > SPHERE_API_VERSION)
				visor_warn(build->visor, "'version': value '%d' targets future Sphere version", api_version);
		}
		else {
			visor_error(build->visor, "'version': must be a number greater than zero, found '%s'", jsal_to_string(-1));
		}
	}
	else {
		visor_warn(build->visor, "'version': missing value: targeting Sphere v2");
	}

	if (jsal_get_prop_string(-5, "apiLevel")) {
		if (api_version < 2) {
			visor_warn(build->visor, "'apiLevel': value doesn't apply to Sphere v1, ignored");
		}
		else if (jsal_is_number(-1)) {
			api_level = jsal_get_int(-1);
			if (api_level < 1)
				visor_error(build->visor, "'apiLevel': must be greater than zero, found '%d'", api_level);
			else if (api_level > SPHERE_API_LEVEL_STABLE && api_version <= SPHERE_API_VERSION)
				visor_warn(build->visor, "'apiLevel': value '%d' targets future Sphere version", api_level);
		}
		else {
			visor_error(build->visor, "'apiLevel': must be a number greater than zero, found '%s'", jsal_to_string(-1));
		}
	}
	else {
		if (api_version >= 2)
			visor_warn(build->visor, "'apiLevel': missing value: targeting API L1");
	}
	
	// note: SGMv1 encodes the resolution width and height as separate fields.
	jsal_get_prop_string(-6, "resolution");
	if (!jsal_is_string(-1)
		|| sscanf(jsal_to_string(-1), "%dx%d", &width, &height) != 2)
	{
		visor_error(build->visor, "'resolution': missing or invalid 'resolution' value");
		jsal_pop(6);
		visor_end_op(build->visor);
		return false;
	}

	jsal_get_prop_string(-7, "main");
	if (jsal_is_string(-1)) {
		// explicitly rebase onto '@/', as Cell uses '$/' by default.
		main_path = fs_full_path(jsal_to_string(-1), "@/");
		if (!path_hop_is(main_path, 0, "@")) {
			visor_error(build->visor, "'main': illegal prefix '%s/' in filename", path_hop(main_path, 0));
			jsal_pop(7);
			visor_end_op(build->visor);
			return false;
		}
		if (!fs_fexist(build->fs, path_cstr(main_path))) {
			visor_error(build->visor, "'main': file not found '%s'", path_cstr(main_path));
			jsal_pop(7);
			visor_end_op(build->visor);
			return false;
		}
		if (api_version <= 1 && path_has_extension(main_path, ".mjs"))
			visor_warn(build->visor, "'main': '%s' will not run as a module", path_cstr(main_path));
		jsal_push_string(path_cstr(main_path));
		jsal_put_prop_string(-7, "main");
	}
	else {
		visor_error(build->visor, "'main': missing or invalid 'main' value");
		jsal_pop(7);
		visor_end_op(build->visor);
		return false;
	}

	if (jsal_get_prop_string(-8, "sandbox")) {
		sandbox_mode = jsal_to_string(-1);
		if (strcmp(sandbox_mode, "full") != 0
			&& strcmp(sandbox_mode, "relaxed") != 0
			&& strcmp(sandbox_mode, "none") != 0)
		{
			visor_error(build->visor, "'sandbox': must be one of 'full', 'relaxed', 'none'");
			jsal_pop(8);
			visor_end_op(build->visor);
			return false;
		}
	}

	// write game.sgm (SGMv1, for compatibility with Sphere 1.x)
	// note: SGMv1 requires the main script path to be relative to '@/scripts'.
	//       this differs from Sv2 (game.json), where it's relative to '@/'.
	file = fs_fopen(build->fs, "@/game.sgm", "wb");
	script_path = fs_relative_path(path_cstr(main_path), "@/scripts");
	fprintf(file, "name=%s\n", jsal_to_string(-8));
	fprintf(file, "author=%s\n", jsal_to_string(-7));
	fprintf(file, "description=%s\n", jsal_to_string(-6));
	fprintf(file, "screen_width=%d\n", width);
	fprintf(file, "screen_height=%d\n", height);
	fprintf(file, "script=%s\n", path_cstr(script_path));
	fclose(file);
	path_free(script_path);
	jsal_pop(8);

	// write game.json (Sphere v2 JSON manifest)
	jsal_stringify(-1);
	json_text = jsal_get_lstring(-1, &json_size);
	fs_fspew(build->fs, "@/game.json", json_text, json_size);
	jsal_pop(2);

	visor_end_op(build->visor);
	path_free(main_path);
	return true;
}

static bool
js_error(int num_args, bool is_ctor, intptr_t magic)
{
	const char* message;

	message = jsal_require_string(0);

	visor_error(s_build->visor, "%s", message);
	return false;
}

static bool
js_files(int num_args, bool is_ctor, intptr_t magic)
{
	const char* pattern;
	path_t*     path;
	bool        recursive = false;
	vector_t*   targets;
	char*       wildcard;

	iter_t iter;
	target_t* *p;

	pattern = jsal_require_string(0);
	if (num_args >= 2)
		recursive = jsal_require_boolean(1);

	// extract the wildcard, if any, from the given path
	path = path_new(pattern);
	if (!path_is_file(path))
		wildcard = strdup("*");
	else {
		wildcard = strdup(path_filename(path));
		path_strip(path);
	}

	// this is potentially recursive, so we defer to make_file_targets() to construct
	// the targets.  note: 'path' should always be a directory at this point.
	targets = vector_new(sizeof(target_t*));
	make_file_targets(s_build->fs, wildcard, path, NULL, targets, recursive, s_build->timestamp);
	free(wildcard);

	if (vector_len(targets) == 0)
		visor_warn(s_build->visor, "no existing files match '%s'", pattern);

	// return all the newly constructed targets as an array.
	jsal_push_new_array();
	iter = vector_enum(targets);
	while ((p = iter_next(&iter))) {
		jsal_push_class_obj(CELL_TARGET, *p, false);
		jsal_put_prop_index(-2, iter.index);
	}
	return true;
}

static bool
js_install(int num_args, bool is_ctor, intptr_t magic)
{
	path_t*   dest_path;
	int       length;
	target_t* source;
	path_t*   name;
	path_t*   path;
	target_t* target;
	tool_t*   tool;

	int i;

	dest_path = path_new_dir(jsal_require_string(0));

	// retrieve the Install tool from the stash
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "installTool");
	tool = jsal_require_class_obj(-1, CELL_TOOL);
	jsal_pop(1);

	if (jsal_is_array(1)) {
		length = jsal_get_length(1);
		for (i = 0; i < length; ++i) {
			jsal_get_prop_index(1, i);
			source = jsal_require_class_obj(-1, CELL_TARGET);
			name = path_dup(target_name(source));
			path = path_rebase(path_dup(name), dest_path);
			target = target_new(name, s_build->fs, path, tool, s_build->timestamp, true);
			target_add_source(target, source);
			vector_push(s_build->targets, &target);
			jsal_pop(1);
		}
	}
	else {
		source = jsal_require_class_obj(1, CELL_TARGET);
		name = path_dup(target_name(source));
		path = path_rebase(path_dup(name), dest_path);
		target = target_new(name, s_build->fs, path, tool, s_build->timestamp, true);
		target_add_source(target, source);
		vector_push(s_build->targets, &target);
	}
	path_free(dest_path);
	return false;
}

static bool
js_require(int num_args, bool is_ctor, intptr_t magic)
{
	const char* const PATHS[] =
	{
		"$/lib",
		"#/cell_modules",
		"#/runtime",
	};

	const char* module_id;
	const char* parent_id = NULL;
	path_t*     path;

	int i;

	module_id = jsal_require_string(0);

	// get the calling module ID
	jsal_push_callee();
	if (jsal_get_prop_string(-1, "id"))
		parent_id = jsal_get_string(-1);

	if (parent_id == NULL && (strncmp(module_id, "./", 2) == 0 || strncmp(module_id, "../", 3) == 0))
		jsal_error(JS_URI_ERROR, "Relative require() outside of a CommonJS module");

	for (i = 0; i < sizeof PATHS / sizeof PATHS[0]; ++i) {
		if ((path = find_module_file(s_build->fs, module_id, parent_id, PATHS[i])))
			break;  // short-circuit
	}
	if (path == NULL)
		jsal_error(JS_URI_ERROR, "Couldn't load JS module '%s'", module_id);
	if (!eval_module_file(s_build->fs, path_cstr(path)))
		jsal_throw();
	return true;
}

static bool
js_warn(int num_args, bool is_ctor, intptr_t magic)
{
	const char* message;

	message = jsal_require_string(0);

	visor_warn(s_build->visor, "%s", message);
	return true;
}

static bool
js_Sphere_get_Compiler(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_sprintf("%s %s", SPHERE_COMPILER_NAME, SPHERE_VERSION);
	return true;
}

static bool
js_Sphere_get_Game(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_hidden_stash();
	jsal_get_prop_string(-1, "manifest");
	cache_value_to_this("Game");
	return true;
}

static bool
js_Sphere_get_Version(int num_args, bool is_ctor, intptr_t magic)
{
	jsal_push_int(2);
	return true;
}

static bool
js_new_DirectoryStream(int num_args, bool is_ctor, intptr_t magic)
{
	const char*  pathname;
	directory_t* stream;

	pathname = jsal_require_pathname(0, NULL);

	if (!(stream = directory_open(s_build->fs, pathname)))
		jsal_error(JS_ERROR, "Couldn't open directory '%s'", pathname);
	jsal_push_class_obj(CELL_DIR_STREAM, stream, true);
	return true;
}

static void
js_DirectoryStream_finalize(void* host_ptr)
{
	directory_close(host_ptr);
}

static bool
js_DirectoryStream_get_fileCount(int num_args, bool is_ctor, intptr_t magic)
{
	directory_t* stream;

	jsal_push_this();
	if (!(stream = jsal_require_class_obj(-1, CELL_DIR_STREAM)))
		jsal_error(JS_ERROR, "DirectoryStream has already been disposed");

	jsal_push_int(directory_num_files(stream));
	return true;
}

static bool
js_DirectoryStream_get_fileName(int num_args, bool is_ctor, intptr_t magic)
{
	directory_t* stream;

	jsal_push_this();
	if (!(stream = jsal_require_class_obj(-1, CELL_DIR_STREAM)))
		jsal_error(JS_ERROR, "DirectoryStream has already been disposed");

	jsal_push_string(directory_pathname(stream));
	return true;
}

static bool
js_DirectoryStream_get_position(int num_args, bool is_ctor, intptr_t magic)
{
	directory_t* stream;

	jsal_push_this();
	if (!(stream = jsal_require_class_obj(-1, CELL_DIR_STREAM)))
		jsal_error(JS_ERROR, "DirectoryStream has already been disposed");

	jsal_push_int(directory_position(stream));
	return true;
}

static bool
js_DirectoryStream_set_position(int num_args, bool is_ctor, intptr_t magic)
{
	int          position;
	directory_t* stream;

	jsal_push_this();
	if (!(stream = jsal_require_class_obj(-1, CELL_DIR_STREAM)))
		jsal_error(JS_ERROR, "DirectoryStream has already been disposed");
	position = jsal_require_int(0);

	if (!directory_seek(stream, position))
		jsal_error(JS_ERROR, "Couldn't set stream position");
	return false;
}

static bool
js_DirectoryStream_iterator(int num_args, bool is_ctor, intptr_t magic)
{
	directory_t* stream;

	jsal_push_this();
	if (!(stream = jsal_require_class_obj(-1, CELL_DIR_STREAM)))
		jsal_error(JS_ERROR, "DirectoryStream has already been disposed");

	return true;
}

static bool
js_DirectoryStream_dispose(int num_args, bool is_ctor, intptr_t magic)
{
	directory_t* directory;

	jsal_push_this();
	directory = jsal_require_class_obj(-1, CELL_DIR_STREAM);

	jsal_set_class_ptr(-1, NULL);
	directory_close(directory);
	return false;
}

static bool
js_DirectoryStream_next(int num_args, bool is_ctor, intptr_t magic)
{
	const path_t* entry_path;
	directory_t*  stream;

	jsal_push_this();
	if (!(stream = jsal_require_class_obj(-1, CELL_DIR_STREAM)))
		jsal_error(JS_ERROR, "DirectoryStream has already been disposed");

	entry_path = directory_next(stream);
	jsal_push_new_object();
	if (entry_path != NULL) {
		jsal_push_boolean(false);
		jsal_put_prop_string(-2, "done");
		jsal_push_new_object();
		if (path_is_file(entry_path))
			jsal_push_string(path_filename(entry_path));
		else
			jsal_push_sprintf("%s/", path_hop(entry_path, path_num_hops(entry_path) - 1));
		jsal_put_prop_string(-2, "fileName");
		jsal_push_string(path_cstr(entry_path));
		jsal_put_prop_string(-2, "fullPath");
		jsal_push_boolean(!path_is_file(entry_path));
		jsal_put_prop_string(-2, "isDirectory");
		jsal_put_prop_string(-2, "value");
	}
	else {
		jsal_push_boolean(true);
		jsal_put_prop_string(-2, "done");
	}
	return true;
}

static bool
js_DirectoryStream_rewind(int num_args, bool is_ctor, intptr_t magic)
{
	directory_t* stream;

	jsal_push_this();
	if (!(stream = jsal_require_class_obj(-1, CELL_DIR_STREAM)))
		jsal_error(JS_ERROR, "DirectoryStream has already been disposed");

	directory_rewind(stream);
	return false;
}

static bool
js_FS_createDirectory(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL);

	if (fs_mkdir(s_build->fs, filename) != 0)
		jsal_error(JS_ERROR, "Couldn't create directory '%s'", filename);
	return false;
}

static bool
js_FS_deleteFile(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL);

	if (!fs_unlink(s_build->fs, filename))
		jsal_error(JS_ERROR, "Couldn't delete file '%s'", filename);
	return false;
}

static bool
js_FS_directoryExists(int num_args, bool is_ctor, intptr_t magic)
{
	const char* dirname;

	dirname = jsal_require_pathname(0, NULL);

	jsal_push_boolean(fs_dir_exists(s_build->fs, dirname));
	return true;
}

static bool
js_FS_fileExists(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL);

	jsal_push_boolean(fs_fexist(s_build->fs, filename));
	return true;
}

static bool
js_FS_fullPath(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	const char* origin_pathname = NULL;

	if (num_args >= 2)
		origin_pathname = jsal_require_pathname(1, NULL);
	filename = jsal_require_pathname(0, origin_pathname);

	jsal_push_string(filename);
	return true;
}

static bool
js_FS_readFile(int num_args, bool is_ctor, intptr_t magic)
{
	lstring_t*  content;
	void*       file_data;
	size_t      file_size;
	const char* filename;

	filename = jsal_require_pathname(0, NULL);

	if (!(file_data = fs_fslurp(s_build->fs, filename, &file_size)))
		jsal_error(JS_ERROR, "Couldn't read file '%s'", filename);
	content = lstr_from_utf8(file_data, file_size, true);
	jsal_push_lstring_t(content);
	return true;
}

static bool
js_FS_relativePath(int num_args, bool is_ctor, intptr_t magic)
{
	const char* base_pathname;
	path_t*     path;
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL);
	base_pathname = jsal_require_pathname(1, NULL);

	path = fs_relative_path(pathname, base_pathname);
	jsal_push_string(path_cstr(path));
	path_free(path);
	return true;
}

static bool
js_FS_removeDirectory(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL);

	if (!fs_rmdir(s_build->fs, filename))
		jsal_error(JS_ERROR, "Couldn't remove directory '%s'", filename);
	return false;
}

static bool
js_FS_rename(int num_args, bool is_ctor, intptr_t magic)
{
	const char* old_name;
	const char* new_name;

	old_name = jsal_require_pathname(0, NULL);
	new_name = jsal_require_pathname(1, NULL);

	if (!fs_rename(s_build->fs, old_name, new_name))
		jsal_error(JS_ERROR, "Couldn't rename '%s' to '%s'", old_name, new_name);
	return false;
}

static bool
js_FS_writeFile(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	lstring_t*  text = NULL;

	filename = jsal_require_pathname(0, NULL);
	text = jsal_require_lstring_t(1);

	if (!fs_fspew(s_build->fs, filename, lstr_cstr(text), lstr_len(text)))
		jsal_error(JS_ERROR, "Couldn't write file '%s'", filename);
	lstr_free(text);
	return false;
}

static bool
js_new_FileStream(int num_args, bool is_ctor, intptr_t magic)
{
	FILE*        file;
	enum file_op file_op;
	const char*  filename;
	const char*  mode;

	filename = jsal_require_pathname(0, NULL);
	file_op = jsal_require_int(1);

	if (file_op < 0 || file_op >= FILE_OP_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid file-op constant");

	if (file_op == FILE_OP_UPDATE && !fs_fexist(s_build->fs, filename))
		file_op = FILE_OP_WRITE;  // ...because 'r+b' requires the file to exist.
	mode = file_op == FILE_OP_READ ? "rb"
		: file_op == FILE_OP_WRITE ? "w+b"
		: file_op == FILE_OP_UPDATE ? "r+b"
		: NULL;
	if (!(file = fs_fopen(s_build->fs, filename, mode)))
		jsal_error(JS_ERROR, "Couldn't open file '%s' in mode '%s'", filename, mode);
	if (file_op == FILE_OP_UPDATE)
		fseek(file, 0, SEEK_END);

	jsal_push_class_obj(CELL_FILE_STREAM, file, true);
	return true;
}

static void
js_FileStream_finalize(void* host_ptr)
{
	if (host_ptr != NULL)
		fclose(host_ptr);
}

static bool
js_FileStream_get_position(int num_args, bool is_ctor, intptr_t magic)
{
	FILE* file;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CELL_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream has already been disposed");

	jsal_push_number(ftell(file));
	return true;
}

static bool
js_FileStream_get_fileSize(int num_args, bool is_ctor, intptr_t magic)
{
	FILE* file;
	long  file_pos;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CELL_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream has already been disposed");

	file_pos = ftell(file);
	fseek(file, 0, SEEK_END);
	jsal_push_number(ftell(file));
	fseek(file, file_pos, SEEK_SET);
	return true;
}

static bool
js_FileStream_set_position(int num_args, bool is_ctor, intptr_t magic)
{
	FILE* file;
	long  new_pos;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CELL_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream has already been disposed");
	new_pos = (long)jsal_require_number(0);

	if (new_pos < 0)
		jsal_error(JS_RANGE_ERROR, "Invalid file position '%ld'", new_pos);
	fseek(file, new_pos, SEEK_SET);
	return false;
}

static bool
js_FileStream_dispose(int num_args, bool is_ctor, intptr_t magic)
{
	FILE* file;

	jsal_push_this();
	file = jsal_require_class_obj(-1, CELL_FILE_STREAM);

	if (file != NULL)
		fclose(file);
	jsal_set_class_ptr(-1, NULL);
	return false;
}

static bool
js_FileStream_read(int num_args, bool is_ctor, intptr_t magic)
{
	void*  data_ptr;
	FILE*  file;
	int    num_bytes = 0;
	long   position;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CELL_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream has already been disposed");
	if (num_args >= 1)
		num_bytes = jsal_require_int(0);

	if (num_bytes < 0)
		jsal_error(JS_RANGE_ERROR, "Invalid read size '%d'", num_bytes);

	if (num_args < 1) {  // if no arguments, read entire file back to front
		position = ftell(file);
		num_bytes = (fseek(file, 0, SEEK_END), ftell(file));
		fseek(file, 0, SEEK_SET);
	}
	jsal_push_new_buffer(JS_ARRAYBUFFER, num_bytes, &data_ptr);
	num_bytes = (int)fread(data_ptr, 1, num_bytes, file);
	if (num_args < 1)  // reset file position after whole-file read
		fseek(file, position, SEEK_SET);
	return true;
}

static bool
js_FileStream_write(int num_args, bool is_ctor, intptr_t magic)
{
	const void* data;
	FILE*       file;
	size_t      num_bytes;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CELL_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream has already been disposed");
	data = jsal_require_buffer_ptr(0, &num_bytes);

	if (fwrite(data, 1, num_bytes, file) != num_bytes)
		jsal_error(JS_ERROR, "Couldn't write '%zu' bytes to file", num_bytes);
	return false;
}

static bool
js_new_Image(int num_args, bool is_ctor, intptr_t magic)
{
	const char* pathname;
	image_t*    image;

	pathname = jsal_require_pathname(0, NULL);

	if (!(image = image_load(s_build->fs, pathname)))
		jsal_error(JS_ERROR, "Couldn't open image file '%s'", pathname);

	jsal_push_class_obj(CELL_IMAGE, image, true);
	return true;
}

static void
js_Image_finalize(void* host_ptr)
{
	if (host_ptr != NULL)
		image_free(host_ptr);
}

static bool
js_Image_get_bitmap(int num_args, bool is_ctor, intptr_t magic)
{
	void*     buffer_ptr;
	size_t    data_size;
	int       height;
	image_t*  image;
	uint8_t*  pixel_data;
	int       width;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CELL_IMAGE);

	pixel_data = (uint8_t*)image_bitmap(image, &data_size);
	width = image_width(image);
	height = image_height(image);
	jsal_push_new_buffer(JS_UINT8ARRAY, data_size, &buffer_ptr);
	memcpy(buffer_ptr, pixel_data, data_size);
	cache_value_to_this("bitmap");
	return true;
}

static bool
js_Image_get_height(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CELL_IMAGE);

	jsal_push_int(image_height(image));
	return true;
}

static bool
js_Image_get_width(int num_args, bool is_ctor, intptr_t magic)
{
	image_t* image;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CELL_IMAGE);

	jsal_push_int(image_width(image));
	return true;
}

static bool
js_Image_saveAs(int num_args, bool is_ctor, intptr_t magic)
{
	size_t      bitmap_size;
	int         height;
	image_t*    image;
	uint32_t*   in_buffer;
	uint32_t*   out_buffer;
	const char* pathname;
	int         width;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CELL_IMAGE);
	pathname = jsal_require_pathname(0, NULL);

	// if the image's pixel buffer has ever been accessed in JavaScript, make sure
	// the modified data gets written back before saving.
	if (jsal_has_own_prop_string(-1, "bitmap")) {
		jsal_get_prop_string(-1, "bitmap");
		width = image_width(image);
		height = image_height(image);
		in_buffer = jsal_get_buffer_ptr(-1, &bitmap_size);

		// ensure we have a valid input buffer and that it's the right size
		if (in_buffer != NULL && bitmap_size == width * height * sizeof(uint32_t)) {
			out_buffer = image_bitmap(image, NULL);
			memcpy(out_buffer, in_buffer, bitmap_size);
		}
	}

	if (!image_save(image, s_build->fs, pathname))
		jsal_error(JS_ERROR, "Couldn't write image file '%s'", pathname);
	return false;
}

static bool
js_Image_slice(int num_args, bool is_ctor, intptr_t magic)
{
	int      height;
	image_t* image;
	image_t* slice;
	int      width;
	int      x;
	int      y;

	jsal_push_this();
	image = jsal_require_class_obj(-1, CELL_IMAGE);
	x = jsal_require_int(0);
	y = jsal_require_int(1);
	width = jsal_require_int(2);
	height = jsal_require_int(3);

	if (width < 0 || height < 0)
		jsal_error(JS_RANGE_ERROR, "Invalid slice size '%dx%d'", width, height);
	if (x < 0 || y < 0 || x + width > image_width(image) || y + height > image_height(image))
		jsal_error(JS_RANGE_ERROR, "Invalid slice dimensions '(%d,%d) %dx%d'", x, y, width, height);

	if (!(slice = image_slice(image, x, y, width, height)))
		jsal_error(JS_ERROR, "Couldn't allocate memory for Image#slice");
	jsal_push_class_obj(CELL_IMAGE, slice, false);
	return true;
}

static bool
js_RNG_fromSeed(int num_args, bool is_ctor, intptr_t magic)
{
	uint64_t seed;
	xoro_t*  xoro;

	seed = (uint64_t)jsal_require_number(0);

	xoro = xoro_new(seed);
	jsal_push_class_obj(CELL_RNG, xoro, false);
	return true;
}

static bool
js_RNG_fromState(int num_args, bool is_ctor, intptr_t magic)
{
	const char* state;
	xoro_t*     xoro;

	state = jsal_require_string(0);

	xoro = xoro_new(0);
	if (!xoro_set_state(xoro, state)) {
		xoro_unref(xoro);
		jsal_error(JS_TYPE_ERROR, "Invalid RNG state string '%s'", state);
	}
	jsal_push_class_obj(CELL_RNG, xoro, false);
	return true;
}

static bool
js_new_RNG(int num_args, bool is_ctor, intptr_t magic)
{
	xoro_t* xoro;

	xoro = xoro_new((uint64_t)clock());
	jsal_push_class_obj(CELL_RNG, xoro, true);
	return true;
}

static void
js_RNG_finalize(void* host_ptr)
{
	xoro_unref(host_ptr);
}

static bool
js_RNG_get_state(int num_args, bool is_ctor, intptr_t magic)
{
	char    state[33];
	xoro_t* xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, CELL_RNG);

	xoro_get_state(xoro, state);
	jsal_push_string(state);
	return true;
}

static bool
js_RNG_set_state(int num_args, bool is_ctor, intptr_t magic)
{
	const char* state;
	xoro_t*     xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, CELL_RNG);
	state = jsal_require_string(0);

	if (!xoro_set_state(xoro, state))
		jsal_error(JS_TYPE_ERROR, "Invalid RNG state string '%s'", state);
	return false;
}

static bool
js_RNG_iterator(int num_args, bool is_ctor, intptr_t magic)
{
	xoro_t* xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, CELL_RNG);

	return true;
}

static bool
js_RNG_next(int num_args, bool is_ctor, intptr_t magic)
{
	xoro_t* xoro;

	jsal_push_this();
	xoro = jsal_require_class_obj(-1, CELL_RNG);

	jsal_push_new_object();
	jsal_push_boolean(false);
	jsal_put_prop_string(-2, "done");
	jsal_push_number(xoro_gen_double(xoro));
	jsal_put_prop_string(-2, "value");
	return true;
}

static void
js_Target_finalize(void* host_ptr)
{
	target_free(host_ptr);
}

static bool
js_Target_get_fileName(int num_args, bool is_ctor, intptr_t magic)
{
	target_t* target;

	jsal_push_this();
	target = jsal_require_class_obj(-1, CELL_TARGET);

	jsal_push_string(path_cstr(target_path(target)));
	return true;
}

static bool
js_Target_get_name(int num_args, bool is_ctor, intptr_t magic)
{
	target_t* target;

	jsal_push_this();
	target = jsal_require_class_obj(-1, CELL_TARGET);

	jsal_push_string(path_cstr(target_name(target)));
	return true;
}

static bool
js_new_TextDecoder(int num_args, bool is_ctor, intptr_t magic)
{
	decoder_t*  decoder;
	bool        fatal = false;
	bool        ignore_bom = false;
	const char* label = "utf-8";

	if (num_args >= 1)
		label = jsal_require_string(0);
	if (num_args >= 2) {
		jsal_require_object_coercible(1);
		if (jsal_get_prop_string(1, "fatal"))
			fatal = jsal_require_boolean(-1);
		if (jsal_get_prop_string(1, "ignoreBOM"))
			ignore_bom = jsal_require_boolean(-1);
	}

	// TextDecoder only supports UTF-8 for now.  in the future it'd be nice to support
	// at least UTF-16 and maybe CP-1252.
	if (strcasecmp(label, "unicode-1-1-utf-8") != 0
		&& strcasecmp(label, "utf-8") != 0
		&& strcasecmp(label, "utf8") != 0)
	{
		jsal_error(JS_TYPE_ERROR, "'%s' encoding is not supported", label);
	}

	decoder = decoder_new(fatal, ignore_bom);
	jsal_push_class_obj(CELL_TEXT_DEC, decoder, true);
	return true;
}

static void
js_TextDecoder_finalize(void* host_ptr)
{
	decoder_free(host_ptr);
}

static bool
js_TextDecoder_get_encoding(int num_args, bool is_ctor, intptr_t magic)
{
	decoder_t* decoder;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, CELL_TEXT_DEC);

	jsal_push_string("utf-8");
	return true;
}

static bool
js_TextDecoder_get_fatal(int num_args, bool is_ctor, intptr_t magic)
{
	decoder_t* decoder;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, CELL_TEXT_DEC);

	jsal_push_boolean(decoder_fatal(decoder));
	return true;
}

static bool
js_TextDecoder_get_ignoreBOM(int num_args, bool is_ctor, intptr_t magic)
{
	decoder_t* decoder;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, CELL_TEXT_DEC);

	jsal_push_boolean(decoder_ignore_bom(decoder));
	return true;
}

static bool
js_TextDecoder_decode(int num_args, bool is_ctor, intptr_t magic)
{
	decoder_t*  decoder;
	lstring_t*  head;
	const char* input = "";
	size_t      length = 0;
	lstring_t*  string;
	bool        streaming = false;
	lstring_t*  tail = NULL;

	jsal_push_this();
	decoder = jsal_require_class_obj(-1, CELL_TEXT_DEC);
	if (num_args >= 1)
		input = jsal_require_buffer_ptr(0, &length);
	if (num_args >= 2) {
		jsal_require_object_coercible(1);
		if (jsal_get_prop_string(1, "stream"))
			streaming = jsal_require_boolean(-1);
	}

	if (!(string = decoder_run(decoder, input, length)))
		jsal_error(JS_TYPE_ERROR, "Data to be decoded is not valid UTF-8 text");
	if (!streaming) {
		if (!(tail = decoder_finish(decoder)))
			jsal_error(JS_TYPE_ERROR, "Data to be decoded is not valid UTF-8 text");
		head = string;
		string = lstr_cat(head, tail);
		lstr_free(head);
		lstr_free(tail);
		jsal_push_lstring_t(string);
		lstr_free(string);
	}
	else {
		jsal_push_lstring_t(string);
		lstr_free(string);
	}
	return true;
}

static bool
js_new_TextEncoder(int num_args, bool is_ctor, intptr_t magic)
{
	encoder_t* encoder;

	encoder = encoder_new();
	jsal_push_class_obj(CELL_TEXT_ENC, encoder, true);
	return true;
}

static void
js_TextEncoder_finalize(void* host_ptr)
{
	encoder_free(host_ptr);
}

static bool
js_TextEncoder_get_encoding(int num_args, bool is_ctor, intptr_t magic)
{
	encoder_t* encoder;

	jsal_push_this();
	encoder = jsal_require_class_obj(-1, CELL_TEXT_ENC);

	jsal_push_string("utf-8");
	return true;
}

static bool
js_TextEncoder_encode(int num_args, bool is_ctor, intptr_t magic)
{
	void*      buffer;
	encoder_t* encoder;
	lstring_t* input;
	uint8_t*   output;
	size_t     size;

	jsal_push_this();
	encoder = jsal_require_class_obj(-1, CELL_TEXT_ENC);
	if (num_args >= 1)
		input = jsal_require_lstring_t(0);
	else
		input = lstr_new("");

	output = encoder_run(encoder, input, &size);
	jsal_push_new_buffer(JS_UINT8ARRAY, size, &buffer);
	memcpy(buffer, output, size);
	lstr_free(input);
	return true;
}

static bool
js_new_Tool(int num_args, bool is_ctor, intptr_t magic)
{
	tool_t*     tool;
	const char* verb = "building";

	jsal_require_function(0);
	if (num_args >= 2)
		verb = jsal_require_string(1);

	jsal_dup(0);
	tool = tool_new(verb);

	jsal_push_class_obj(CELL_TOOL, tool, true);
	return true;
}

static void
js_Tool_finalize(void* host_ptr)
{
	tool_unref(host_ptr);
}

static bool
js_Tool_stage(int num_args, bool is_ctor, intptr_t magic)
{
	int       length;
	path_t*   name;
	path_t*   out_path;
	target_t* source;
	target_t* target;
	tool_t*   tool;

	int i;

	jsal_push_this();
	tool = jsal_require_class_obj(-1, CELL_TOOL);
	out_path = path_new(jsal_require_string(0));
	jsal_require_array(1);
	if (num_args >= 3)
		jsal_require_object_coercible(2);

	name = path_new(path_filename(out_path));
	if (num_args >= 3) {
		if (jsal_get_prop_string(2, "name")) {
			path_free(name);
			name = path_new(jsal_require_string(-1));
		}
		jsal_pop(1);
	}

	target = target_new(name, s_build->fs, out_path, tool, s_build->timestamp, true);
	length = jsal_get_length(1);
	for (i = 0; i < length; ++i) {
		jsal_get_prop_index(1, i);
		source = jsal_require_class_obj(-1, CELL_TARGET);
		target_add_source(target, source);
		jsal_pop(1);
	}
	path_free(out_path);
	vector_push(s_build->targets, &target);

	jsal_push_class_obj(CELL_TARGET, target_ref(target), false);
	return true;
}

static bool
js_Z_deflate(int num_args, bool is_ctor, intptr_t magic)
{
	void*       buffer;
	const void* input_data;
	size_t      input_size;
	int         level = 6;
	void*       output_data;
	size_t      output_size;

	input_data = jsal_require_buffer_ptr(0, &input_size);
	if (num_args >= 2)
		level = jsal_require_int(1);

	if (level < 0 || level > 9)
		jsal_error(JS_RANGE_ERROR, "Invalid compression level '%d'", level);

	if (!(output_data = z_deflate(input_data, input_size, level, &output_size)))
		jsal_error(JS_ERROR, "Couldn't deflate THE PIG (it's too fat)");
	jsal_push_new_buffer(JS_ARRAYBUFFER, output_size, &buffer);
	memcpy(buffer, output_data, output_size);
	free(output_data);
	return true;
}

static bool
js_Z_inflate(int num_args, bool is_ctor, intptr_t magic)
{
	void*       buffer;
	const void* input_data;
	size_t      input_size;
	int         max_size = 0;
	void*       output_data;
	size_t      output_size;

	input_data = jsal_require_buffer_ptr(0, &input_size);
	if (num_args >= 2)
		max_size = jsal_require_int(1);

	if (max_size < 0)
		jsal_error(JS_RANGE_ERROR, "Invalid maximum size '%d'", max_size);

	if (!(output_data = z_inflate(input_data, input_size, max_size, &output_size)))
		jsal_error(JS_ERROR, "Couldn't inflate THE PIG (why would you even do this?)");
	jsal_push_new_buffer(JS_ARRAYBUFFER, output_size, &buffer);
	memcpy(buffer, output_data, output_size);
	free(output_data);
	return true;
}
