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

static duk_ret_t js_CreatePerson         (duk_context* ctx);
static duk_ret_t js_DestroyPerson        (duk_context* ctx);
static duk_ret_t js_IsPersonObstructed   (duk_context* ctx);
static duk_ret_t js_GetCurrentPerson     (duk_context* ctx);
static duk_ret_t js_GetPersonDirection   (duk_context* ctx);
static duk_ret_t js_GetPersonLayer       (duk_context* ctx);
static duk_ret_t js_GetPersonList        (duk_context* ctx);
static duk_ret_t js_GetPersonX           (duk_context* ctx);
static duk_ret_t js_GetPersonY           (duk_context* ctx);
static duk_ret_t js_SetPersonDirection   (duk_context* ctx);
static duk_ret_t js_SetPersonLayer       (duk_context* ctx);
static duk_ret_t js_SetPersonScript      (duk_context* ctx);
static duk_ret_t js_SetPersonX           (duk_context* ctx);
static duk_ret_t js_SetPersonY           (duk_context* ctx);
static duk_ret_t js_CallPersonScript     (duk_context* ctx);
static duk_ret_t js_GetObstructingPerson (duk_context* ctx);
static duk_ret_t js_QueuePersonCommand   (duk_context* ctx);

static void destroy_person       (const char* name);
static void free_person          (person_t* person);
static void set_person_direction (person_t* person, const char* direction);
static void set_person_name      (person_t* person, const char* name);

static const person_t* s_current_person = NULL;
static int             s_num_persons    = 0;
static person_t*       *s_persons       = NULL;

