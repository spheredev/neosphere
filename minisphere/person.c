#include "minisphere.h"
#include "api.h"

#include "person.h"

typedef struct person person_t;

static duk_ret_t _js_CreatePerson  (duk_context* ctx);
static duk_ret_t _js_DestroyPerson (duk_context* ctx);
static duk_ret_t _js_SetPersonX    (duk_context* ctx);
static duk_ret_t _js_SetPersonY    (duk_context* ctx);

static person_t* _get_person           (const char* name);
static void      _set_person_direction (person_t* person, const char* direction);
static void      _set_person_name      (person_t* person, const char* name);

struct person
{
	char* name;
	char* direction;
	int   layer;
	float speed;
	float x, y;
};

void
init_person_api(void)
{
	register_api_func(g_duktape, NULL, "CreatePerson", &_js_CreatePerson);
	register_api_func(g_duktape, NULL, "DestroyPerson", &_js_DestroyPerson);
	register_api_func(g_duktape, NULL, "SetPersonX", _js_SetPersonX);
	register_api_func(g_duktape, NULL, "SetPersonY", _js_SetPersonY);

	register_api_const(g_duktape, "SCRIPT_ON_CREATE", 0);
	register_api_const(g_duktape, "SCRIPT_ON_DESTROY", 1);
	register_api_const(g_duktape, "SCRIPT_ON_ACTIVATE_TOUCH", 2);
	register_api_const(g_duktape, "SCRIPT_ON_ACTIVATE_TALK", 3);
	register_api_const(g_duktape, "SCRIPT_COMMAND_GENERATOR", 4);
}

static duk_ret_t
_js_CreatePerson(duk_context* ctx)
{
	const char* name;
	const char* spriteset_file;
	bool        destroy_with_map;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	spriteset_file = duk_require_string(ctx, 1);
	destroy_with_map = duk_require_boolean(ctx, 2);
	person = calloc(1, sizeof(person_t));
	_set_person_name(person, name);
	_set_person_direction(person, "north");
	person->x = 0; person->y = 0; person->layer = 0;
	person->speed = 1.0;
	return 0;
}

static duk_ret_t
_js_DestroyPerson(duk_context* ctx)
{
	person_t* person;

	person = _get_person(duk_to_string(ctx, 0));
	if (person != NULL) {
		free(person->name);
		free(person->direction);
		free(person);
	}
	return 0;
}

static duk_ret_t
_js_SetPersonX(duk_context* ctx)
{
	person_t* person;

	person = _get_person(duk_to_string(ctx, 0));
	if (person != NULL) person->x = duk_to_number(ctx, 1);
	return 0;
}

static duk_ret_t
_js_SetPersonY(duk_context* ctx)
{
	person_t* person;

	person = _get_person(duk_to_string(ctx, 0));
	if (person != NULL) person->x = duk_to_number(ctx, 1);
	return 0;
}

static person_t*
_get_person(const char* name)
{
	return NULL;
}

static void
_set_person_direction(person_t* person, const char* direction)
{
	person->direction = realloc(person->direction, (strlen(direction) + 1) * sizeof(char));
	strcpy(person->direction, direction);
}

static void
_set_person_name(person_t* person, const char* name)
{
	person->name = realloc(person->name, (strlen(name) + 1) * sizeof(char));
	strcpy(person->name, name);
}
