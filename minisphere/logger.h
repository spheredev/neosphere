typedef struct logger logger_t;

extern logger_t* open_log_file   (const char* path);
extern logger_t* ref_logger      (logger_t* logger);
extern void      free_logger     (logger_t* logger);
extern bool      begin_log_block (logger_t* logger, const char* title);
extern void      end_log_block   (logger_t* logger);
extern void      write_log_line  (logger_t* logger, const char* prefix, const char* text);

extern void init_logging_api (void);
