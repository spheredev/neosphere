#include "cell.h"
#include "fs.h"

#include "tinydir.h"

#if defined(_WIN32)
#include <direct.h>
#endif

struct fs
{
	path_t* root_path;
	path_t* game_path;
	path_t* system_path;
	path_t* user_path;
};

static char* resolve (const fs_t* fs, const char* filename);

fs_t*
fs_new(const char* root_dir, const char* game_dir, const char* user_path)
{
	fs_t* fs;

	fs = calloc(1, sizeof(fs_t));
	fs->root_path = path_new_dir(root_dir);
	fs->game_path = path_new_dir(game_dir);
	fs->system_path = path_append(path_strip(path_new_self()), "system/");
	if (user_path != NULL)
		fs->user_path = path_new_dir(user_path);
	return fs;
}

void
fs_free(fs_t* fs)
{
	path_free(fs->root_path);
	path_free(fs->game_path);
	path_free(fs->system_path);
	path_free(fs->user_path);
	free(fs);
}

int
fs_fcopy(const fs_t* fs, const char* destination, const char* source, int overwrite)
{
	char* resolved_dest;
	char* resolved_src;

	resolved_dest = resolve(fs, destination);
	resolved_src = resolve(fs, source);
	if (resolved_dest == NULL || resolved_src == NULL) {
		errno = EACCES;  // sandboxing violation
		return -1;
	}

	return tinydir_copy(resolved_src, resolved_dest, !overwrite);
}

bool
fs_fexist(const fs_t* fs, const char* filename)
{
	struct stat sb;

	return fs_stat(fs, filename, &sb) == 0;
}

FILE*
fs_fopen(const fs_t* fs, const char* filename, const char* mode)
{
	FILE* file;
	char* resolved_name;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return NULL;
	}

	file = fopen(resolved_name, mode);
	free(resolved_name);
	return file;
}

void*
fs_fslurp(const fs_t* fs, const char* filename, size_t* out_size)
{
	void* buffer;
	char* resolved_name;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return NULL;
	}

	buffer = fslurp(resolved_name, out_size);
	free(resolved_name);
	return buffer;
}

bool
fs_fspew(const fs_t* fs, const char* filename, void* data, size_t size)
{
	char* resolved_name;
	bool  retval;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return false;
	}

	retval = fspew(data, size, resolved_name);
	free(resolved_name);
	return retval;
}

bool
fs_is_game_dir(const fs_t* fs, const char* dirname)
{
	path_t* full_path;
	bool    retval;
	char*   resolved_name;

	if (!(resolved_name = resolve(fs, dirname)))
		return false;
	full_path = path_new_dir(resolved_name);
	retval = path_cmp(full_path, fs->game_path);
	path_free(full_path);
	return retval;
}

vector_t*
fs_list_dir(const fs_t* fs, const char* dirname)
{
	tinydir_dir  dir_info;
	tinydir_file file_info;
	vector_t*    list;
	path_t*      origin_path;
	path_t*      path;
	char*        resolved_name;

	if (!(resolved_name = resolve(fs, dirname)))
		return NULL;

	if (tinydir_open(&dir_info, resolved_name) != 0)
		return NULL;
	origin_path = path_new_dir(dirname);
	list = vector_new(sizeof(path_t*));
	while (dir_info.has_next) {
		tinydir_readfile(&dir_info, &file_info);
		tinydir_next(&dir_info);
		if (strcmp(file_info.name, ".") == 0 || strcmp(file_info.name, "..") == 0)
			continue;
		path = file_info.is_dir
			? path_new_dir(file_info.name)
			: path_new(file_info.name);
		path_rebase(path, origin_path);
		vector_push(list, &path);
	}
	path_free(origin_path);
	free(resolved_name);
	return list;
}

