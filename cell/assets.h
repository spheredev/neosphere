#ifndef CELL__ASSETS_H__INCLUDED
#define CELL__ASSETS_H__INCLUDED

#include "path.h"

typedef struct asset asset_t;

extern void     initialize_assets (void);
extern void     shutdown_assets   (void);
extern asset_t* new_file_asset    (const path_t* path);
extern void     free_asset        (asset_t* asset);
extern bool     build_asset       (asset_t* asset);
extern bool     install_asset     (const asset_t* asset, const path_t* path);

#endif // CELL__ASSETS_H__INCLUDED
