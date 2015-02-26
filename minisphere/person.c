#include "minisphere.h"
#include "api.h"
#include "map_engine.h"
#include "spriteset.h"

#include "person.h"

struct person
{
	char*        name;
	int          anim_frames;
	char*        direction;
	int          frame;
	bool         is_persistent;
	int          layer;
	int          revert_delay;
	int          revert_frames;
	float        speed;
	spriteset_t* sprite;
	float        x, y;
	int          num_commands;
	int          *commands;
};

static duk_ret_t js_CreatePerson       (duk_context* ctx);
static duk_ret_t js_DestroyPerson      (duk_context* ctx);
static duk_ret_t js_GetPersonLayer     (duk_context* ctx);
static duk_ret_t js_GetPersonList      (duk_context* ctx);
static duk_ret_t js_GetPersonX         (duk_context* ctx);
static duk_ret_t js_GetPersonY         (duk_context* ctx);
static duk_ret_t js_SetPersonX         (duk_context* ctx);
static duk_ret_t js_SetPersonY         (duk_context* ctx);
static duk_ret_t js_QueuePersonCommand (duk_context* ctx);

static void _add_person           (const char* name, const char* sprite_file, bool is_persistent);
static void _delete_person        (const char* name);
static void _free_person          (person_t* person);
static void _set_person_direction (person_t* person, const char* direction);
static void _set_person_name      (person_t* person, const char* name);

static int       s_num_persons = 0;
static person_t* *s_persons    = NULL;

void
init_person_api(void)
{
	register_api_func(g_duktape, NULL, "CreatePerson", js_CreatePerson);
	register_api_func(g_duktape, NULL, "DestroyPerson", js_DestroyPerson);
	register_api_func(g_duktape, NULL, "GetPersonLayer", js_GetPersonLayer);
	register_api_func(g_duktape, NULL, "GetPersonList", js_GetPersonList);
	register_api_func(g_duktape, NULL, "GetPersonX", js_GetPersonX);
	register_api_func(g_duktape, NULL, "GetPersonY", js_GetPersonY);
	register_api_func(g_duktape, NULL, "SetPersonX", js_SetPersonX);
	register_api_func(g_duktape, NULL, "SetPersonY", js_SetPersonY);
	register_api_func(g_duktape, NULL, "QueuePersonCommand", js_QueuePersonCommand);

	// movement script specifier constants
	register_api_const(g_duktape, "SCRIPT_ON_CREATE", 0);
	register_api_const(g_duktape, "SCRIPT_ON_DESTROY", 1);
	register_api_const(g_duktape, "SCRIPT_ON_ACTIVATE_TOUCH", 2);
	register_api_const(g_duktape, "SCRIPT_ON_ACTIVATE_TALK", 3);
	register_api_const(g_duktape, "SCRIPT_COMMAND_GENERATOR", 4);

	// person movement commands
	register_api_const(g_duktape, "COMMAND_WAIT", COMMAND_WAIT);
	register_api_const(g_duktape, "COMMAND_ANIMATE", COMMAND_ANIMATE);
	register_api_const(g_duktape, "COMMAND_FACE_NORTH", COMMAND_FACE_NORTH);
	register_api_const(g_duktape, "COMMAND_FACE_NORTHEAST", COMMAND_FACE_NORTHEAST);
	register_api_const(g_duktape, "COMMAND_FACE_EAST", COMMAND_FACE_EAST);
	register_api_const(g_duktape, "COMMAND_FACE_SOUTHEAST", COMMAND_FACE_SOUTHEAST);
	register_api_const(g_duktape, "COMMAND_FACE_SOUTH", COMMAND_FACE_SOUTH);
	register_api_const(g_duktape, "COMMAND_FACE_SOUTHWEST", COMMAND_FACE_SOUTHWEST);
	register_api_const(g_duktape, "COMMAND_FACE_WEST", COMMAND_FACE_WEST);
	register_api_const(g_duktape, "COMMAND_FACE_NORTHWEST", COMMAND_FACE_NORTHWEST);
	register_api_const(g_duktape, "COMMAND_MOVE_NORTH", COMMAND_MOVE_NORTH);
	register_api_const(g_duktape, "COMMAND_MOVE_NORTHEAST", COMMAND_MOVE_NORTHEAST);
	register_api_const(g_duktape, "COMMAND_MOVE_EAST", COMMAND_MOVE_EAST);
	register_api_const(g_duktape, "COMMAND_MOVE_SOUTHEAST", COMMAND_MOVE_SOUTHEAST);
	register_api_const(g_duktape, "COMMAND_MOVE_SOUTH", COMMAND_MOVE_SOUTH);
	register_api_const(g_duktape, "COMMAND_MOVE_SOUTHWEST", COMMAND_MOVE_SOUTHWEST);
	register_api_const(g_duktape, "COMMAND_MOVE_WEST", COMMAND_MOVE_WEST);
	register_api_const(g_duktape, "COMMAND_MOVE_NORTHWEST", COMMAND_MOVE_NORTHWEST);
}

