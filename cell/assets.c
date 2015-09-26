#include "assets.h"

#include "tinydir.h"
#include "vector.h"
#include "cell.h"

#include <sys/stat.h>

typedef
enum asset_type
{
	ASSET_FILE,
	ASSET_SGM,
} asset_type_t;

struct file_info
{
	path_t* path;
	path_t* subdir;
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

asset_t*
new_file_asset(const path_t* path)
{
	asset_t* asset;

	asset = calloc(1, sizeof(asset_t));
	asset->type = ASSET_FILE;
	asset->file.path = path_dup(path);
	return asset;
}

asset_t*
new_sgm_asset(sgm_info_t sgm)
{
	asset_t* asset;

	asset = calloc(1, sizeof(asset_t));
	asset->type = ASSET_SGM;
	asset->sgm = sgm;
	return asset;
}

void
free_asset(asset_t* asset)
{
	path_free(asset->obj_path);
	free(asset);
}

bool
build_asset(asset_t* asset, const path_t* staging_path, bool *out_is_new)
{
	FILE* file;

	*out_is_new = false;
	switch (asset->type) {
	case ASSET_FILE:
		// a file asset represents a direct copy from source to destination, so
		// there's no need to do anything other than record the source location.
		asset->obj_path = path_dup(asset->file.path);
		return true;
	case ASSET_SGM:
		asset->obj_path = path_rebase(path_new("game.sgm"), staging_path);
		if (!(file = fopen(path_cstr(asset->obj_path), "wt"))) {
			printf("error: failed to write '%s'\n", path_cstr(asset->obj_path));
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
		break;
	default:
		return false;
	}
	if (*out_is_new) {
		printf("    %s\n", path_cstr(asset->obj_path));
	}
	return true;
}

const path_t*
get_asset_path(const asset_t* asset)
{
	return asset->obj_path;
}
