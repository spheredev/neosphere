#ifndef MINISPHERE__SPHEREFS_H__INCLUDED
#define MINISPHERE__SPHEREFS_H__INCLUDED

typedef struct spherefs spherefs_t;
typedef struct sfs_file sfs_file_t;
typedef enum   sfs_seek sfs_seek_t;

spherefs_t* create_sandbox_fs (const char* path);
spherefs_t* create_spk_fs     (const char* path);
void        free_fs           (spherefs_t* fs);
sfs_file_t* sfs_fopen         (spherefs_t* fs, const char* path, const char* base_dir, const char* mode);
void        sfs_fclose        (sfs_file_t* file);
long        sfs_fread         (sfs_file_t* file, void* buf, long num_bytes);
bool        sfs_fseek         (sfs_file_t* file, long offset, sfs_seek_t origin);
long        sfs_ftell         (sfs_file_t* file);

enum sfs_seek
{
	SFS_SEEK_SET,
	SFS_SEEK_CUR,
	SFS_SEEK_END,
};

#endif // MINISPHERE__SPHEREFS_H__INCLUDED
