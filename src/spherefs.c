#include "minisphere.h"
#include "spk.h"
#include "vector.h"

#include "spherefs.h"

static bool resolve_path (sandbox_t* fs, const char* filename, const char* base_dir, ALLEGRO_PATH* *out_path, int *out_fs_type);

struct sandbox
{
	ALLEGRO_PATH*   fs_root;
	ALLEGRO_CONFIG* sgm;
	spk_t*          spk;
	int             type;
};

struct sfs_file
{
	FILE*       fs_file;
	int         fs_type;
	spk_file_t* spk_file;
};

enum fs_type
{
	SPHEREFS_SANDBOX,
	SPHEREFS_SPK
};

sandbox_t*
new_fs_sandbox(const char* path)
{
	const char*   extension;
	sandbox_t*    fs;
	ALLEGRO_PATH* sgm_path = NULL;

	extension = strrchr(path, '.');
	if (extension != NULL && strcmp(extension, ".spk") == 0)
		return new_spk_sandbox(path);
	
	if (!(fs = calloc(1, sizeof(sandbox_t))))
		goto on_error;
	fs->type = SPHEREFS_SANDBOX;
	if (extension != NULL && strcmp(extension, ".sgm") == 0)
		fs->fs_root = al_create_path(path);
	else
		fs->fs_root = al_create_path_for_directory(path);
	al_set_path_filename(fs->fs_root, NULL);
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

sandbox_t*
new_spk_sandbox(const char* path)
{
	ALLEGRO_FILE* al_file = NULL;
	sandbox_t*    fs;
	size_t        sgm_size;
	void*         sgm_text = NULL;

	if (!(fs = calloc(1, sizeof(sandbox_t))))
		goto on_error;
	fs->type = SPHEREFS_SPK;
	if (!(fs->spk = open_spk(path))) goto on_error;
	
	if (!(sgm_text = spk_fslurp(fs->spk, "game.sgm", &sgm_size)))
		goto on_error;
	al_file = al_open_memfile(sgm_text, sgm_size, "rb");
	if (!(fs->sgm = al_load_config_file_f(al_file)))
		goto on_error;
	al_fclose(al_file);
	free(sgm_text);
	return fs;
		
on_error:
	if (al_file != NULL)
		al_fclose(al_file);
	free(sgm_text);
	if (fs != NULL) {
		free_spk(fs->spk);
		free(fs);
	}
	return NULL;
}

void
free_sandbox(sandbox_t* fs)
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

const char*
get_sgm_author(sandbox_t* fs)
{
	return al_get_config_value(fs->sgm, NULL, "author");
}

void
get_sgm_metrics(sandbox_t* fs, int *out_x_res, int *out_y_res)
{
	*out_x_res = atoi(al_get_config_value(fs->sgm, NULL, "screen_width"));
	*out_y_res = atoi(al_get_config_value(fs->sgm, NULL, "screen_height"));
}

const char*
get_sgm_name(sandbox_t* fs)
{
	return al_get_config_value(fs->sgm, NULL, "name");
}

const char*
get_sgm_script(sandbox_t* fs)
{
	return al_get_config_value(fs->sgm, NULL, "script");
}

const char*
get_sgm_summary(sandbox_t* fs)
{
	return al_get_config_value(fs->sgm, NULL, "description");
}

vector_t*
list_filenames(sandbox_t* fs, const char* dirname, const char* base_dir, bool want_dirs)
{
	ALLEGRO_PATH*     dir_path;
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_PATH*     file_path;
	lstring_t*        filename;
	int               flag;
	int               fs_type;
	ALLEGRO_FS_ENTRY* fse = NULL;
	vector_t*         list = NULL;

	if (!resolve_path(fs, dirname, base_dir, &dir_path, &fs_type))
		goto on_error;
	if (!(list = new_vector(sizeof(lstring_t*))))
		goto on_error;
	switch (fs_type) {
	case SPHEREFS_SANDBOX:
		flag = want_dirs ? ALLEGRO_FILEMODE_ISDIR : ALLEGRO_FILEMODE_ISFILE;
		fse = al_create_fs_entry(al_path_cstr(dir_path, ALLEGRO_NATIVE_PATH_SEP));
		if (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fse)) {
			while (file_info = al_read_directory(fse)) {
				file_path = al_create_path(al_get_fs_entry_name(file_info));
				filename = lstring_from_cstr(al_get_path_filename(file_path));
				if (al_get_fs_entry_mode(file_info) & flag)
					push_back_vector(list, &filename);
				al_destroy_path(file_path);
			}
		}
		al_destroy_fs_entry(fse);
		break;
	case SPHEREFS_SPK:
		list = list_spk_filenames(fs->spk, al_path_cstr(dir_path, '/'));
		break;
	}
	al_destroy_path(dir_path);
	return list;

on_error:
	al_destroy_fs_entry(fse);
	al_destroy_path(dir_path);
	free_vector(list);
	return NULL;
}