void
init_person_api(void)
{
	register_api_func(g_duktape, NULL, "CreatePerson", js_CreatePerson);
	register_api_func(g_duktape, NULL, "DestroyPerson", js_DestroyPerson);
	register_api_func(g_duktape, NULL, "IsPersonObstructed", js_IsPersonObstructed);
	register_api_func(g_duktape, NULL, "GetCurrentPerson", js_GetCurrentPerson);
	register_api_func(g_duktape, NULL, "GetPersonDirection", js_GetPersonDirection);
	register_api_func(g_duktape, NULL, "GetPersonLayer", js_GetPersonLayer);
	register_api_func(g_duktape, NULL, "GetPersonList", js_GetPersonList);
	register_api_func(g_duktape, NULL, "GetPersonX", js_GetPersonX);
	register_api_func(g_duktape, NULL, "GetPersonY", js_GetPersonY);
	register_api_func(g_duktape, NULL, "SetPersonDirection", js_SetPersonDirection);
	register_api_func(g_duktape, NULL, "SetPersonLayer", js_SetPersonLayer);
	register_api_func(g_duktape, NULL, "SetPersonScript", js_SetPersonScript);
	register_api_func(g_duktape, NULL, "SetPersonX", js_SetPersonX);
	register_api_func(g_duktape, NULL, "SetPersonY", js_SetPersonY);
	register_api_func(g_duktape, NULL, "CallPersonScript", js_CallPersonScript);
	register_api_func(g_duktape, NULL, "GetObstructingPerson", js_GetObstructingPerson);
	register_api_func(g_duktape, NULL, "QueuePersonCommand", js_QueuePersonCommand);

	// movement script specifier constants
	register_api_const(g_duktape, "SCRIPT_ON_CREATE", PERSON_SCRIPT_ON_CREATE);
	register_api_const(g_duktape, "SCRIPT_ON_DESTROY", PERSON_SCRIPT_ON_DESTROY);
	register_api_const(g_duktape, "SCRIPT_ON_ACTIVATE_TOUCH", PERSON_SCRIPT_ON_ACT_TOUCH);
	register_api_const(g_duktape, "SCRIPT_ON_ACTIVATE_TALK", PERSON_SCRIPT_ON_ACT_TALK);
	register_api_const(g_duktape, "SCRIPT_COMMAND_GENERATOR", PERSON_SCRIPT_GENERATOR);

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

const char*
get_person_name(const person_t* person)
{
	return person->name;
}

void
get_person_xy(const person_t* person, float* out_x, float* out_y, int map_width, int map_height, bool normalize)
{
	if (normalize) {
		*out_x = fmod(fmod(person->x, map_width) + map_width, map_width);
		*out_y = fmod(fmod(person->y, map_height) + map_height, map_height);
	}
	else {
		*out_x = person->x;
		*out_y = person->y;
	}
}

bool
set_person_script(person_t* person, int type, const lstring_t* script)
{
	const char* script_name;

	script_name = type == PERSON_SCRIPT_ON_CREATE ? "onCreatePerson"
		: type == PERSON_SCRIPT_ON_DESTROY ? "onDestroyPerson"
		: type == PERSON_SCRIPT_ON_ACT_TOUCH ? "onTouchPerson"
		: type == PERSON_SCRIPT_ON_ACT_TALK ? "onTalkToPerson"
		: type == PERSON_SCRIPT_GENERATOR ? "onGeneratePersonCommands"
		: NULL;
	if (script_name == NULL) return false;
	duk_push_global_stash(g_duktape);
	if (!duk_get_prop_string(g_duktape, -1, script_name)) {
		duk_pop(g_duktape);
		duk_push_object(g_duktape);
		duk_put_prop_string(g_duktape, -2, script_name);
		duk_get_prop_string(g_duktape, -1, script_name);
	}
	duk_push_sprintf(g_duktape, "[%s.%s]", person->name, script_name);
	duk_compile_lstring_filename(g_duktape, 0x0, script->cstr, script->length);
	duk_put_prop_string(g_duktape, -2, get_person_name(person));
	duk_pop_2(g_duktape);
	return true;
}

void
set_person_xyz(person_t* person, int x, int y, int z)
{
	person->x = x; person->y = y; person->layer = z;
}

bool
call_person_script(const person_t* person, int script_type)
{
	const char* script_name;

	script_name = script_type == PERSON_SCRIPT_ON_CREATE ? "onCreatePerson"
		: script_type == PERSON_SCRIPT_ON_DESTROY ? "onDestroyPerson"
		: script_type == PERSON_SCRIPT_ON_ACT_TOUCH ? "onTouchPerson"
		: script_type == PERSON_SCRIPT_ON_ACT_TALK ? "onTalkToPerson"
		: script_type == PERSON_SCRIPT_GENERATOR ? "onGeneratePersonCommands"
		: NULL;
	if (script_name == NULL) return false;
	duk_push_global_stash(g_duktape);
	if (duk_get_prop_string(g_duktape, -1, script_name)) {
		duk_get_prop_string(g_duktape, -1, get_person_name(person));
		if (duk_is_callable(g_duktape, -1)) {
			s_current_person = person;
			duk_call(g_duktape, 0);
			s_current_person = NULL;
		}
		duk_pop_2(g_duktape);
	}
	else {
		duk_pop(g_duktape);
	}
	duk_pop(g_duktape);
	return true;
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
	case COMMAND_FACE_NORTH: set_person_direction(person, "north"); break;
	case COMMAND_FACE_NORTHEAST: set_person_direction(person, "northeast"); break;
	case COMMAND_FACE_EAST: set_person_direction(person, "east"); break;
	case COMMAND_FACE_SOUTHEAST: set_person_direction(person, "southeast"); break;
	case COMMAND_FACE_SOUTH: set_person_direction(person, "south"); break;
	case COMMAND_FACE_SOUTHWEST: set_person_direction(person, "southwest"); break;
	case COMMAND_FACE_WEST: set_person_direction(person, "west"); break;
	case COMMAND_FACE_NORTHWEST: set_person_direction(person, "northwest"); break;
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
render_persons(int layer, int cam_x, int cam_y, int map_width, int map_height)
{
	spriteset_t* sprite;
	float        x, y;
	int          i;

	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->layer != layer)
			continue;
		sprite = s_persons[i]->sprite;
		get_person_xy(s_persons[i], &x, &y, map_width, map_height, true);
		x -= cam_x; y -= cam_y;
		draw_sprite(sprite, s_persons[i]->direction, x, y, s_persons[i]->frame);
	}
}

