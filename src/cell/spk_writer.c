/**
 *  Cell, the Sphere packaging compiler
 *  Copyright (c) 2015-2017, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "cell.h"
#include "spk_writer.h"

#include "fs.h"
#include "vector.h"

#pragma pack(push, 1)
struct spk_header
{
	char     magic[4];
	uint16_t version;
	uint32_t num_files;
	uint32_t idx_offset;
	uint8_t  reserved[2];
};
#pragma pack(pop)

struct spk_entry
{
	char*    pathname;
	uint32_t offset;
	uint32_t file_size;
	uint32_t pack_size;
};

struct spk_writer
{
	FILE*     file;
	vector_t* index;
};

spk_writer_t*
spk_create(const char* filename)
{
	const uint16_t VERSION = 1;

	spk_writer_t* writer;

	writer = calloc(1, sizeof(spk_writer_t));
	if (!(writer->file = fopen(filename, "wb")))
		return NULL;
	fseek(writer->file, sizeof(struct spk_header), SEEK_SET);

	writer->index = vector_new(sizeof(struct spk_entry));
	return writer;
}

void
spk_close(spk_writer_t* writer)
{
	const uint16_t VERSION = 1;

	struct spk_header hdr;
	uint32_t          idx_offset;
	uint16_t          path_size;
	struct spk_entry  *p_entry;

	iter_t iter;

	if (writer == NULL)
		return;

	// write package index
	idx_offset = ftell(writer->file);
	iter = vector_enum(writer->index);
	while (p_entry = iter_next(&iter)) {
		// for compatibility with Sphere 1.5, we have to include the NUL terminator
		// in the filename. this, despite the fact that there is an explicit length
		// field in the header...
		path_size = (uint16_t)strlen(p_entry->pathname) + 1;

		fwrite(&VERSION, sizeof(uint16_t), 1, writer->file);
		fwrite(&path_size, sizeof(uint16_t), 1, writer->file);
		fwrite(&p_entry->offset, sizeof(uint32_t), 1, writer->file);
		fwrite(&p_entry->file_size, sizeof(uint32_t), 1, writer->file);
		fwrite(&p_entry->pack_size, sizeof(uint32_t), 1, writer->file);
		fwrite(p_entry->pathname, 1, path_size, writer->file);

		// free the pathname buffer now, we no longer need it and
		// it saves us a few lines of code later.
		free(p_entry->pathname);
	}

	// write the SPK header
	fseek(writer->file, 0, SEEK_SET);
	memset(&hdr, 0, sizeof(struct spk_header));
	memcpy(hdr.magic, ".spk", 4);
	hdr.version = 1;
	hdr.num_files = (uint32_t)vector_len(writer->index);
	hdr.idx_offset = idx_offset;
	fwrite(&hdr, sizeof(struct spk_header), 1, writer->file);

	// finally, close the file
	fclose(writer->file);
	vector_free(writer->index);
	free(writer);
}

bool
spk_add_file(spk_writer_t* writer, fs_t* fs, const char* filename, const char* spk_pathname)
{
	uLong            bufsize;
	struct spk_entry idx_entry;
	void*            file_data = NULL;
	size_t           file_size;
	long             offset;
	void*            packdata;

	if (!(file_data = fs_fslurp(fs, filename, &file_size)))
		goto on_error;
	packdata = malloc(bufsize = compressBound((uLong)file_size));
	compress(packdata, &bufsize, file_data, (uLong)file_size);
	offset = ftell(writer->file);
	fwrite(packdata, bufsize, 1, writer->file);
	free(file_data);
	free(packdata);

	idx_entry.pathname = strdup(spk_pathname);
	idx_entry.file_size = (uint32_t)file_size;
	idx_entry.pack_size = bufsize;
	idx_entry.offset = offset;
	vector_push(writer->index, &idx_entry);
	return true;

on_error:
	free(file_data);
	return false;
}
