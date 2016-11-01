#ifndef MINISPHERE__LOGGER_H__INCLUDED
#define MINISPHERE__LOGGER_H__INCLUDED

typedef struct logger logger_t;

logger_t* log_open        (const char* filename);
logger_t* log_ref         (logger_t* logger);
void      log_close       (logger_t* logger);
bool      log_begin_block (logger_t* logger, const char* title);
void      log_end_block   (logger_t* logger);
void      log_write       (logger_t* logger, const char* prefix, const char* text);

#endif // MINISPHERE__LOGGER_H__INCLUDED
