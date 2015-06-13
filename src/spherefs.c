#include "minisphere.h"
#include "spk.h"

#include "spherefs.h"

struct spherefs
{
	ALLEGRO_CONFIG* sgm;
	int             type;
	union {
		ALLEGRO_PATH* fs_root;
		spk_t*        spk;
	};
};

struct sphere_file
{
	spherefs_t* fs;
	union {
		FILE*       fs_file;
		spk_file_t* spk_file;
	};
};

enum fs_type
{
	SPHEREFS_SANDBOX,
	SPHEREFS_SPK
};

spherefs_t*
create_spk_fs(const char* path)
{
	ALLEGRO_FILE* al_file = NULL;
	spherefs_t*   fs;
	spk_file_t*   file = NULL;
	long          sgm_size;
	char*         sgm_text = NULL;

	if (!(fs = calloc(1, sizeof(spherefs_t))))
		goto on_error;
	fs->type = SPHEREFS_SPK;
	if (!(fs->spk = open_spk(path))) goto on_error;
	
	if (!(file = spk_fopen(fs->spk, "game.sgm")))
		goto on_error;
	sgm_size = (spk_fseek(file, 0, SPK_SEEK_END), spk_ftell(file));
	if (!(sgm_text = malloc(sgm_size))) goto on_error;
	spk_fread(file, sgm_text, sgm_size);
	spk_fclose(file);
	al_file = al_open_memfile(sgm_text, sgm_size, "rb");
	if (!(fs->sgm = al_load_config_file_f(al_file)))
		goto on_error;
	al_fclose(al_file);
	return fs;
		
on_error:
	if (al_file != NULL)
		al_fclose(al_file);
	spk_fclose(file);
	free(sgm_text);
	if (fs != NULL) {
		free_spk(fs->spk);
		free(fs);
	}
	return NULL;
}

void
free_fs(spherefs_t* fs)
{
	if (fs == NULL)
		return;
	al_destroy_config(fs->sgm);
	switch (fs->type) {
	case SPHEREFS_SPK:
		free_spk(fs->spk);
	}
	free(fs);
}

sphere_file_t*
sphere_fopen(spherefs_t* fs, const char* path)
{
	sphere_file_t* file;

	if (!(file = calloc(1, sizeof(sphere_file_t))))
		goto on_error;
	file->fs = fs;
	switch (file->fs->type) {
	case SPHEREFS_SANDBOX:
		if (!(file->fs_file = fopen(path, "rb")))
			goto on_error;
		break;
	case SPHEREFS_SPK:
		if (!(file->spk_file = spk_fopen(fs->spk, path)))
			goto on_error;
		break;
	}
	return file;

on_error:
	free(file);
	return NULL;
}

void
sphere_fclose(sphere_file_t* file)
{
	if (file == NULL)
		return;
	switch (file->fs->type) {
	case SPHEREFS_SANDBOX:
		fclose(file->fs_file);
		break;
	case SPHEREFS_SPK:
		spk_fclose(file->spk_file);
		break;
	}
	free(file);
}

long
sphere_fread(sphere_file_t* file, void* buf, long size)
{
	long read_size;
	
	switch (file->fs->type) {
	case SPHEREFS_SANDBOX:
		read_size = (long)fread(buf, 1, size, file->fs_file);
		break;
	case SPHEREFS_SPK:
		read_size = (long)spk_fread(file->spk_file, buf, size);
		break;
	}
	return read_size;
}

long
sphere_ftell(sphere_file_t* file)
{
	switch (file->fs->type) {
	case SPHEREFS_SANDBOX:
		return ftell(file->fs_file);
	case SPHEREFS_SPK:
		return spk_ftell(file->spk_file);
	}
}