path_t*
fs_make_path(const char* filename, const char* base_dir_name, bool legacy)
{
	// note: fs_make_path() collapses '../' path hops unconditionally, as per
	//       SphereFS spec. this ensures an unpackaged game can't subvert the
	//       sandbox by navigating outside of its directory via a symbolic link.

	path_t* base_path = NULL;
	path_t* path;
	char*   prefix;

	path = path_new(filename);
	if (path_is_rooted(path))  // absolute path?
		return path;

	if (legacy && path_num_hops(path) >= 1 && path_hop_cmp(path, 0, "~")) {
		path_remove_hop(path, 0);
		path_insert_hop(path, 0, "@");
	}

	base_path = path_new_dir(base_dir_name != NULL ? base_dir_name : "./");
	if (path_num_hops(path) == 0)
		path_rebase(path, base_path);
	else if (path_hop_cmp(path, 0, "#")
		|| path_hop_cmp(path, 0, "~")
		|| path_hop_cmp(path, 0, "@"))
	{
		prefix = strdup(path_hop(path, 0));
		path_remove_hop(path, 0);
		path_collapse(path, true);
		path_insert_hop(path, 0, prefix);
		free(prefix);
	}
	else
		path_rebase(path, base_path);
	path_collapse(path, true);
	path_free(base_path);
	return path;
}

int
fs_mkdir(const fs_t* fs, const char* dirname)
{
	char* resolved_name;
	int   retval;

	if (!(resolved_name = resolve(fs, dirname))) {
		errno = EACCES;
		return -1;
	}
	retval = tinydir_mkdir(resolved_name);
	free(resolved_name);
	return retval;
}

int
fs_rename(const fs_t* fs, const char* old_name, const char* new_name)
{
	char* resolved_old;
	char* resolved_new;
	int   retval;
	
	resolved_old = resolve(fs, old_name);
	resolved_new = resolve(fs, new_name);
	if (resolved_old == NULL || resolved_new == NULL)
		goto access_denied;
	retval = rename(resolved_old, resolved_new);
	free(resolved_old);
	free(resolved_new);
	return retval;

access_denied:
	free(resolved_old);
	free(resolved_new);
	errno = EACCES;
	return -1;
}

int
fs_rmdir(const fs_t* fs, const char* dirname)
{
	char* resolved_name;
	int   retval;

	if (!(resolved_name = resolve(fs, dirname))) {
		errno = EACCES;
		return -1;
	}
	retval = rmdir(resolved_name);
	free(resolved_name);
	return retval;
}

int
fs_stat(const fs_t* fs, const char* filename, struct stat* p_stat)
{
	char* resolved_name;
	int   result;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return -1;
	}

	result = stat(resolved_name, p_stat);
	free(resolved_name);
	return result;
}

int
fs_unlink(const fs_t* fs, const char* filename)
{
	char* resolved_name;
	int   result;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return -1;
	}

	result = unlink(resolved_name);
	free(resolved_name);
	return result;
}

static char*
resolve(const fs_t* fs, const char* filename)
{
	char*   resolved_name;
	path_t* path;
	
	path = path_new(filename);
	if (path_is_rooted(path))
		goto on_error;

	if (path_num_hops(path) == 0)
		path_rebase(path, fs->root_path);
	else if (path_hop_cmp(path, 0, "@")) {
		path_remove_hop(path, 0);
		path_rebase(path, fs->game_path);
	}
	else if (path_hop_cmp(path, 0, "#")) {
		path_remove_hop(path, 0);
		path_rebase(path, fs->system_path);
	}
	else if (path_hop_cmp(path, 0, "~"))
		if (fs->user_path == NULL)
			// no user directory set, ~/ is a sandbox violation.
			goto on_error;
		else {
			path_remove_hop(path, 0);
			path_rebase(path, fs->user_path);
		}
	else
		path_rebase(path, fs->root_path);
	
	resolved_name = strdup(path_cstr(path));
	path_free(path);
	return resolved_name;

on_error:
	path_free(path);
	return NULL;
}
