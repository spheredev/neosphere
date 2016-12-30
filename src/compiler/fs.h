#ifndef CELL__FS_H__INCLUDED
#define CELL__FS_H__INCLUDED

typedef struct fs fs_t;

fs_t*     fs_new      (const char* source_dirname, const char* out_dirname, const char* cell_dirname);
void      fs_free     (fs_t* fs);
vector_t* fs_list_dir (const fs_t* fs, const path_t* dir_path);
bool      fs_resolve  (const fs_t* fs, path_t* path);

#endif // CELL__FS_H__INCLUDED
