#ifndef MINISPHERE__SHADER_H__INCLUDED
#define MINISPHERE__SHADER_H__INCLUDED

typedef struct shader    shader_t;
typedef enum shader_type shader_type_t;

extern shader_t* create_shader (const char* path, shader_type_t type);
extern shader_t* ref_shader    (shader_t* shader);
extern void      free_shader   (shader_t* shader);
extern bool      apply_shader  (shader_t* shader);
extern void      reset_shaders (void);

extern void init_shader_api (void);

enum shader_type
{
	SHADER_TYPE_PIXEL,
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_MAX
};

#endif // MINISPHERE__SHADER_H__INCLUDED
