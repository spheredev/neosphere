#ifndef CELL__FS_H__INCLUDED
#define CELL__FS_H__INCLUDED

typedef struct fs fs_t;

fs_t*     fs_new      (const char* origin_dir, const char* game_dir);
void      fs_free     (fs_t* fs);
FILE*     fs_fopen    (const fs_t* fs, const char* filename, const char* mode);
vector_t* fs_list_dir (const fs_t* fs, const char* dirname, bool recursive);
int       fs_stat     (const fs_t* fs, const char* filename, struct stat* p_stat);
bool      fs_resolve  (const fs_t* fs, path_t* path);

#endif // CELL__FS_H__INCLUDED
