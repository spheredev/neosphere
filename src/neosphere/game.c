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

#include "neosphere.h"
#include "game.h"

#include "font.h"
#include "geometry.h"
#include "image.h"
#include "jsal.h"
#include "kev_file.h"
#include "package.h"
#include "tinydir.h"
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
	int            api_level;
	lstring_t*     author;
	lstring_t*     compiler;
	image_t*       default_arrow;
	image_t*       default_arrow_down;
	image_t*       default_arrow_up;
	font_t*        default_font;
	windowstyle_t* default_windowstyle;
	vector_t*      file_type_map;
	bool           fullscreen;
	js_ref_t*      manifest;
	lstring_t*     name;
	package_t*     package;
	size2_t        resolution;
	path_t*        root_path;
	lstring_t*     save_id;
	path_t*        script_path;
	lstring_t*     summary;
	enum fs_type   type;
	int            version;
};

struct directory
{
	vector_t* entries;
	game_t*   game;
	int       position;
	path_t*   path;
	bool      recursive;
};

struct file
{
	asset_t*      asset;
	enum fs_type  fs_type;
	game_t*       game;
	ALLEGRO_FILE* handle;
	const char*   path;
};

static bool      help_list_dir       (vector_t* list, const char* dirname, const path_t* origin_path, bool want_dirs, bool recursive);
static void      load_default_assets (game_t* game);
static bool      parse_json_data     (game_t* game, const char* pathname);
static vector_t* read_directory      (const game_t* game, const char* dirname, bool want_dirs, bool recursive);
static bool      resolve_pathname    (const game_t* game, const char* pathname, path_t* *out_path, enum fs_type *out_fs_type);

static unsigned int s_next_game_id = 1;

