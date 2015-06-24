#ifndef MINISPHERE__SHADER_H__INCLUDED
#define MINISPHERE__SHADER_H__INCLUDED

typedef struct shader    shader_t;
typedef enum shader_type shader_type_t;

extern bool      initialize_shaders (void);
extern void      shutdown_shaders   (void);
extern shader_t* get_system_shader  (void);
extern shader_t* create_shader      (const char* vs_path, const char* fs_path);
extern shader_t* ref_shader         (shader_t* shader);
extern void      free_shader        (shader_t* shader);
extern bool      apply_shader       (shader_t* shader);
extern void      reset_shader       (void);

extern void init_shader_api (void);

enum shader_type
{
	SHADER_TYPE_PIXEL,
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_MAX
};

#endif // MINISPHERE__SHADER_H__INCLUDED
