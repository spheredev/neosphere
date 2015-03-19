#include "minisphere.h"
#include "api.h"
#include "lstring.h"

#include "logger.h"

struct logger
{
	int               refcount;
	ALLEGRO_FILE*     file;
	int               num_blocks;
	int               max_blocks;
	struct log_block* blocks;
};

struct log_block
{
	lstring_t* name;
};

static duk_ret_t js_OpenLog           (duk_context* ctx);
static duk_ret_t js_Logger_finalize   (duk_context* ctx);
static duk_ret_t js_Logger_toString   (duk_context* ctx);
static duk_ret_t js_Logger_beginBlock (duk_context* ctx);
static duk_ret_t js_Logger_endBlock   (duk_context* ctx);
static duk_ret_t js_Logger_write      (duk_context* ctx);

static void duk_push_sphere_logger (duk_context* ctx, logger_t* log);

logger_t*
open_log_file(const char* path)
{
	lstring_t* log_entry;
	logger_t*  logger = NULL;
	time_t     now;
	char       timestamp[100];

	if (!(logger = calloc(1, sizeof(logger_t)))) goto on_error;
	if (!(logger->file = al_fopen(path, "a"))) goto on_error;
	time(&now);
	strftime(timestamp, 100, "%a %Y %b %d %H:%M:%S", localtime(&now));
	log_entry = lstring_format("LOG OPENED: %s\n", timestamp);
	al_fputs(logger->file, log_entry->cstr);
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
	log_entry = lstring_format("LOG CLOSED: %s\n\n", timestamp);
	al_fputs(logger->file, log_entry->cstr);
	free_lstring(log_entry);
	al_fclose(logger->file);
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
	write_log_line(logger, "BEGIN", block_name->cstr);
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
	write_log_line(logger, "END", block_name->cstr);
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
	al_fputs(logger->file, timestamp);
	for (i = 0; i < logger->num_blocks; ++i) al_fputc(logger->file, '\t');
	if (prefix != NULL) {
		al_fputs(logger->file, prefix);
		al_fputc(logger->file, ' ');
	}
	al_fputs(logger->file, text);
	al_fputc(logger->file, '\n');
}

void
init_logging_api(void)
{
	register_api_func(g_duktape, NULL, "OpenLog", js_OpenLog);
}

static void
duk_push_sphere_logger(duk_context* ctx, logger_t* logger)
{
	ref_logger(logger);
	
	duk_push_object(ctx);
	duk_push_pointer(ctx, logger); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_c_function(ctx, js_Logger_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, js_Logger_toString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "toString");
	duk_push_c_function(ctx, js_Logger_beginBlock, DUK_VARARGS); duk_put_prop_string(ctx, -2, "beginBlock");
	duk_push_c_function(ctx, js_Logger_endBlock, DUK_VARARGS); duk_put_prop_string(ctx, -2, "endBlock");
	duk_push_c_function(ctx, js_Logger_write, DUK_VARARGS); duk_put_prop_string(ctx, -2, "write");
}

static duk_ret_t
js_OpenLog(duk_context* ctx)
{
	const char* filename = duk_get_string(ctx, 0);
	
	char* path = get_asset_path(filename, "logs", true);
	logger_t* logger = open_log_file(path);
	free(path);
	if (logger == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "OpenLog(): Failed to open file for logging '%s'", filename);
	duk_push_sphere_logger(ctx, logger);
	free_logger(logger);
	return 1;
}

static duk_ret_t
js_Logger_finalize(duk_context* ctx)
{
	logger_t* logger;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); logger = duk_get_pointer(ctx, -1); duk_pop(ctx);
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
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); logger = duk_get_pointer(ctx, -1); duk_pop(ctx);
	if (!begin_log_block(logger, title))
		duk_error(ctx, DUK_ERR_ERROR, "Log:beginBlock(): Failed to create new log block (internal error)");
	return 0;
}

static duk_ret_t
js_Logger_endBlock(duk_context* ctx)
{
	logger_t* logger;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); logger = duk_get_pointer(ctx, -1); duk_pop(ctx);
	end_log_block(logger);
	return 0;
}

static duk_ret_t
js_Logger_write(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	
	logger_t* logger;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); logger = duk_get_pointer(ctx, -1); duk_pop(ctx);
	write_log_line(logger, NULL, text);
	return 0;
}
