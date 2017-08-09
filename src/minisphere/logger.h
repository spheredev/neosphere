#ifndef MINISPHERE__LOGGER_H__INCLUDED
#define MINISPHERE__LOGGER_H__INCLUDED

typedef struct logger logger_t;

logger_t* logger_new         (const char* filename);
logger_t* logger_ref         (logger_t* logger);
void      logger_unref       (logger_t* logger);
bool      logger_begin_block (logger_t* logger, const char* title);
void      logger_end_block   (logger_t* logger);
void      logger_write       (logger_t* logger, const char* prefix, const char* text);

#endif // MINISPHERE__LOGGER_H__INCLUDED
