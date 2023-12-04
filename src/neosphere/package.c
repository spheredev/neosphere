/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
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
 *  * Neither the name of Spherical nor the names of its contributors may be
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

#include "neosphere.h"
#include "package.h"

#include "compress.h"
#include "vector.h"

struct asset
{
	package_t*    package;
	uint8_t*      buffer;
	char*         filename;
	ALLEGRO_FILE* handle;
};

struct package
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

static unsigned int s_next_package_id = 1;

package_t*
package_open(const char* path)
{
	package_t*           package;
	struct spk_entry     spk_entry;
	struct spk_entry_hdr spk_entry_hdr;
	struct spk_header    spk_hdr;

	uint32_t i;

	console_log(2, "opening package #%u '%s'", s_next_package_id, path);

	if (!(package = calloc(1, sizeof(package_t))))
		goto on_error;

	if (!(package->file = al_fopen(path, "rb")))
		goto on_error;
	if (al_fread(package->file, &spk_hdr, sizeof(struct spk_header)) != sizeof(struct spk_header))
		goto on_error;
	if (memcmp(spk_hdr.signature, ".spk", 4) != 0)
		goto on_error;
	if (spk_hdr.version != 1)
		goto on_error;

	package->path = path_new(path);

	// load the package index
	console_log(4, "reading package index for package #%u", s_next_package_id);
	package->index = vector_new(sizeof(struct spk_entry));
	al_fseek(package->file, spk_hdr.index_offset, ALLEGRO_SEEK_SET);
	for (i = 0; i < spk_hdr.num_files; ++i) {
		if (al_fread(package->file, &spk_entry_hdr, sizeof(struct spk_entry_hdr)) != sizeof(struct spk_entry_hdr))
			goto on_error;
		if (spk_entry_hdr.version != 1)
			goto on_error;
		spk_entry.pack_size = spk_entry_hdr.compress_size;
		spk_entry.file_size = spk_entry_hdr.file_size;
		spk_entry.offset = spk_entry_hdr.offset;
		al_fread(package->file, spk_entry.file_path, spk_entry_hdr.filename_size);
		spk_entry.file_path[spk_entry_hdr.filename_size] = '\0';
		if (!vector_push(package->index, &spk_entry))
			goto on_error;
	}

	package->id = s_next_package_id++;
	return package_ref(package);

on_error:
	console_log(2, "failed to open package #%u", s_next_package_id++);
	if (package != NULL) {
		path_free(package->path);
		if (package->file != NULL)
			al_fclose(package->file);
		vector_free(package->index);
		free(package);
	}
	return NULL;
}

package_t*
package_ref(package_t* it)
{
	++it->refcount;
	return it;
}

void
package_unref(package_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;

	console_log(4, "disposing package #%u no longer in use", it->id);
	vector_free(it->index);
	al_fclose(it->file);
	free(it);
}

bool
package_dir_exists(const package_t* it, const char* dirname)
{
	struct spk_entry* entry;
	path_t*           path;
	const char*       pathname;

	iter_t iter;

	path = path_new_dir(dirname);
	pathname = path_cstr(path);
	iter = vector_enum(it->index);
	while ((entry = iter_next(&iter))) {
		// SPK doesn't really have directories, so we fake it by checking
		// if any of the stored filenames begin with the directory path as a
		// prefix.
		if (strstr(entry->file_path, pathname) == entry->file_path) {
			path_free(path);
			return true;
		}
	}
	path_free(path);
	return false;
}

bool
package_file_exists(const package_t* it, const char* filename)
{
	struct spk_entry* entry;
	path_t*           path;
	const char*       pathname;

	iter_t iter;

	path = path_new(filename);
	pathname = path_cstr(path);
	iter = vector_enum(it->index);
	while ((entry = iter_next(&iter))) {
		// SPK doesn't really have directories, so we fake it by checking
		// if any of the stored filenames begin with the directory path as a
		// prefix.
		if (strcmp(entry->file_path, pathname) == 0) {
			path_free(path);
			return true;
		}
	}
	path_free(path);
	return false;
}

vector_t*
package_list_dir(package_t* package, const char* dirname, bool want_dirs, bool recursive)
{
	// HERE BE DRAGONS!
	// this function is kind of a monstrosity because the SPK format doesn't have
	// any real concept of a directory - each asset is stored with its full path
	// as its filename.  as such we have to do some ugly parsing and de-duplication,
	// particularly in the case of directories.

	struct spk_entry* file_info;
	char*             found_dirname;
	bool              is_in_set;
	vector_t*         list;
	const char*       maybe_dirname;
	const char*       maybe_filename;
	const char*       match;
	path_t*           path;

	iter_t iter, iter2;
	path_t** item;

	list = vector_new(sizeof(path_t*));
	iter = vector_enum(package->index);
	while ((file_info = iter_next(&iter))) {
		if (!want_dirs) {  // list files
			if (!(match = strstr(file_info->file_path, dirname)))
				continue;
			if (match != file_info->file_path)
				continue;
			maybe_filename = match + strlen(dirname);
			if (dirname[strlen(dirname) - 1] != '/') {
				if (maybe_filename[0] != '/')
					continue;  // oops, matched a partial file name
				++maybe_filename;  // account for directory separator
			}
			if (strchr(maybe_filename, '/') && !recursive)
				continue;  // ignore files in subdirectories

			// if we got to this point, we have a valid filename
			path = path_new(maybe_filename);
			vector_push(list, &path);
		}
		else {  // list directories
			if (!(match = strstr(file_info->file_path, dirname)))
				continue;
			if (match != file_info->file_path)
				continue;
			maybe_dirname = match + strlen(dirname);
			if (dirname[strlen(dirname) - 1] != '/') {
				if (maybe_dirname[0] != '/')
					continue;  // oops, matched a partial file name
				++maybe_dirname;  // account for directory separator
			}
			if (!(maybe_filename = strchr(maybe_dirname, '/')))
				continue;  // ignore files
			if (strchr(++maybe_filename, '/') && !recursive)
				continue;  // ignore subdirectories

			// if we got to this point, we have a valid directory name
			found_dirname = strdup(maybe_dirname);
			*strchr(found_dirname, '/') = '\0';
			path = path_new(found_dirname);
			iter2 = vector_enum(list);
			is_in_set = false;
			while ((item = iter_next(&iter2)))
				is_in_set |= path_is(path, *item) == 0;
			if (!is_in_set)  // avoid duplicate listings
				vector_push(list, &path);
			free(found_dirname);
		}
	}
	return list;
}

