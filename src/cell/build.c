/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2021, Fat Cerberus
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
#include "build.h"

#include "api.h"
#include "compress.h"
#include "encoding.h"
#include "fs.h"
#include "image.h"
#include "module.h"
#include "spk_writer.h"
#include "target.h"
#include "tool.h"
#include "utility.h"
#include "visor.h"
#include "xoroshiro.h"
#include "wildmatch.h"

struct source
{
	char*      filename;
	lstring_t* text;
};

struct source_map
{
	char*     filename;
	js_ref_t* map;
};

struct build
{
	bool      crashed;
	bool      debuggable;
	fs_t*     fs;
	js_ref_t* install_tool;
	js_ref_t* manifest;
	vector_t* old_artifacts;
	vector_t* sources;
	vector_t* source_maps;
	vector_t* targets;
	time_t    timestamp;
	visor_t*  visor;
};

enum file_op
{
	FILE_OP_READ,
	FILE_OP_WRITE,
	FILE_OP_UPDATE,
	FILE_OP_MAX,
};

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
static bool js_FS_directoryOf                (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_extensionOf                (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_fileExists                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_fileNameOf                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_fullPath                   (int num_args, bool is_ctor, intptr_t magic);
static bool js_FS_match                      (int num_args, bool is_ctor, intptr_t magic);
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
static bool js_SSj_addSource                 (int num_args, bool is_ctor, intptr_t magic);
static bool js_SSj_sourceMap                 (int num_args, bool is_ctor, intptr_t magic);
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
static bool    install_target       (int num_args, bool is_ctor, intptr_t magic);
static void    make_file_targets    (fs_t* fs, const char* wildcard, const path_t* path, const path_t* subdir, vector_t* targets, bool recursive, time_t timestamp);
static bool    package_dir          (build_t* build, spk_writer_t* spk, const char* from_dirname, const char* to_dirname, bool recursive);
static int     sort_targets_by_path (const void* p_a, const void* p_b);
static bool    write_manifests      (build_t* build);

static build_t* s_build;

build_t*
build_new(const path_t* source_path, const path_t* out_path, bool debuggable)
{
	vector_t* artifacts;
	build_t*  build;
	char*     filename;
	fs_t*     fs;
	char*     json;
	size_t    json_size;
	int       num_artifacts;
	vector_t* sources;
	vector_t* source_maps;
	visor_t*  visor;

	int i;

	if (!(build = calloc(1, sizeof(build_t))))
		return NULL;
	visor = visor_new();
	fs = fs_new(path_cstr(source_path), path_cstr(out_path), NULL);

	visor_begin_op(visor, "setting up the Cell build environment");

	// initialize the Cellscript API
	api_init();
	modules_init(fs, false);
	api_define_func(NULL, "error", js_error, 0);
	api_define_func(NULL, "files", js_files, 0);
	api_define_func(NULL, "install", js_install, 0);
	api_define_func(NULL, "warn", js_warn, 0);
	api_define_static_prop("Sphere", "Compiler", js_Sphere_get_Compiler, NULL, 0);
	api_define_static_prop("Sphere", "Game", js_Sphere_get_Game, NULL, 0);
	api_define_static_prop("Sphere", "Version", js_Sphere_get_Version, NULL, 0);
	api_define_class("DirectoryStream", CELL_DIR_STREAM, js_new_DirectoryStream, js_DirectoryStream_finalize, 0);
	api_define_prop("DirectoryStream", "fileCount", false, js_DirectoryStream_get_fileCount, NULL);
	api_define_prop("DirectoryStream", "fileName", false, js_DirectoryStream_get_fileName, NULL);
	api_define_prop("DirectoryStream", "position", false, js_DirectoryStream_get_position, js_DirectoryStream_set_position);
	api_define_method("DirectoryStream", "@@iterator", js_DirectoryStream_iterator, 0);
	api_define_method("DirectoryStream", "dispose", js_DirectoryStream_dispose, 0);
	api_define_method("DirectoryStream", "next", js_DirectoryStream_next, 0);
	api_define_method("DirectoryStream", "rewind", js_DirectoryStream_rewind, 0);
	api_define_func("FS", "createDirectory", js_FS_createDirectory, 0);
	api_define_func("FS", "deleteFile", js_FS_deleteFile, 0);
	api_define_func("FS", "directoryExists", js_FS_directoryExists, 0);
	api_define_func("FS", "directoryOf", js_FS_directoryOf, 0);
	api_define_func("FS", "extensionOf", js_FS_extensionOf, 0);
	api_define_func("FS", "fileExists", js_FS_fileExists, 0);
	api_define_func("FS", "fileNameOf", js_FS_fileNameOf, 0);
	api_define_func("FS", "fullPath", js_FS_fullPath, 0);
	api_define_func("FS", "match", js_FS_match, 0);
	api_define_func("FS", "readFile", js_FS_readFile, 0);
	api_define_func("FS", "relativePath", js_FS_relativePath, 0);
	api_define_func("FS", "removeDirectory", js_FS_removeDirectory, 0);
	api_define_func("FS", "rename", js_FS_rename, 0);
	api_define_func("FS", "writeFile", js_FS_writeFile, 0);
	api_define_class("FileStream", CELL_FILE_STREAM, js_new_FileStream, js_FileStream_finalize, 0);
	api_define_prop("FileStream", "fileSize", false, js_FileStream_get_fileSize, NULL);
	api_define_prop("FileStream", "position", false, js_FileStream_get_position, js_FileStream_set_position);
	api_define_method("FileStream", "dispose", js_FileStream_dispose, 0);
	api_define_method("FileStream", "read", js_FileStream_read, 0);
	api_define_method("FileStream", "write", js_FileStream_write, 0);
	api_define_class("Image", CELL_IMAGE, js_new_Image, js_Image_finalize, 0);
	api_define_prop("Image", "bitmap", false, js_Image_get_bitmap, NULL);
	api_define_prop("Image", "height", false, js_Image_get_height, NULL);
	api_define_prop("Image", "width", false, js_Image_get_width, NULL);
	api_define_method("Image", "saveAs", js_Image_saveAs, 0);
	api_define_method("Image", "slice", js_Image_slice, 0);
	api_define_class("RNG", CELL_RNG, js_new_RNG, js_RNG_finalize, 0);
	api_define_func("RNG", "fromSeed", js_RNG_fromSeed, 0);
	api_define_func("RNG", "fromState", js_RNG_fromState, 0);
	api_define_prop("RNG", "state", false, js_RNG_get_state, js_RNG_set_state);
	api_define_method("RNG", "@@iterator", js_RNG_iterator, 0);
	api_define_method("RNG", "next", js_RNG_next, 0);
	api_define_func("SSj", "addSource", js_SSj_addSource, 0);
	api_define_func("SSj", "sourceMap", js_SSj_sourceMap, 0);
	api_define_class("Target", CELL_TARGET, NULL, js_Target_finalize, 0);
	api_define_prop("Target", "fileName", false, js_Target_get_fileName, NULL);
	api_define_prop("Target", "name", false, js_Target_get_name, NULL);
	api_define_class("TextDecoder", CELL_TEXT_DEC, js_new_TextDecoder, js_TextDecoder_finalize, 0);
	api_define_prop("TextDecoder", "encoding", false, js_TextDecoder_get_encoding, NULL);
	api_define_prop("TextDecoder", "fatal", false, js_TextDecoder_get_fatal, NULL);
	api_define_prop("TextDecoder", "ignoreBOM", false, js_TextDecoder_get_ignoreBOM, NULL);
	api_define_method("TextDecoder", "decode", js_TextDecoder_decode, 0);
	api_define_class("TextEncoder", CELL_TEXT_ENC, js_new_TextEncoder, js_TextEncoder_finalize, 0);
	api_define_prop("TextEncoder", "encoding", false, js_TextEncoder_get_encoding, NULL);
	api_define_method("TextEncoder", "encode", js_TextEncoder_encode, 0);
	api_define_class("Tool", CELL_TOOL, js_new_Tool, js_Tool_finalize, 0);
	api_define_method("Tool", "stage", js_Tool_stage, 0);
	api_define_func("Z", "deflate", js_Z_deflate, 0);
	api_define_func("Z", "inflate", js_Z_inflate, 0);

	api_define_const("FileOp", "Read", FILE_OP_READ);
	api_define_const("FileOp", "Write", FILE_OP_WRITE);
	api_define_const("FileOp", "Update", FILE_OP_UPDATE);

	// game manifest (gets JSON encoded at end of build)
	jsal_push_new_object();
	build->manifest = jsal_pop_ref();

	source_maps = vector_new(sizeof(struct source_map));
	sources = vector_new(sizeof(struct source));

	// create a Tool for the install() function to use
	jsal_push_new_function(install_target, "doInstall", 0, false, 0);
	jsal_push_class_obj(CELL_TOOL, tool_new("installing"), false);
	build->install_tool = jsal_pop_ref();

	// load artifacts from previous build
	artifacts = vector_new(sizeof(char*));
	if ((json = fs_fslurp(fs, "@/artifacts.json", &json_size))) {
		jsal_push_lstring(json, json_size);
		free(json);
		if (jsal_try_parse(-1)) {
			if (jsal_get_prop_string(-1, "builtTargets") && jsal_is_object(-1)) {
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
		jsal_pop(1);
	}

	visor_end_op(visor);

	s_build = build;

	build->visor = visor;
	build->fs = fs;
	build->old_artifacts = artifacts;
	build->targets = vector_new(sizeof(target_t*));
	build->source_maps = source_maps;
	build->sources = sources;
	build->debuggable = debuggable;
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

	iter = vector_enum(build->old_artifacts);
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
	int         error_column = 0;
	int         error_line = 0;
	char*       error_stack = NULL;
	char*       error_url = NULL;
	bool        is_ok = true;
	struct stat stats;

	if (fs_stat(build->fs, filename, &stats) != 0)
		return false;

	visor_begin_op(build->visor, "evaluating '%s'", filename);
	build->timestamp = stats.st_mtime;
	if (!module_eval(filename, false)) {
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
		visor_error(build->visor, "Cellscript crash due to uncaught exception");
		visor_end_op(build->visor);
		if (error_url != NULL && error_line > 0)
			printf("\nCRASH: error at '%s':%d:%d\n", error_url, error_line, error_column);
		else if (error_url != NULL)
			printf("\nCRASH: error in '%s'\n", error_url);
		else
			printf("\nCRASH: uncaught JavaScript exception\n");
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
build_init_dir(build_t* it)
{
	char          author[256];
	char*         author_for_js;
	directory_t*  dir;
	const path_t* in_path;
	int           num_overwriting = 0;
	path_t*       origin_path;
	path_t*       out_path;
	char*         cellscript_output;
	char*         mainjs_output;
	char          summary[256];
	char*         summary_for_js;
	char          resolution[256];
	char*         resolution_for_js;
	char*         cellscript_template;
	char*         mainjs_template;
	time_t        current_time;
	struct tm     tm;
	char          current_year[5];
	char          title[256];
	char*         title_for_js;

	visor_begin_op(it->visor, "gathering information");
	origin_path = path_new("#/template/");
	dir = directory_open(it->fs, path_cstr(origin_path), true);
	while ((in_path = directory_next(dir))) {
		out_path = path_dup(in_path);
		path_relativize(out_path, origin_path);
		path_insert_hop(out_path, 0, "$");
		if (fs_fexist(it->fs, path_cstr(out_path))) {
			visor_print(it->visor, "found existing file '%s'", path_cstr(out_path));
			++num_overwriting;
		}
		path_free(out_path);
	}

	if (num_overwriting <= 0) {
		visor_prompt(it->visor, "title of new game?", title, sizeof title);
		visor_prompt(it->visor, "author's name?", author, sizeof author);
		visor_prompt(it->visor, "one-line summary?", summary, sizeof summary);
		visor_prompt(it->visor, "screen resolution (default is 320x240)?", resolution, sizeof resolution);
		visor_end_op(it->visor);
	}
	else {
		// existing files would be overwritten, not safe to continue
		visor_error(it->visor, "initialization would overwrite %d existing file(s)", num_overwriting);
		visor_end_op(it->visor);
		directory_close(dir);
		path_free(origin_path);
		return false;
	}
	
	visor_begin_op(it->visor, "copying in project files");
	fs_mkdir(it->fs, "$/");
	directory_seek(dir, 0);
	while ((in_path = directory_next(dir))) {
		out_path = path_dup(in_path);
		path_relativize(out_path, origin_path);
		path_insert_hop(out_path, 0, "$");
		visor_begin_op(it->visor, "copying in '%s'", path_cstr(out_path));
		fs_fcopy(it->fs, path_cstr(out_path), path_cstr(in_path), false);
		visor_end_op(it->visor);
		path_free(out_path);
	}
	directory_close(dir);
	path_free(origin_path);
	visor_end_op(it->visor);

	visor_begin_op(it->visor, "preparing Cellscript for use");
	title_for_js = strescq(title, '"');
	author_for_js = strescq(author, '"');
	summary_for_js = strescq(summary, '"');
	if (resolution[0] == '\0') {
		visor_warn(it->visor, "no resolution entered, using default value");
		resolution_for_js = strescq("320x240", '\'');
	}
	else {
		resolution_for_js = strescq(resolution, '\'');
	}
	current_time = time(NULL);
	tm = *localtime(&current_time);
	sprintf(current_year, "%d", tm.tm_year + 1900);
	cellscript_template = fs_fslurp(it->fs, "$/Cellscript.js.tmpl", NULL);
	cellscript_output = strfmt(cellscript_template, title_for_js, author_for_js, summary_for_js, resolution_for_js, NULL);
	fs_fspew(it->fs, "$/Cellscript.js", cellscript_output, strlen(cellscript_output));
	fs_unlink(it->fs, "$/Cellscript.js.tmpl");
	mainjs_template = fs_fslurp(it->fs, "$/scripts/main.js.tmpl", NULL);
	mainjs_output = strfmt(mainjs_template, title_for_js, current_year, author_for_js, NULL);
	fs_fspew(it->fs, "$/scripts/main.js", mainjs_output, strlen(mainjs_output));
	fs_unlink(it->fs, "$/scripts/main.js.tmpl");
	free(title_for_js);
	free(author_for_js);
	free(summary_for_js);
	free(resolution_for_js);
	free(cellscript_output);
	free(cellscript_template);
	free(mainjs_output);
	free(mainjs_template);
	visor_end_op(it->visor);

	return true;
}

bool
build_package(build_t* build, const char* filename, bool rebuilding)
{
	path_t*       in_path;
	path_t*       out_path;
	spk_writer_t* spk;

	iter_t iter;

	if (!build_run(build, rebuilding))
		return false;

	visor_begin_op(build->visor, "packaging game to '%s'", filename);
	spk = spk_create(filename);
	spk_add_file(spk, build->fs, "@/game.json", "game.json");
	spk_add_file(spk, build->fs, "@/game.sgm", "game.sgm");
	package_dir(build, spk, "#/game_modules", "#/game_modules", true);
	package_dir(build, spk, "#/lib", "#/lib", true);
	package_dir(build, spk, "#/runtime", "#/runtime", true);
	package_dir(build, spk, "#/scripts", "#/scripts", true);
	package_dir(build, spk, "#/shaders", "#/shaders", true);
	package_dir(build, spk, "#/", "#/", false);
	iter = vector_enum(visor_filenames(build->visor));
	while (iter_next(&iter)) {
		in_path = path_new(*(const char**)iter.ptr);
		if (path_num_hops(in_path) == 0 || !path_hop_is(in_path, 0, "@") || !path_is_file(in_path)) {
			path_free(in_path);
			continue;
		}
		out_path = path_dup(in_path);
		path_remove_hop(out_path, 0);
		visor_begin_op(build->visor, "packaging '%s'", path_cstr(out_path));
		spk_add_file(spk, build->fs, path_cstr(in_path), path_cstr(out_path));
		path_free(out_path);
		visor_end_op(build->visor);
	}
	if (build->debuggable)
		spk_add_file(spk, build->fs, "@/artifacts.json", "artifacts.json");
	spk_close(spk);
	visor_end_op(build->visor);
	return true;
}

bool
build_run(build_t* build, bool rebuilding)
{
	const char*        filename;
	vector_t*          filenames;
	const char*        json;
	size_t             json_size;
	const char*        last_filename = "";
	int                num_matches = 1;
	const path_t*      path;
	struct source*     source;
	struct source_map* source_map;
	const path_t*      source_path;
	vector_t*          sorted_targets;
	target_t**         target_ptr;

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

	// build all primary targets
	iter = vector_enum(build->targets);
	while ((target_ptr = iter_next(&iter))) {
		path = target_path(*target_ptr);
		if (path_num_hops(path) == 0 || !path_hop_is(path, 0, "@"))
			continue;
		target_build(*target_ptr, build->visor, rebuilding);
	}
	visor_end_op(build->visor);

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
	jsal_push_new_object();
	jsal_push_sprintf("%s %s", SPHERE_COMPILER_NAME, SPHERE_VERSION);
	jsal_put_prop_string(-2, "compiler");
	jsal_push_new_array();
	iter = vector_enum(filenames);
	while (iter_next(&iter)) {
		jsal_push_string(*(char**)iter.ptr);
		jsal_put_prop_index(-2, iter.index);
	}
	jsal_put_prop_string(-2, "builtTargets");
	if (build->debuggable) {
		jsal_push_new_object();
		iter = vector_enum(build->targets);
		while ((target_ptr = iter_next(&iter))) {
			path = target_path(*target_ptr);
			if (path_num_hops(path) == 0 || !path_hop_is(path, 0, "@") || !path_is_file(path))
				continue;
			if (!(source_path = target_source_path(*target_ptr)))
				continue;
			jsal_push_string(path_cstr(path));
			jsal_push_string(path_cstr(source_path));
			jsal_put_prop(-3);
		}
		jsal_put_prop_string(-2, "fileMap");

		jsal_push_new_object();
		iter = vector_enum(build->sources);
		while ((source = iter_next(&iter))) {
			jsal_push_string(source->filename);
			jsal_push_lstring_t(source->text);
			jsal_put_prop(-3);
		}
		jsal_put_prop_string(-2, "sources");

		jsal_push_new_object();
		iter = vector_enum(build->source_maps);
		while ((source_map = iter_next(&iter))) {
			jsal_push_string(source_map->filename);
			jsal_push_ref_weak(source_map->map);
			jsal_put_prop(-3);
		}
		jsal_put_prop_string(-2, "sourceMaps");
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
	jsal_to_propdesc_value(false, false, true);
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
	iter_i = vector_enum(build->old_artifacts);
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
	if (result == 0) {
		// touch file to prevent "target file unchanged" warning
		fs_utime(s_build->fs, target_path, NULL);
	}
	jsal_push_boolean(result == 0);
	return true;
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

	if (!(list = fs_list_dir(fs, path_cstr(path), false)))
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

static bool
package_dir(build_t* build, spk_writer_t* spk, const char* from_dirname, const char* to_dirname, bool recursive)
{
	vector_t* file_list = NULL;
	path_t*   from_path;
	path_t*   from_dir_path;
	path_t*   to_dir_path;
	path_t*   to_path;

	iter_t iter;

	from_dir_path = path_new_dir(from_dirname);
	to_dir_path = path_new_dir(to_dirname);
	if (!(file_list = fs_list_dir(build->fs, path_cstr(from_dir_path), false)))
		goto on_error;
	iter = vector_enum(file_list);
	while (iter_next(&iter)) {
		from_path = *(path_t**)iter.ptr;
		to_path = path_dup(from_path);
		path_relativize(to_path, from_dir_path);
		path_rebase(to_path, to_dir_path);
		if (path_is_file(from_path))
			spk_add_file(spk, build->fs, path_cstr(from_path), path_cstr(to_path));
		else if (recursive)
			package_dir(build, spk, path_cstr(from_path), path_cstr(to_path), true);
		path_free(to_path);
		path_free(from_path);
	}
	vector_free(file_list);
	return true;

on_error:
	vector_free(file_list);
	path_free(from_dir_path);
	path_free(to_dir_path);
	return false;
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
	int         api_level = 1;
	int         api_version = 2;
	FILE*       file;
	int         height;
	size_t      json_size;
	const char* json_text;
	path_t*     main_path;
	const char* save_id;
	path_t*     script_path;
	size_t      span;
	int         width;

	visor_begin_op(build->visor, "writing Sphere manifest files");

	// TODO: fix the stack management in this function so this extra push
	//       isn't necessary
	jsal_push_ref_weak(build->manifest);
	jsal_push_ref_weak(build->manifest);

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
		visor_warn(build->visor, "'version': missing value: targeting Sphere v2 platform");
	}

	if (jsal_get_prop_string(-5, "apiLevel")) {
		if (api_version < 2) {
			visor_warn(build->visor, "'apiLevel': value doesn't apply to Sphere v1, ignored");
		}
		else if (jsal_is_number(-1)) {
			api_level = jsal_get_int(-1);
			if (api_level < 1)
				visor_error(build->visor, "'apiLevel': must be greater than zero, found '%d'", api_level);
			else if (api_level > SPHERE_API_LEVEL && api_version <= SPHERE_API_VERSION)
				visor_warn(build->visor, "'apiLevel': value '%d' targets future Sphere version", api_level);
			else if (api_level > SPHERE_API_LEVEL_STABLE && api_version <= SPHERE_API_VERSION)
				visor_warn(build->visor, "'apiLevel': value '%d' targets unreleased API", api_level);
		}
		else {
			visor_error(build->visor, "'apiLevel': must be a number greater than zero, found '%s'", jsal_to_string(-1));
		}
	}
	else {
		if (api_version >= 2)
			visor_warn(build->visor, "'apiLevel': missing value: targeting API level 1");
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
		if (api_version <= 1 && path_extension_is(main_path, ".mjs"))
			visor_warn(build->visor, "'main': '%s' will not run as a module", path_cstr(main_path));
		if (api_level >= 4 && path_extension_is(main_path, ".cjs")) {
			visor_error(build->visor, "CommonJS main '%s' unsupported when targeting API 4+", path_cstr(main_path));
			jsal_pop(7);
			visor_end_op(build->visor);
			return false;
		}
		jsal_push_string(path_cstr(main_path));
		jsal_put_prop_string(-7, "main");
	}
	else {
		visor_error(build->visor, "'main': missing or invalid 'main' value");
		jsal_pop(7);
		visor_end_op(build->visor);
		return false;
	}

	jsal_get_prop_string(-8, "saveID");
	if ((save_id = jsal_get_string(-1))) {
		span = strspn(save_id, "abcdefghijklmnopqrstuvwxyzABCDEFGHIKLMNOPQRSTUVWXYZ0123456789-_.");
		if (span != strlen(save_id))
			visor_error(build->visor, "'saveID': invalid character '%c' in save ID", save_id[span]);
	}
	else {
		visor_warn(build->visor, "'saveID': no save ID - '~/' prefix will be disabled");
	}

	// write game.sgm (legacy manifest, for compatibility with Sphere 1.x)
	// note: SGM requires the main script path to be relative to '@/scripts'.
	//       this differs from Sphere v2 (game.json), where it's relative to '@/'.
	file = fs_fopen(build->fs, "@/game.sgm", "wb");
	fprintf(file, "name=%s\n", jsal_to_string(-8));
	fprintf(file, "author=%s\n", jsal_to_string(-7));
	fprintf(file, "description=%s\n", jsal_to_string(-6));
	if (api_version < 2) {
		script_path = fs_relative_path(path_cstr(main_path), "@/scripts");
		fprintf(file, "screen_width=%d\n", width);
		fprintf(file, "screen_height=%d\n", height);
		fprintf(file, "script=%s\n", path_cstr(script_path));
		path_free(script_path);
	}
	fclose(file);
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

	// extract the wildcard, if any, from the given path.  if only a directory name is given,
	// assume '*' for the wildcard pattern.
	path = path_new(pattern);
	if (path_is_file(path)) {
		wildcard = strdup(path_filename(path));
		path_strip(path);
	}
	else {
		wildcard = strdup("*");
	}

	// this is potentially recursive, so we defer to make_file_targets() to construct
	// the targets.  note: 'path' is assumed to refer to a directory here.
	targets = vector_new(sizeof(target_t*));
	make_file_targets(s_build->fs, wildcard, path, NULL, targets, recursive, s_build->timestamp);
	path_free(path);
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
	vector_free(targets);
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

	jsal_push_ref_weak(s_build->install_tool);
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
	jsal_push_ref_weak(s_build->manifest);
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
	bool         recursive = false;
	directory_t* stream;

	pathname = jsal_require_pathname(0, NULL, false);
	if (num_args >= 2 && jsal_is_object_coercible(1)) {
		jsal_require_object(1);
		jsal_get_prop_string(1, "recursive");
		if (!jsal_is_undefined(-1))
			recursive = jsal_require_boolean(-1);
	}

	if (!(stream = directory_open(s_build->fs, pathname, recursive)))
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
	int           depth;
	const path_t* entry_path;
	const char*   extension;
	path_t*       rel_path;
	directory_t*  stream;

	jsal_push_this();
	if (!(stream = jsal_require_class_obj(-1, CELL_DIR_STREAM)))
		jsal_error(JS_ERROR, "DirectoryStream has already been disposed");

	entry_path = directory_next(stream);
	jsal_push_new_object();
	if (entry_path != NULL) {
		rel_path = path_dup(entry_path);
		path_relativize(rel_path, directory_path(stream));
		extension = path_extension(entry_path);
		depth = path_num_hops(rel_path);
		if (!path_is_file(rel_path))
			--depth;  // directory path, don't count the last hop

		jsal_push_boolean(false);
		jsal_put_prop_string(-2, "done");
		jsal_push_new_object();
		jsal_push_boolean(!path_is_file(entry_path));
		jsal_put_prop_string(-2, "isDirectory");
		jsal_push_string(path_cstr(rel_path));
		jsal_put_prop_string(-2, "fileName");
		jsal_push_string(path_cstr(entry_path));
		jsal_put_prop_string(-2, "fullPath");
		jsal_push_int(depth);
		jsal_put_prop_string(-2, "depth");
		if (path_is_file(entry_path)) {
			if (extension != NULL)
				jsal_push_string(extension);
			else
				jsal_push_null();
			jsal_put_prop_string(-2, "extension");
		}
		jsal_put_prop_string(-2, "value");

		path_free(rel_path);
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

	filename = jsal_require_pathname(0, NULL, true);

	if (fs_mkdir(s_build->fs, filename) != 0)
		jsal_error(JS_ERROR, "Couldn't create directory '%s'", filename);
	return false;
}

static bool
js_FS_deleteFile(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL, true);

	if (!fs_unlink(s_build->fs, filename))
		jsal_error(JS_ERROR, "Couldn't delete file '%s'", filename);
	return false;
}

static bool
js_FS_directoryExists(int num_args, bool is_ctor, intptr_t magic)
{
	const char* dirname;

	dirname = jsal_require_pathname(0, NULL, false);

	jsal_push_boolean(fs_dir_exists(s_build->fs, dirname));
	return true;
}

static bool
js_FS_directoryOf(int num_args, bool is_ctor, intptr_t magic)
{
	path_t*     path;
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false);

	path = path_strip(path_new(pathname));
	jsal_push_string(path_cstr(path));
	path_free(path);
	return true;
}

static bool
js_FS_extensionOf(int num_args, bool is_ctor, intptr_t magic)
{
	const char* extension;
	path_t*     path;
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false);

	path = path_new(pathname);
	if (path_is_file(path)) {
		extension = path_extension(path);
		if (extension != NULL)
			jsal_push_string(extension);
		else
			jsal_push_null();
		path_free(path);
		return true;
	}
	else {
		// there's no meaningful answer we can give for the filename extension of a directory;
		// throw a TypeError in that case.
		path_free(path);
		jsal_error(JS_TYPE_ERROR, "'FS.extensionOf' cannot be called on a directory");
	}
}

static bool
js_FS_fileExists(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL, false);

	jsal_push_boolean(fs_fexist(s_build->fs, filename));
	return true;
}

static bool
js_FS_fileNameOf(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	path_t*     path;
	const char* pathname;

	pathname = jsal_require_pathname(0, NULL, false);

	path = path_new(pathname);
	filename = path_filename(path);

	if (filename != NULL)
		jsal_push_string(filename);
	else
		jsal_push_undefined();
	path_free(path);
	return true;
}

static bool
js_FS_fullPath(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	const char* origin_pathname = NULL;

	if (num_args >= 2)
		origin_pathname = jsal_require_pathname(1, NULL, false);
	filename = jsal_require_pathname(0, origin_pathname, false);

	jsal_push_string(filename);
	return true;
}

static bool
js_FS_match(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	bool        matched = false;
	const char* pattern;

	filename = jsal_require_pathname(0, NULL, false);
	if (jsal_is_array(1)) {
		jsal_push_new_iterator(1);
		while (!matched && jsal_next(-1)) {
			pattern = jsal_require_pathname(-1, NULL, false);
			if (wildmatch(pattern, filename, WM_WILDSTAR) == WM_MATCH)
				matched = true;
			jsal_pop(1);
		}
		jsal_pop(1);
		jsal_push_boolean(matched);
		return true;
	}
	else if (jsal_is_string(1)) {
		pattern = jsal_require_pathname(1, NULL, false);
		jsal_push_boolean(wildmatch(pattern, filename, WM_WILDSTAR) == WM_MATCH);
		return true;
	}
	else {
		jsal_error(JS_TYPE_ERROR, "'%s' is not a string or array of strings", jsal_to_string(1));
	}
}

static bool
js_FS_readFile(int num_args, bool is_ctor, intptr_t magic)
{
	lstring_t*  content;
	void*       file_data;
	size_t      file_size;
	const char* filename;

	filename = jsal_require_pathname(0, NULL, false);

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

	pathname = jsal_require_pathname(0, NULL, false);
	base_pathname = jsal_require_pathname(1, NULL, false);

	path = fs_relative_path(pathname, base_pathname);
	jsal_push_string(path_cstr(path));
	path_free(path);
	return true;
}

static bool
js_FS_removeDirectory(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;

	filename = jsal_require_pathname(0, NULL, true);

	if (!fs_rmdir(s_build->fs, filename))
		jsal_error(JS_ERROR, "Couldn't remove directory '%s'", filename);
	return false;
}

static bool
js_FS_rename(int num_args, bool is_ctor, intptr_t magic)
{
	const char* old_name;
	const char* new_name;

	old_name = jsal_require_pathname(0, NULL, true);
	new_name = jsal_require_pathname(1, NULL, true);

	if (!fs_rename(s_build->fs, old_name, new_name))
		jsal_error(JS_ERROR, "Couldn't rename '%s' to '%s'", old_name, new_name);
	return false;
}

static bool
js_FS_writeFile(int num_args, bool is_ctor, intptr_t magic)
{
	const char* filename;
	lstring_t*  text = NULL;

	filename = jsal_require_pathname(0, NULL, true);
	text = jsal_require_lstring_t(1);

	visor_begin_op(s_build->visor, "writing '%s'", filename);
	if (!fs_fspew(s_build->fs, filename, lstr_cstr(text), lstr_len(text)))
		jsal_error(JS_ERROR, "Couldn't write file '%s'", filename);
	lstr_free(text);
	visor_add_file(s_build->visor, filename);
	visor_end_op(s_build->visor);
	return false;
}

static bool
js_new_FileStream(int num_args, bool is_ctor, intptr_t magic)
{
	FILE*        file;
	enum file_op file_op;
	const char*  filename;
	const char*  mode;

	jsal_require_string(0);
	file_op = jsal_require_int(1);
	if (file_op < 0 || file_op >= FILE_OP_MAX)
		jsal_error(JS_RANGE_ERROR, "Invalid file-op constant");

	filename = jsal_require_pathname(0, NULL, file_op != FILE_OP_READ);
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
	long  bytes_read;
	void* data_ptr;
	FILE* file;
	long  num_bytes = 0;
	long  position;

	jsal_push_this();
	if (!(file = jsal_require_class_obj(-1, CELL_FILE_STREAM)))
		jsal_error(JS_ERROR, "FileStream has already been disposed");
	if (num_args >= 1)
		num_bytes = jsal_require_int(0);

	if (num_bytes < 0)
		jsal_error(JS_RANGE_ERROR, "Invalid read size '%ld'", num_bytes);

	if (num_args < 1) {  // if no arguments, read entire file back to front
		position = ftell(file);
		fseek(file, 0, SEEK_END);
		num_bytes = ftell(file);
		fseek(file, 0, SEEK_SET);
	}
	jsal_push_new_buffer(JS_ARRAYBUFFER, num_bytes, &data_ptr);
	bytes_read = (long)fread(data_ptr, 1, num_bytes, file);
	if (bytes_read < num_bytes)
		jsal_error(JS_RANGE_ERROR, "Script tried to read past end of file");
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

	pathname = jsal_require_pathname(0, NULL, false);

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
	pathname = jsal_require_pathname(0, NULL, true);

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

static bool
js_SSj_addSource(int num_args, bool is_ctor, intptr_t magic)
{
	const char*   filename;
	path_t*       short_path;
	path_t*       short_path_origin;
	struct source source;
	lstring_t*    text;

	filename = jsal_require_pathname(0, NULL, false);
	text = jsal_require_lstring_t(1);

	short_path = path_new(filename);
	if (!path_hop_is(short_path, 0, "$")) {
		path_free(short_path);
		jsal_error(JS_TYPE_ERROR, "File '%s' is not in source tree", path_cstr(short_path));
	}
	short_path_origin = path_new("$/");
	path_relativize(short_path, short_path_origin);
	path_free(short_path_origin);
	source.filename = strdup(path_cstr(short_path));
	source.text = text;
	vector_push(s_build->sources, &source);
	path_free(short_path);
	return false;
}

static bool
js_SSj_sourceMap(int num_args, bool is_ctor, intptr_t magic)
{
	const char*       filename;
	struct source_map source_map;

	filename = jsal_require_string(0);
	if (!jsal_is_object(1) && !jsal_is_string(1))
		jsal_error(JS_TYPE_ERROR, "'%s' is not an object or string", jsal_to_string(1));

	// validate the source map provided as input
	if (jsal_is_string(1))
		jsal_parse(1);
	if (!jsal_is_object(1)
		|| !jsal_get_prop_string(1, "version")
		|| !jsal_get_prop_string(1, "mappings")
		|| jsal_get_int(-2) != 3
		|| !jsal_is_string(-1))
	{
		jsal_error(JS_TYPE_ERROR, "Invalid source map provided to 'SSj.sourceMap'");
	}

	source_map.filename = strdup(filename);
	source_map.map = jsal_ref(1);
	vector_push(s_build->source_maps, &source_map);
	return false;
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
	decoder_t*     decoder;
	lstring_t*     head;
	const uint8_t* input = NULL;
	size_t         length = 0;
	lstring_t*     string;
	bool           streaming = false;
	lstring_t*     tail = NULL;

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
	const char* filename;
	int         length;
	path_t*     name = NULL;
	path_t*     out_path;
	target_t*   source;
	target_t*   target;
	tool_t*     tool;

	int i;

	jsal_push_this();
	tool = jsal_require_class_obj(-1, CELL_TOOL);
	out_path = path_new(jsal_require_pathname(0, NULL, true));
	jsal_require_array(1);
	if (num_args >= 3)
		jsal_require_object_coercible(2);

	filename = path_filename(out_path);
	if (filename != NULL)
		name = path_new(filename);
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
