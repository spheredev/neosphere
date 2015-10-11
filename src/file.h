#ifndef MINISPHERE__FILE_H__INCLUDED
#define MINISPHERE__FILE_H__INCLUDED

typedef struct kev_file kev_file_t;

extern kev_file_t* open_kev_file    (const char* filename, bool is_sfs_compliant);
extern void        close_kev_file   (kev_file_t* file);
extern int         get_record_count (kev_file_t* file);
extern const char* get_record_name  (kev_file_t* file, int index);
extern bool        read_bool_rec    (kev_file_t* file, const char* key, bool def_value);
extern double      read_number_rec  (kev_file_t* file, const char* key, double def_value);
extern const char* read_string_rec  (kev_file_t* file, const char* key, const char* def_value);
extern bool        save_kev_file    (kev_file_t* file);
extern void        write_bool_rec   (kev_file_t* file, const char* key, bool value);
extern void        write_number_rec (kev_file_t* file, const char* key, double value);
extern void        write_string_rec (kev_file_t* file, const char* key, const char* value);

extern void init_file_api (void);

#endif // MINISPHERE__FILE_H__INCLUDED
