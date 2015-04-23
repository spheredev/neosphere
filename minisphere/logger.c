#include "minisphere.h"
#include "api.h"
#include "lstring.h"

#include "logger.h"

struct logger
{
	int               refcount;
	FILE*             file;
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

logger_t*
open_log_file(const char* path)
{
	lstring_t* log_entry;
	logger_t*  logger = NULL;
	time_t     now;
	char       timestamp[100];

	if (path == NULL) return NULL;
	if (!(logger = calloc(1, sizeof(logger_t)))) goto on_error;
	if (!(logger->file = fopen(path, "a"))) goto on_error;
	time(&now);
	strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S", localtime(&now));
	log_entry = new_lstring("LOG OPENED: %s\n", timestamp);
	fputs(lstring_cstr(log_entry), logger->file);
	free_lstring(log_entry);
	return ref_logger(logger);

on_error: // oh no!
	free(logger);
	return NULL;
}

logger_t*
ref_logger(logger_t* logger)
{
	++logger->refcount;
	return logger;
}

void
free_logger(logger_t* logger)
{
	lstring_t* log_entry;
	time_t     now;
	char       timestamp[100];

	if (logger == NULL || --logger->refcount > 0)
		return;
	time(&now); strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S", localtime(&now));
	log_entry = new_lstring("LOG CLOSED: %s\n\n", timestamp);
	fputs(lstring_cstr(log_entry), logger->file);
	free_lstring(log_entry);
	fclose(logger->file);
	free(logger);
}

bool
begin_log_block(logger_t* logger, const char* title)
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
	if (!(block_name = lstring_from_cstr(title))) return false;
	write_log_line(logger, "BEGIN", lstring_cstr(block_name));
	logger->blocks[logger->num_blocks].name = block_name;
	++logger->num_blocks;
	return true;
}

void
end_log_block(logger_t* logger)
{
	lstring_t* block_name;
	
	--logger->num_blocks;
	block_name = logger->blocks[logger->num_blocks].name;
	write_log_line(logger, "END", lstring_cstr(block_name));
	free_lstring(block_name);
}

void
write_log_line(logger_t* logger, const char* prefix, const char* text)
{
	time_t now;
	char   timestamp[100];
	
	int i;
	
	time(&now);
	strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S -- ", localtime(&now));
	fputs(timestamp, logger->file);
	for (i = 0; i < logger->num_blocks; ++i) fputc('\t', logger->file);
	if (prefix != NULL) {
		fputs(prefix, logger->file);
		fputc(' ', logger->file);
	}
	fputs(text, logger->file);
	fputc('\n', logger->file);
}

void
init_logging_api(void)
{
	// Logger object
	register_api_func(g_duk, NULL, "OpenLog", js_OpenLog);
	register_api_ctor(g_duk, "Logger", js_new_Logger, js_Logger_finalize);
	register_api_func(g_duk, "Logger", "toString", js_Logger_toString);
	register_api_func(g_duk, "Logger", "beginBlock", js_Logger_toString);
	register_api_func(g_duk, "Logger", "endBlock", js_Logger_toString);
	register_api_func(g_duk, "Logger", "write", js_Logger_toString);
}

static duk_ret_t
js_OpenLog(duk_context* ctx)
{
	duk_require_string(ctx, 0);
	
	js_new_Logger(ctx);
	return 1;
}

static duk_ret_t
js_new_Logger(duk_context* ctx)
{
	const char* filename = duk_get_string(ctx, 0);

	char* path = get_asset_path(filename, "logs", true);
	logger_t* logger = open_log_file(path);
	free(path);
	if (logger == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "OpenLog(): Failed to open file for logging '%s'", filename);
	duk_push_sphere_obj(ctx, "Logger", logger);
	return 1;
}

static duk_ret_t
js_Logger_finalize(duk_context* ctx)
{
	logger_t* logger;

	logger = duk_require_sphere_obj(ctx, 0, "Logger");
	free_logger(logger);
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
	if (!begin_log_block(logger, title))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Log:beginBlock(): Failed to create new log block");
	return 0;
}

static duk_ret_t
js_Logger_endBlock(duk_context* ctx)
{
	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	end_log_block(logger);
	return 0;
}

static duk_ret_t
js_Logger_write(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	
	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	write_log_line(logger, NULL, text);
	return 0;
}
