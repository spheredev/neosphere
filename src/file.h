typedef struct kv_file kv_file_t;

extern kv_file_t*  open_file        (const char* filename);
extern void        close_file       (kv_file_t* file);
extern int         get_record_count (kv_file_t* file);
extern const char* get_record_name  (kv_file_t* file, int index);
extern bool        read_bool_rec    (kv_file_t* file, const char* key, bool def_value);
extern double      read_number_rec  (kv_file_t* file, const char* key, double def_value);
extern const char* read_string_rec  (kv_file_t* file, const char* key, const char* def_value);
extern bool        save_file        (kv_file_t* file);
extern void        write_bool_rec   (kv_file_t* file, const char* key, bool value);
extern void        write_number_rec (kv_file_t* file, const char* key, double value);
extern void        write_string_rec (kv_file_t* file, const char* key, const char* value);

extern void init_file_api (void);
