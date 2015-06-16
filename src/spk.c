#include "minisphere.h"
#include "vector.h"

#include "spk.h"

struct spk
{
	unsigned int  refcount;
	ALLEGRO_PATH* path;
	ALLEGRO_FILE* file;
	vector_t*     index;
};

struct spk_entry
{
	char   file_path[SPHERE_PATH_MAX];
	size_t pack_size;
	size_t file_size;
	long   offset;
};

struct spk_file
{
	spk_t*        spk;
	uint8_t*      buffer;
	ALLEGRO_FILE* handle;
};

#pragma pack(push, 1)
struct spk_header
{
	char     signature[4];
	uint16_t version;
	uint32_t num_files;
	uint32_t index_offset;
	uint8_t  reserved[2];
};

struct spk_entry_hdr
{
	uint16_t version;
	uint16_t filename_size;
	uint32_t offset;
	uint32_t file_size;
	uint32_t compress_size;
};
#pragma pack(pop)

spk_t*
open_spk(const char* path)
{
	spk_t*               spk;
	struct spk_entry     spk_entry;
	struct spk_entry_hdr spk_entry_hdr;
	struct spk_header    spk_hdr;

	uint32_t i;

	if (!(spk = calloc(1, sizeof(spk_t)))) goto on_error;
	if (!(spk->path = al_create_path(path)))
		goto on_error;
	
	if (!(spk->file = al_fopen(path, "rb"))) goto on_error;
	if (al_fread(spk->file, &spk_hdr, sizeof(struct spk_header)) != sizeof(struct spk_header))
		goto on_error;
	if (memcmp(spk_hdr.signature, ".spk", 4) != 0) goto on_error;
	if (spk_hdr.version != 1) goto on_error;
	
	// load the package index
	if (!(spk->index = new_vector(sizeof(struct spk_entry))))
		goto on_error;
	al_fseek(spk->file, spk_hdr.index_offset, ALLEGRO_SEEK_SET);
	for (i = 0; i < spk_hdr.num_files; ++i) {
		if (al_fread(spk->file, &spk_entry_hdr, sizeof(struct spk_entry_hdr)) != sizeof(struct spk_entry_hdr))
			goto on_error;
		if (spk_entry_hdr.version != 1) goto on_error;
		spk_entry.pack_size = spk_entry_hdr.compress_size;
		spk_entry.file_size = spk_entry_hdr.file_size;
		spk_entry.offset = spk_entry_hdr.offset;
		al_fread(spk->file, spk_entry.file_path, spk_entry_hdr.filename_size);
		spk_entry.file_path[spk_entry_hdr.filename_size] = '\0';
		if (!push_back_vector(spk->index, &spk_entry)) goto on_error;
	}

	return ref_spk(spk);

on_error:
	if (spk != NULL) {
		al_destroy_path(spk->path);
		if (spk->file != NULL)
			al_fclose(spk->file);
		free_vector(spk->index);
		free(spk);
	}
	return NULL;
}

spk_t*
ref_spk(spk_t* spk)
{
	++spk->refcount;
	return spk;
}

void
free_spk(spk_t* spk)
{
	if (spk == NULL || --spk->refcount > 0)
		return;
	free_vector(spk->index);
	al_fclose(spk->file);
	free(spk);
}

spk_file_t*
spk_fopen(spk_t* spk, const char* path, const char* mode)
{
	ALLEGRO_FILE* al_file = NULL;
	void*         buffer = NULL;
	spk_file_t*   file = NULL;
	size_t        file_size;
	ALLEGRO_PATH* home_path;
	const char*   local_filename;
	ALLEGRO_PATH* local_dir_path;
	ALLEGRO_PATH* local_path = NULL;

	// get path to local cache file
	home_path = al_get_standard_path(ALLEGRO_USER_HOME_PATH);
	al_append_path_component(home_path, "Sphere");
	al_append_path_component(home_path, ".spkcache");
	al_append_path_component(home_path, al_get_path_filename(spk->path));
	local_path = al_create_path(path);
	al_rebase_path(home_path, local_path);
	al_destroy_path(home_path);
	
	// ensure all subdirectories exist
	local_filename = al_path_cstr(local_path, ALLEGRO_NATIVE_PATH_SEP);
	if (mode[0] == 'w' || mode[0] == 'a' || strchr(mode, '+')) {
		local_dir_path = al_clone_path(local_path);
		al_set_path_filename(local_dir_path, NULL);
		al_make_directory(al_path_cstr(local_dir_path, ALLEGRO_NATIVE_PATH_SEP));
		al_destroy_path(local_dir_path);
	}

	if (!(file = calloc(1, sizeof(spk_file_t))))
		goto on_error;
	
	if (al_filename_exists(local_filename)) {
		// local cache file already exists, open it	directly	
		if (!(al_file = al_fopen(local_filename, mode)))
			goto on_error;
	}
	else {
		if (!(buffer = spk_fslurp(spk, path, &file_size)) && mode[0] == 'r')
			goto on_error;
		if (strcmp(mode, "r") != 0 && strcmp(mode, "rb") != 0) {
			if (buffer != NULL && mode[0] != 'w') {
				// if a game requests write access to an existing file,
				// we extract it. this ensures file operations originating from
				// inside an SPK are transparent to the game.
				if (!(al_file = al_fopen(local_filename, "w")))
					goto on_error;
				al_fwrite(al_file, buffer, file_size);
				al_fclose(al_file);
			}
			free(buffer); buffer = NULL;
			if (!(al_file = al_fopen(local_filename, mode)))
				goto on_error;
		}
		else {
			// read-only: access unpacked file from memory (performance)
			if (!(al_file = al_open_memfile(buffer, file_size, mode)))
				goto on_error;
		}
	}

	al_destroy_path(local_path);
	
	file->buffer = buffer;
	file->handle = al_file;
	file->spk = ref_spk(spk);
	return file;

on_error:
	al_destroy_path(local_path);
	if (al_file != NULL)
		al_fclose(al_file);
	free(buffer);
	free(file);
	return NULL;
}

