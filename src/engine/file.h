#ifndef MINISPHERE__FILE_H__INCLUDED
#define MINISPHERE__FILE_H__INCLUDED

typedef struct kevfile kevfile_t;

kevfile_t*  kev_open         (sandbox_t* fs, const char* filename, bool can_create);
void        kev_close        (kevfile_t* file);
int         kev_num_keys     (kevfile_t* file);
const char* kev_get_key      (kevfile_t* file, int index);
bool        kev_read_bool    (kevfile_t* file, const char* key, bool def_value);
double      kev_read_float   (kevfile_t* file, const char* key, double def_value);
const char* kev_read_string  (kevfile_t* file, const char* key, const char* def_value);
bool        kev_save         (kevfile_t* file);
void        kev_write_bool   (kevfile_t* file, const char* key, bool value);
void        kev_write_float  (kevfile_t* file, const char* key, double value);
void        kev_write_string (kevfile_t* file, const char* key, const char* value);

void init_file_api (void);

#endif // MINISPHERE__FILE_H__INCLUDED
