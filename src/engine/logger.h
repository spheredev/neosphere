typedef struct logger logger_t;

logger_t* log_open        (const char* filename);
logger_t* log_ref         (logger_t* logger);
void      log_close       (logger_t* logger);
bool      log_begin_block (logger_t* logger, const char* title);
void      log_end_block   (logger_t* logger);
void      log_write       (logger_t* logger, const char* prefix, const char* text);

void init_logging_api (void);