asset_t*
asset_fopen(package_t* package, const char* pathname, const char* mode)
{
	ALLEGRO_FILE* al_file = NULL;
	asset_t*      asset = NULL;
	void*         buffer = NULL;
	path_t*       cache_path;
	size_t        file_size;
	const char*   local_filename;
	path_t*       local_path;

	console_log(4, "opening '%s' (%s) from package #%u", pathname, mode, package->id);

	// get path to local cache file
	cache_path = path_rebase(path_new("assetCache"), app_data_path());
	path_append_dir(cache_path, path_filename(package->path));
	local_path = path_rebase(path_new(pathname), cache_path);
	path_free(cache_path);

	// ensure all subdirectories exist
	local_filename = path_cstr(local_path);
	if (mode[0] == 'w' || mode[0] == 'a' || strchr(mode, '+'))
		path_mkdir(local_path);

	if (!(asset = calloc(1, sizeof(asset_t))))
		goto on_error;

	if (al_filename_exists(local_filename)) {
		// local cache file already exists, open it	directly
		console_log(4, "using locally cached file for #%u:'%s'", package->id, pathname);
		if (!(al_file = al_fopen(local_filename, mode)))
			goto on_error;
	}
	else {
		if (!(buffer = asset_fslurp(package, pathname, &file_size)) && mode[0] == 'r')
			goto on_error;
		if (strcmp(mode, "r") != 0 && strcmp(mode, "rb") != 0) {
			if (buffer != NULL && mode[0] != 'w') {
				// if a game requests write access to an existing file,
				// we extract it. this ensures file operations originating from
				// inside an SPK are transparent to the game.
				console_log(4, "extracting #%u:'%s', write access requested", package->id, pathname);
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

	asset->buffer = buffer;
	asset->filename = strdup(pathname);
	asset->handle = al_file;
	asset->package = package_ref(package);
	return asset;

on_error:
	console_log(4, "failed to open '%s' from package #%u", pathname, package->id);
	path_free(local_path);
	if (al_file != NULL)
		al_fclose(al_file);
	free(buffer);
	free(asset);
	return NULL;
}

void
asset_fclose(asset_t* file)
{
	if (file == NULL)
		return;
	console_log(4, "closing '%s' from package #%u", file->filename, file->package->id);
	al_fclose(file->handle);
	free(file->buffer);
	free(file->filename);
	package_unref(file->package);
	free(file);
}

int
asset_fputc(int ch, asset_t* file)
{
	return al_fputc(file->handle, ch);
}

int
asset_fputs(const char* string, asset_t* file)
{
	return al_fputs(file->handle, string);
}

size_t
asset_fread(void* buf, size_t size, size_t count, asset_t* file)
{
	return al_fread(file->handle, buf, size * count) / size;
}

bool
asset_fseek(asset_t* file, long long offset, spk_seek_origin_t origin)
{
	return al_fseek(file->handle, offset, origin);
}

void*
asset_fslurp(package_t* package, const char* path, size_t *out_size)
{
	struct spk_entry* entry;
	void*             packdata = NULL;
	void*             unpacked = NULL;
	size_t            unpack_size;

	iter_t iter;

	console_log(3, "unpacking '%s' from package #%u", path, package->id);

	iter = vector_enum(package->index);
	while ((entry = iter_next(&iter))) {
		if (strcasecmp(path, entry->file_path) == 0)
			break;
	}
	if (entry == NULL)
		goto on_error;
	if (!(packdata = malloc(entry->pack_size)))
		goto on_error;
	al_fseek(package->file, entry->offset, ALLEGRO_SEEK_SET);
	if (al_fread(package->file, packdata, entry->pack_size) < entry->pack_size)
		goto on_error;
	if (!(unpacked = z_inflate(packdata, entry->pack_size, entry->file_size, &unpack_size)))
		goto on_error;
	free(packdata);

	*out_size = unpack_size;
	return unpacked;

on_error:
	console_log(3, "couldn't unpack '%s' from package #%u", path, package->id);
	free(packdata);
	free(unpacked);
	return NULL;
}

long long
asset_ftell(asset_t* file)
{
	return al_ftell(file->handle);
}

size_t
asset_fwrite(const void* buf, size_t size, size_t count, asset_t* file)
{
	return al_fwrite(file->handle, buf, size * count) / size;
}
