#ifndef MINISPHERE__SPHEREFS_H__INCLUDED
#define MINISPHERE__SPHEREFS_H__INCLUDED

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

sandbox_t*  new_sandbox     (const char* path);
void        free_sandbox    (sandbox_t* fs);
const char* get_sgm_author  (sandbox_t* fs);
void        get_sgm_metrics (sandbox_t* fs, int *out_x_res, int *out_y_res);
const char* get_sgm_name    (sandbox_t* fs);
const char* get_sgm_script  (sandbox_t* fs);
const char* get_sgm_summary (sandbox_t* fs);
vector_t*   list_filenames  (sandbox_t* fs, const char* dirname, const char* base_dir, bool want_dirs);
path_t*     make_sfs_path   (const char* filename, const char* base_dir_name);

sfs_file_t* sfs_fopen  (sandbox_t* fs, const char* path, const char* base_dir, const char* mode);
void        sfs_fclose (sfs_file_t* file);
bool        sfs_fexist (sandbox_t* fs, const char* filename, const char* base_dir);
int         sfs_fputc  (int ch, sfs_file_t* file);
int         sfs_fputs  (const char* string, sfs_file_t* file);
size_t      sfs_fread  (void* buf, size_t size, size_t count, sfs_file_t* file);
bool        sfs_fseek  (sfs_file_t* file, long long offset, sfs_whence_t whence);
bool        sfs_fspew  (sandbox_t* fs, const char* filename, const char* base_dir, void* buf, size_t size);
void*       sfs_fslurp (sandbox_t* fs, const char* filename, const char* base_dir, size_t *out_size);
long long   sfs_ftell  (sfs_file_t* file);
size_t      sfs_fwrite (const void* buf, size_t size, size_t count, sfs_file_t* file);
bool        sfs_mkdir  (sandbox_t* fs, const char* dirname, const char* base_dir);
bool        sfs_rmdir  (sandbox_t* fs, const char* dirname, const char* base_dir);
bool        sfs_rename (sandbox_t* fs, const char* filename1, const char* filename2, const char* base_dir);
bool        sfs_unlink (sandbox_t* fs, const char* filename, const char* base_dir);

#endif // MINISPHERE__SPHEREFS_H__INCLUDED
