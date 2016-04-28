#ifndef MINISPHERE__FILE_H__INCLUDED
#define MINISPHERE__FILE_H__INCLUDED

typedef struct kevfile kevfile_t;

extern kevfile_t*  kev_open         (sandbox_t* fs, const char* filename);
extern void        kev_close        (kevfile_t* file);
extern int         kev_num_keys     (kevfile_t* file);
extern const char* kev_get_key      (kevfile_t* file, int index);
extern bool        kev_read_bool    (kevfile_t* file, const char* key, bool def_value);
extern double      kev_read_float   (kevfile_t* file, const char* key, double def_value);
extern const char* kev_read_string  (kevfile_t* file, const char* key, const char* def_value);
extern bool        kev_save         (kevfile_t* file);
extern void        kev_write_bool   (kevfile_t* file, const char* key, bool value);
extern void        kev_write_float  (kevfile_t* file, const char* key, double value);
extern void        kev_write_string (kevfile_t* file, const char* key, const char* value);

extern void init_file_api (void);

#endif // MINISPHERE__FILE_H__INCLUDED