void
spk_fclose(spk_file_t* file)
{
	if (file == NULL)
		return;
	al_fclose(file->handle);
	free(file->buffer);
	free_spk(file->spk);
	free(file);
}

int
spk_fputc(int ch, spk_file_t* file)
{
	return al_fputc(file->handle, ch);
}

int
spk_fputs(const char* string, spk_file_t* file)
{
	return al_fputs(file->handle, string);
}

size_t
spk_fread(void* buf, size_t size, size_t count, spk_file_t* file)
{
	return al_fread(file->handle, buf, size * count) / size;
}

bool
spk_fseek(spk_file_t* file, long long offset, spk_seek_origin_t origin)
{
	return al_fseek(file->handle, offset, origin);
}

long long
spk_ftell(spk_file_t* file)
{
	return al_ftell(file->handle);
}

size_t
spk_fwrite(const void* buf, size_t size, size_t count, spk_file_t* file)
{
	return al_fwrite(file->handle, buf, size * count) / size;
}

void*
spk_fslurp(spk_t* spk, const char* path, size_t *out_size)
{
	void*             packdata = NULL;
	void*             unpacked = NULL;
	z_stream          z;
	struct spk_entry* p_entry;

	iter_t iter;

	memset(&z, 0, sizeof(z_stream));
	iter = iterate_vector(spk->index);
	while (p_entry = next_vector_item(&iter)) {
		if (strcmp(path, p_entry->file_path) == 0)
			break;
	}
	if (p_entry == NULL) goto on_error;
	if (!(packdata = malloc(p_entry->pack_size)))
		goto on_error;
	al_fseek(spk->file, p_entry->offset, ALLEGRO_SEEK_SET);
	if (al_fread(spk->file, packdata, p_entry->pack_size) < p_entry->pack_size)
		goto on_error;
	if (!(unpacked = malloc(p_entry->file_size))) goto on_error;
	z.avail_in = (uInt)p_entry->pack_size;
	z.next_in = packdata;
	z.avail_out = (uInt)p_entry->file_size;
	z.next_out = unpacked;
	if (inflateInit(&z) != Z_OK) goto on_error;
	if (inflate(&z, Z_FINISH) != Z_STREAM_END)
		goto on_error;
	inflateEnd(&z);
	free(packdata);
	*out_size = p_entry->file_size;
	return unpacked;

on_error:
	inflateEnd(&z);
	free(packdata);
	free(unpacked);
	return NULL;
}

vector_t*
list_spk_filenames(spk_t* spk, const char* dirname)
{
	lstring_t*        filename;
	vector_t*         list;
	const char*       maybe_filename;
	const char*       match;
	struct spk_entry* p_entry;
	
	iter_t iter;
	
	list = new_vector(sizeof(lstring_t*));
	iter = iterate_vector(spk->index);
	while (p_entry = next_vector_item(&iter)) {
		match = strstr(p_entry->file_path, dirname);
		maybe_filename = match + strlen(dirname);
		if (dirname[strlen(dirname) - 1] != '/')
			++maybe_filename;  // account for directory separator
		if (match == p_entry->file_path && strchr(maybe_filename, '/') == NULL) {
			filename = lstring_from_cstr(maybe_filename);
			push_back_vector(list, &filename);
		}
	}
	return list;
}
