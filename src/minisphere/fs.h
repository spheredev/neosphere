#ifndef MINISPHERE__SPHEREFS_H__INCLUDED
#define MINISPHERE__SPHEREFS_H__INCLUDED

#include "geometry.h"

typedef struct file file_t;
typedef struct game game_t;

typedef
enum whence
{
	WHENCE_SET,
	WHENCE_CUR,
	WHENCE_END,
} whence_t;

game_t*          fs_open         (const char* game_path);
game_t*          fs_ref          (game_t* fs);
void             fs_close        (game_t* fs);
bool             fs_dir_exists   (const game_t* fs, const char* dirname, const char* base_dir);
bool             fs_file_exists  (game_t* fs, const char* filename, const char* base_dir);
int              fs_version      (const game_t* fs);
const lstring_t* fs_manifest     (const game_t* fs);
const char*      fs_author       (const game_t* fs);
bool             fs_fullscreen   (const game_t* fs);
const char*      fs_name         (const game_t* fs);
const path_t*    fs_path         (const game_t* fs);
size2_t          fs_resolution   (const game_t* fs);
const char*      fs_summary      (const game_t* fs);
const char*      fs_save_id      (const game_t* fs);
const path_t*    fs_script_path  (const game_t* fs);
path_t*          fs_canonicalize (const char* filename, const char* base_dir_name, bool legacy);
vector_t*        fs_list_dir     (const game_t* fs, const char* dirname, const char* base_dir, bool want_dirs);
bool             fs_mkdir        (game_t* fs, const char* dirname, const char* base_dir);
void*            fs_read_file    (game_t* fs, const char* filename, const char* base_dir, size_t *out_size);
bool             fs_rename       (game_t* fs, const char* filename1, const char* filename2, const char* base_dir);
bool             fs_rmdir        (game_t* fs, const char* dirname, const char* base_dir);
bool             fs_unlink       (game_t* fs, const char* filename, const char* base_dir);
bool             fs_write_file   (game_t* fs, const char* filename, const char* base_dir, const void* buf, size_t size);
file_t*          file_open       (game_t* fs, const char* filename, const char* base_dir, const char* mode);
void             file_close      (file_t* file);
const char*      file_pathname   (file_t* file);
long long        file_position   (file_t* file);
int              file_putc       (int ch, file_t* file);
int              file_puts       (const char* string, file_t* file);
size_t           file_read       (void* buf, size_t size, size_t count, file_t* file);
bool             file_seek       (file_t* file, long long offset, whence_t whence);
size_t           file_write      (const void* buf, size_t size, size_t count, file_t* file);

#endif // MINISPHERE__SPHEREFS_H__INCLUDED
