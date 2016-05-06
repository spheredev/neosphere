#include "minisphere.h"
#include "api.h"
#include "lstring.h"

#include "logger.h"

struct logger
{
	unsigned int      refcount;
	unsigned int      id;
	sfs_file_t*       file;
	int               num_blocks;
	int               max_blocks;
	struct log_block* blocks;
};

struct log_block
{
	lstring_t* name;
};

static duk_ret_t js_OpenLog           (duk_context* ctx);
static duk_ret_t js_new_Logger        (duk_context* ctx);
static duk_ret_t js_Logger_finalize   (duk_context* ctx);
static duk_ret_t js_Logger_toString   (duk_context* ctx);
static duk_ret_t js_Logger_beginBlock (duk_context* ctx);
static duk_ret_t js_Logger_endBlock   (duk_context* ctx);
static duk_ret_t js_Logger_write      (duk_context* ctx);

static unsigned int s_next_logger_id = 0;

logger_t*
log_open(const char* filename)
{
	lstring_t* log_entry;
	logger_t*  logger = NULL;
	time_t     now;
	char       timestamp[100];

	console_log(2, "creating logger #%u for `%s`", s_next_logger_id, filename);
	
	logger = calloc(1, sizeof(logger_t));
	if (!(logger->file = sfs_fopen(g_fs, filename, NULL, "a")))
		goto on_error;
	time(&now);
	strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S", localtime(&now));
	log_entry = lstr_newf("LOG OPENED: %s\n", timestamp);
	sfs_fputs(lstr_cstr(log_entry), logger->file);
	lstr_free(log_entry);
	
	logger->id = s_next_logger_id++;
	return log_ref(logger);

on_error: // oh no!
	console_log(2, "failed to open file for logger #%u", s_next_logger_id++);
	free(logger);
	return NULL;
}

logger_t*
log_ref(logger_t* logger)
{
	++logger->refcount;
	return logger;
}

void
log_close(logger_t* logger)
{
	lstring_t* log_entry;
	time_t     now;
	char       timestamp[100];

	if (logger == NULL || --logger->refcount > 0)
		return;

	console_log(3, "disposing logger #%u no longer in use", logger->id);
	time(&now); strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S", localtime(&now));
	log_entry = lstr_newf("LOG CLOSED: %s\n\n", timestamp);
	sfs_fputs(lstr_cstr(log_entry), logger->file);
	lstr_free(log_entry);
	sfs_fclose(logger->file);
	free(logger);
}

bool
log_begin_block(logger_t* logger, const char* title)
{
	lstring_t*        block_name;
	struct log_block* blocks;
	int               new_count;
	
	new_count = logger->num_blocks + 1;
	if (new_count > logger->max_blocks) {
		if (!(blocks = realloc(logger->blocks, new_count * 2))) return false;
		logger->blocks = blocks;
		logger->max_blocks = new_count * 2;
	}
	if (!(block_name = lstr_newf("%s", title))) return false;
	log_write(logger, "BEGIN", lstr_cstr(block_name));
	logger->blocks[logger->num_blocks].name = block_name;
	++logger->num_blocks;
	return true;
}

void
log_end_block(logger_t* logger)
{
	lstring_t* block_name;
	
	--logger->num_blocks;
	block_name = logger->blocks[logger->num_blocks].name;
	log_write(logger, "END", lstr_cstr(block_name));
	lstr_free(block_name);
}

void
log_write(logger_t* logger, const char* prefix, const char* text)
{
	time_t now;
	char   timestamp[100];
	
	int i;
	
	time(&now);
	strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S -- ", localtime(&now));
	sfs_fputs(timestamp, logger->file);
	for (i = 0; i < logger->num_blocks; ++i)
		sfs_fputc('\t', logger->file);
	if (prefix != NULL) {
		sfs_fputs(prefix, logger->file);
		sfs_fputc(' ', logger->file);
	}
	sfs_fputs(text, logger->file);
	sfs_fputc('\n', logger->file);
}

void
init_logging_api(void)
{
	// Logger object
	api_register_method(g_duk, NULL, "OpenLog", js_OpenLog);
	api_register_ctor(g_duk, "Logger", js_new_Logger, js_Logger_finalize);
	api_register_method(g_duk, "Logger", "toString", js_Logger_toString);
	api_register_method(g_duk, "Logger", "beginBlock", js_Logger_beginBlock);
	api_register_method(g_duk, "Logger", "endBlock", js_Logger_endBlock);
	api_register_method(g_duk, "Logger", "write", js_Logger_write);
}

static duk_ret_t
js_OpenLog(duk_context* ctx)
{
	const char* filename;
	logger_t*   logger;

	filename = duk_require_path(ctx, 0, "logs", true);
	if (!(logger = log_open(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenLog(): unable to open file for logging `%s`", filename);
	duk_push_sphere_obj(ctx, "Logger", logger);
	return 1;
}

static duk_ret_t
js_new_Logger(duk_context* ctx)
{
	const char* filename;
	logger_t*   logger;

	filename = duk_require_path(ctx, 0, NULL, false);
	if (!(logger = log_open(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Logger(): unable to open file for logging `%s`", filename);
	duk_push_sphere_obj(ctx, "Logger", logger);
	return 1;
}

static duk_ret_t
js_Logger_finalize(duk_context* ctx)
{
	logger_t* logger;

	logger = duk_require_sphere_obj(ctx, 0, "Logger");
	log_close(logger);
	return 0;
}

static duk_ret_t
js_Logger_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object log]");
	return 1;
}

static duk_ret_t
js_Logger_beginBlock(duk_context* ctx)
{
	const char* title = duk_to_string(ctx, 0);
	
	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	if (!log_begin_block(logger, title))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Log:beginBlock(): unable to create new log block");
	return 0;
}

static duk_ret_t
js_Logger_endBlock(duk_context* ctx)
{
	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	log_end_block(logger);
	return 0;
}

static duk_ret_t
js_Logger_write(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	
	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	log_write(logger, NULL, text);
	return 0;
}
