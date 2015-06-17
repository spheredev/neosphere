typedef struct file file_t;

extern file_t*     open_file        (const char* filename);
extern void        close_file       (file_t* file);
extern int         get_record_count (file_t* file);
extern const char* get_record_name  (file_t* file, int index);
extern bool        read_bool_rec    (file_t* file, const char* key, bool def_value);
extern double      read_number_rec  (file_t* file, const char* key, double def_value);
extern const char* read_string_rec  (file_t* file, const char* key, const char* def_value);
extern bool        save_file        (file_t* file);
extern void        write_bool_rec   (file_t* file, const char* key, bool value);
extern void        write_number_rec (file_t* file, const char* key, double value);
extern void        write_string_rec (file_t* file, const char* key, const char* value);

extern void init_file_api (void);