void
get_person_xy(const person_t* person, float* out_x, float* out_y, int map_width, int map_height)
{
	*out_x = fmod(person->x, map_width);
	*out_y = fmod(person->y, map_height);
}

void
command_person(person_t* person, int command)
{
	switch (command) {
	case COMMAND_ANIMATE:
		if (--person->anim_frames <= 0) {
			++person->frame;
			person->anim_frames = 8;
		}
		break;
	case COMMAND_FACE_NORTH: _set_person_direction(person, "north"); break;
	case COMMAND_FACE_NORTHEAST: _set_person_direction(person, "northeast"); break;
	case COMMAND_FACE_EAST: _set_person_direction(person, "east"); break;
	case COMMAND_FACE_SOUTHEAST: _set_person_direction(person, "southeast"); break;
	case COMMAND_FACE_SOUTH: _set_person_direction(person, "south"); break;
	case COMMAND_FACE_SOUTHWEST: _set_person_direction(person, "southwest"); break;
	case COMMAND_FACE_WEST: _set_person_direction(person, "west"); break;
	case COMMAND_FACE_NORTHWEST: _set_person_direction(person, "northwest"); break;
	case COMMAND_MOVE_NORTH:
		command_person(person, COMMAND_ANIMATE);
		person->y -= person->speed;
		person->revert_frames = person->revert_delay;
		break;
	case COMMAND_MOVE_EAST:
		command_person(person, COMMAND_ANIMATE);
		person->x += person->speed;
		person->revert_frames = person->revert_delay;
		break;
	case COMMAND_MOVE_SOUTH:
		command_person(person, COMMAND_ANIMATE);
		person->y += person->speed;
		person->revert_frames = person->revert_delay;
		break;
	case COMMAND_MOVE_WEST:
		command_person(person, COMMAND_ANIMATE);
		person->x -= person->speed;
		person->revert_frames = person->revert_delay;
		break;
	}
}

person_t*
find_person(const char* name)
{
	int i;

	for (i = 0; i < s_num_persons; ++i) {
		if (strcmp(name, s_persons[i]->name) == 0)
			return s_persons[i];
	}
	return NULL;
}

void
render_persons(int cam_x, int cam_y)
{
	spriteset_t* sprite;
	float        x, y;
	int          i;

	al_hold_bitmap_drawing(true);
	for (i = 0; i < s_num_persons; ++i) {
		sprite = s_persons[i]->sprite;
		x = (int)s_persons[i]->x - cam_x; y = (int)s_persons[i]->y - cam_y;
		draw_sprite(sprite, s_persons[i]->direction, x, y, s_persons[i]->frame);
	}
	al_hold_bitmap_drawing(false);
}

void
reset_persons(map_t* map)
{
	point3_t  map_origin;
	person_t* person;
	
	int i, j;

	map_origin = get_map_origin();
	for (i = 0; i < s_num_persons; ++i) {
		person = s_persons[i];
		if (person->is_persistent) {
			person->x = map_origin.x;
			person->y = map_origin.y;
			person->layer = map_origin.z;
		}
		else {
			--s_num_persons;
			for (j = i; j < s_num_persons; ++j) s_persons[j] = s_persons[j + 1];
			--i;
		}
	}
	s_persons = realloc(s_persons, s_num_persons * sizeof(person_t*));
}

