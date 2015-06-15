#include "minisphere.h"
#include "vector.h"

#include "spk.h"

struct spk
{
	unsigned int  refcount;
	FILE*         file;
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
	spk_t*   spk;
	size_t   size;
	uint8_t* buffer;
	uint8_t* ptr;
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
	if (!(spk->file = fopen(path, "rb"))) goto on_error;
	if (fread(&spk_hdr, sizeof(struct spk_header), 1, spk->file) != 1)
		goto on_error;
	if (memcmp(spk_hdr.signature, ".spk", 4) != 0) goto on_error;
	if (spk_hdr.version != 1) goto on_error;
	
	// load the package index
	if (!(spk->index = new_vector(sizeof(struct spk_entry))))
		goto on_error;
	fseek(spk->file, spk_hdr.index_offset, SEEK_SET);
	for (i = 0; i < spk_hdr.num_files; ++i) {
		if (fread(&spk_entry_hdr, sizeof(struct spk_entry_hdr), 1, spk->file) != 1)
			goto on_error;
		if (spk_entry_hdr.version != 1) goto on_error;
		spk_entry.pack_size = spk_entry_hdr.compress_size;
		spk_entry.file_size = spk_entry_hdr.file_size;
		spk_entry.offset = spk_entry_hdr.offset;
		fread(spk_entry.file_path, spk_entry_hdr.filename_size, 1, spk->file);
		spk_entry.file_path[spk_entry_hdr.filename_size] = '\0';
		if (!push_back_vector(spk->index, &spk_entry)) goto on_error;
	}

	return ref_spk(spk);

on_error:
	if (spk != NULL) {
		if (spk->file != NULL)
			fclose(spk->file);
		if (spk->index != NULL)
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
	fclose(spk->file);
	free(spk);
}

spk_file_t*
spk_fopen(spk_t* spk, const char* path)
{
	void*       buffer;
	spk_file_t* file = NULL;
	size_t      file_size;

	if (!(buffer = spk_fslurp(spk, path, &file_size)))
		goto on_error;
	if (!(file = calloc(1, sizeof(spk_file_t))))
		goto on_error;
	file->buffer = buffer;
	file->size = file_size;
	file->ptr = file->buffer;
	file->spk = ref_spk(spk);
	return file;

on_error:
	free(buffer);
	free(file);
	return NULL;
}

void
spk_fclose(spk_file_t* file)
{
	if (file == NULL)
		return;
	free(file->buffer);
	free_spk(file->spk);
	free(file);
}

size_t
spk_fread(void* buf, size_t count, size_t size, spk_file_t* file)
{
	size_t bytes_left;
	size_t num_bytes;
	
	bytes_left = file->size - (file->ptr - file->buffer);
	num_bytes = count * size;
	if (num_bytes > bytes_left)
		num_bytes = bytes_left;
	memcpy(buf, file->ptr, num_bytes);
	file->ptr += num_bytes;
	return num_bytes / size;
}

bool
spk_fseek(spk_file_t* file, long offset, spk_seek_origin_t origin)
{
	uint8_t* new_ptr;

	new_ptr = origin == SPK_SEEK_SET ? file->buffer + offset
		: origin == SPK_SEEK_CUR ? file->ptr + offset
		: origin == SPK_SEEK_END ? file->buffer + file->size + offset
		: NULL;
	if (new_ptr < file->buffer || new_ptr > file->buffer + file->size)
		return false;
	file->ptr = new_ptr;
	return true;
}

long
spk_ftell(spk_file_t* file)
{
	return file->ptr - file->buffer;
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
	fseek(spk->file, p_entry->offset, SEEK_SET);
	if (fread(packdata, 1, p_entry->pack_size, spk->file) < p_entry->pack_size)
		goto on_error;
	if (!(unpacked = malloc(p_entry->file_size)))
		goto on_error;
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
		maybe_filename = match + strlen(dirname) + 1;
		if (match == p_entry->file_path && strchr(maybe_filename, '/') == NULL) {
			filename = lstring_from_cstr(maybe_filename);
			push_back_vector(list, &filename);
		}
	}
	return list;
}
