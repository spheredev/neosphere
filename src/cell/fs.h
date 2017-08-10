#ifndef CELL__FS_H__INCLUDED
#define CELL__FS_H__INCLUDED

typedef struct directory directory_t;
typedef struct fs        fs_t;

fs_t*         fs_new              (const char* origin_dir, const char* game_dir, const char* home_dir);
void          fs_free             (fs_t* fs);
bool          fs_dir_exists       (const fs_t* fs, const char* dirname);
int           fs_fcopy            (const fs_t* fs, const char* destination, const char* source, int overwrite);
bool          fs_fexist           (const fs_t* fs, const char* filename);
FILE*         fs_fopen            (const fs_t* fs, const char* filename, const char* mode);
void*         fs_fslurp           (const fs_t* fs, const char* filename, size_t* out_size);
bool          fs_fspew            (const fs_t* fs, const char* filename, const void* data, size_t size);
bool          fs_is_game_dir      (const fs_t* fs, const char* dirname);
path_t*       fs_build_path       (const char* filename, const char* base_dir_name);
vector_t*     fs_list_dir         (const fs_t* fs, const char* dirname);
int           fs_mkdir            (const fs_t* fs, const char* dirname);
int           fs_rename           (const fs_t* fs, const char* old_name, const char* new_name);
int           fs_rmdir            (const fs_t* fs, const char* dirname);
int           fs_stat             (const fs_t* fs, const char* filename, struct stat* p_stat);
int           fs_unlink           (const fs_t* fs, const char* filename);
int           fs_utime            (const fs_t* fs, const char* filename, struct utimbuf* in_times);
directory_t*  directory_open      (fs_t* fs, const char* dirname);
void          directory_close     (directory_t* it);
int           directory_num_files (directory_t* it);
const char*   directory_pathname  (const directory_t* it);
int           directory_position  (const directory_t* it);
const path_t* directory_next      (directory_t* it);
void          directory_rewind    (directory_t* it);
bool          directory_seek      (directory_t* it, int position);

#endif // CELL__FS_H__INCLUDED
