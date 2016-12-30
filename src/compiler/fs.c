#include "cell.h"
#include "fs.h"

struct fs
{
	path_t* cell_path;
	path_t* out_path;
	path_t* source_path;
};

fs_t*
fs_new(const char* source_dirname, const char* out_dirname, const char* cell_dirname)
{
	fs_t* fs;

	fs = calloc(1, sizeof(fs_t));
	fs->source_path = path_new_dir(source_dirname);
	fs->out_path = path_new_dir(out_dirname);
	fs->cell_path = path_new_dir(cell_dirname);
	return fs;
}

void
fs_free(fs_t* fs)
{
	path_free(fs->source_path);
	path_free(fs->out_path);
	path_free(fs->cell_path);
	free(fs);
}

vector_t*
fs_list_dir(const fs_t* fs, const path_t* dir_path)
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
	path_t* origin_path;
	
	if (path_num_hops(path) == 0)
		path_rebase(path, fs->source_path);
	else if (path_hop_cmp(path, 0, "@")) {
		path_remove_hop(path, 0);
		path_rebase(path, fs->out_path);
	}
	else if (path_hop_cmp(path, 0, "#")) {
		origin_path = path_rebase(path_new("system/"), fs->cell_path);
		if (!path_resolve(origin_path, NULL)) {
			path_free(origin_path);
			origin_path = path_rebase(path_new("../share/minisphere/system/"), fs->cell_path);
		}
		path_remove_hop(path, 0);
		path_rebase(path, origin_path);
		path_free(origin_path);
	}
	else
		path_rebase(path, fs->source_path);
	return true;
}
