#include "cell.h"
#include "assets.h"

typedef
enum asset_type
{
	ASSET_FILE,
	ASSET_S2GM,
	ASSET_SGM,
} asset_type_t;

struct file_info
{
	path_t* path;
};

struct asset
{
	time_t       src_mtime;
	path_t*      obj_path;
	asset_type_t type;
	union {
		struct file_info file;
		sgm_info_t       sgm;
		char*            s2gm_json;
	};
};

asset_t*
new_file_asset(const path_t* path)
{
	asset_t*    asset;
	struct stat sb;
	
	if (stat(path_cstr(path), &sb) != 0) {
		fprintf(stderr, "[error] failed to access file '%s'", path_cstr(path));
		return NULL;
	}
	
	asset = calloc(1, sizeof(asset_t));
	asset->src_mtime = sb.st_mtime;
	asset->type = ASSET_FILE;
	asset->file.path = path_dup(path);
	return asset;
}

asset_t*
new_sgm_asset(sgm_info_t sgm, time_t src_mtime)
{
	asset_t* asset;

	asset = calloc(1, sizeof(asset_t));
	asset->src_mtime = src_mtime;
	asset->type = ASSET_SGM;
	asset->sgm = sgm;
	return asset;
}

asset_t*
new_s2gm_asset(const char* json, time_t src_mtime)
{
	asset_t* asset;

	asset = calloc(1, sizeof(asset_t));
	asset->src_mtime = src_mtime;
	asset->type = ASSET_S2GM;
	asset->s2gm_json = strdup(json);
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
	FILE*       file;
	path_t*     origin;
	struct stat sb;
	path_t*     script_path;

	*out_is_new = false;
	switch (asset->type) {
	case ASSET_FILE:
		// a file asset represents a direct copy from source to destination, so
		// there's no need to do anything other than record the source location.
		asset->obj_path = path_dup(asset->file.path);
		return true;
	case ASSET_S2GM:
		asset->obj_path = path_rebase(path_new("game.s2gm"), staging_path);
		if (stat(path_cstr(asset->obj_path), &sb) == 0 && difftime(sb.st_mtime, asset->src_mtime) >= 0.0)
			return true;
		if (!(file = fopen(path_cstr(asset->obj_path), "wt"))) {
			printf("[error] failed to write '%s'\n", path_cstr(asset->obj_path));
			return false;
		}
		fwrite(asset->s2gm_json, strlen(asset->s2gm_json), 1, file);
		fclose(file);
		*out_is_new = true;
		break;
	case ASSET_SGM:
		asset->obj_path = path_rebase(path_new("game.sgm"), staging_path);
		if (stat(path_cstr(asset->obj_path), &sb) == 0 && difftime(sb.st_mtime, asset->src_mtime) >= 0.0)
			return true;
		if (!(file = fopen(path_cstr(asset->obj_path), "wt"))) {
			printf("[error] failed to write '%s'\n", path_cstr(asset->obj_path));
			return false;
		}
		script_path = path_new(asset->sgm.script);
		origin = path_new("scripts/");
		path_collapse(script_path, true);
		path_relativize(script_path, origin);
		path_free(origin);
		fprintf(file, "name=%s\n", asset->sgm.name);
		fprintf(file, "author=%s\n", asset->sgm.author);
		fprintf(file, "description=%s\n", asset->sgm.description);
		fprintf(file, "screen_width=%d\n", asset->sgm.width);
		fprintf(file, "screen_height=%d\n", asset->sgm.height);
		fprintf(file, "script=%s\n", path_cstr(script_path));
		fclose(file);
		path_free(script_path);
		*out_is_new = true;
		break;
	default:
		return false;
	}
	return true;
}

const path_t*
get_asset_path(const asset_t* asset)
{
	return asset->obj_path;
}
