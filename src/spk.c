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
	lstring_t* file_path;
	long       pack_size;
	long       file_size;
	long       offset;
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
	struct spk_entry*    p_entry;

	iter_t   iter;
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
		if (!(spk_entry.file_path = read_lstring_raw(spk->file, spk_entry_hdr.filename_size, true)))
			goto on_error;
		if (!push_back_vector(spk->index, &spk_entry)) goto on_error;
	}

	return ref_spk(spk);

on_error:
	if (spk != NULL) {
		if (spk->file != NULL)
			fclose(spk->file);
		if (spk->index != NULL) {
			iter = iterate_vector(spk->index);
			while (p_entry = next_vector_item(&iter))
				free_lstring(p_entry->file_path);
			free_vector(spk->index);
		}
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
	struct spk_entry* p_entry;
	
	iter_t     iter;
	
	if (spk == NULL || --spk->refcount > 0)
		return;
	iter = iterate_vector(spk->index);
	while (p_entry = next_vector_item(&iter))
		free_lstring(p_entry->file_path);
	free_vector(spk->index);
	fclose(spk->file);
	free(spk);
}

spk_file_t*
spk_fopen(spk_t* spk, const char* path)
{
	
	void*             packdata = NULL;
	spk_file_t*       file;
	z_stream          z;
	struct spk_entry* p_entry;

	iter_t iter;

	if (!(file = calloc(1, sizeof(spk_file_t))))
		goto on_error;
	iter = iterate_vector(spk->index);
	while (p_entry = next_vector_item(&iter)) {
		if (strcmp(path, lstr_cstr(p_entry->file_path)) == 0)
			break;
	}
	if (p_entry == NULL) goto on_error;
	if (!(packdata = malloc(p_entry->pack_size)))
		goto on_error;
	fseek(spk->file, p_entry->offset, SEEK_SET);
	if (fread(packdata, 1, p_entry->pack_size, spk->file) < p_entry->pack_size)
		goto on_error;
	memset(&z, 0, sizeof(z_stream));
	if (!(file->buffer = malloc(p_entry->file_size)))
		goto on_error;
	z.avail_in = p_entry->pack_size;
	z.next_in = packdata;
	z.avail_out = p_entry->file_size;
	z.next_out = file->buffer;
	if (inflateInit(&z) != Z_OK) goto on_error;
	if (inflate(&z, Z_FINISH) != Z_STREAM_END)
		goto on_error;
	inflateEnd(&z);
	free(packdata);
	file->size = p_entry->file_size;
	file->ptr = file->buffer;
	file->spk = ref_spk(spk);
	return file;

on_error:
	inflateEnd(&z);
	free(packdata);
	if (file != NULL) {
		free(file->buffer);
		free(file);
	}
	return NULL;
}

void
spk_fclose(spk_file_t* file)
{
	free(file->buffer);
	free_spk(file->spk);
	free(file);
}

size_t
spk_fread(spk_file_t* file, void* buf, size_t num_bytes)
{
	size_t bytes_left;
	
	bytes_left = file->size - (file->ptr - file->buffer);
	if (num_bytes > bytes_left)
		num_bytes = bytes_left;
	memcpy(buf, file->ptr, num_bytes);
	file->ptr += num_bytes;
	return num_bytes;
}

bool
spk_fseek(spk_file_t* file, long new_pos, spk_seek_origin_t origin)
{
	uint8_t* new_ptr;

	new_ptr = origin == SPK_SEEK_SET ? file->buffer + new_pos
		: origin == SPK_SEEK_CUR ? file->ptr + new_pos
		: origin == SPK_SEEK_END ? file->buffer + file->size + new_pos
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
