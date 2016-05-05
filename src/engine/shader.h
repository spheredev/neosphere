#ifndef MINISPHERE__SHADER_H__INCLUDED
#define MINISPHERE__SHADER_H__INCLUDED

typedef struct shader shader_t;

typedef
enum shader_type
{
	SHADER_TYPE_PIXEL,
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_MAX
} shader_type_t;

void      initialize_shaders (bool enable_shading);
void      shutdown_shaders   (void);
bool      are_shaders_active (void);
shader_t* shader_new         (const char* vs_path, const char* fs_path);
shader_t* shader_ref         (shader_t* shader);
void      shader_free        (shader_t* shader);
bool      shader_use       (shader_t* shader);
void      reset_shader       (void);

void init_shader_api (void);

#endif // MINISPHERE__SHADER_H__INCLUDED
