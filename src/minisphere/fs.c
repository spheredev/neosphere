#include "minisphere.h"
#include "fs.h"

#include "geometry.h"
#include "kev_file.h"
#include "spk.h"

enum fs_type
{
	FS_UNKNOWN,
	FS_LOCAL,
	FS_SPK,
};

struct game
{
	unsigned int refcount;
	unsigned int id;
	lstring_t*   author;
	bool         fullscreen;
	lstring_t*   manifest;
	lstring_t*   name;
	size2_t      resolution;
	path_t*      root_path;
	lstring_t*   save_id;
	path_t*      script_path;
	lstring_t*   sourcemap;
	spk_t*       spk;
	lstring_t*   summary;
	int          type;
	int          version;
};

struct file
{
	game_t*       game;
	enum fs_type  fs_type;
	ALLEGRO_FILE* handle;
	const char*   path;
	spk_file_t*   spk_file;
};

static duk_ret_t duk_load_s2gm (duk_context* ctx, void* udata);
static bool      resolve_path  (const game_t* game, const char* filename, const char* base_dir, path_t* *out_path, enum fs_type *out_fs_type);

static unsigned int s_next_game_id = 1;

game_t*
game_open(const char* game_path)
{
	game_t*     game;
	path_t*     path;
	size2_t     resolution;
	size_t      sgm_size;
	kev_file_t* sgm_file;
	char*       sgm_text = NULL;
	spk_t*      spk;
	void*       sourcemap_data;
	size_t      sourcemap_size;

	console_log(1, "opening `%s` from game #%u", game_path, s_next_game_id);
	
	game = game_ref(calloc(1, sizeof(game_t)));
	
	game->id = s_next_game_id;
	path = path_new(game_path);
	if (!path_resolve(path, NULL))
		goto on_error;
	if (spk = open_spk(path_cstr(path))) {  // Sphere Package (.spk)
		game->type = FS_SPK;
		game->root_path = path_dup(path);
		game->spk = spk;
	}
	else if (path_has_extension(path, ".sgm") || path_filename_is(path, "game.json")) {  // game manifest
		game->type = FS_LOCAL;
		game->root_path = path_strip(path_dup(path));
	}
	else if (path_is_file(path)) {  // non-SPK file, assume JS script
		game->type = FS_LOCAL;
		game->root_path = path_strip(path_dup(path));

		// synthesize a game manifest for the script.  this way we make this trick of running
		// a bare script transparent to the rest of the engine, keeping things simple.
		console_log(1, "synthesizing manifest for `%s` from game #%u", path_cstr(path),
			s_next_game_id);
		game->version = 1;
		game->name = lstr_new(path_filename(path));
		game->author = lstr_new("Author Unknown");
		game->summary = lstr_new(path_cstr(path));
		game->resolution = size2(320, 240);
		game->script_path = path_insert_hop(path_new(path_filename(path)), 0, "@");
		game->fullscreen = false;
		duk_push_object(g_duk);
		duk_push_lstring_t(g_duk, game->name);
		duk_put_prop_string(g_duk, -2, "name");
		duk_push_lstring_t(g_duk, game->author);
		duk_put_prop_string(g_duk, -2, "author");
		duk_push_lstring_t(g_duk, game->summary);
		duk_put_prop_string(g_duk, -2, "summary");
		duk_push_sprintf(g_duk, "%dx%d", game->resolution.width, game->resolution.height);
		duk_put_prop_string(g_duk, -2, "resolution");
		duk_push_string(g_duk, path_cstr(game->script_path)); duk_put_prop_string(g_duk, -2, "main");
		game->manifest = lstr_new(duk_json_encode(g_duk, -1));
		duk_pop(g_duk);
	}
	else {  // default case, unpacked game folder
		game->type = FS_LOCAL;
		game->root_path = path_strip(path_dup(path));
	}
	path_free(path);
	path = NULL;

	// try to load the game manifest if one hasn't been synthesized already
	if (game->name == NULL) {
		if (sgm_text = game_read_file(game, "game.json", NULL, &sgm_size)) {
			console_log(1, "parsing JSON manifest for game #%u", s_next_game_id);
			game->manifest = lstr_from_cp1252(sgm_text, sgm_size);
			duk_push_pointer(g_duk, game);
			duk_push_lstring_t(g_duk, game->manifest);
			if (duk_safe_call(g_duk, duk_load_s2gm, NULL, 2, 1) != DUK_EXEC_SUCCESS) {
				console_log(0, "!!! %s", duk_to_string(g_duk, -1));
				console_log(0, "   @ [game.json:?]");
				duk_pop(g_duk);
				goto on_error;
			}
			duk_pop(g_duk);
			free(sgm_text);
			sgm_text = NULL;
		}
		else if (sgm_file = kev_open(game, "game.sgm", false)) {
			console_log(1, "parsing SGM manifest for game #%u", s_next_game_id);
			game->version = 1;
			game->name = lstr_new(kev_read_string(sgm_file, "name", "Untitled"));
			game->author = lstr_new(kev_read_string(sgm_file, "author", "Author Unknown"));
			game->summary = lstr_new(kev_read_string(sgm_file, "description", "No information available."));
			game->resolution = size2(
				kev_read_float(sgm_file, "screen_width", 320),
				kev_read_float(sgm_file, "screen_height", 240));
			game->script_path = game_canonicalize(game, kev_read_string(sgm_file, "script", "main.js"), "scripts", true);
			game->fullscreen = true;
			kev_close(sgm_file);

			// generate a JSON manifest (used by, e.g., Sphere.Game)
			duk_push_object(g_duk);
			duk_push_lstring_t(g_duk, game->name);
			duk_put_prop_string(g_duk, -2, "name");
			duk_push_lstring_t(g_duk, game->author);
			duk_put_prop_string(g_duk, -2, "author");
			duk_push_lstring_t(g_duk, game->summary);
			duk_put_prop_string(g_duk, -2, "summary");
			duk_push_sprintf(g_duk, "%dx%d", game->resolution.width, game->resolution.height);
			duk_put_prop_string(g_duk, -2, "resolution");
			duk_push_string(g_duk, path_cstr(game->script_path));
			duk_put_prop_string(g_duk, -2, "main");
			game->manifest = lstr_new(duk_json_encode(g_duk, -1));
			duk_pop(g_duk);
		}
		else
			goto on_error;
	}

	resolution = game_resolution(game);
	console_log(1, "         title: %s", game_name(game));
	console_log(1, "        author: %s", game_author(game));
	console_log(1, "    resolution: %dx%d", resolution.width, resolution.height);
	console_log(1, "       save ID: %s", game_save_id(game));

	// load the source map
	if (sourcemap_data = game_read_file(game, "source.json", NULL, &sourcemap_size))
		game->sourcemap = lstr_from_cp1252(sourcemap_data, sourcemap_size);
	free(sourcemap_data);

	s_next_game_id++;
	return game;

on_error:
	console_log(1, "couldn't open game #%u ", s_next_game_id++);
	path_free(path);
	free(sgm_text);
	if (game != NULL) {
		unref_spk(game->spk);
		free(game);
	}
	return NULL;
}

