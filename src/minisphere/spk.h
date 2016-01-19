#ifndef MINISPHERE__SPK_H__INCLUDED
#define MINISPHERE__SPK_H__INCLUDED

typedef struct spk             spk_t;
typedef struct spk_file        spk_file_t;

typedef
enum spk_seek_origin
{
	SPK_SEEK_SET,
	SPK_SEEK_CUR,
	SPK_SEEK_END
} spk_seek_origin_t;

spk_t*      open_spk           (const char* path);
spk_t*      ref_spk            (spk_t* spk);
void        free_spk           (spk_t* spk);
vector_t*   list_spk_filenames (spk_t* spk, const char* dirname, bool want_dirs);

spk_file_t* spk_fopen  (spk_t* spk, const char* path, const char* mode);
void        spk_fclose (spk_file_t* file);
int         spk_fputc  (int ch, spk_file_t* file);
int         spk_fputs  (const char* string, spk_file_t* file);
size_t      spk_fread  (void* buf, size_t size, size_t count, spk_file_t* file);
bool        spk_fseek  (spk_file_t* file, long long offset, spk_seek_origin_t origin);
void*       spk_fslurp (spk_t* spk, const char* path, size_t *out_size);
long long   spk_ftell  (spk_file_t* file);
size_t      spk_fwrite (const void* buf, size_t size, size_t count, spk_file_t* file);

#endif // MINISPHERE__SPK_H__INCLUDED