game_t*
game_open(const char* game_path)
{
	const char* compiler;
	game_t*     game;
	void*       json_data = NULL;
	const char* main_filename;
	package_t*  package;
	path_t*     path;
	size2_t     resolution;
	const char* res_string;
	int         res_x;
	int         res_y;
	const char* save_id;
	const char* script_filename;
	kev_file_t* sgm_file = NULL;

	console_log(1, "opening '%s' as game #%u", game_path, s_next_game_id);

	game = game_ref(calloc(1, sizeof(game_t)));

	game->id = s_next_game_id;
	path = path_new(game_path);
	if (!path_resolve(path, NULL))
		goto on_error;
	if ((package = package_open(path_cstr(path)))) {  // Sphere Package (.spk)
		game->type = FS_PACKAGE;
		game->root_path = path_dup(path);
		game->package = package;
	}
	else if (path_extension_is(path, ".sgm") || path_filename_is(path, "game.json")) {  // game manifest
		game->type = FS_LOCAL;
		game->root_path = path_strip(path_dup(path));
	}
#if defined(NEOSPHERE_SPHERUN)
	else if (path_is_file(path)) {  // non-SPK file, assume JS script
		game->type = FS_LOCAL;
		game->root_path = path_strip(path_dup(path));

		// synthesize a game manifest for the script.  this way we make this trick of running
		// a bare script transparent to the rest of the engine, keeping things simple.
		console_log(1, "synthesizing manifest for '%s' from game #%u", path_cstr(path),
			s_next_game_id);
		game->version = 2;
		game->api_level = SPHERE_API_LEVEL_STABLE;
		game->name = lstr_new(path_filename(path));
		game->author = lstr_new("Author Unknown");
		game->summary = lstr_new(path_cstr(path));
		game->resolution = mk_size2(320, 240);
		game->script_path = path_insert_hop(path_new(path_filename(path)), 0, "@");
		game->fullscreen = false;
		jsal_push_new_object();
		jsal_push_int(game->version);
		jsal_put_prop_string(-2, "version");
		jsal_push_int(game->api_level);
		jsal_put_prop_string(-2, "apiLevel");
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
		game->manifest = jsal_pop_ref();
	}
#endif
	else {  // default case, unpacked game folder
		game->type = FS_LOCAL;
		game->root_path = path_strip(path_dup(path));
	}
	path_free(path);
	path = NULL;

	// try to load the game manifest if one hasn't been synthesized already
	if (game->name == NULL) {
		if ((sgm_file = kev_open(game, "@/game.sgm", false))) {
			console_log(1, "parsing Sphere game manifest for game #%u", game->id);
			game->version = kev_read_int(sgm_file, "version", 1);
			if (game->version < 1) {
				console_error("invalid 'version' value '%d' in game manifest", game->version);
				goto on_error;
			}
			game->name = lstr_new(kev_read_string(sgm_file, "name", "Untitled"));
			game->author = lstr_new(kev_read_string(sgm_file, "author", "Author Unknown"));
			game->summary = lstr_new(kev_read_string(sgm_file, "description", "No information available."));
			compiler = kev_read_string(sgm_file, "compiler", NULL);
			if (compiler != NULL)
				game->compiler = lstr_new(compiler);
			main_filename = kev_read_string(sgm_file, "main", "");
			script_filename = kev_read_string(sgm_file, "script", "");
			if (game->version >= 2 || main_filename[0] != '\0') {
				// this is a Sphere v2 game manifest
				if (game->version < 2)
					game->version = 2;
				game->api_level = kev_read_int(sgm_file, "api", 1);
				game->script_path = game_full_path(game, main_filename, NULL, false);
				res_string = kev_read_string(sgm_file, "resolution", "320x240");
				if (sscanf(res_string, "%dx%d", &res_x, &res_y) != 2) {
					console_error("invalid resolution string '%s' in game manifest", res_string);
					goto on_error;
				}
				game->resolution = mk_size2(res_x, res_y);
				save_id = kev_read_string(sgm_file, "saveID", "");
				if (save_id[0] != '\0')
					game->save_id = lstr_new(save_id);
			}
			else {
				// legacy Sphere v1 manifest, likely created by Sphere 1.x
				if (script_filename[0] == '\0') {
					console_error("missing 'script' field in Sphere v1 game manifest");
					goto on_error;
				}
				game->script_path = game_full_path(game, script_filename, "@/scripts", true);
				game->resolution = mk_size2(
					kev_read_int(sgm_file, "screen_width", 320),
					kev_read_int(sgm_file, "screen_height", 240));
			}
			if (!game_file_exists(game, path_cstr(game->script_path))) {
				console_error("JS module '%s' named in game manifest not found", path_cstr(game->script_path));
				goto on_error;
			}
			kev_close(sgm_file);
			sgm_file = NULL;
		}
		if ((game_file_exists(game, "@/game.json"))) {
			if (!parse_json_data(game, "@/game.json")) {
				console_error("%s", jsal_to_string(-1));
				console_error("   @ [@/game.json:0]");
				jsal_pop(1);
				goto on_error;
			}
		}
		else if (game->version > 0) {
			// no `game.json`, synthesize JSON metadata (used by, e.g., Sphere.Game)
			jsal_push_new_object();
			jsal_push_int(game->version);
			jsal_put_prop_string(-2, "version");
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
			game->manifest = jsal_pop_ref();
		}
		else {
			goto on_error;
		}
	}

	load_default_assets(game);

	resolution = game_resolution(game);
	console_log(1, "         title: %s", game_name(game));
	console_log(1, "        author: %s", game_author(game));
	console_log(1, "    resolution: %d x %d", resolution.width, resolution.height);
	console_log(1, "       save ID: %s", game_save_id(game));

	s_next_game_id++;
	return game;

on_error:
	console_log(1, "couldn't open game #%u ", s_next_game_id++);
	path_free(path);
	free(json_data);
	kev_close(sgm_file);
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
	char* string;

	iter_t iter;

	if (it == NULL || --it->refcount > 0)
		return;

	console_log(3, "disposing game #%u no longer in use", it->id);

	if (it->file_type_map != NULL) {
		iter = vector_enum(it->file_type_map);
		while (iter_next(&iter)) {
			string = *(char**)iter.ptr;
			free(string);
		}
		vector_free(it->file_type_map);
	}

	if (it->type == FS_PACKAGE)
		package_unref(it->package);
	path_free(it->script_path);
	path_free(it->root_path);
	jsal_unref(it->manifest);
	free(it);
}

