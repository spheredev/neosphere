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
#include "logger.h"

#include "lstring.h"

struct block
{
	lstring_t* name;
};

struct logger
{
	unsigned int  refcount;
	unsigned int  id;
	file_t*       file;
	int           num_blocks;
	int           max_blocks;
	struct block* blocks;
};

static unsigned int s_next_logger_id = 0;

logger_t*
logger_new(const char* filename)
{
	lstring_t* log_entry;
	logger_t*  logger = NULL;
	time_t     now;
	char       timestamp[100];

	console_log(2, "creating logger #%u for '%s'", s_next_logger_id, filename);

	if (!(logger = calloc(1, sizeof(logger_t))))
		goto on_error;
	if (!(logger->file = file_open(g_game, filename, "a")))
		goto on_error;
	time(&now);
	strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S", localtime(&now));
	log_entry = lstr_newf("LOG OPENED: %s\n", timestamp);
	file_puts(logger->file, lstr_cstr(log_entry));
	lstr_free(log_entry);

	logger->id = s_next_logger_id++;
	return logger_ref(logger);

on_error: // oh no!
	console_log(2, "failed to open file for logger #%u", s_next_logger_id++);
	free(logger);
	return NULL;
}

logger_t*
logger_ref(logger_t* logger)
{
	++logger->refcount;
	return logger;
}

void
logger_unref(logger_t* logger)
{
	lstring_t* log_entry;
	time_t     now;
	char       timestamp[100];

	if (logger == NULL || --logger->refcount > 0)
		return;

	console_log(3, "disposing logger #%u no longer in use", logger->id);
	time(&now); strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S", localtime(&now));
	log_entry = lstr_newf("LOG CLOSED: %s\n\n", timestamp);
	file_puts(logger->file, lstr_cstr(log_entry));
	lstr_free(log_entry);
	file_close(logger->file);
	free(logger);
}

bool
logger_begin_block(logger_t* logger, const char* title)
{
	lstring_t*    block_name;
	struct block* blocks;
	int           new_count;

	new_count = logger->num_blocks + 1;
	if (new_count > logger->max_blocks) {
		if (!(blocks = realloc(logger->blocks, new_count * 2))) return false;
		logger->blocks = blocks;
		logger->max_blocks = new_count * 2;
	}
	if (!(block_name = lstr_newf("%s", title))) return false;
	logger_write(logger, "BEGIN", lstr_cstr(block_name));
	logger->blocks[logger->num_blocks].name = block_name;
	++logger->num_blocks;
	return true;
}

void
logger_end_block(logger_t* logger)
{
	lstring_t* block_name;

	--logger->num_blocks;
	block_name = logger->blocks[logger->num_blocks].name;
	logger_write(logger, "END", lstr_cstr(block_name));
	lstr_free(block_name);
}

void
logger_write(logger_t* logger, const char* prefix, const char* text)
{
	time_t now;
	char   timestamp[100];

	int i;

	time(&now);
	strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S -- ", localtime(&now));
	file_puts(logger->file, timestamp);
	for (i = 0; i < logger->num_blocks; ++i)
		file_puts(logger->file, "\t");
	if (prefix != NULL) {
		file_puts(logger->file, prefix);
		file_puts(logger->file, " ");
	}
	file_puts(logger->file, text);
	file_puts(logger->file, "\n");
}
