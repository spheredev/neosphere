#include "cell.h"
#include "fs.h"

#include "tinydir.h"

struct fs
{
	path_t* build_path;
	path_t* game_path;
	path_t* system_path;
};

static char* resolve (const fs_t* fs, const char* filename);

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

static char*
resolve(const fs_t* fs, const char* filename)
{
	char*   resolved_name;
	path_t* path;
	
	path = path_new(filename);
	if (path_is_rooted(path))
		goto on_error;

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
		goto on_error;
	else
		path_rebase(path, fs->build_path);
	
	resolved_name = strdup(path_cstr(path));
	path_free(path);
	return resolved_name;

on_error:
	path_free(path);
	return NULL;
}
