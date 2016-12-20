#include "minisphere.h"
#include "spk.h"

#include "vector.h"

struct spk
{
	unsigned int  refcount;
	unsigned int  id;
	path_t*       path;
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
	char*         filename;
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

static unsigned int s_next_spk_id = 0;

spk_t*
open_spk(const char* path)
{
	spk_t*               spk;
	struct spk_entry     spk_entry;
	struct spk_entry_hdr spk_entry_hdr;
	struct spk_header    spk_hdr;

	uint32_t i;

	console_log(2, "opening SPK #%u as `%s`", s_next_spk_id, path);
	
	spk = calloc(1, sizeof(spk_t));

	if (!(spk->file = al_fopen(path, "rb"))) goto on_error;
	if (al_fread(spk->file, &spk_hdr, sizeof(struct spk_header)) != sizeof(struct spk_header))
		goto on_error;
	if (memcmp(spk_hdr.signature, ".spk", 4) != 0) goto on_error;
	if (spk_hdr.version != 1) goto on_error;
	
	spk->path = path_new(path);

	// load the package index
	console_log(4, "reading package index for SPK #%u", s_next_spk_id);
	spk->index = vector_new(sizeof(struct spk_entry));
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
		if (!vector_push(spk->index, &spk_entry)) goto on_error;
	}

	spk->id = s_next_spk_id++;
	return ref_spk(spk);

on_error:
	console_log(2, "failed to open SPK #%u", s_next_spk_id++);
	if (spk != NULL) {
		path_free(spk->path);
		if (spk->file != NULL)
			al_fclose(spk->file);
		vector_free(spk->index);
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
	
	console_log(4, "disposing SPK #%u no longer in use", spk->id);
	vector_free(spk->index);
	al_fclose(spk->file);
	free(spk);
}

spk_file_t*
spk_fopen(spk_t* spk, const char* path, const char* mode)
{
	ALLEGRO_FILE* al_file = NULL;
	void*         buffer = NULL;
	path_t*       cache_path;
	spk_file_t*   file = NULL;
	size_t        file_size;
	const char*   local_filename;
	path_t*       local_path;

	console_log(4, "opening `%s` (%s) from SPK #%u", path, mode, spk->id);
	
	// get path to local cache file
	cache_path = path_rebase(path_new("minisphere/.spkcache/"), homepath());
	path_append_dir(cache_path, path_filename(spk->path));
	local_path = path_rebase(path_new(path), cache_path);
	path_free(cache_path);
	
	// ensure all subdirectories exist
	local_filename = path_cstr(local_path);
	if (mode[0] == 'w' || mode[0] == 'a' || strchr(mode, '+'))
		path_mkdir(local_path);

	if (!(file = calloc(1, sizeof(spk_file_t))))
		goto on_error;
	
	if (al_filename_exists(local_filename)) {
		// local cache file already exists, open it	directly	
		console_log(4, "using locally cached file for #%u:`%s`", spk->id, path);
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
				console_log(4, "extracting #%u:`%s`, write access requested", spk->id, path);
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

	path_free(local_path);
	
	file->buffer = buffer;
	file->filename = strdup(path);
	file->handle = al_file;
	file->spk = ref_spk(spk);
	return file;

on_error:
	console_log(4, "failed to open `%s` from SPK #%u", path, spk->id);
	path_free(local_path);
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
	console_log(4, "closing `%s` from SPK #%u", file->filename, file->spk->id);
	al_fclose(file->handle);
	free(file->buffer);
	free(file->filename);
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
	struct spk_entry* fileinfo;
	void*             packdata = NULL;
	void*             unpacked = NULL;
	uLong             unpack_size;

	iter_t iter;

	console_log(3, "unpacking `%s` from SPK #%u", path, spk->id);
	
	iter = vector_enum(spk->index);
	while (fileinfo = vector_next(&iter)) {
		if (strcasecmp(path, fileinfo->file_path) == 0)
			break;
	}
	if (fileinfo == NULL) goto on_error;
	if (!(packdata = malloc(fileinfo->pack_size)))
		goto on_error;
	al_fseek(spk->file, fileinfo->offset, ALLEGRO_SEEK_SET);
	if (al_fread(spk->file, packdata, fileinfo->pack_size) < fileinfo->pack_size)
		goto on_error;
	if (!(unpacked = malloc(fileinfo->file_size + 1)))
		goto on_error;
	unpack_size = (uLong)fileinfo->file_size;
	if (uncompress(unpacked, &unpack_size, packdata, (uLong)fileinfo->pack_size) != Z_OK)
		goto on_error;
	*((char*)unpacked + unpack_size) = '\0';
	free(packdata);
	
	*out_size = unpack_size;
	return unpacked;

on_error:
	console_log(3, "failed to unpack `%s` from SPK #%u", path, spk->id);
	free(packdata);
	free(unpacked);
	return NULL;
}

vector_t*
list_spk_filenames(spk_t* spk, const char* dirname, bool want_dirs)
{
	// HERE BE DRAGONS!
	// this function is kind of a monstrosity because the SPK format doesn't have
	// any real concept of a directory - each asset is stored with its full path
	// as its filename. as such we have to do some ugly parsing and de-duplication,
	// particularly in the case of directories.
	
	lstring_t*        filename;
	char*             found_dirname;
	bool              is_in_set;
	vector_t*         list;
	const char*       maybe_dirname;
	const char*       maybe_filename;
	const char*       match;
	struct spk_entry* p_entry;
	
	iter_t iter, iter2;
	lstring_t** item;
	
	list = vector_new(sizeof(lstring_t*));
	iter = vector_enum(spk->index);
	while (p_entry = vector_next(&iter)) {
		if (!want_dirs) {  // list files
			if (!(match = strstr(p_entry->file_path, dirname)))
				continue;
			if (match != p_entry->file_path) continue;
			maybe_filename = match + strlen(dirname);
			if (dirname[strlen(dirname) - 1] != '/') {
				if (maybe_filename[0] != '/')
					continue;  // oops, matched a partial file name
				++maybe_filename;  // account for directory separator
			}
			if (strchr(maybe_filename, '/'))
				continue;  // ignore files in subdirectories
			
			// if we got to this point, we have a valid filename
			filename = lstr_newf("%s", maybe_filename);
			vector_push(list, &filename);
		}
		else {  // list directories
			if (!(match = strstr(p_entry->file_path, dirname)))
				continue;
			if (match != p_entry->file_path) continue;
			maybe_dirname = match + strlen(dirname);
			if (dirname[strlen(dirname) - 1] != '/') {
				if (maybe_dirname[0] != '/')
					continue;  // oops, matched a partial file name
				++maybe_dirname;  // account for directory separator
			}
			if (!(maybe_filename = strchr(maybe_dirname, '/')))
				continue;  // ignore files
			if (strchr(++maybe_filename, '/'))
				continue;  // ignore subdirectories

			// if we got to this point, we have a valid directory name
			found_dirname = strdup(maybe_dirname);
			*strchr(found_dirname, '/') = '\0';
			filename = lstr_newf("%s", found_dirname);
			iter2 = vector_enum(list);
			is_in_set = false;
			while (item = vector_next(&iter2)) {
				is_in_set |= lstr_cmp(filename, *item) == 0;
			}
			if (!is_in_set)  // avoid duplicate listings
				vector_push(list, &filename);
			free(found_dirname);
		}
	}
	return list;
}
