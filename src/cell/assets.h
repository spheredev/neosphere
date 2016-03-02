#ifndef CELL__ASSETS_H__INCLUDED
#define CELL__ASSETS_H__INCLUDED

#include <time.h>

typedef struct asset asset_t;

typedef
struct sgm_info
{
	char name[256];
	char author[256];
	char description[256];
	int  width;
	int  height;
	char script[256];
} sgm_info_t;

extern asset_t*      asset_new_file    (const path_t* path);
extern asset_t*      asset_new_raw     (const path_t* name, const void* buffer, size_t size, time_t src_mtime);
extern asset_t*      asset_new_sgm     (sgm_info_t sgm, time_t src_mtime);
extern void          asset_free        (asset_t* asset);
extern const path_t* asset_name        (const asset_t* asset);
extern const path_t* asset_object_path (const asset_t* asset);
extern bool          asset_build       (asset_t* asset, const path_t* int_path, bool *out_is_new);

#endif // CELL__ASSETS_H__INCLUDED
