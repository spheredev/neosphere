#ifndef MINISPHERE__SPHEREFS_H__INCLUDED
#define MINISPHERE__SPHEREFS_H__INCLUDED

#include "lstring.h"
#include "path.h"
#include "vector.h"

typedef struct sandbox  sandbox_t;
typedef struct sfs_file sfs_file_t;
typedef struct sfs_list sfs_list_t;

typedef
enum sfs_whence
{
	SFS_SEEK_SET,
	SFS_SEEK_CUR,
	SFS_SEEK_END,
} sfs_whence_t;

sandbox_t*       new_sandbox         (const char* pathname);
sandbox_t*       ref_sandbox         (sandbox_t* fs);
void             free_sandbox        (sandbox_t* fs);
const lstring_t* get_game_manifest   (const sandbox_t* fs);
const path_t*    get_game_path       (const sandbox_t* fs);
void             get_sgm_resolution (sandbox_t* fs, int *out_width, int *out_height);
const char*      get_sgm_name        (sandbox_t* fs);
const char*      get_sgm_author      (sandbox_t* fs);
const char*      get_sgm_summary     (sandbox_t* fs);
const path_t*    get_sgm_script_path (sandbox_t* fs);
vector_t*        list_filenames      (sandbox_t* fs, const char* dirname, const char* base_dir, bool want_dirs);
path_t*          make_sfs_path       (const char* filename, const char* base_dir_name, bool legacy);

sfs_file_t* sfs_fopen      (sandbox_t* fs, const char* path, const char* base_dir, const char* mode);
void        sfs_fclose     (sfs_file_t* file);
bool        sfs_fexist     (sandbox_t* fs, const char* filename, const char* base_dir);
int         sfs_fputc      (int ch, sfs_file_t* file);
int         sfs_fputs      (const char* string, sfs_file_t* file);
size_t      sfs_fread      (void* buf, size_t size, size_t count, sfs_file_t* file);
bool        sfs_fseek      (sfs_file_t* file, long long offset, sfs_whence_t whence);
bool        sfs_fspew      (sandbox_t* fs, const char* filename, const char* base_dir, void* buf, size_t size);
void*       sfs_fslurp     (sandbox_t* fs, const char* filename, const char* base_dir, size_t *out_size);
long long   sfs_ftell      (sfs_file_t* file);
size_t      sfs_fwrite     (const void* buf, size_t size, size_t count, sfs_file_t* file);
bool        sfs_mkdir      (sandbox_t* fs, const char* dirname, const char* base_dir);
bool        sfs_rmdir      (sandbox_t* fs, const char* dirname, const char* base_dir);
bool        sfs_rename     (sandbox_t* fs, const char* filename1, const char* filename2, const char* base_dir);
bool        sfs_unlink     (sandbox_t* fs, const char* filename, const char* base_dir);
bool        sfs_read_int   (sfs_file_t* file, intmax_t* p_value, int size, bool little_endian);
bool        sfs_read_uint  (sfs_file_t* file, intmax_t* p_value, int size, bool little_endian);
bool        sfs_write_int  (sfs_file_t* file, intmax_t value, int size, bool little_endian);
bool        sfs_write_uint (sfs_file_t* file, intmax_t value, int size, bool little_endian);

#endif // MINISPHERE__SPHEREFS_H__INCLUDED
