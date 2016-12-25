#include "cell.h"
#include "fs.h"

struct fs
{
	path_t* in_path;
	path_t* out_path;
};

fs_t*
fs_new(const path_t* in_path, const path_t* out_path)
{
	fs_t* fs;

	fs = calloc(1, sizeof(fs_t));
	
	fs->in_path = path_dup(in_path);
	fs->out_path = path_dup(out_path);

	return fs;
}

void
fs_free(fs_t* fs)
{
	path_free(fs->in_path);
	path_free(fs->out_path);
	free(fs);
}

const path_t*
fs_in_path(fs_t* fs)
{
	return fs->in_path;
}

const path_t*
fs_out_path(fs_t* fs)
{
	return fs->out_path;
}
