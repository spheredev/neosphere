#include "minisphere.h"
#include "spk.h"
#include "vector.h"

#include "spherefs.h"

enum fs_type
{
	SPHEREFS_UNKNOWN,
	SPHEREFS_LOCAL,
	SPHEREFS_SPK,
};

struct sandbox
{
	ALLEGRO_PATH*   fs_root;
	ALLEGRO_CONFIG* sgm;
	spk_t*          spk;
	int             type;
};

struct sfs_file
{
	enum fs_type  fs_type;
	ALLEGRO_FILE* handle;
	spk_file_t*   spk_file;
};

static bool resolve_path (sandbox_t* fs, const char* filename, const char* base_dir, ALLEGRO_PATH* *out_path, enum fs_type *out_fs_type);

sandbox_t*
new_sandbox(const char* path)
{
	ALLEGRO_FILE* al_file = NULL;
	const char*   extension;
	sandbox_t*    fs;
	int           res_x, res_y;
	ALLEGRO_PATH* sgm_path = NULL;
	size_t        sgm_size;
	void*         sgm_text = NULL;
	spk_t*        spk;

	if (!(fs = calloc(1, sizeof(sandbox_t))))
		goto on_error;
	extension = strrchr(path, '.');
	if (spk = open_spk(path)) {  // Sphere Package (.spk)
		console_log(1, "Opening game package '%s'", path);
		fs->type = SPHEREFS_SPK;
		fs->spk = spk;
		if (!(sgm_text = spk_fslurp(fs->spk, "game.sgm", &sgm_size)))
			goto on_error;
		al_file = al_open_memfile(sgm_text, sgm_size, "rb");
		if (!(fs->sgm = al_load_config_file_f(al_file)))
			goto on_error;
		al_fclose(al_file);
		free(sgm_text);
	}
	else {  // default case, unpacked game folder
		console_log(1, "Opening game '%s'", path);
		fs->type = SPHEREFS_LOCAL;
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
	}
	get_sgm_metrics(fs, &res_x, &res_y);
	console_log(1, "  Title: %s", get_sgm_name(fs));
	console_log(1, "  Author: %s", get_sgm_author(fs));
	console_log(1, "  Resolution: %ix%i", res_x, res_y);
	return fs;

on_error:
	if (al_file != NULL)
		al_fclose(al_file);
	free(sgm_text);
	if (sgm_path != NULL)
		al_destroy_path(sgm_path);
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
	case SPHEREFS_LOCAL:
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
	enum fs_type      fs_type;
	ALLEGRO_FS_ENTRY* fse = NULL;
	vector_t*         list = NULL;
	int               type_flag;

	if (!resolve_path(fs, dirname, base_dir, &dir_path, &fs_type))
		goto on_error;
	if (!(list = new_vector(sizeof(lstring_t*))))
		goto on_error;
	switch (fs_type) {
	case SPHEREFS_LOCAL:
		type_flag = want_dirs ? ALLEGRO_FILEMODE_ISDIR : ALLEGRO_FILEMODE_ISFILE;
		fse = al_create_fs_entry(al_path_cstr(dir_path, ALLEGRO_NATIVE_PATH_SEP));
		if (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fse)) {
			while (file_info = al_read_directory(fse)) {
				file_path = al_create_path(al_get_fs_entry_name(file_info));
				filename = lstr_newf("%s", al_get_path_filename(file_path));
				if (al_get_fs_entry_mode(file_info) & type_flag)
					push_back_vector(list, &filename);
				al_destroy_path(file_path);
			}
		}
		al_destroy_fs_entry(fse);
		break;
	case SPHEREFS_SPK:
		list = list_spk_filenames(fs->spk, al_path_cstr(dir_path, '/'), want_dirs);
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
	case SPHEREFS_LOCAL:
		if (strchr(mode, 'w') || strchr(mode, '+') || strchr(mode, 'a')) {
			// write access requested, ensure directory exists
			dir_path = al_clone_path(file_path);
			al_set_path_filename(dir_path, NULL);
			al_make_directory(al_path_cstr(dir_path, ALLEGRO_NATIVE_PATH_SEP));
			al_destroy_path(dir_path);
		}
		path = al_path_cstr(file_path, ALLEGRO_NATIVE_PATH_SEP);
		if (!(file->handle = al_fopen(path, mode)))
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

void
sfs_fclose(sfs_file_t* file)
{
	if (file == NULL)
		return;
	switch (file->fs_type) {
	case SPHEREFS_LOCAL:
		al_fclose(file->handle);
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
	case SPHEREFS_LOCAL:
		return al_fputc(file->handle, ch);
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
	case SPHEREFS_LOCAL:
		return al_fputs(file->handle, string);
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
	case SPHEREFS_LOCAL:
		return al_fread(file->handle, buf, size * count) / size;
	case SPHEREFS_SPK:
		return spk_fread(buf, size, count, file->spk_file);
	default:
		return 0;
	}
}

void*
sfs_fslurp(sandbox_t* fs, const char* filename, const char* base_dir, size_t *out_size)
{
	sfs_file_t* file = NULL;
	size_t      data_size;
	void*       slurp;

	if (!(file = sfs_fopen(fs, filename, base_dir, "rb")))
		goto on_error;
	sfs_fseek(file, 0, SFS_SEEK_END);
	data_size = sfs_ftell(file);
	if (!(slurp = malloc(data_size + 1)))
		goto on_error;
	sfs_fseek(file, 0, SFS_SEEK_SET);
	sfs_fread(slurp, data_size, 1, file);
	sfs_fclose(file);
	*((char*)slurp + data_size) = '\0';  // nifty NUL terminator
	
	if (out_size) *out_size = data_size;
	return slurp;

on_error:
	sfs_fclose(file);
	return NULL;
}

bool
sfs_fspew(sandbox_t* fs, const char* filename, const char* base_dir, void* buf, size_t size)
{
	sfs_file_t* file = NULL;

	if (!(file = sfs_fopen(fs, filename, base_dir, "wb")))
		return false;
	sfs_fwrite(buf, size, 1, file);
	sfs_fclose(file);
	return true;
}

bool
sfs_fseek(sfs_file_t* file, long long offset, sfs_whence_t whence)
{
	switch (file->fs_type) {
	case SPHEREFS_LOCAL:
		return al_fseek(file->handle, offset, whence) == 0;
	case SPHEREFS_SPK:
		return spk_fseek(file->spk_file, offset, whence);
	}
	return false;
}

long long
sfs_ftell(sfs_file_t* file)
{
	switch (file->fs_type) {
	case SPHEREFS_LOCAL:
		return al_ftell(file->handle);
	case SPHEREFS_SPK:
		return spk_ftell(file->spk_file);
	}
	return -1;
}

size_t
sfs_fwrite(const void* buf, size_t size, size_t count, sfs_file_t* file)
{
	switch (file->fs_type) {
	case SPHEREFS_LOCAL:
		return al_fwrite(file->handle, buf, size * count) / size;
	case SPHEREFS_SPK:
		return spk_fwrite(buf, size, count, file->spk_file);
	default:
		return 0;
	}
}

bool
sfs_mkdir(sandbox_t* fs, const char* dirname, const char* base_dir)
{
	enum fs_type  fs_type;
	ALLEGRO_PATH* path;
	
	if (!resolve_path(fs, dirname, base_dir, &path, &fs_type))
		return false;
	switch (fs_type) {
	case SPHEREFS_LOCAL:
		return al_make_directory(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
	case SPHEREFS_SPK:
		return false;
	default:
		return false;
	}
}

bool
sfs_rmdir(sandbox_t* fs, const char* dirname, const char* base_dir)
{
	enum fs_type  fs_type;
	ALLEGRO_PATH* path;

	if (!resolve_path(fs, dirname, base_dir, &path, &fs_type))
		return false;
	switch (fs_type) {
	case SPHEREFS_LOCAL:
		return rmdir(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP)) == 0;
	case SPHEREFS_SPK:
		return false;
	default:
		return false;
	}
}

bool
sfs_rename(sandbox_t* fs, const char* name1, const char* name2, const char* base_dir)
{
	enum fs_type  fs_type;
	ALLEGRO_PATH* path1;
	ALLEGRO_PATH* path2;
	int           result;

	if (!resolve_path(fs, name1, base_dir, &path1, &fs_type))
		return false;
	if (!resolve_path(fs, name2, base_dir, &path2, &fs_type))
		return false;
	switch (fs_type) {
	case SPHEREFS_LOCAL:
		result = rename(
			al_path_cstr(path1, ALLEGRO_NATIVE_PATH_SEP),
			al_path_cstr(path2, ALLEGRO_NATIVE_PATH_SEP));
		return result == 0;
	case SPHEREFS_SPK:
		return false;
	default:
		return false;
	}
}

bool
sfs_unlink(sandbox_t* fs, const char* filename, const char* base_dir)
{
	enum fs_type  fs_type;
	ALLEGRO_PATH* path;

	if (!resolve_path(fs, filename, base_dir, &path, &fs_type))
		return false;
	switch (fs_type) {
	case SPHEREFS_LOCAL:
		return unlink(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP)) == 0;
	case SPHEREFS_SPK:
		return false;
	default:
		return false;
	}
}