int
game_api_level(const game_t* it)
{
	return it->version >= 2 ? it->api_level : 0;
}

const char*
game_author(const game_t* it)
{
	return lstr_cstr(it->author);
}

const char*
game_compiler(const game_t* it)
{
	return it->compiler != NULL
		? lstr_cstr(it->compiler)
		: NULL;
}

bool
game_dir_exists(const game_t* it, const char* dirname)
{
	path_t*      dir_path = NULL;
	enum fs_type fs_type;
	struct stat  stats;

	if (!resolve_pathname(it, dirname, &dir_path, &fs_type))
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

	if (!resolve_pathname(it, filename, &path, &fs_type))
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
	if (path_rooted(path))  // absolute path?
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

	if (game_dir_exists(it, path_cstr(path)))
		path_to_dir(path);
	return path;
}

bool
game_fullscreen(const game_t* it)
{
	return it->fullscreen;
}

const js_ref_t*
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
game_relative_path(const game_t* it, const char* pathname, const char* base_dir_name)
{
	path_t* base_path;
	path_t* path;

	path = game_full_path(it, pathname, NULL, false);
	if (path_rooted(path))
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
game_is_prefix_path(const game_t* it, const char* pathname)
{
	// this may look unsafe but it actually isn't: if the `strpbrk()` check passes,
	// then we know the string is at least one character long, and accessing `pathname[1]`
	// is thus safe.
	return strpbrk(pathname, "@#~$%") == pathname
		&& (pathname[1] == '/' || pathname[1] == '\\');
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
		|| (strcmp(prefix, "@") == 0 && v1_mode);
}

bool
game_mkdir(game_t* it, const char* dirname)
{
	enum fs_type  fs_type;
	path_t*       path;

	if (!resolve_pathname(it, dirname, &path, &fs_type))
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
game_rename(game_t* it, const char* old_pathname, const char* new_pathname)
{
	enum fs_type new_fs_type;
	path_t*      new_path;
	enum fs_type old_fs_type;
	path_t*      old_path;

	if (!resolve_pathname(it, old_pathname, &old_path, &old_fs_type))
		return false;
	if (!resolve_pathname(it, new_pathname, &new_path, &new_fs_type))
		return false;
	if (new_fs_type != old_fs_type)
		return false;  // can't cross file system boundaries
	switch (old_fs_type) {
	case FS_LOCAL:
		// here's where we have to be careful.  on some platforms rename() with the same
		// filename for old and new will delete the file (!), and it will also happily overwrite
		// any existing file with the same name.
		if (path_is(old_path, new_path))
			return true;  // avoid rename() deleting file if oldname == newname
		if (game_file_exists(it, new_pathname) || game_dir_exists(it, new_pathname))
			return false; // don't overwrite existing file
		return rename(path_cstr(old_path), path_cstr(new_path)) == 0;
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

	if (!resolve_pathname(it, dirname, &path, &fs_type))
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

	if (!resolve_pathname(it, filename, &path, &fs_type))
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
directory_open(game_t* game, const char* dirname, bool recursive)
{
	directory_t* directory;

	if (!game_dir_exists(game, dirname))
		return NULL;

	if (!(directory = calloc(1, sizeof(directory_t))))
		return NULL;
	directory->game = game_ref(game);
	directory->path = path_new_dir(dirname);
	directory->recursive = recursive;
	return directory;
}

void
directory_close(directory_t* it)
{
	iter_t iter;

	if (it->entries != NULL) {
		iter = vector_enum(it->entries);
		while (iter_next(&iter))
			path_free(*(path_t**)iter.ptr);
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

const path_t*
directory_path(const directory_t* it)
{
	return it->path;
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
	path_t*    entry_path;
	vector_t*  file_list;
	vector_t*  path_list;

	iter_t iter;

	path_list = vector_new(sizeof(path_t*));

	file_list = read_directory(it->game, path_cstr(it->path), false, it->recursive);
	iter = vector_enum(file_list);
	while (iter_next(&iter)) {
		entry_path = *(path_t**)iter.ptr;
		vector_push(path_list, &entry_path);
	}
	vector_free(file_list);

	if (!it->recursive) {
		dir_list = read_directory(it->game, path_cstr(it->path), true, false);
		iter = vector_enum(dir_list);
		while (iter_next(&iter)) {
			entry_path = *(path_t**)iter.ptr;
			vector_push(path_list, &entry_path);
		}
		vector_free(dir_list);
	}

	if (it->entries != NULL) {
		iter = vector_enum(it->entries);
		while (iter_next(&iter))
			path_free(*(path_t**)iter.ptr);
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

	if (!(file = calloc(1, sizeof(file_t))))
		goto on_error;

	if (!resolve_pathname(game, filename, &file_path, &file->fs_type))
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

static bool
help_list_dir(vector_t* list, const char* dirname, const path_t* origin_path, bool want_dirs, bool recursive)
{
	tinydir_dir  dir_info;
	tinydir_file file_info;
	path_t*      path;
	path_t*      subdir_origin;
	path_t*      subdir_path;

	size_t i;

	if (tinydir_open_sorted(&dir_info, dirname) != 0)
		return false;
	for (i = 0; i < dir_info.n_files; ++i) {
		tinydir_readfile_n(&dir_info, &file_info, i);
		if (strcmp(file_info.name, ".") == 0 || strcmp(file_info.name, "..") == 0)
			continue;
		if ((bool)file_info.is_dir == want_dirs) {
			path = file_info.is_dir
				? path_new_dir(file_info.name)
				: path_new(file_info.name);
			path_rebase(path, origin_path);
			vector_push(list, &path);
		}
		if (file_info.is_dir && recursive) {
			subdir_path = path_new_dir(dirname);
			subdir_origin = path_dup(origin_path);
			path_append_dir(subdir_path, file_info.name);
			path_append_dir(subdir_origin, file_info.name);
			if (!help_list_dir(list, path_cstr(subdir_path), subdir_origin, want_dirs, recursive))
				goto on_error;
			path_free(subdir_path);
			path_free(subdir_origin);
		}
	}
	tinydir_close(&dir_info);
	return true;

on_error:
	tinydir_close(&dir_info);
	return false;
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
parse_json_data(game_t* game, const char* pathname)
{
	// note: when `game.sgm` is present, `game.json` is purely auxilliary. thus, if the `game_t`
	//       has been initialized as a Sphere v1 game prior to this call, it should not be upgraded
	//       to Sphere v2 unless there is proof otherwise (an explicit API level or JS entrypoint in
	//       the JSON data, for example).

	int         api_version = 0;
	js_ref_t*   error_ref;
	size_t      json_size;
	const char* json_text;
	const char* res_string;
	int         res_x;
	int         res_y;
	int         stack_top;

	console_log(1, "parsing JSON metadata for game #%u", game->id);

	stack_top = jsal_get_top();

	json_text = game_read_file(game, pathname, &json_size);
	jsal_push_lstring(json_text, json_size);
	if (!jsal_try_parse(-1))
		goto on_json_error;

	game->manifest = jsal_ref(-1);

	if (jsal_get_prop_string(-1, "version") && jsal_is_number(-1)) {
		api_version = jsal_get_number(-1);
		if (api_version >= game->version) {
			// main manifest is legacy, ignore the script path there
			path_free(game->script_path);
			game->version = api_version;
			game->script_path = NULL;
		}
		else {
			jsal_push_new_error(JS_ERROR, "'version' in JSON metadata earlier than main manifest (%d < %d)",
				api_version, game->version);
			goto on_json_error;
		}
	}
	else if (game->version == 0) {
		game->version = 2;
	}

	if (jsal_get_prop_string(-2, "apiLevel") && jsal_is_number(-1)) {
		if (api_version == 0)
			api_version = 2;
		game->api_level = jsal_get_number(-1);
		if (game->version < api_version) {
			// main manifest is legacy, ignore the script path there
			path_free(game->script_path);
			game->version = api_version;
			game->script_path = NULL;
		}
	}
	else if (game->api_level == 0) {
		game->api_level = 1;
	}

	if (jsal_get_prop_string(-3, "main") && jsal_is_string(-1)) {
		if (api_version == 0)
			game->version = 2;
		path_free(game->script_path);
		game->script_path = game_full_path(game, jsal_get_string(-1), NULL, false);
		if (!path_hop_is(game->script_path, 0, "@")) {
			jsal_push_new_error(JS_ERROR, "Illegal SphereFS prefix '%s/' in 'main'",
				path_hop(game->script_path, 0));
			goto on_json_error;
		}
		if (!game_file_exists(game, path_cstr(game->script_path))) {
			jsal_push_new_error(JS_ERROR, "JS module '%s' named in JSON metadata not found",
				path_cstr(game->script_path));
			goto on_json_error;
		}
	}
	else if (game->script_path == NULL && game->version >= 2) {
		jsal_push_new_error(JS_ERROR, "No JS entry point found in game manifest or JSON metadata");
		goto on_json_error;
	}

	if (jsal_get_prop_string(-4, "name") && jsal_is_string(-1))
		game->name = lstr_new(jsal_get_string(-1));
	else if (game->name == NULL)
		game->name = lstr_new("Untitled");

	if (jsal_get_prop_string(-5, "author") && jsal_is_string(-1))
		game->author = lstr_new(jsal_get_string(-1));
	else if (game->author == NULL)
		game->author = lstr_new("Author Unknown");

	if (jsal_get_prop_string(-6, "summary") && jsal_is_string(-1))
		game->summary = lstr_new(jsal_get_string(-1));
	else if (game->summary == NULL)
		game->summary = lstr_new("No information available.");

	if (jsal_get_prop_string(-7, "resolution") && jsal_is_string(-1)) {
		res_string = jsal_get_string(-1);
		if (sscanf(res_string, "%dx%d", &res_x, &res_y) != 2) {
			jsal_push_new_error(JS_ERROR, "Invalid resolution string '%s' in JSON metadata", res_string);
			goto on_json_error;
		}
		game->resolution = mk_size2(res_x, res_y);
	}
	else if (game->resolution.width == 0 || game->resolution.height == 0) {
		game->resolution = mk_size2(320, 240);
	}

	if (jsal_get_prop_string(-8, "saveID") && jsal_is_string(-1))
		game->save_id = lstr_new(jsal_get_string(-1));

	if (jsal_get_prop_string(-9, "fullScreen") && jsal_is_boolean(-1))
		game->fullscreen = jsal_get_boolean(-1);

	jsal_set_top(stack_top);

	jsal_push_ref_weak(game->manifest);
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
	jsal_push_int(game->version);
	jsal_put_prop_string(-2, "version");
	jsal_push_int(game->api_level);
	jsal_put_prop_string(-2, "apiLevel");
	jsal_pop(1);

	return true;

on_json_error:
	console_error("error in JSON metadata '%s'", path_cstr(game_path(game)));
	error_ref = jsal_ref(-1);
	jsal_set_top(stack_top);
	jsal_push_ref(error_ref);
	jsal_unref(error_ref);
	return false;
}

static vector_t*
read_directory(const game_t* game, const char* dirname, bool want_dirs, bool recursive)
{
	path_t*      dir_path;
	enum fs_type fs_type;
	vector_t*    list = NULL;
	path_t*      origin_path = NULL;
	path_t*      path;

	int i;

	if (!resolve_pathname(game, dirname, &dir_path, &fs_type))
		goto on_error;
	origin_path = path_new_dir(dirname);
	if (!(list = vector_new(sizeof(path_t*))))
		goto on_error;
	switch (fs_type) {
	case FS_LOCAL:
		if (!help_list_dir(list, path_cstr(dir_path), origin_path, want_dirs, recursive))
			goto on_error;
		break;
	case FS_PACKAGE:
		list = package_list_dir(game->package, path_cstr(dir_path), want_dirs, recursive);
		for (i = 0; i < vector_len(list); ++i) {
			path = *(path_t**)vector_get(list, i);
			path_rebase(path, origin_path);
		}
		break;
	}
	path_free(dir_path);
	path_free(origin_path);
	return list;

on_error:
	path_free(dir_path);
	path_free(origin_path);
	vector_free(list);
	return NULL;
}

static bool
resolve_pathname(const game_t* game, const char* pathname, path_t* *out_path, enum fs_type *out_fs_type)
{
	// the path resolver is the core of SphereFS.  it handles all canonization of paths
	// so that the game doesn't have to care whether it's running from a local directory,
	// Sphere SPK package, etc.

	path_t* origin;

	*out_path = path_new(pathname);
	if (path_rooted(*out_path)) {  // absolute path
		*out_fs_type = FS_LOCAL;
		return true;
	}
	path_free(*out_path);
	*out_path = NULL;

	// process SphereFS path
	if (strlen(pathname) >= 2 && memcmp(pathname, "@/", 2) == 0) {
		// the @/ prefix is an alias for the game directory.  it is used in contexts where
		// a bare SphereFS filename may be ambiguous, e.g. in an `import` statement.
		if (game == NULL)
			goto on_error;
		*out_path = path_new(&pathname[2]);
		if (game->type == FS_LOCAL)
			path_rebase(*out_path, game->root_path);
		*out_fs_type = game->type;
	}
	else if (strlen(pathname) >= 2 && memcmp(pathname, "$/", 2) == 0) {
		// the $/ prefix refers to the location of the game's startup script.
		*out_path = path_new(&pathname[2]);
		origin = path_strip(path_dup(game_script_path(game)));
		path_rebase(*out_path, origin);
		if (game->type == FS_LOCAL)
			path_rebase(*out_path, game->root_path);
		path_free(origin);
		*out_fs_type = game->type;
	}
	else if (strlen(pathname) >= 2 && memcmp(pathname, "~/", 2) == 0) {
		// the ~/ prefix refers to the game's save data directory.  to improve sandboxing and
		// make things easier for developers using stock code, each game gets its own save data
		// directory.
		if (game_save_id(game) == NULL)
			goto on_error;  // no save ID, can't resolve path
		*out_path = path_new(&pathname[2]);
		origin = path_rebase(path_new("Sphere Saves/"), home_path());
		path_append_dir(origin, game_save_id(game));
		path_rebase(*out_path, origin);
		path_free(origin);
		*out_fs_type = FS_LOCAL;
	}
	else if (strlen(pathname) >= 2 && memcmp(pathname, "#/", 2) == 0) {
		// the #/ prefix refers to the engine's "system" directory.

		// for an SPK package, see if the asset has been packaged and use it if so
		*out_path = path_new(pathname);
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
		*out_path = path_new(&pathname[2]);
		origin = path_rebase(path_new("system/"), assets_path());
		if (!path_resolve(origin, NULL)) {
			path_free(origin);
			origin = path_rebase(path_new("../share/sphere/system/"), assets_path());
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
		*out_path = game_full_path(game, pathname, NULL, false);
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
