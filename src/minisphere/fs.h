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

game_t*          game_open         (const char* game_path);
game_t*          game_ref          (game_t* game);
void             game_unref        (game_t* game);
const char*      game_author       (const game_t* game);
bool             game_dir_exists   (const game_t* game, const char* dirname);
bool             game_file_exists  (game_t* game, const char* filename);
bool             game_fullscreen   (const game_t* game);
const lstring_t* game_manifest     (const game_t* game);
const char*      game_name         (const game_t* game);
const path_t*    game_path         (const game_t* game);
size2_t          game_resolution   (const game_t* game);
const char*      game_save_id      (const game_t* game);
const path_t*    game_script_path  (const game_t* game);
const char*      game_summary      (const game_t* game);
int              game_version      (const game_t* game);
path_t*          game_canonicalize (const game_t* game, const char* filename, const char* base_dir_name, bool legacy);
vector_t*        game_list_dir     (const game_t* game, const char* dirname, bool want_dirs);
bool             game_mkdir        (game_t* game, const char* dirname);
void*            game_read_file    (game_t* game, const char* filename, size_t *out_size);
bool             game_rename       (game_t* game, const char* filename1, const char* filename2);
bool             game_rmdir        (game_t* game, const char* dirname);
bool             game_unlink       (game_t* game, const char* filename);
bool             game_write_file   (game_t* game, const char* filename, const void* buf, size_t size);
file_t*          file_open         (game_t* game, const char* filename, const char* mode);
void             file_close        (file_t* file);
const char*      file_pathname     (const file_t* file);
long long        file_position     (const file_t* file);
int              file_putc         (int ch, file_t* file);
int              file_puts         (const char* string, file_t* file);
size_t           file_read         (void* buf, size_t size, size_t count, file_t* file);
bool             file_seek         (file_t* file, long long offset, whence_t whence);
size_t           file_write        (const void* buf, size_t size, size_t count, file_t* file);

#endif // MINISPHERE__SPHEREFS_H__INCLUDED
