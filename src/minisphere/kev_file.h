#ifndef MINISPHERE__FILE_H__INCLUDED
#define MINISPHERE__FILE_H__INCLUDED

#include "spherefs.h"

typedef struct kev_file kev_file_t;

kev_file_t*  kev_open         (sandbox_t* fs, const char* filename, bool can_create);
void         kev_close        (kev_file_t* file);
int          kev_num_keys     (kev_file_t* file);
const char*  kev_get_key      (kev_file_t* file, int index);
bool         kev_read_bool    (kev_file_t* file, const char* key, bool def_value);
double       kev_read_float   (kev_file_t* file, const char* key, double def_value);
const char*  kev_read_string  (kev_file_t* file, const char* key, const char* def_value);
bool         kev_save         (kev_file_t* file);
void         kev_write_bool   (kev_file_t* file, const char* key, bool value);
void         kev_write_float  (kev_file_t* file, const char* key, double value);
void         kev_write_string (kev_file_t* file, const char* key, const char* value);

#endif // MINISPHERE__FILE_H__INCLUDED
