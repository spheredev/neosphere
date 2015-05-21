#include "minisphere.h"
#include "api.h"

#include "shader.h"

struct shader
{
	unsigned int    refcount;
	ALLEGRO_SHADER* ptr;
};

shader_t*
create_shader(const char* source, shader_type_t type)
{
	int       attach_type;
	shader_t* shader;

	if (!(shader = calloc(1, sizeof(shader_t)))) goto on_error;
	if (!(shader->ptr = al_create_shader(ALLEGRO_SHADER_GLSL)))
		goto on_error;
	attach_type = type == SHADER_TYPE_PIXEL ? ALLEGRO_PIXEL_SHADER
		: type == SHADER_TYPE_VERTEX ? ALLEGRO_VERTEX_SHADER
		: 0x0;
	if (!al_attach_shader_source(shader->ptr, attach_type, source)) goto on_error;
	if (!al_build_shader(shader->ptr)) goto on_error;
	return ref_shader(shader);

on_error:
	if (shader != NULL) {
		if (shader->ptr != NULL)
			al_destroy_shader(shader->ptr);
		free(shader);
	}
	return NULL;
}

shader_t*
ref_shader(shader_t* shader)
{
	++shader->refcount;
	return shader;
}

void
free_shader(shader_t* shader)
{
	if (shader == NULL || --shader->refcount > 0)
		return;
	al_destroy_shader(shader->ptr);
	free(shader);
}

bool
apply_shader(shader_t* shader)
{
	return al_use_shader(shader->ptr);
}

void
reset_shader(void)
{
	al_use_shader(NULL);
}
