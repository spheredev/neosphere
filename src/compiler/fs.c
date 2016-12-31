#include "cell.h"
#include "fs.h"

struct fs
{
	path_t* build_path;
	path_t* game_path;
	path_t* system_path;
};

fs_t*
fs_new(const char* build_dir, const char* game_dir)
{
	fs_t* fs;

	fs = calloc(1, sizeof(fs_t));
	fs->build_path = path_new_dir(build_dir);
	fs->game_path = path_new_dir(game_dir);
	fs->system_path = path_append(path_strip(path_new_self()), "system/");
	return fs;
}

void
fs_free(fs_t* fs)
{
	path_free(fs->build_path);
	path_free(fs->game_path);
	path_free(fs->system_path);
	free(fs);
}

FILE*
fs_fopen(const fs_t* fs, const char* filename, const char* mode)
{
	path_t* path;

	path = path_new(filename);
	if (!fs_resolve(fs, path)) {
		errno = EACCES;  // sandboxing violation
		return NULL;
	}
	return fopen(path_cstr(path), mode);
}

vector_t*
fs_list_dir(const fs_t* fs, const path_t* dir_path, bool recursive)
{
	vector_t* list;
	path_t*   path;

	path = path_dup(dir_path);
	if (!fs_resolve(fs, path))
		return NULL;

	list = vector_new(sizeof(path_t*));
	return list;
}

bool
fs_resolve(const fs_t* fs, path_t* path)
{
	if (path_is_rooted(path))
		return false;
	
	if (path_num_hops(path) == 0)
		path_rebase(path, fs->build_path);
	else if (path_hop_cmp(path, 0, "@")) {
		path_remove_hop(path, 0);
		path_rebase(path, fs->game_path);
	}
	else if (path_hop_cmp(path, 0, "#")) {
		path_remove_hop(path, 0);
		path_rebase(path, fs->system_path);
	}
	else if (path_hop_cmp(path, 0, "~"))
		// ~/ is required by the SphereFS specification, but is meaningless
		// at compile time.  treat it as a sandboxing violation.
		return false;
	else
		path_rebase(path, fs->build_path);
	return true;
}

int
fs_stat(const fs_t* fs, const char* filename, struct stat* p_stat)
{
	path_t* path;

	path = path_new(filename);
	if (!fs_resolve(fs, path)) {
		errno = EACCES;  // sandboxing violation
		return -1;
	}
	return stat(path_cstr(path), p_stat);
}
