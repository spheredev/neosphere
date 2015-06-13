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

struct sfs_file
{
	int fs_type;
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
create_sandbox_fs(const char* path)
{
	spherefs_t*   fs;
	ALLEGRO_PATH* sgm_path = NULL;

	if (!(fs = calloc(1, sizeof(spherefs_t))))
		goto on_error;
	fs->type = SPHEREFS_SANDBOX;

	fs->fs_root = al_create_path_for_directory(path);
	sgm_path = al_clone_path(fs->fs_root);
	al_set_path_filename(sgm_path, "game.sgm");
	if (!(fs->sgm = al_load_config_file(al_path_cstr(sgm_path, ALLEGRO_NATIVE_PATH_SEP))))
		goto on_error;
	al_destroy_path(sgm_path);
	return fs;

on_error:
	if (sgm_path != NULL)
		al_destroy_path(sgm_path);
	free(fs);
	return NULL;
}

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
	free(sgm_text);
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
	case SPHEREFS_SANDBOX:
		al_destroy_path(fs->fs_root);
	case SPHEREFS_SPK:
		free_spk(fs->spk);
	}
	free(fs);
}

sfs_file_t*
sfs_fopen(spherefs_t* fs, const char* filename, const char* base_dir, const char* mode)
{
	ALLEGRO_PATH* origin;
	sfs_file_t*   file;
	ALLEGRO_PATH* file_path;
	const char*   path;

	if (!(file = calloc(1, sizeof(sfs_file_t))))
		goto on_error;
	origin = al_create_path_for_directory(base_dir);
	file_path = al_create_path(filename);
	al_rebase_path(origin, file_path);
	file->fs_type = fs->type;
	switch (file->fs_type) {
	case SPHEREFS_SANDBOX:
		al_rebase_path(fs->fs_root, file_path);
		path = al_path_cstr(file_path, ALLEGRO_NATIVE_PATH_SEP);
		if (!(file->fs_file = fopen(path, "rb")))
			goto on_error;
		break;
	case SPHEREFS_SPK:
		path = al_path_cstr(file_path, '/');
		if (!(file->spk_file = spk_fopen(fs->spk, path)))
			goto on_error;
		break;
	}
	al_destroy_path(file_path);
	al_destroy_path(origin);
	return file;

on_error:
	free(file);
	return NULL;
}

void
sfs_fclose(sfs_file_t* file)
{
	if (file == NULL)
		return;
	switch (file->fs_type) {
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
sfs_fread(sfs_file_t* file, void* buf, long size)
{
	long read_size;
	
	switch (file->fs_type) {
	case SPHEREFS_SANDBOX:
		read_size = (long)fread(buf, 1, size, file->fs_file);
		break;
	case SPHEREFS_SPK:
		read_size = (long)spk_fread(file->spk_file, buf, size);
		break;
	}
	return read_size;
}

bool
sfs_fseek(sfs_file_t* file, long offset, int origin)
{
	switch (file->fs_type) {
	case SPHEREFS_SANDBOX:
		return fseek(file->fs_file, offset, origin) == 0;
	case SPHEREFS_SPK:
		return spk_fseek(file->spk_file, offset, origin);
	}
	return false;
}

long
sfs_ftell(sfs_file_t* file)
{
	switch (file->fs_type) {
	case SPHEREFS_SANDBOX:
		return ftell(file->fs_file);
	case SPHEREFS_SPK:
		return spk_ftell(file->spk_file);
	}
	return -1;
}
