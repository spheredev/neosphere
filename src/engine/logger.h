typedef struct logger logger_t;

logger_t* open_log_file   (const char* filename);
logger_t* ref_logger      (logger_t* logger);
void      free_logger     (logger_t* logger);
bool      begin_log_block (logger_t* logger, const char* title);
void      end_log_block   (logger_t* logger);
void      write_log_line  (logger_t* logger, const char* prefix, const char* text);

void init_logging_api (void);