game_t*
game_ref(game_t* game)
{
	if (game == NULL)
		return NULL;

	++game->refcount;
	return game;
}

void
game_unref(game_t* game)
{
	if (game == NULL || --game->refcount > 0)
		return;

	console_log(3, "disposing game #%u no longer in use", game->id);
	if (game->type == FS_SPK)
		unref_spk(game->spk);
	lstr_free(game->sourcemap);
	path_free(game->script_path);
	path_free(game->root_path);
	lstr_free(game->manifest);
	free(game);
}

const char*
game_author(const game_t* game)
{
	return lstr_cstr(game->author);
}

bool
game_dir_exists(const game_t* game, const char* dirname, const char* base_dir)
{
	path_t*      dir_path = NULL;
	enum fs_type fs_type;
	struct stat  stats;

	if (!resolve_path(game, dirname, base_dir, &dir_path, &fs_type))
		goto on_error;
	switch (fs_type) {
	case FS_LOCAL:
		if (stat(path_cstr(dir_path), &stats) != 0)
			goto on_error;
		path_free(dir_path);
		return (stats.st_mode & S_IFDIR) == S_IFDIR;
	case FS_SPK:
		if (!spk_dir_exists(game->spk, path_cstr(dir_path)))
			goto on_error;
		path_free(dir_path);
		return true;
	}

on_error:
	path_free(dir_path);
	return false;
}

