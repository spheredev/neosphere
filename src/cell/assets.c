#include "cell.h"
#include "assets.h"

typedef
enum asset_type
{
	ASSET_FILE,
	ASSET_RAW,
	ASSET_SGM,
} asset_type_t;

struct raw_info
{
	void*  buffer;
	size_t size;
};

struct file_info
{
	path_t* path;
};

struct asset
{
	path_t*      name;
	path_t*      object_path;
	time_t       src_mtime;
	asset_type_t type;
	union {
		struct file_info file;
		struct raw_info  data;
		sgm_info_t       sgm;
	};
};

asset_t*
asset_new_file(const path_t* path)
{
	asset_t*    asset;
	struct stat sb;
	
	if (stat(path_cstr(path), &sb) != 0) {
		fprintf(stderr, "ERROR: failed to stat '%s'\n", path_cstr(path));
		return NULL;
	}
	
	asset = calloc(1, sizeof(asset_t));
	asset->src_mtime = sb.st_mtime;
	asset->type = ASSET_FILE;
	asset->file.path = path_dup(path);
	return asset;
}

asset_t*
asset_new_sgm(sgm_info_t sgm, time_t src_mtime)
{
	asset_t* asset;

	asset = calloc(1, sizeof(asset_t));
	asset->name = path_new("game.sgm");
	asset->src_mtime = src_mtime;
	asset->type = ASSET_SGM;
	asset->sgm = sgm;
	return asset;
}

asset_t*
asset_new_raw(const path_t* name, const void* buffer, size_t size, time_t src_mtime)
{
	asset_t* asset;

	asset = calloc(1, sizeof(asset_t));
	asset->name = path_dup(name);
	asset->src_mtime = src_mtime;
	asset->type = ASSET_RAW;
	asset->data.buffer = malloc(size);
	asset->data.size = size;
	memcpy(asset->data.buffer, buffer, size);
	return asset;
}

void
asset_free(asset_t* asset)
{
	switch (asset->type) {
	case ASSET_RAW:
		free(asset->data.buffer);
		break;
	case ASSET_FILE:
		path_free(asset->file.path);
		break;
	}
	path_free(asset->object_path);
	path_free(asset->name);
	free(asset);
}

bool
asset_build(asset_t* asset, const path_t* staging_path, bool *out_is_new)
{
	FILE*       file;
	const char* filename;
	path_t*     origin;
	struct stat sb;
	path_t*     script_path;

	*out_is_new = false;
	asset->object_path = asset->type != ASSET_FILE
		? path_rebase(path_dup(asset->name), staging_path)
		: path_dup(asset->file.path);
	filename = path_cstr(asset->object_path);
	if (stat(filename, &sb) == 0 && difftime(sb.st_mtime, asset->src_mtime) >= 0.0)
		return true;  // asset is up-to-date

	*out_is_new = true;
	switch (asset->type) {
	case ASSET_FILE:
		// a file asset is just a direct copy from source to destination, so
		// there's nothing to build.
		return true;

	case ASSET_RAW:
		if (!fspew(asset->data.buffer, asset->data.size, filename))
			goto on_error;
		return true;

	case ASSET_SGM:
		// SGMv1 requires the script name to be relative to ~/scripts
		script_path = path_new(asset->sgm.script);
		origin = path_new("scripts/");
		path_collapse(script_path, true);
		path_relativize(script_path, origin);
		path_free(origin);

		// write out the manifest
		if (!(file = fopen(filename, "wt")))
			goto on_error;
		fprintf(file, "name=%s\n", asset->sgm.name);
		fprintf(file, "author=%s\n", asset->sgm.author);
		fprintf(file, "description=%s\n", asset->sgm.description);
		fprintf(file, "screen_width=%d\n", asset->sgm.width);
		fprintf(file, "screen_height=%d\n", asset->sgm.height);
		fprintf(file, "script=%s\n", path_cstr(script_path));
		fclose(file);
		path_free(script_path);
		return true;

	default:
		fprintf(stderr, "ERROR: internal: unknown asset type %d '%s'\n",
			asset->type, path_cstr(asset->name));
		return false;
	}

on_error:
	fprintf(stderr, "\nERROR: failed to build '%s', errno = %d\n", path_cstr(asset->name), errno);
	return false;
}

const path_t*
asset_name(const asset_t* asset)
{
	return asset->name;
}

const path_t*
asset_object_path(const asset_t* asset)
{
	return asset->object_path;
}
