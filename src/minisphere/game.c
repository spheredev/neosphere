/**
 *  miniSphere JavaScript game engine
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

#include "minisphere.h"
#include "game.h"

#include "font.h"
#include "geometry.h"
#include "image.h"
#include "jsal.h"
#include "kev_file.h"
#include "package.h"
#include "windowstyle.h"

enum fs_type
{
	FS_UNKNOWN,
	FS_LOCAL,
	FS_PACKAGE,
};

struct game
{
	unsigned int   refcount;
	unsigned int   id;
	lstring_t*     author;
	char*          compiler;
	image_t*       default_arrow;
	image_t*       default_arrow_down;
	image_t*       default_arrow_up;
	font_t*        default_font;
	windowstyle_t* default_windowstyle;
	bool           fullscreen;
	lstring_t*     manifest;
	lstring_t*     name;
	size2_t        resolution;
	path_t*        root_path;
	fs_safety_t    safety;
	lstring_t*     save_id;
	path_t*        script_path;
	package_t*     package;
	lstring_t*     summary;
	int            type;
	int            version;
};

struct directory
{
	vector_t* entries;
	game_t*   game;
	int       position;
	path_t*   path;
};

struct file
{
	asset_t*      asset;
	enum fs_type  fs_type;
	game_t*       game;
	ALLEGRO_FILE* handle;
	const char*   path;
};

static void      load_default_assets (game_t* game);
static vector_t* read_directory      (const game_t* game, const char* dirname, bool want_dirs);
static bool      resolve_path        (const game_t* game, const char* filename, path_t* *out_path, enum fs_type *out_fs_type);
static bool      try_load_s2gm       (game_t* game, const lstring_t* json_text);

static unsigned int s_next_game_id = 1;

game_t*
game_open(const char* game_path)
{
	game_t*     game;
	package_t*  package;
	path_t*     path;
	size2_t     resolution;
	kev_file_t* sgm_file;
	size_t      sgm_size;
	char*       sgm_text = NULL;

	console_log(1, "opening '%s' from game #%u", game_path, s_next_game_id);

	game = game_ref(calloc(1, sizeof(game_t)));
	game->safety = FS_SAFETY_FULL;

	game->id = s_next_game_id;
	path = path_new(game_path);
	if (!path_resolve(path, NULL))
		goto on_error;
	if ((package = package_open(path_cstr(path)))) {  // Sphere Package (.spk)
		game->type = FS_PACKAGE;
		game->root_path = path_dup(path);
		game->package = package;
	}
	else if (path_has_extension(path, ".sgm") || path_filename_is(path, "game.json")) {  // game manifest
		game->type = FS_LOCAL;
		game->root_path = path_strip(path_dup(path));
	}
	else if (path_is_file(path)) {  // non-SPK file, assume JS script
		game->type = FS_LOCAL;
		game->root_path = path_strip(path_dup(path));
		game->safety = FS_SAFETY_RELAXED;

		// synthesize a game manifest for the script.  this way we make this trick of running
		// a bare script transparent to the rest of the engine, keeping things simple.
		console_log(1, "synthesizing manifest for '%s' from game #%u", path_cstr(path),
			s_next_game_id);
		game->version = path_has_extension(path, ".mjs") ? 2 : 1;
		game->name = lstr_new(path_filename(path));
		game->author = lstr_new("Author Unknown");
		game->summary = lstr_new(path_cstr(path));
		game->resolution = mk_size2(320, 240);
		game->script_path = path_insert_hop(path_new(path_filename(path)), 0, "@");
		game->fullscreen = false;
		jsal_push_new_object();
		jsal_push_lstring_t(game->name);
		jsal_put_prop_string(-2, "name");
		jsal_push_lstring_t(game->author);
		jsal_put_prop_string(-2, "author");
		jsal_push_lstring_t(game->summary);
		jsal_put_prop_string(-2, "summary");
		jsal_push_sprintf("%dx%d", game->resolution.width, game->resolution.height);
		jsal_put_prop_string(-2, "resolution");
		jsal_push_string(path_cstr(game->script_path));
		jsal_put_prop_string(-2, "main");

		jsal_stringify(-1);
		game->manifest = lstr_new(jsal_get_string(-1));
		jsal_pop(1);
	}
	else {  // default case, unpacked game folder
		game->type = FS_LOCAL;
		game->root_path = path_strip(path_dup(path));
	}
	path_free(path);
	path = NULL;

	// try to load the game manifest if one hasn't been synthesized already
	if (game->name == NULL) {
		if ((sgm_text = game_read_file(game, "@/game.json", &sgm_size))) {
			console_log(1, "parsing JSON manifest for game #%u", s_next_game_id);
			game->manifest = lstr_from_cp1252(sgm_text, sgm_size);
			if (!try_load_s2gm(game, game->manifest)) {
				console_log(0, "%s", jsal_to_string(-1));
				console_log(0, "   @ [@/game.json:0]");
				jsal_pop(1);
				goto on_error;
			}
			free(sgm_text);
			sgm_text = NULL;
		}
		else if ((sgm_file = kev_open(game, "@/game.sgm", false))) {
			console_log(1, "parsing SGM manifest for game #%u", s_next_game_id);
			game->version = 1;
			game->name = lstr_new(kev_read_string(sgm_file, "name", "Untitled"));
			game->author = lstr_new(kev_read_string(sgm_file, "author", "Author Unknown"));
			game->summary = lstr_new(kev_read_string(sgm_file, "description", "No information available."));
			game->resolution = mk_size2(
				kev_read_float(sgm_file, "screen_width", 320),
				kev_read_float(sgm_file, "screen_height", 240));
			game->script_path = game_full_path(game, kev_read_string(sgm_file, "script", "main.js"), "@/scripts", true);
			game->fullscreen = true;
			kev_close(sgm_file);

			// generate a JSON manifest (used by, e.g., Sphere.Game)
			jsal_push_new_object();
			jsal_push_lstring_t(game->name);
			jsal_put_prop_string(-2, "name");
			jsal_push_lstring_t(game->author);
			jsal_put_prop_string(-2, "author");
			jsal_push_lstring_t(game->summary);
			jsal_put_prop_string(-2, "summary");
			jsal_push_sprintf("%dx%d", game->resolution.width, game->resolution.height);
			jsal_put_prop_string(-2, "resolution");
			jsal_push_string(path_cstr(game->script_path));
			jsal_put_prop_string(-2, "main");

			jsal_stringify(-1);
			game->manifest = lstr_new(jsal_get_string(-1));
			jsal_pop(1);
		}
		else {
			goto on_error;
		}
	}

	load_default_assets(game);

	resolution = game_resolution(game);
	console_log(1, "         title: %s", game_name(game));
	console_log(1, "        author: %s", game_author(game));
	console_log(1, "    resolution: %dx%d", resolution.width, resolution.height);
	console_log(1, "       save ID: %s", game_save_id(game));

	s_next_game_id++;
	return game;

on_error:
	console_log(1, "couldn't open game #%u ", s_next_game_id++);
	path_free(path);
	free(sgm_text);
	if (game != NULL) {
		package_unref(game->package);
		free(game);
	}
	return NULL;
}

game_t*
game_ref(game_t* it)
{
	if (it == NULL)
		return NULL;

	++it->refcount;
	return it;
}

void
game_unref(game_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;

	console_log(3, "disposing game #%u no longer in use", it->id);
	if (it->type == FS_PACKAGE)
		package_unref(it->package);
	free(it->compiler);
	path_free(it->script_path);
	path_free(it->root_path);
	lstr_free(it->manifest);
	free(it);
}

const char*
game_author(const game_t* it)
{
	return lstr_cstr(it->author);
}

const char*
game_compiler(const game_t* it)
{
	return it->compiler;
}

bool
game_dir_exists(const game_t* it, const char* dirname)
{
	path_t*      dir_path = NULL;
	enum fs_type fs_type;
	struct stat  stats;

	if (!resolve_path(it, dirname, &dir_path, &fs_type))
		goto on_error;
	switch (fs_type) {
	case FS_LOCAL:
		if (stat(path_cstr(dir_path), &stats) != 0)
			goto on_error;
		path_free(dir_path);
		return (stats.st_mode & S_IFDIR) == S_IFDIR;
	case FS_PACKAGE:
		if (!package_dir_exists(it->package, path_cstr(dir_path)))
			goto on_error;
		path_free(dir_path);
		return true;
	}

on_error:
	path_free(dir_path);
	return false;
}

image_t*
game_default_arrow(const game_t* it)
{
	return it->default_arrow;
}

image_t*
game_default_arrow_down(const game_t* it)
{
	return it->default_arrow_down;
}

image_t*
game_default_arrow_up(const game_t* it)
{
	return it->default_arrow_up;
}

font_t*
game_default_font(const game_t* it)
{
	return it->default_font;
}

windowstyle_t*
game_default_windowstyle(const game_t* it)
{
	return it->default_windowstyle;
}

bool
game_file_exists(const game_t* it, const char* filename)
{
	enum fs_type fs_type;
	path_t*      path = NULL;
	struct stat  stats;

	if (!resolve_path(it, filename, &path, &fs_type))
		goto on_error;
	switch (fs_type) {
	case FS_LOCAL:
		if (stat(path_cstr(path), &stats) != 0)
			goto on_error;
		path_free(path);
		return (stats.st_mode & S_IFREG) == S_IFREG;
	case FS_PACKAGE:
		if (!package_file_exists(it->package, path_cstr(path)))
			goto on_error;
		path_free(path);
		return true;
	}

on_error:
	path_free(path);
	return false;
}

path_t*
game_full_path(const game_t* it, const char* filename, const char* base_dir_name, bool v1_mode)
{
	// note: '../' path hops are collapsed unconditionally, per SphereFS specification.
	//       this ensures the game can't subvert its sandbox by navigating outside through
	//       a symbolic link.

	path_t* base_path = NULL;
	path_t* path;
	char*   prefix;

	path = path_new(filename);
	if (path_is_rooted(path))  // absolute path?
		return path;
	if (base_dir_name != NULL) {
		base_path = game_full_path(it, base_dir_name, NULL, v1_mode);
		path_to_dir(base_path);
	}
	if (path_num_hops(path) > 0)
		prefix = strdup(path_hop(path, 0));
	else
		prefix = strdup("");

	// in legacy contexts only: '~/' is an alias for '@/'.
	if (v1_mode && strcmp(prefix, "~") == 0) {
		path_remove_hop(path, 0);
		path_insert_hop(path, 0, "@");
		free(prefix);
		prefix = strdup(path_hop(path, 0));
	}

	// '$/' is not a first-class prefix but an alias for '@/<scriptsDir>', so that's
	// what we want to canonicalize to if possible.
	if (strcmp(prefix, "$") == 0 && it->script_path != NULL) {
		path_remove_hop(path, 0);
		path_rebase(path, it->script_path);
		free(prefix);
		prefix = strdup(path_hop(path, 0));
	}

	// if the path doesn't contain a SphereFS prefix, it's relative and we need
	// to rebase it.
	if (!strpbrk(prefix, "@#~$%") || strlen(prefix) != 1) {
		if (base_path != NULL)
			path_rebase(path, base_path);
		else
			path_insert_hop(path, 0, "@");
		free(prefix);
		prefix = strdup(path_hop(path, 0));
	}
	path_remove_hop(path, 0);
	path_collapse(path, true);
	path_insert_hop(path, 0, prefix);
	free(prefix);
	path_free(base_path);
	return path;
}

bool
game_fullscreen(const game_t* it)
{
	return it->fullscreen;
}

const lstring_t*
game_manifest(const game_t* it)
{
	return it->manifest;
}

const char*
game_name(const game_t* it)
{
	return lstr_cstr(it->name);
}

const path_t*
game_path(const game_t* it)
{
	return it->root_path;
}

path_t*
game_relative_path(const game_t* it, const char* filename, const char* base_dir_name)
{
	path_t* base_path;
	path_t* path;

	path = game_full_path(it, filename, NULL, false);
	if (path_is_rooted(path))
		return path;
	base_path = game_full_path(it, base_dir_name, NULL, false);
	path_to_dir(base_path);
	if (path_hop_is(path, 0, path_hop(base_path, 0)))
		path_relativize(path, base_path);
	path_free(base_path);
	return path;
}

size2_t
game_resolution(const game_t* it)
{
	return it->resolution;
}

fs_safety_t
game_safety(const game_t* it)
{
	return it->safety;
}

const char*
game_save_id(const game_t* it)
{
	return it->save_id != NULL ? lstr_cstr(it->save_id)
		: NULL;
}

const path_t*
game_script_path(const game_t* it)
{
	return it->script_path;
}

const char*
game_summary(const game_t* it)
{
	return lstr_cstr(it->summary);
}

int
game_version(const game_t* it)
{
	return it->version;
}

bool
game_is_writable(const game_t* it, const char* pathname, bool v1_mode)
{
	// note: for compatibility with Sphere 1.x, if 'v1_mode' is true, then the game package
	//       is always treated as writable.

	const char* prefix;
	path_t*     path;

	path = game_full_path(g_game, pathname, NULL, v1_mode);
	prefix = path_hop(path, 0);  // safe, prefix is always present
	return strcmp(prefix, "~") == 0
		|| (strcmp(prefix, "@") == 0 && (v1_mode || it->safety < FS_SAFETY_FULL))
		|| it->safety == FS_SAFETY_NONE;
}

bool
game_mkdir(game_t* it, const char* dirname)
{
	enum fs_type  fs_type;
	path_t*       path;

	if (!resolve_path(it, dirname, &path, &fs_type))
		return false;
	switch (fs_type) {
	case FS_LOCAL:
		return path_mkdir(path);
	case FS_PACKAGE:
		return false;
	default:
		return false;
	}
}

void*
game_read_file(game_t* it, const char* filename, size_t *out_size)
{
	char*   data;
	size_t  data_size;
	file_t* file = NULL;

	if (!(file = file_open(it, filename, "rb")))
		goto on_error;
	file_seek(file, 0, WHENCE_END);
	data_size = file_position(file);
	if (!(data = malloc(data_size + 1)))
		goto on_error;
	file_seek(file, 0, WHENCE_SET);
	file_read(file, data, 1, data_size);
	file_close(file);
	data[data_size] = '\0';  // nifty NUL terminator

	if (out_size != NULL)
		*out_size = data_size;
	return data;

on_error:
	file_close(file);
	return NULL;
}

bool
game_rename(game_t* it, const char* name1, const char* name2)
{
	enum fs_type fs_type_1;
	enum fs_type fs_type_2;
	path_t*      path1;
	path_t*      path2;

	if (!resolve_path(it, name1, &path1, &fs_type_1))
		return false;
	if (!resolve_path(it, name2, &path2, &fs_type_2))
		return false;
	if (fs_type_2 != fs_type_1)
		return false;  // can't cross file system boundaries
	switch (fs_type_1) {
	case FS_LOCAL:
		// here's where we have to be careful.  on some platforms rename() with the same
		// filename for old and new will delete the file (!), and it will also happily overwrite
		// any existing file with the same name.
		if (path_is(path1, path2))
			return true;  // avoid rename() deleting file if name1 == name2
		if (game_file_exists(it, name2) || game_dir_exists(it, name2))
			return false; // don't overwrite existing file
		return rename(path_cstr(path1), path_cstr(path2)) == 0;
	case FS_PACKAGE:
		return false;  // SPK packages are not writable
	default:
		return false;
	}
}

bool
game_rmdir(game_t* it, const char* dirname)
{
	enum fs_type fs_type;
	path_t*      path;

	if (!resolve_path(it, dirname, &path, &fs_type))
		return false;
	switch (fs_type) {
	case FS_LOCAL:
		return rmdir(path_cstr(path)) == 0;
	case FS_PACKAGE:
		return false;
	default:
		return false;
	}
}

bool
game_write_file(game_t* it, const char* filename, const void* buf, size_t size)
{
	file_t* file = NULL;

	if (!(file = file_open(it, filename, "wb")))
		return false;
	file_write(file, buf, 1, size);
	file_close(file);
	return true;
}

bool
game_unlink(game_t* it, const char* filename)
{
	enum fs_type fs_type;
	path_t*      path;

	if (!resolve_path(it, filename, &path, &fs_type))
		return false;
	switch (fs_type) {
	case FS_LOCAL:
		return unlink(path_cstr(path)) == 0;
	case FS_PACKAGE:
		return false;
	default:
		return false;
	}
}

directory_t*
directory_open(game_t* game, const char* dirname)
{
	directory_t* directory;

	if (!game_dir_exists(game, dirname))
		return NULL;

	directory = calloc(1, sizeof(directory_t));
	directory->game = game_ref(game);
	directory->path = path_new_dir(dirname);
	return directory;
}

void
directory_close(directory_t* it)
{
	iter_t iter;

	if (it->entries != NULL) {
		iter = vector_enum(it->entries);
		while (iter_next(&iter)) {
			path_free(*(path_t**)iter.ptr);
		}
		vector_free(it->entries);
	}
	path_free(it->path);
	game_unref(it->game);
	free(it);
}

int
directory_num_files(directory_t* it)
{
	if (it->entries == NULL)
		directory_rewind(it);
	return vector_len(it->entries);
}

const char*
directory_pathname(const directory_t* it)
{
	return path_cstr(it->path);
}

int
directory_position(const directory_t* it)
{
	return it->position;
}

const path_t*
directory_next(directory_t* it)
{
	path_t* path;

	if (it->entries == NULL)
		directory_rewind(it);

	if (it->position >= vector_len(it->entries))
		return NULL;
	path = *(path_t**)vector_get(it->entries, it->position++);
	return path;
}

void
directory_rewind(directory_t* it)
{
	vector_t*  dir_list;
	lstring_t* entry_name;
	path_t*    entry_path;
	vector_t*  file_list;
	vector_t*  path_list;

	iter_t iter;

	path_list = vector_new(sizeof(path_t*));

	file_list = read_directory(it->game, path_cstr(it->path), false);
	iter = vector_enum(file_list);
	while (iter_next(&iter)) {
		entry_name = *(lstring_t**)iter.ptr;
		entry_path = path_new(lstr_cstr(entry_name));
		path_rebase(entry_path, it->path);
		vector_push(path_list, &entry_path);
		lstr_free(entry_name);
	}
	vector_free(file_list);

	dir_list = read_directory(it->game, path_cstr(it->path), true);
	iter = vector_enum(dir_list);
	while (iter_next(&iter)) {
		entry_name = *(lstring_t**)iter.ptr;
		entry_path = path_new_dir(lstr_cstr(entry_name));
		path_rebase(entry_path, it->path);
		vector_push(path_list, &entry_path);
		lstr_free(entry_name);
	}
	vector_free(dir_list);

	if (it->entries != NULL) {
		iter = vector_enum(it->entries);
		while (iter_next(&iter)) {
			path_free(*(path_t**)iter.ptr);
		}
		vector_free(it->entries);
	}
	it->entries = path_list;
	it->position = 0;
}

bool
directory_seek(directory_t* it, int position)
{
	if (it->entries == NULL)
		directory_rewind(it);

	if (position > vector_len(it->entries))
		return false;
	it->position = position;
	return true;
}

file_t*
file_open(game_t* game, const char* filename, const char* mode)
{
	path_t* dir_path;
	file_t* file;
	path_t* file_path = NULL;

	file = calloc(1, sizeof(file_t));

	if (!resolve_path(game, filename, &file_path, &file->fs_type))
		goto on_error;
	switch (file->fs_type) {
	case FS_LOCAL:
		if (strchr(mode, 'w') || strchr(mode, '+') || strchr(mode, 'a')) {
			// write access requested, ensure directory exists
			dir_path = path_strip(path_dup(file_path));
			path_mkdir(dir_path);
			path_free(dir_path);
		}
		if (!(file->handle = al_fopen(path_cstr(file_path), mode)))
			goto on_error;
		break;
	case FS_PACKAGE:
		if (!(file->asset = asset_fopen(game->package, path_cstr(file_path), mode)))
			goto on_error;
		break;
	default:
		goto on_error;
	}
	path_free(file_path);

	file->game = game_ref(game);
	file->path = strdup(filename);
	return file;

on_error:
	path_free(file_path);
	free(file);
	return NULL;
}

void
file_close(file_t* it)
{
	if (it == NULL)
		return;
	switch (it->fs_type) {
	case FS_LOCAL:
		al_fclose(it->handle);
		break;
	case FS_PACKAGE:
		asset_fclose(it->asset);
		break;
	}
	game_unref(it->game);
	free(it);
}

const char*
file_pathname(const file_t* it)
{
	return it->path;
}

long long
file_position(const file_t* it)
{
	switch (it->fs_type) {
	case FS_LOCAL:
		return al_ftell(it->handle);
	case FS_PACKAGE:
		return asset_ftell(it->asset);
	}
	return -1;
}

int
file_puts(file_t* it, const char* string)
{
	switch (it->fs_type) {
	case FS_LOCAL:
		return al_fputs(it->handle, string);
	case FS_PACKAGE:
		return asset_fputs(string, it->asset);
	default:
		return false;
	}
}

size_t
file_read(file_t* it, void* buf, size_t count, size_t size)
{
	size_t num_bytes;

	if (size == 0 || count == 0)
		return 0;  // dodges an Allegro assert

	switch (it->fs_type) {
	case FS_LOCAL:
		num_bytes = al_fread(it->handle, buf, size * count);
		return size > 0 ? num_bytes / size : 0;
	case FS_PACKAGE:
		return asset_fread(buf, size, count, it->asset);
	default:
		return 0;
	}
}

bool
file_seek(file_t* it, long long offset, whence_t whence)
{
	switch (it->fs_type) {
	case FS_LOCAL:
		return al_fseek(it->handle, offset, whence);
	case FS_PACKAGE:
		return asset_fseek(it->asset, offset, whence);
	}
	return false;
}

size_t
file_write(file_t* it, const void* buf, size_t count, size_t size)
{
	if (size == 0 || count == 0)
		return 0;  // dodges an Allegro assert

	switch (it->fs_type) {
	case FS_LOCAL:
		return al_fwrite(it->handle, buf, size * count) / size;
	case FS_PACKAGE:
		return asset_fwrite(buf, size, count, it->asset);
	default:
		return 0;
	}
}

static void
load_default_assets(game_t* game)
{
	path_t*     path;
	kev_file_t* system_ini;

	system_ini = kev_open(game, "#/system.ini", false);

	// system default font
	path = game_full_path(game,
		kev_read_string(system_ini, "Font", "system.rfn"),
		"#/", true);
	game->default_font = font_load(path_cstr(path));
	path_free(path);

	// system default windowstyle
	path = game_full_path(game,
		kev_read_string(system_ini, "WindowStyle", "system.rws"),
		"#/", true);
	game->default_windowstyle = winstyle_load(path_cstr(path));
	path_free(path);

	// system default pointer image
	path = game_full_path(game,
		kev_read_string(system_ini, "Arrow", "pointer.png"),
		"#/", true);
	game->default_arrow = image_load(path_cstr(path));
	path_free(path);

	// system default up arrow image
	path = game_full_path(game,
		kev_read_string(system_ini, "UpArrow", "up_arrow.png"),
		"#/", true);
	game->default_arrow_up = image_load(path_cstr(path));
	path_free(path);

	// system default down arrow image
	path = game_full_path(game,
		kev_read_string(system_ini, "DownArrow", "down_arrow.png"),
		"#/", true);
	game->default_arrow_down = image_load(path_cstr(path));
	path_free(path);

	kev_close(system_ini);
}

static bool
try_load_s2gm(game_t* game, const lstring_t* json_text)
{
	// note: this whole thing needs to be cleaned up.  it's pretty bad when Chakra
	//       does the JSON parsing for us yet the JSON manifest loader is STILL more
	//       complicated than the game.sgm loader.

	js_ref_t*   error_ref;
	int         res_x;
	int         res_y;
	const char* sandbox_mode;
	int         stack_top;

	stack_top = jsal_get_top();

	// load required entries
	jsal_push_lstring_t(json_text);
	if (!jsal_try_parse(-1))
		goto on_json_error;

	if (!jsal_get_prop_string(-1, "name") || !jsal_is_string(-1))
		goto on_error;
	game->name = lstr_new(jsal_get_string(-1));

	if (!jsal_get_prop_string(-2, "resolution") || !jsal_is_string(-1))
		goto on_error;
	sscanf(jsal_get_string(-1), "%dx%d", &res_x, &res_y);
	game->resolution = mk_size2(res_x, res_y);

	if (!jsal_get_prop_string(-3, "main") || !jsal_is_string(-1))
		goto on_error;
	game->script_path = game_full_path(game, jsal_get_string(-1), NULL, false);
	if (!path_hop_is(game->script_path, 0, "@")) {
		jsal_error(JS_TYPE_ERROR, "illegal SphereFS prefix '%s/' in '%s'", path_hop(game->script_path, 0),
			path_cstr(game->script_path));
	}

	// game summary is optional, use a default summary if one is not provided.
	if (jsal_get_prop_string(-4, "version") && jsal_is_number(-1))
		game->version = jsal_get_number(-1);
	else
		game->version = 2;

	if (jsal_get_prop_string(-5, "author") && jsal_is_string(-1))
		game->author = lstr_new(jsal_get_string(-1));
	else
		game->author = lstr_new("Author Unknown");

	if (jsal_get_prop_string(-6, "summary") && jsal_is_string(-1))
		game->summary = lstr_new(jsal_get_string(-1));
	else
		game->summary = lstr_new("No information available.");

	if (jsal_get_prop_string(-7, "saveID") && jsal_is_string(-1))
		game->save_id = lstr_new(jsal_get_string(-1));

	if (jsal_get_prop_string(-8, "fullScreen") && jsal_is_boolean(-1))
		game->fullscreen = jsal_get_boolean(-1);
	else
		game->fullscreen = game->version < 2;

	if (jsal_get_prop_string(-9, "sandbox") && jsal_is_string(-1)) {
		sandbox_mode = jsal_get_string(-1);
		game->safety = strcmp(sandbox_mode, "none") == 0 ? FS_SAFETY_NONE
			: strcmp(sandbox_mode, "relaxed") == 0 ? FS_SAFETY_RELAXED
			: FS_SAFETY_FULL;
	}
	else {
		game->safety = FS_SAFETY_FULL;
	}

	// load build metadata
	if (jsal_get_prop_string(-10, "$COMPILER") && jsal_is_string(-1))
		game->compiler = strdup(jsal_get_string(-1));

	jsal_set_top(stack_top);
	return true;

on_json_error:
	error_ref = jsal_ref(-1);
	jsal_set_top(stack_top);
	jsal_push_ref(error_ref);
	jsal_unref(error_ref);
	return false;

on_error:
	jsal_set_top(stack_top);
	jsal_push_new_error(JS_ERROR, "couldn't load JSON manifest");
	return false;
}

static vector_t*
read_directory(const game_t* game, const char* dirname, bool want_dirs)
{
	path_t*           dir_path;
	ALLEGRO_FS_ENTRY* file_info;
	path_t*           file_path;
	lstring_t*        filename;
	enum fs_type      fs_type;
	ALLEGRO_FS_ENTRY* fse = NULL;
	vector_t*         list = NULL;
	int               type_flag;

	if (!resolve_path(game, dirname, &dir_path, &fs_type))
		goto on_error;
	if (!(list = vector_new(sizeof(lstring_t*))))
		goto on_error;
	switch (fs_type) {
	case FS_LOCAL:
		type_flag = want_dirs ? ALLEGRO_FILEMODE_ISDIR : ALLEGRO_FILEMODE_ISFILE;
		fse = al_create_fs_entry(path_cstr(dir_path));
		if (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fse)) {
			while ((file_info = al_read_directory(fse))) {
				file_path = path_new(al_get_fs_entry_name(file_info));
				filename = lstr_new(path_filename(file_path));
				if (al_get_fs_entry_mode(file_info) & type_flag)
					vector_push(list, &filename);
				path_free(file_path);
				al_destroy_fs_entry(file_info);
			}
		}
		al_destroy_fs_entry(fse);
		break;
	case FS_PACKAGE:
		list = package_list_dir(game->package, path_cstr(dir_path), want_dirs);
		break;
	}
	path_free(dir_path);
	return list;

on_error:
	al_destroy_fs_entry(fse);
	path_free(dir_path);
	vector_free(list);
	return NULL;
}

static bool
resolve_path(const game_t* game, const char* filename, path_t* *out_path, enum fs_type *out_fs_type)
{
	// the path resolver is the core of SphereFS. it handles all canonization of paths
	// so that the game doesn't have to care whether it's running from a local directory,
	// Sphere SPK package, etc.

	path_t* origin;

	*out_path = path_new(filename);
	if (path_is_rooted(*out_path)) {  // absolute path
		*out_fs_type = FS_LOCAL;
		return true;
	}
	path_free(*out_path);
	*out_path = NULL;

	// process SphereFS path
	if (strlen(filename) >= 2 && memcmp(filename, "@/", 2) == 0) {
		// the @/ prefix is an alias for the game directory.  it is used in contexts
		// where a bare SphereFS filename may be ambiguous, e.g. in a require() call.
		if (game == NULL)
			goto on_error;
		*out_path = path_new(filename + 2);
		if (game->type == FS_LOCAL)
			path_rebase(*out_path, game->root_path);
		*out_fs_type = game->type;
	}
	else if (strlen(filename) >= 2 && memcmp(filename, "$/", 2) == 0) {
		// the $/ prefix refers to the location of the game's startup script.
		*out_path = path_new(filename + 2);
		origin = path_strip(path_dup(game_script_path(game)));
		path_rebase(*out_path, origin);
		if (game->type == FS_LOCAL)
			path_rebase(*out_path, game->root_path);
		path_free(origin);
		*out_fs_type = game->type;
	}
	else if (strlen(filename) >= 2 && memcmp(filename, "~/", 2) == 0) {
		// the ~/ prefix refers to the game's save data directory.  to improve sandboxing and
		// make things easier for developers using stock code, each game gets its own save data
		// directory.
		*out_path = path_new(filename + 2);
		origin = path_rebase(path_new("miniSphere/Save Data/"), home_path());
		path_append_dir(origin, game_save_id(game));
		path_rebase(*out_path, origin);
		path_free(origin);
		*out_fs_type = FS_LOCAL;
	}
	else if (strlen(filename) >= 2 && memcmp(filename, "#/", 2) == 0) {
		// the #/ prefix refers to the engine's "system" directory.

		// for an SPK package, see if the asset has been packaged and use it if so
		*out_path = path_new(filename);
		if (game != NULL && game->type == FS_PACKAGE
		    && package_file_exists(game->package, path_cstr(*out_path)))
		{
			*out_fs_type = FS_PACKAGE;
			return true;
		}
		else {
			path_free(*out_path);
		}

		// asset not in SPK package, use physical system directory
		*out_path = path_new(filename + 2);
		origin = path_rebase(path_new("system/"), assets_path());
		if (!path_resolve(origin, NULL)) {
			path_free(origin);
			origin = path_rebase(path_new("../share/minisphere/system/"), assets_path());
		}
		path_rebase(*out_path, origin);
		path_free(origin);
		*out_fs_type = FS_LOCAL;
	}
	else {  // no prefix: relative to '@/'
		// note: this shouldn't actually happen, since 'game_full_path()' always adds a prefix.
		//       however, there might still be some places internally where an unqualified path is
		//       used, so better to handle it here.
		if (game == NULL)
			goto on_error;
		*out_path = game_full_path(game, filename, NULL, false);
		if (path_num_hops(*out_path) > 0 && path_hop_is(*out_path, 0, "@"))
			path_remove_hop(*out_path, 0);
		if (game->type == FS_LOCAL)  // convert to absolute path
			path_rebase(*out_path, game->root_path);
		*out_fs_type = game->type;
	}

	return true;

on_error:
	path_free(*out_path);
	*out_path = NULL;
	*out_fs_type = FS_UNKNOWN;
	return false;
}