bool
game_file_exists(game_t* game, const char* filename, const char* base_dir)
{
	file_t* file;

	if (!(file = file_open(game, filename, base_dir, "rb")))
		return false;
	file_close(file);
	return true;
}

bool
game_fullscreen(const game_t* game)
{
	return game->fullscreen;
}

const lstring_t*
game_manifest(const game_t* game)
{
	return game->manifest;
}

const char*
game_name(const game_t* game)
{
	return lstr_cstr(game->name);
}

const path_t*
game_path(const game_t* game)
{
	return game->root_path;
}

size2_t
game_resolution(const game_t* game)
{
	return game->resolution;
}

const char*
game_save_id(const game_t* game)
{
	return game->save_id != NULL ? lstr_cstr(game->save_id)
		: NULL;
}

const path_t*
game_script_path(const game_t* game)
{
	return game->script_path;
}

const char*
game_summary(const game_t* game)
{
	return lstr_cstr(game->summary);
}

int
game_version(const game_t* game)
{
	return game->version;
}

path_t*
game_canonicalize(const game_t* game, const char* filename, const char* base_dir_name, bool legacy)
{
	// note: game_canonicalize() collapses '../' path hops unconditionally, as per
	//       SphereFS spec. this ensures an unpackaged game can't subvert the
	//       sandbox by navigating outside of its directory via a symbolic link.

	path_t* base_path = NULL;
	path_t* path;
	char*   prefix;

	path = path_new(filename);
	if (path_is_rooted(path))  // absolute path?
		return path;

	if (legacy && path_num_hops(path) >= 1 && path_hop_is(path, 0, "~")) {
		path_remove_hop(path, 0);
		path_insert_hop(path, 0, "@");
	}
	
	if (base_dir_name != NULL) {
		base_path = game_canonicalize(game, base_dir_name, NULL, legacy);
		path_to_dir(base_path);
	}
	if (path_num_hops(path) > 0)
		prefix = strdup(path_hop(path, 0));
	else
		prefix = strdup("");
	
	// canonicalize `$/` to `@/<scriptsDir>`.  this ensures each file has only
	// one canonical name.
	if (strcmp(prefix, "$") == 0) {
		path_remove_hop(path, 0);
		path_rebase(path, game_script_path(game));
		free(prefix);
		prefix = strdup(path_hop(path, 0));
	}
	
	// no need to check for a `$` prefix now, as the last step removes it
	// from the equation.
	if (!strpbrk(prefix, "@#~") || strlen(prefix) != 1) {
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

vector_t*
game_list_dir(const game_t* game, const char* dirname, const char* base_dir, bool want_dirs)
{
	path_t*           dir_path;
	ALLEGRO_FS_ENTRY* file_info;
	path_t*           file_path;
	lstring_t*        filename;
	enum fs_type      fs_type;
	ALLEGRO_FS_ENTRY* fse = NULL;
	vector_t*         list = NULL;
	int               type_flag;

	if (!resolve_path(game, dirname, base_dir, &dir_path, &fs_type))
		goto on_error;
	if (!(list = vector_new(sizeof(lstring_t*))))
		goto on_error;
	switch (fs_type) {
	case FS_LOCAL:
		type_flag = want_dirs ? ALLEGRO_FILEMODE_ISDIR : ALLEGRO_FILEMODE_ISFILE;
		fse = al_create_fs_entry(path_cstr(dir_path));
		if (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fse)) {
			while (file_info = al_read_directory(fse)) {
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
	case FS_SPK:
		list = list_spk_filenames(game->spk, path_cstr(dir_path), want_dirs);
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

bool
game_mkdir(game_t* game, const char* dirname, const char* base_dir)
{
	enum fs_type  fs_type;
	path_t*       path;

	if (!resolve_path(game, dirname, base_dir, &path, &fs_type))
		return false;
	switch (fs_type) {
	case FS_LOCAL:
		return path_mkdir(path);
	case FS_SPK:
		return false;
	default:
		return false;
	}
}

void*
game_read_file(game_t* game, const char* filename, const char* base_dir, size_t *out_size)
{
	size_t  data_size;
	file_t* file = NULL;
	void*   slurp;

	if (!(file = file_open(game, filename, base_dir, "rb")))
		goto on_error;
	file_seek(file, 0, WHENCE_END);
	data_size = file_position(file);
	if (!(slurp = malloc(data_size + 1)))
		goto on_error;
	file_seek(file, 0, WHENCE_SET);
	file_read(slurp, data_size, 1, file);
	file_close(file);
	*((char*)slurp + data_size) = '\0';  // nifty NUL terminator

	if (out_size) *out_size = data_size;
	return slurp;

on_error:
	file_close(file);
	return NULL;
}

bool
game_rename(game_t* game, const char* name1, const char* name2, const char* base_dir)
{
	enum fs_type fs_type_1;
	enum fs_type fs_type_2;
	path_t*      path1;
	path_t*      path2;

	if (!resolve_path(game, name1, base_dir, &path1, &fs_type_1))
		return false;
	if (!resolve_path(game, name2, base_dir, &path2, &fs_type_2))
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
		if (game_file_exists(game, name2, NULL) || game_dir_exists(game, name2, NULL))
			return false; // don't overwrite existing file
		return rename(path_cstr(path1), path_cstr(path2)) == 0;
	case FS_SPK:
		return false;  // SPK packages are not writable
	default:
		return false;
	}
}

bool
game_rmdir(game_t* game, const char* dirname, const char* base_dir)
{
	enum fs_type fs_type;
	path_t*      path;

	if (!resolve_path(game, dirname, base_dir, &path, &fs_type))
		return false;
	switch (fs_type) {
	case FS_LOCAL:
		return rmdir(path_cstr(path)) == 0;
	case FS_SPK:
		return false;
	default:
		return false;
	}
}

bool
game_write_file(game_t* game, const char* filename, const char* base_dir, const void* buf, size_t size)
{
	file_t* file = NULL;

	if (!(file = file_open(game, filename, base_dir, "wb")))
		return false;
	file_write(buf, size, 1, file);
	file_close(file);
	return true;
}

bool
game_unlink(game_t* game, const char* filename, const char* base_dir)
{
	enum fs_type fs_type;
	path_t*      path;

	if (!resolve_path(game, filename, base_dir, &path, &fs_type))
		return false;
	switch (fs_type) {
	case FS_LOCAL:
		return unlink(path_cstr(path)) == 0;
	case FS_SPK:
		return false;
	default:
		return false;
	}
}

file_t*
file_open(game_t* game, const char* filename, const char* base_dir, const char* mode)
{
	path_t* dir_path;
	file_t* file;
	path_t* file_path = NULL;

	file = calloc(1, sizeof(file_t));
	
	if (!resolve_path(game, filename, base_dir, &file_path, &file->fs_type))
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
	case FS_SPK:
		if (!(file->spk_file = spk_fopen(game->spk, path_cstr(file_path), mode)))
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
file_close(file_t* file)
{
	if (file == NULL)
		return;
	switch (file->fs_type) {
	case FS_LOCAL:
		al_fclose(file->handle);
		break;
	case FS_SPK:
		spk_fclose(file->spk_file);
		break;
	}
	game_unref(file->game);
	free(file);
}

const char*
file_pathname(const file_t* file)
{
	return file->path;
}

long long
file_position(const file_t* file)
{
	switch (file->fs_type) {
	case FS_LOCAL:
		return al_ftell(file->handle);
	case FS_SPK:
		return spk_ftell(file->spk_file);
	}
	return -1;
}

int
file_putc(int ch, file_t* file)
{
	switch (file->fs_type) {
	case FS_LOCAL:
		return al_fputc(file->handle, ch);
	case FS_SPK:
		return spk_fputc(ch, file->spk_file);
	default:
		return EOF;
	}
}

int
file_puts(const char* string, file_t* file)
{
	switch (file->fs_type) {
	case FS_LOCAL:
		return al_fputs(file->handle, string);
	case FS_SPK:
		return spk_fputs(string, file->spk_file);
	default:
		return false;
	}
}

size_t
file_read(void* buf, size_t size, size_t count, file_t* file)
{
	switch (file->fs_type) {
	case FS_LOCAL:
		return al_fread(file->handle, buf, size * count) / size;
	case FS_SPK:
		return spk_fread(buf, size, count, file->spk_file);
	default:
		return 0;
	}
}

bool
file_seek(file_t* file, long long offset, whence_t whence)
{
	switch (file->fs_type) {
	case FS_LOCAL:
		return al_fseek(file->handle, offset, whence) == 0;
	case FS_SPK:
		return spk_fseek(file->spk_file, offset, whence);
	}
	return false;
}

size_t
file_write(const void* buf, size_t size, size_t count, file_t* file)
{
	switch (file->fs_type) {
	case FS_LOCAL:
		return al_fwrite(file->handle, buf, size * count) / size;
	case FS_SPK:
		return spk_fwrite(buf, size, count, file->spk_file);
	default:
		return 0;
	}
}

static duk_ret_t
duk_load_s2gm(duk_context* ctx, void* udata)
{
	// note: this whole thing needs to be cleaned up.  it's pretty bad when Duktape
	//       does the JSON parsing for us yet the JSON manifest loader is STILL more
	//       complicated than the game.sgm loader.
	
	// arguments: -2 = game_t* game (pointer)
	//            -1 = game.json text (string)
	
	game_t*    game;
	duk_idx_t  json_idx;
	int        res_x;
	int        res_y;
	
	game = duk_get_pointer(ctx, -2);
	json_idx = duk_normalize_index(ctx, -1);
	
	// load required entries
	duk_dup(ctx, json_idx);
	duk_json_decode(ctx, -1);
	if (!duk_get_prop_string(g_duk, -1, "name") || !duk_is_string(g_duk, -1))
		goto on_error;
	game->name = lstr_new(duk_get_string(g_duk, -1));
	
	if (!duk_get_prop_string(g_duk, -2, "resolution") || !duk_is_string(g_duk, -1))
		goto on_error;
	sscanf(duk_get_string(g_duk, -1), "%dx%d", &res_x, &res_y);
	game->resolution = size2(res_x, res_y);
	
	if (!duk_get_prop_string(g_duk, -3, "main") || !duk_is_string(g_duk, -1))
		goto on_error;
	game->script_path = game_canonicalize(game, duk_get_string(g_duk, -1), NULL, false);

	// game summary is optional, use a default summary if one is not provided.
	if (duk_get_prop_string(g_duk, -4, "version") && duk_is_number(g_duk, -1))
		game->version = duk_get_number(g_duk, -1);
	else
		game->version = 2;
	
	if (duk_get_prop_string(g_duk, -5, "author") && duk_is_string(g_duk, -1))
		game->author = lstr_new(duk_get_string(g_duk, -1));
	else
		game->author = lstr_new("Author Unknown");
	
	if (duk_get_prop_string(g_duk, -6, "summary") && duk_is_string(g_duk, -1))
		game->summary = lstr_new(duk_get_string(g_duk, -1));
	else
		game->summary = lstr_new("No information available.");
	
	if (duk_get_prop_string(g_duk, -7, "saveID") && duk_is_string(g_duk, -1))
		game->save_id = lstr_new(duk_get_string(g_duk, -1));

	if (duk_get_prop_string(g_duk, -8, "fullScreen") && duk_is_boolean(g_duk, -1))
		game->fullscreen = duk_get_boolean(g_duk, -1);
	else
		game->fullscreen = true;

	return 0;

on_error:
	return -1;
}

static bool
resolve_path(const game_t* game, const char* filename, const char* base_dir, path_t* *out_path, enum fs_type *out_fs_type)
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
	else {  // default case: assume relative path
		if (game == NULL)
			goto on_error;
		*out_path = game_canonicalize(game, filename, base_dir, false);
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
