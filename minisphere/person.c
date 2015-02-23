#include "minisphere.h"
#include "api.h"
#include "spriteset.h"

#include "person.h"

static duk_ret_t _js_CreatePerson   (duk_context* ctx);
static duk_ret_t _js_DestroyPerson  (duk_context* ctx);
static duk_ret_t _js_GetPersonLayer (duk_context* ctx);
static duk_ret_t _js_GetPersonList  (duk_context* ctx);
static duk_ret_t _js_GetPersonX     (duk_context* ctx);
static duk_ret_t _js_GetPersonY     (duk_context* ctx);
static duk_ret_t _js_SetPersonX     (duk_context* ctx);
static duk_ret_t _js_SetPersonY     (duk_context* ctx);

static void      _add_person           (const char* name, const char* sprite_file, bool persist);
static void      _delete_person        (const char* name);
static void      _set_person_direction (person_t* person, const char* direction);
static void      _set_person_name      (person_t* person, const char* name);

static int       s_num_persons = 0;
static person_t* s_persons     = NULL;

void
init_person_api(void)
{
	register_api_func(g_duktape, NULL, "CreatePerson", &_js_CreatePerson);
	register_api_func(g_duktape, NULL, "DestroyPerson", &_js_DestroyPerson);
	register_api_func(g_duktape, NULL, "GetPersonLayer", _js_GetPersonLayer);
	register_api_func(g_duktape, NULL, "GetPersonList", _js_GetPersonList);
	register_api_func(g_duktape, NULL, "GetPersonX", _js_GetPersonX);
	register_api_func(g_duktape, NULL, "GetPersonY", _js_GetPersonY);
	register_api_func(g_duktape, NULL, "SetPersonX", _js_SetPersonX);
	register_api_func(g_duktape, NULL, "SetPersonY", _js_SetPersonY);

	register_api_const(g_duktape, "SCRIPT_ON_CREATE", 0);
	register_api_const(g_duktape, "SCRIPT_ON_DESTROY", 1);
	register_api_const(g_duktape, "SCRIPT_ON_ACTIVATE_TOUCH", 2);
	register_api_const(g_duktape, "SCRIPT_ON_ACTIVATE_TALK", 3);
	register_api_const(g_duktape, "SCRIPT_COMMAND_GENERATOR", 4);
}

person_t*
find_person(const char* name)
{
	int i;

	for (i = 0; i < s_num_persons; ++i) {
		if (strcmp(name, s_persons[i].name) == 0)
			return &s_persons[i];
	}
	return NULL;
}

void
render_persons(int cam_x, int cam_y)
{
	spriteset_t* sprite;
	float        x, y;
	int          i;

	for (i = 0; i < s_num_persons; ++i) {
		sprite = s_persons[i].sprite;
		x = s_persons[i].x - cam_x; y = s_persons[i].y - cam_y;
		draw_sprite(sprite, s_persons[i].direction, x, y, 0);
	}
}

static duk_ret_t
_js_CreatePerson(duk_context* ctx)
{
	const char* name;
	const char* sprite_file;
	bool        persist;

	name = duk_to_string(ctx, 0);
	sprite_file = duk_require_string(ctx, 1);
	persist = duk_require_boolean(ctx, 2);
	_add_person(name, sprite_file, persist);
	return 0;
}

static duk_ret_t
_js_DestroyPerson(duk_context* ctx)
{
	const char* name;

	name = duk_to_string(ctx, 0);
	if (find_person(name) != NULL) {
		_delete_person(name);
		return 0;
	}
	else {
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "DestroyPerson(): Person entity '%s' doesn't exist!", name);
	}
}

static duk_ret_t
_js_GetPersonDirection(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	person = find_person(name);
	if (person != NULL) {
		duk_push_string(ctx, person->direction);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonDirection(): Person entity '%s' does not exist", name);
	}
}

static duk_ret_t
_js_GetPersonLayer(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	person = find_person(name);
	if (person != NULL) {
		duk_push_int(ctx, person->layer);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonLayer(): Person entity '%s' does not exist", name);
	}
}

static duk_ret_t
_js_GetPersonList(duk_context* ctx)
{
	int i;
	
	duk_push_array(ctx);
	for (i = 0; i < s_num_persons; ++i) {
		duk_push_string(ctx, s_persons[i].name);
		duk_put_prop_index(ctx, -2, i);
	}
	return 1;
}

static duk_ret_t
_js_GetPersonX(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	person = find_person(name);
	if (person != NULL) {
		duk_push_number(ctx, person->x);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonX(): Person entity '%s' does not exist", name);
	}
}

static duk_ret_t
_js_GetPersonY(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	person = find_person(name);
	if (person != NULL) {
		duk_push_number(ctx, person->y);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonY(): Person entity '%s' does not exist", name);
	}
}

static duk_ret_t
_js_SetPersonX(duk_context* ctx)
{
	person_t* person;

	person = find_person(duk_to_string(ctx, 0));
	if (person != NULL) person->x = duk_to_number(ctx, 1);
	return 0;
}

static duk_ret_t
_js_SetPersonY(duk_context* ctx)
{
	person_t* person;

	person = find_person(duk_to_string(ctx, 0));
	if (person != NULL) person->x = duk_to_number(ctx, 1);
	return 0;
}

static void
_add_person(const char* name, const char* sprite_file, bool persist)
{
	char*     path;
	person_t* person;
	
	++s_num_persons;
	s_persons = realloc(s_persons, s_num_persons * sizeof(person_t));
	person = &s_persons[s_num_persons - 1];
	memset(person, 0, sizeof(person_t));
	_set_person_name(person, name);
	_set_person_direction(person, "north");
	path = get_asset_path(sprite_file, "spritesets", false);
	person->sprite = load_spriteset(path);
	free(path);
	person->x = 0; person->y = 0; person->layer = 0;
	person->speed = 1.0;
}

static void
_delete_person(const char* name)
{
	int i, j;

	for (i = 0; i < s_num_persons; ++i) {
		if (strcmp(s_persons[i].name, name) == 0) {
			free_spriteset(s_persons[i].sprite);
			free(s_persons[i].name);
			free(s_persons[i].direction);
			for (j = i; j < s_num_persons - 1; ++j) s_persons[j] = s_persons[j + 1];
			--s_num_persons; --i;
		}
			
	}
	s_persons = realloc(s_persons, s_num_persons * sizeof(person_t));
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