sfs_file_t*
sfs_fopen(sandbox_t* fs, const char* filename, const char* base_dir, const char* mode)
{
	ALLEGRO_PATH* dir_path;
	sfs_file_t*   file;
	ALLEGRO_PATH* file_path;
	const char*   path;

	if (!(file = calloc(1, sizeof(sfs_file_t))))
		goto on_error;
	if (!resolve_path(fs, filename, base_dir, &file_path, &file->fs_type))
		goto on_error;
	switch (file->fs_type) {
	case SPHEREFS_SANDBOX:
		if (strchr(mode, 'w') || strchr(mode, '+') || strchr(mode, 'a')) {
			// write access requested, ensure directory exists
			dir_path = al_clone_path(file_path);
			al_set_path_filename(dir_path, NULL);
			al_make_directory(al_path_cstr(dir_path, ALLEGRO_NATIVE_PATH_SEP));
			al_destroy_path(dir_path);
		}
		path = al_path_cstr(file_path, ALLEGRO_NATIVE_PATH_SEP);
		if (!(file->fs_file = fopen(path, mode)))
			goto on_error;
		break;
	case SPHEREFS_SPK:
		path = al_path_cstr(file_path, '/');
		if (!(file->spk_file = spk_fopen(fs->spk, path, mode)))
			goto on_error;
		break;
	}
	al_destroy_path(file_path);
	return file;

on_error:
	free(file);
	return NULL;
}

