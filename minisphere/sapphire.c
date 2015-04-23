#include "minisphere.h"
#include "api.h"

#include "sapphire.h"

static duk_ret_t js_new_Shape      (duk_context* ctx);
static duk_ret_t js_Shape_finalize (duk_context* ctx);

struct vertex
{
	float   x, y;
	float   u, v;
	color_t color;
};

struct shape
{
	image_t* texture;
	int      max_vertices;
	int      num_vertices;
	vertex_t *vertices;
};

struct group
{
	int     num_shapes;
	shape_t *shapes;
};

shape_t*
new_shape(image_t* texture)
{
	shape_t* shape;
	
	if (!(shape = calloc(1, sizeof(shape_t))))
		goto on_error;
	shape->texture = ref_image(texture);
	return shape;

on_error:
	free(shape);
	return NULL;
}

void
free_shape(shape_t* shape)
{
	free_image(shape->texture);
	free(shape);
}

bool
add_vertex(shape_t* shape, vertex_t vertex)
{
	int       new_max;
	vertex_t  *new_buffer;

	if (shape->num_vertices + 1 > shape->max_vertices) {
		new_max = (shape->num_vertices + 1) * 2;
		if (!(new_buffer = realloc(shape->vertices, new_max * sizeof(vertex_t))))
			return false;
		shape->vertices = new_buffer;
		shape->max_vertices = new_max;
	}
	++shape->num_vertices;
	shape->vertices[shape->num_vertices - 1] = vertex;
	return true;
}

image_t*
get_shape_texture(shape_t* shape)
{
	return shape->texture;
}

void
set_shape_texture(shape_t* shape, image_t* texture)
{
	image_t* old_texture;
	
	old_texture = shape->texture;
	shape->texture = ref_image(texture);
	free_image(old_texture);
}

void
init_sapphire_api(void)
{
	// Shape object
	register_api_ctor(g_duk, "Shape", js_new_Shape, js_Shape_finalize);
}

static duk_ret_t
js_new_Shape(duk_context* ctx)
{
	duk_require_object_coercible(ctx, 0);
	image_t* texture = duk_require_sphere_obj(ctx, 1, "Image");

	shape_t* shape;

	shape = new_shape(texture);
	// TODO: add vertices to empty shape
	duk_push_sphere_obj(ctx, "Shape", shape);
	return 1;
}

static duk_ret_t
js_Shape_finalize(duk_context* ctx)
{
	shape_t* shape;

	shape = duk_require_sphere_obj(ctx, 0, "Shape");
	free_shape(shape);
	return 0;
}
