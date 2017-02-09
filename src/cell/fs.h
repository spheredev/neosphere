#ifndef CELL__FS_H__INCLUDED
#define CELL__FS_H__INCLUDED

typedef struct fs fs_t;

fs_t*     fs_new         (const char* origin_dir, const char* game_dir, const char* home_dir);
void      fs_free        (fs_t* fs);
int       fs_fcopy       (const fs_t* fs, const char* destination, const char* source, int overwrite);
bool      fs_fexist      (const fs_t* fs, const char* filename);
FILE*     fs_fopen       (const fs_t* fs, const char* filename, const char* mode);
bool      fs_fspew       (const fs_t* fs, const char* filename, const void* data, size_t size);
void*     fs_fslurp      (const fs_t* fs, const char* filename, size_t* out_size);
bool      fs_is_game_dir (const fs_t* fs, const char* dirname);
vector_t* fs_list_dir    (const fs_t* fs, const char* dirname);
path_t*   fs_make_path   (const char* filename, const char* base_dir_name, bool legacy);
int       fs_mkdir       (const fs_t* fs, const char* dirname);
int       fs_rename      (const fs_t* fs, const char* old_name, const char* new_name);
int       fs_rmdir       (const fs_t* fs, const char* dirname);
int       fs_stat        (const fs_t* fs, const char* filename, struct stat* p_stat);
int       fs_unlink      (const fs_t* fs, const char* filename);
int       fs_utime       (const fs_t* fs, const char* filename, struct utimbuf* in_times);

#endif // CELL__FS_H__INCLUDED
