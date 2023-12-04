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
#include "kev_file.h"

struct kev_file
{
	unsigned int    id;
	game_t*         game;
	ALLEGRO_CONFIG* conf;
	char*           filename;
	bool            modified;
};

static unsigned int s_next_file_id = 0;

kev_file_t*
kev_open(game_t* game, const char* filename, bool can_create)
{
	kev_file_t*   kev_file;
	ALLEGRO_FILE* memfile = NULL;
	void*         file_data;
	size_t        file_size;

	console_log(2, "opening kevfile #%u '%s'", s_next_file_id, filename);
	if (!(kev_file = calloc(1, sizeof(kev_file_t))))
		return NULL;
	if ((file_data = game_read_file(game, filename, &file_size))) {
		memfile = al_open_memfile(file_data, file_size, "rb");
		if (!(kev_file->conf = al_load_config_file_f(memfile)))
			goto on_error;
		al_fclose(memfile);
		free(file_data);
	}
	else {
		console_log(3, "    '%s' doesn't exist", filename);
		if (!can_create || !(kev_file->conf = al_create_config()))
			goto on_error;
	}
	kev_file->game = game_ref(game);
	kev_file->filename = strdup(filename);
	kev_file->id = s_next_file_id++;
	return kev_file;

on_error:
	console_log(2, "    couldn't open kevfile #%u", s_next_file_id++);
	if (memfile != NULL) al_fclose(memfile);
	if (kev_file->conf != NULL)
		al_destroy_config(kev_file->conf);
	free(kev_file);
	return NULL;
}

void
kev_close(kev_file_t* it)
{
	if (it == NULL)
		return;

	console_log(3, "disposing kevfile #%u no longer in use", it->id);
	if (it->modified)
		kev_save(it);
	al_destroy_config(it->conf);
	game_unref(it->game);
	free(it);
}

int
kev_num_keys(kev_file_t* it)
{
	ALLEGRO_CONFIG_ENTRY* iter;
	const char*           key;
	int                   sum;

	sum = 0;
	key = al_get_first_config_entry(it->conf, NULL, &iter);
	while (key != NULL) {
		++sum;
		key = al_get_next_config_entry(&iter);
	}
	return sum;
}

const char*
kev_get_key(kev_file_t* it, int index)
{
	ALLEGRO_CONFIG_ENTRY* iter;
	const char*           name;

	int i = 0;

	name = al_get_first_config_entry(it->conf, NULL, &iter);
	while (name != NULL) {
		if (i == index)
			return name;
		++i;
		name = al_get_next_config_entry(&iter);
	}
	return NULL;
}

bool
kev_read_bool(kev_file_t* it, const char* key, bool def_value)
{
	const char* string;
	bool        value;

	if (it == NULL)
		return def_value;

	string = kev_read_string(it, key, def_value ? "true" : "false");
	value = strcasecmp(string, "true") == 0;
	return value;
}

double
kev_read_float(kev_file_t* it, const char* key, double def_value)
{
	char        def_string[500];
	const char* string;
	double      value;

	if (it == NULL)
		return def_value;

	sprintf(def_string, "%g", def_value);
	string = kev_read_string(it, key, def_string);
	value = atof(string);
	return value;
}

int
kev_read_int(kev_file_t* it, const char* key, int def_value)
{
	char        def_string[500];
	const char* string;
	double      value;

	if (it == NULL)
		return def_value;

	sprintf(def_string, "%d", def_value);
	string = kev_read_string(it, key, def_string);
	value = atoi(string);
	return value;
}

const char*
kev_read_string(kev_file_t* it, const char* key, const char* def_value)
{
	const char* value;

	if (it == NULL)
		return def_value;

	console_log(2, "reading key '%s' from kevfile #%u", key, it->id);
	if (!(value = al_get_config_value(it->conf, NULL, key)))
		value = def_value;
	return value;
}

bool
kev_save(kev_file_t* it)
{
	void*         buffer = NULL;
	file_t*       file = NULL;
	size_t        file_size;
	bool          is_aok = false;
	ALLEGRO_FILE* memfile = NULL;
	void*         new_buffer;
	size_t        next_buf_size;

	console_log(3, "saving kevfile #%u as '%s'", it->id, it->filename);
	next_buf_size = 4096;
	while (!is_aok) {
		if (!(new_buffer = realloc(buffer, next_buf_size)))
			goto on_error;
		buffer = new_buffer;
		if (!(memfile = al_open_memfile(buffer, next_buf_size, "wb")))
			goto on_error;
		next_buf_size *= 2;
		al_save_config_file_f(memfile, it->conf);
		is_aok = !al_feof(memfile);
		file_size = al_ftell(memfile);
		al_fclose(memfile); memfile = NULL;
	}
	if (!(file = file_open(it->game, it->filename, "wt")))
		goto on_error;
	file_write(file, buffer, 1, file_size);
	file_close(file);
	free(buffer);
	return true;

on_error:
	if (memfile != NULL)
		al_fclose(memfile);
	file_close(file);
	free(buffer);
	return false;
}

void
kev_write_bool(kev_file_t* it, const char* key, bool value)
{
	console_log(3, "writing boolean to kevfile #%u, key '%s'", it->id, key);
	al_set_config_value(it->conf, NULL, key, value ? "true" : "false");
	it->modified = true;
}

void
kev_write_float(kev_file_t* it, const char* key, double value)
{
	char string[500];

	console_log(3, "writing float to kevfile #%u, key '%s'", it->id, key);
	sprintf(string, "%g", value);
	al_set_config_value(it->conf, NULL, key, string);
	it->modified = true;
}

void
kev_write_int(kev_file_t* it, const char* key, int value)
{
	char string[500];

	console_log(3, "writing int to kevfile #%u, key '%s'", it->id, key);
	sprintf(string, "%d", value);
	al_set_config_value(it->conf, NULL, key, string);
	it->modified = true;
}

void
kev_write_string(kev_file_t* it, const char* key, const char* value)
{
	console_log(3, "writing string to kevfile #%u, key '%s'", it->id, key);
	al_set_config_value(it->conf, NULL, key, value);
	it->modified = true;
}