void
update_persons(void)
{
	int       command;
	person_t* person;
	
	int i;

	for (i = 0; i < s_num_persons; ++i) {
		person = s_persons[i];
		if (--person->revert_frames <= 0) person->frame = 0;
		if (person->num_commands > 0) {
			--person->num_commands;
			command = person->commands[person->num_commands];
			command_person(person, command);
		}
	}
}

static void
_add_person(const char* name, const char* sprite_file, bool is_persistent)
{
	char*     path;
	person_t* person;

	++s_num_persons;
	s_persons = realloc(s_persons, s_num_persons * sizeof(person_t*));
	person = s_persons[s_num_persons - 1] = calloc(1, sizeof(person_t));
	_set_person_name(person, name);
	_set_person_direction(person, "north");
	path = get_asset_path(sprite_file, "spritesets", false);
	person->sprite = load_spriteset(path);
	free(path);
	person->is_persistent = is_persistent;
	person->x = 0; person->y = 0; person->layer = 0;
	person->speed = 1.0;
	person->revert_delay = 8;
}

static void
_free_person(person_t* person)
{
	free_spriteset(person->sprite);
	free(person->name);
	free(person->direction);
	free(person);
}

static void
_delete_person(const char* name)
{
	int i, j;

	for (i = 0; i < s_num_persons; ++i) {
		if (strcmp(s_persons[i]->name, name) == 0) {
			_free_person(s_persons[i]);
			for (j = i; j < s_num_persons - 1; ++j) s_persons[j] = s_persons[j + 1];
			--s_num_persons; --i;
		}

	}
	s_persons = realloc(s_persons, s_num_persons * sizeof(person_t*));
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

static duk_ret_t
js_CreatePerson(duk_context* ctx)
{
	bool        destroy_with_map;
	const char* name;
	const char* sprite_file;

	name = duk_to_string(ctx, 0);
	sprite_file = duk_require_string(ctx, 1);
	destroy_with_map = duk_require_boolean(ctx, 2);
	_add_person(name, sprite_file, !destroy_with_map);
	return 0;
}

static duk_ret_t
js_DestroyPerson(duk_context* ctx)
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
js_GetPersonDirection(duk_context* ctx)
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
js_GetPersonLayer(duk_context* ctx)
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
js_GetPersonList(duk_context* ctx)
{
	int i;
	
	duk_push_array(ctx);
	for (i = 0; i < s_num_persons; ++i) {
		duk_push_string(ctx, s_persons[i]->name);
		duk_put_prop_index(ctx, -2, i);
	}
	return 1;
}

static duk_ret_t
js_GetPersonX(duk_context* ctx)
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
js_GetPersonY(duk_context* ctx)
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
js_SetPersonX(duk_context* ctx)
{
	person_t* person;

	person = find_person(duk_to_string(ctx, 0));
	if (person != NULL) person->x = duk_to_number(ctx, 1);
	return 0;
}

static duk_ret_t
js_SetPersonY(duk_context* ctx)
{
	person_t* person;

	person = find_person(duk_to_string(ctx, 0));
	if (person != NULL) person->x = duk_to_number(ctx, 1);
	return 0;
}

static duk_ret_t
js_QueuePersonCommand(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int command = duk_require_int(ctx, 1);
	bool immediate = duk_require_boolean(ctx, 2);
	
	person_t* person;

	person = find_person(name);
	if (person != NULL) {
		if (!immediate) {
			++person->num_commands;
			person->commands = realloc(person->commands, person->num_commands * sizeof(int));
			person->commands[person->num_commands - 1] = command;
		}
		else {
			command_person(person, command);
		}
		return true;
	}
	else {
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "QueuePersonCommand(): Person entity '%s' does not exist", name);
	}
}
