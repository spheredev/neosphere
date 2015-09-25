#include "assets.h"

#include "tinydir.h"
#include "vector.h"
#include "cell.h"

static bool mkdir_r (const char* path);

static vector_t* s_assets;

typedef
enum asset_type
{
	ASSET_FILE
} asset_type_t;

struct file_info
{
	path_t* path;
};

struct asset
{
	path_t*      obj_path;
	asset_type_t type;
	union {
		struct file_info file;
	};
};

void
initialize_assets(void)
{
	s_assets = new_vector(sizeof(asset_t*));
}

void
shutdown_assets(void)
{
	asset_t* *p_asset;
	iter_t   iter;

	iter = iterate_vector(s_assets);
	while (p_asset = next_vector_item(&iter))
		free_asset(*p_asset);
	free_vector(s_assets);
}

asset_t*
new_file_asset(const path_t* path)
{
	asset_t* asset;

	asset = calloc(1, sizeof(asset_t));
	asset->type = ASSET_FILE;
	asset->file.path = path_dup(path);
	push_back_vector(s_assets, &asset);
	return asset;
}

void
free_asset(asset_t* asset)
{
	path_free(asset->obj_path);
	free(asset);
}

bool
build_asset(asset_t* asset)
{
	switch (asset->type) {
	case ASSET_FILE:
		asset->obj_path = path_dup(asset->file.path);
		return false;
	}
	return false;
}

bool
install_asset(const asset_t* asset, const path_t* path)
{
	path_t* out_path;

	if (asset->obj_path == NULL)
		return false;
	
	out_path = path_rebase(path_dup(path), g_out_path);
	path_append(out_path, path_filename_cstr(asset->obj_path), false);
	mkdir_r(path_cstr(path));
	tinydir_copy(path_cstr(asset->obj_path), path_cstr(out_path), false);
	path_free(out_path);
	return true;
}

static bool
mkdir_r(const char* path)
{
	char  parent[1024] = "";
	char* parse;
	char* token;

	parse = strdup(path);
	token = strtok(parse, "/");
	do {
		strcat(parent, token);
		strcat(parent, "/");
		if (tinydir_mkdir(parent) != 0)
			goto on_error;
	} while (token = strtok(NULL, "/"));
	free(parse);
	return true;

on_error:
	free(parse);
	return false;
}