static bool
resolve_path(sandbox_t* fs, const char* filename, const char* base_dir, ALLEGRO_PATH* *out_path, enum fs_type *out_fs_type)
{
	// the path resolver is the core of SphereFS. it handles all canonization of paths
	// so that the game doesn't have to care whether it's running from a local directory,
	// Sphere SPK package, etc.
	
	ALLEGRO_PATH* origin = NULL;

	int i;

	*out_path = al_create_path(filename);
	al_make_path_canonical(*out_path);

	if (filename[0] == '/' || filename[1] == ':')  // absolute path
		*out_fs_type = SPHEREFS_LOCAL;
	else if (strlen(filename) >= 2 && memcmp(filename, "~/", 2) == 0) {  // ~/ prefix
		// the ~/ prefix is maintained for backwards compatibility.
		// counterintuitively, it references the directory containing game.sgm,
		// NOT the user's home directory.
		if (fs == NULL) goto on_error;
		al_destroy_path(*out_path);
		*out_path = al_create_path(filename + 2);
		if (fs->type == SPHEREFS_LOCAL)
			al_rebase_path(fs->fs_root, *out_path);
		*out_fs_type = fs->type;
	}
	else if (strlen(filename) >= 5 && filename[0] == '~' && filename[4] == '/') {  // SphereFS ~xxx/ prefix
		al_destroy_path(*out_path);
		*out_path = al_create_path(filename + 5);
		if (memcmp(filename, "~sgm/", 5) == 0) {  // game.sgm root
			if (fs == NULL) goto on_error;
			if (fs->type == SPHEREFS_LOCAL)
				al_rebase_path(fs->fs_root, *out_path);
			*out_fs_type = fs->type;
		}
		else if (memcmp(filename, "~sys/", 5) == 0) {  // engine "system" directory
			origin = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
			al_append_path_component(origin, "system");
			al_rebase_path(origin, *out_path);
			al_destroy_path(origin);
			*out_fs_type = SPHEREFS_LOCAL;
		}
		else if (memcmp(filename, "~usr/", 5) == 0) {  // user profile
			origin = al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
			al_append_path_component(origin, "minisphere");
			al_make_directory(al_path_cstr(origin, ALLEGRO_NATIVE_PATH_SEP));
			al_rebase_path(origin, *out_path);
			al_destroy_path(origin);
			*out_fs_type = SPHEREFS_LOCAL;
		}
		else  // unrecognized prefix
			goto on_error;
	}
	else {  // default case: assume relative path
		if (fs == NULL) goto on_error;
		origin = al_create_path_for_directory(base_dir);
		al_rebase_path(origin, *out_path);
		if (fs->type == SPHEREFS_LOCAL)  // convert to absolute path
			al_rebase_path(fs->fs_root, *out_path);
		else {
			// SPK requires a fully canonized path, so we have to collapse
			// any '..' components prior to returning the path.
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
	*out_path = NULL;
	*out_fs_type = SPHEREFS_UNKNOWN;
	return false;
}