void
reset_persons(map_t* map, bool keep_existing)
{
	point3_t  map_origin;
	person_t* person;
	
	int i, j;

	map_origin = get_map_origin();
	for (i = 0; i < s_num_persons; ++i) {
		person = s_persons[i];
		if (person->is_persistent || keep_existing) {
			person->x = map_origin.x;
			person->y = map_origin.y;
			person->layer = map_origin.z;
			call_person_script(person, PERSON_SCRIPT_ON_CREATE);
		}
		else {
			call_person_script(person, PERSON_SCRIPT_ON_DESTROY);
			free_person(person);
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
	
	int i, j;

	for (i = 0; i < s_num_persons; ++i) {
		person = s_persons[i];
		if (--person->revert_frames <= 0) person->frame = 0;
		if (person->num_commands == 0) {
			call_person_script(person, PERSON_SCRIPT_GENERATOR);
		}
		if (person->num_commands > 0) {
			command = person->commands[0];
			--person->num_commands;
			for (j = 0; j < person->num_commands; ++j)
				person->commands[j] = person->commands[j + 1];
			command_person(person, command);
		}
	}
}

person_t*
create_person(const char* name, const char* sprite_file, bool is_persistent)
{
	char*     path;
	person_t* person;

	++s_num_persons;
	s_persons = realloc(s_persons, s_num_persons * sizeof(person_t*));
	person = s_persons[s_num_persons - 1] = calloc(1, sizeof(person_t));
	set_person_name(person, name);
	path = get_asset_path(sprite_file, "spritesets", false);
	person->sprite = load_spriteset(path);
	free(path);
	set_person_direction(person, person->sprite->poses[0].name);
	person->is_persistent = is_persistent;
	person->x = 0; person->y = 0; person->layer = 0;
	person->speed = 1.0;
	person->revert_delay = 8;
	return person;
}

static void
free_person(person_t* person)
{
	free_spriteset(person->sprite);
	free(person->name);
	free(person->direction);
	free(person);
}

static void
destroy_person(const char* name)
{
	int i, j;

	for (i = 0; i < s_num_persons; ++i) {
		if (strcmp(s_persons[i]->name, name) == 0) {
			call_person_script(s_persons[i], PERSON_SCRIPT_ON_DESTROY);
			free_person(s_persons[i]);
			for (j = i; j < s_num_persons - 1; ++j) s_persons[j] = s_persons[j + 1];
			--s_num_persons; --i;
		}

	}
	s_persons = realloc(s_persons, s_num_persons * sizeof(person_t*));
}

static void
set_person_direction(person_t* person, const char* direction)
{
	person->direction = realloc(person->direction, (strlen(direction) + 1) * sizeof(char));
	strcpy(person->direction, direction);
}

static void
set_person_name(person_t* person, const char* name)
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
	create_person(name, sprite_file, !destroy_with_map);
	return 0;
}

static duk_ret_t
js_DestroyPerson(duk_context* ctx)
{
	const char* name;

	name = duk_to_string(ctx, 0);
	if (find_person(name) != NULL) {
		destroy_person(name);
		return 0;
	}
	else {
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "DestroyPerson(): Person entity '%s' doesn't exist!", name);
	}
}

static duk_ret_t
js_IsPersonObstructed(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonDirection(): Person '%s' doesn't exist", name);
	// TODO: implement collision detection
	duk_push_false(ctx);
	return 1;
}

static duk_ret_t
js_GetCurrentPerson(duk_context* ctx)
{
	if (s_current_person == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "GetCurrentPerson(): Cannot be called outside of a person script");
	duk_push_string(ctx, s_current_person->name);
	return 1;
}

static duk_ret_t
js_GetPersonDirection(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	
	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonDirection(): Person '%s' doesn't exist", name);
	duk_push_string(ctx, person->direction);
	return 1;
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
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonLayer(): Person '%s' doesn't exist", name);
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

	name = duk_require_string(ctx, 0);
	person = find_person(name);
	if (person == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonX(): Person '%s' doesn't exist", name);
	duk_push_number(ctx, person->x);
	return 1;
}

static duk_ret_t
js_GetPersonY(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_require_string(ctx, 0);
	person = find_person(name);
	if (person == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonY(): Person '%s' doesn't exist", name);
	duk_push_number(ctx, person->y);
	return 1;
}

static duk_ret_t
js_SetPersonDirection(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	const char* new_dir = duk_require_string(ctx, 1);

	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonDirection(): Person '%s' doesn't exist", name);
	set_person_direction(person, new_dir);
	return 0;
}

static duk_ret_t
js_SetPersonLayer(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int z = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "SetPersonLayer(): Person '%s' doesn't exist", name);
	person->layer = z;
	return 0;
}

static duk_ret_t
js_SetPersonScript(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int type = duk_require_int(ctx, 1);
	lstring_t* script = duk_require_lstring_t(ctx, 2);
	
	person_t*  person;
	
	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "SetPersonScript(): Person '%s' doesn't exist", name);
	if (!set_person_script(person, type, script))
		duk_error(ctx, DUK_ERR_API_ERROR, "SetPersonScript(): Failed to set person script; likely cause: invalid script type constant");
	free_lstring(script);
	return 0;
}

static duk_ret_t
js_SetPersonX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);

	person_t* person;

	person = find_person(name);
	if (person == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "SetPersonX(): Person '%s' doesn't exist", name);
	person->x = x;
	return 0;
}

static duk_ret_t
js_SetPersonY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int y = duk_require_int(ctx, 1);
	
	person_t* person;

	person = find_person(name);
	if (person == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "SetPersonY(): Person '%s' doesn't exist", name);
	person->y = y;
	return 0;
}

static duk_ret_t
js_CallPersonScript(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int type = duk_require_int(ctx, 1);
	
	person_t*   person;

	person = find_person(name);
	if (person == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "CallPersonScript(): Person '%s' doesn't exist", name);
	if (!call_person_script(person, type))
		duk_error(ctx, DUK_ERR_ERROR, "CallPersonScript(): Failed to call person script, caller probably passed invalid script type constant");
	return 0;
}

static duk_ret_t
js_GetObstructingPerson(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetObstructingPerson(): Map engine must be running");
	// TODO: implement collision detection
	duk_push_string(ctx, "");
	return 1;
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
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "QueuePersonCommand(): Person '%s' doesn't exist", name);
	}
}
