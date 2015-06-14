#ifndef MINISPHERE__SPK_H__INCLUDED
#define MINISPHERE__SPK_H__INCLUDED

typedef struct spk             spk_t;
typedef struct spk_file        spk_file_t;
typedef enum   spk_seek_origin spk_seek_origin_t;

spk_t*      open_spk    (const char* path);
spk_t*      ref_spk     (spk_t* spk);
void        free_spk    (spk_t* spk);
spk_file_t* spk_fopen  (spk_t* spk, const char* path);
void        spk_fclose (spk_file_t* file);
size_t      spk_fread  (void* buf, size_t count, size_t size, spk_file_t* file);
bool        spk_fseek  (spk_file_t* file, long new_pos, spk_seek_origin_t origin);
void*       spk_fslurp (spk_t* spk, const char* path, size_t *out_size);
long        spk_ftell  (spk_file_t* file);


enum spk_seek_origin
{
	SPK_SEEK_SET,
	SPK_SEEK_CUR,
	SPK_SEEK_END
};

#endif // MINISPHERE__SPK_H__INCLUDED
