#ifndef CELL__FS_H__INCLUDED
#define CELL__FS_H__INCLUDED

typedef struct fs fs_t;

fs_t*         fs_new      (const path_t* in_path, const path_t* out_path);
void          fs_free     (fs_t* fs);
const path_t* fs_in_path  (fs_t* fs);
const path_t* fs_out_path (fs_t* fs);

#endif // CELL__FS_H__INCLUDED
