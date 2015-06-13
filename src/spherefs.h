#ifndef MINISPHERE__SPHEREFS_H__INCLUDED
#define MINISPHERE__SPHEREFS_H__INCLUDED

typedef struct spherefs    spherefs_t;
typedef struct sphere_file sphere_file_t;

spherefs_t*    create_spk_fs (const char* path);
void           free_fs       (spherefs_t* fs);
sphere_file_t* sphere_fopen  (spherefs_t* fs, const char* path);
void           sphere_fclose (sphere_file_t* file);
long           sphere_fread  (sphere_file_t* file, void* buf, long num_bytes);
long           sphere_ftell  (sphere_file_t* file);

#endif // MINISPHERE__SPHEREFS_H__INCLUDED