void*
sfs_fslurp(sandbox_t* fs, const char* filename, const char* base_dir, size_t *out_size)
{
	ALLEGRO_FILE* file;
	ALLEGRO_PATH* file_path = NULL;
	int64_t       file_size;
	int           fs_type;
	const char*   path;
	void*         slurp;
	
	if (!resolve_path(fs, filename, base_dir, &file_path, &fs_type))
		goto on_error;
	switch (fs_type) {
	case SPHEREFS_SANDBOX:
		path = al_path_cstr(file_path, ALLEGRO_NATIVE_PATH_SEP);
		if (!(file = al_fopen(path, "rb")))
			goto on_error;
		al_fseek(file, 0, ALLEGRO_SEEK_END);
		if ((file_size = al_ftell(file)) > SIZE_MAX)
			goto on_error;  // file is hunger-pig sized
		if (!(slurp = malloc(*out_size = (size_t)file_size))) goto on_error;
		al_fseek(file, 0, ALLEGRO_SEEK_SET);
		al_fread(file, slurp, *out_size);
		al_fclose(file);
		break;
	case SPHEREFS_SPK:
		if (!(slurp = spk_fslurp(fs->spk, al_path_cstr(file_path, '/'), out_size)))
			goto on_error;
		break;
	}
	return slurp;

on_error:
	al_destroy_path(file_path);
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

bool
sfs_fexist(sandbox_t* fs, const char* filename, const char* base_dir)
{
	sfs_file_t*   file;
	
	if (!(file = sfs_fopen(fs, filename, base_dir, "rb")))
		return false;
	sfs_fclose(file);
	return true;
}

int
sfs_fputc(int ch, sfs_file_t* file)
{
	switch (file->fs_type) {
	case SPHEREFS_SANDBOX:
		return fputc(ch, file->fs_file);
	case SPHEREFS_SPK:
		return spk_fputc(ch, file->spk_file);
	default:
		return EOF;
	}
}

int
sfs_fputs(const char* string, sfs_file_t* file)
{
	switch (file->fs_type) {
	case SPHEREFS_SANDBOX:
		return fputs(string, file->fs_file);
	case SPHEREFS_SPK:
		return spk_fputs(string, file->spk_file);
	default:
		return false;
	}
}

size_t
sfs_fread(void* buf, size_t size, size_t count, sfs_file_t* file)
{
	switch (file->fs_type) {
	case SPHEREFS_SANDBOX:
		return fread(buf, size, count, file->fs_file);
	case SPHEREFS_SPK:
		return spk_fread(buf, size, count, file->spk_file);
	default:
		return 0;
	}
}

size_t
sfs_fwrite(const void* buf, size_t size, size_t count, sfs_file_t* file)
{
	switch (file->fs_type) {
	case SPHEREFS_SANDBOX:
		return fwrite(buf, size, count, file->fs_file);
	case SPHEREFS_SPK:
		return spk_fwrite(buf, size, count, file->spk_file);
	default:
		return 0;
	}
}

bool
sfs_fseek(sfs_file_t* file, long offset, sfs_seek_t origin)
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

static bool
resolve_path(sandbox_t* fs, const char* filename, const char* base_dir, ALLEGRO_PATH* *out_path, int *out_fs_type)
{
	// the path resolver is the core of SphereFS. it handles all canonization of paths
	// so that the game doesn't have to care whether it's running from a local directory,
	// Sphere SPK package, etc.
	
	ALLEGRO_PATH* origin = NULL;

	int i;

	*out_path = al_create_path(filename);

	if (filename[0] == '/' || filename[1] == ':') {  // absolute path
		*out_fs_type = SPHEREFS_SANDBOX;
	}
	else if (strlen(filename) >= 2 && memcmp(filename, "~/", 2) == 0) {  // BC ~/ prefix
		// ~/ has complex semantics. usually it aliases the location containing game.sgm,
		// but in certain cases it may refer elsewhere.
		al_destroy_path(*out_path);
		*out_path = al_create_path(&filename[2]);
		if (fs->type == SPHEREFS_SANDBOX)
			al_rebase_path(fs->fs_root, *out_path);
		*out_fs_type = fs->type;
	}
	else if (strlen(filename) >= 5 && filename[0] == '~' && filename[4] == '/') {  // SphereFS prefix
		al_destroy_path(*out_path);
		*out_path = al_create_path(&filename[5]);
		if (memcmp(filename, "~sgm/", 5) == 0) {  // game.sgm root
			if (fs->type == SPHEREFS_SANDBOX)
				al_rebase_path(fs->fs_root, *out_path);
			*out_fs_type = fs->type;
		}
		else if (memcmp(filename, "~sys/", 5) == 0) {  // engine "system" directory
			origin = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
			al_append_path_component(origin, "system");
			al_rebase_path(origin, *out_path);
			al_destroy_path(origin);
			*out_fs_type = SPHEREFS_SANDBOX;
		}
		else if (memcmp(filename, "~usr/", 5) == 0) {  // user profile
			origin = al_get_standard_path(ALLEGRO_USER_HOME_PATH);
			al_append_path_component(origin, "Sphere");
			al_make_directory(al_path_cstr(origin, ALLEGRO_NATIVE_PATH_SEP));
			al_rebase_path(origin, *out_path);
			al_destroy_path(origin);
			*out_fs_type = SPHEREFS_SANDBOX;
		}
		else  // unknown alias
			goto on_error;
	}
	else {  // default case, relative path
		origin = al_create_path_for_directory(base_dir);
		al_rebase_path(origin, *out_path);
		if (fs->type == SPHEREFS_SANDBOX)  // canonize to absolute
			al_rebase_path(fs->fs_root, *out_path);
		else {
			// SPK requires a fully canonized path, so we have to collapse
			// any '..' prior to returning the path.
			for (i = 0; i < al_get_path_num_components(*out_path); ++i) {
				if (strcmp(al_get_path_component(*out_path, i), "..") == 0) {
					al_remove_path_component(*out_path, i--);
					if (i >= 0)
						al_remove_path_component(*out_path, i);
				}
			}
		}
		al_destroy_path(origin);
		*out_fs_type = fs->type;
	}
	
	return true;

on_error:
	al_destroy_path(*out_path);
	al_destroy_path(origin);
	return false;
}
