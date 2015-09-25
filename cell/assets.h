#ifndef CELL__ASSETS_H__INCLUDED
#define CELL__ASSETS_H__INCLUDED

#include "path.h"

typedef struct asset     asset_t;

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

extern void     initialize_assets (void);
extern void     shutdown_assets   (void);
extern asset_t* new_file_asset    (const path_t* path);
extern asset_t* new_sgm_asset     (sgm_info_t sgm);
extern void     free_asset        (asset_t* asset);
extern bool     build_asset       (asset_t* asset, bool *out_is_new);
extern bool     install_asset     (const asset_t* asset, const path_t* path, bool *out_is_new);

#endif // CELL__ASSETS_H__INCLUDED
