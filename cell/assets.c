#include "assets.h"

#include "tinydir.h"
#include "vector.h"
#include "cell.h"

static vector_t* s_assets = NULL;

typedef
enum asset_type
{
	ASSET_FILE,
	ASSET_SGM,
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
		sgm_info_t       sgm;
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

	if (s_assets == NULL) return;
	
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

asset_t*
new_sgm_asset(sgm_info_t sgm)
{
	asset_t* asset;

	asset = calloc(1, sizeof(asset_t));
	asset->type = ASSET_SGM;
	asset->sgm = sgm;
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
build_asset(asset_t* asset, bool *out_is_new)
{
	FILE* file;
	
	*out_is_new = false;
	switch (asset->type) {
	case ASSET_FILE:
		asset->obj_path = path_dup(asset->file.path);
		return true;
	case ASSET_SGM:
		asset->obj_path = path_rebase(path_new(".cell/game.sgm"), g_in_path);
		if (!(file = fopen(path_cstr(asset->obj_path), "wt"))) {
			printf("error: failed to write '%s'", path_cstr(asset->obj_path));
			return false;
		}
		fprintf(file, "name=%s\n", asset->sgm.name);
		fprintf(file, "author=%s\n", asset->sgm.author);
		fprintf(file, "description=%s\n", asset->sgm.description);
		fprintf(file, "screen_width=%d\n", asset->sgm.width);
		fprintf(file, "screen_height=%d\n", asset->sgm.height);
		fprintf(file, "script=%s\n", asset->sgm.script);
		fclose(file);
		*out_is_new = true;
		return true;
	}
	return false;
}

bool
install_asset(const asset_t* asset, const path_t* path, bool *out_is_new)
{
	bool    retval = false;
	path_t* out_path;

	if (asset->obj_path == NULL)
		return false;
	
	if (g_spk_writer == NULL) {
		out_path = path_rebase(path_dup(path), g_out_path);
		mkdir_r(path_cstr(out_path));
		path_append(out_path, path_filename_cstr(asset->obj_path), false);
		retval = tinydir_copy(path_cstr(asset->obj_path), path_cstr(out_path), false) == 0;
		path_free(out_path);
		return retval;
	} else {
		out_path = path_collapse(path_dup(path), true);
		path_append(out_path, path_filename_cstr(asset->obj_path), false);
		spk_pack(g_spk_writer, path_cstr(asset->obj_path), path_cstr(out_path));
		return true;
	}
}
