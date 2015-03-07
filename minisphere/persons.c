#include "minisphere.h"
#include "api.h"
#include "map_engine.h"
#include "obsmap.h"
#include "spriteset.h"

#include "persons.h"

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
	int          scripts[PERSON_SCRIPT_MAX];
	float        speed_x, speed_y;
	spriteset_t* sprite;
	float        x, y;
	int          x_offset, y_offset;
	int          num_commands;
	int          *commands;
};

static duk_ret_t js_CreatePerson            (duk_context* ctx);
static duk_ret_t js_DestroyPerson           (duk_context* ctx);
static duk_ret_t js_IsCommandQueueEmpty     (duk_context* ctx);
static duk_ret_t js_IsPersonObstructed      (duk_context* ctx);
static duk_ret_t js_DoesPersonExist         (duk_context* ctx);
static duk_ret_t js_GetCurrentPerson        (duk_context* ctx);
static duk_ret_t js_GetObstructingPerson    (duk_context* ctx);
static duk_ret_t js_GetObstructingTile      (duk_context* ctx);
static duk_ret_t js_GetPersonDirection      (duk_context* ctx);
static duk_ret_t js_GetPersonFrameRevert    (duk_context* ctx);
static duk_ret_t js_GetPersonLayer          (duk_context* ctx);
static duk_ret_t js_GetPersonList           (duk_context* ctx);
static duk_ret_t js_GetPersonSpeed          (duk_context* ctx);
static duk_ret_t js_GetPersonX              (duk_context* ctx);
static duk_ret_t js_GetPersonY              (duk_context* ctx);
static duk_ret_t js_GetTalkDistance         (duk_context* ctx);
static duk_ret_t js_SetDefaultPersonScript  (duk_context* ctx);
static duk_ret_t js_SetPersonDirection      (duk_context* ctx);
static duk_ret_t js_SetPersonFrameRevert    (duk_context* ctx);
static duk_ret_t js_SetPersonLayer          (duk_context* ctx);
static duk_ret_t js_SetPersonScript         (duk_context* ctx);
static duk_ret_t js_SetPersonSpeed          (duk_context* ctx);
static duk_ret_t js_SetPersonSpriteset      (duk_context* ctx);
static duk_ret_t js_SetPersonX              (duk_context* ctx);
static duk_ret_t js_SetPersonY              (duk_context* ctx);
static duk_ret_t js_SetTalkDistance         (duk_context* ctx);
static duk_ret_t js_CallDefaultPersonScript (duk_context* ctx);
static duk_ret_t js_CallPersonScript        (duk_context* ctx);
static duk_ret_t js_ClearPersonCommands     (duk_context* ctx);
static duk_ret_t js_QueuePersonCommand      (duk_context* ctx);

static int  compare_persons      (const void* a, const void* b);
static void free_person          (person_t* person);
static void set_person_direction (person_t* person, const char* direction);
static void set_person_name      (person_t* person, const char* name);
static void set_person_spriteset (person_t* person, spriteset_t* spriteset);
static void sort_persons         (void);

static const person_t* s_current_person = NULL;
static int             s_def_scripts[PERSON_SCRIPT_MAX];
static bool            s_is_talking     = false;
static int             s_talk_distance  = 8;
static int             s_num_persons    = 0;
static person_t*       *s_persons       = NULL;

person_t*
create_person(const char* name, const char* sprite_file, bool is_persistent)
{
	point3_t  map_origin = get_map_origin();
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
	person->x = map_origin.x;
	person->y = map_origin.y;
	person->layer = map_origin.z;
	person->speed_x = 1.0;
	person->speed_y = 1.0;
	person->anim_frames = get_sprite_frame_delay(person->sprite, person->direction, 0);
	sort_persons();
	return person;
}

void
destroy_person(person_t* person)
{
	int i, j;

	call_person_script(person, PERSON_SCRIPT_ON_DESTROY, true);
	free_person(person);
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i] == person) {
			for (j = i; j < s_num_persons - 1; ++j) s_persons[j] = s_persons[j + 1];
			--s_num_persons; --i;
		}
	}
	sort_persons();
}

bool
is_person_obstructed_at(const person_t* person, float x, float y, person_t** out_obstructing_person, int* out_tile_index)
{
	rect_t           area;
	rect_t           base, my_base;
	bool             collision = false;
	float            cur_x, cur_y;
	bool             is_obstructed = false;
	int              layer;
	const obsmap_t*  obsmap;
	int              tile_w, tile_h;
	const tileset_t* tileset;

	int i, i_x, i_y;
	
	if (out_obstructing_person) *out_obstructing_person = NULL;
	if (out_tile_index) *out_tile_index = -1;
	get_person_xyz(person, &cur_x, &cur_y, &layer, true);
	my_base = translate_rect(get_person_base(person), x - cur_x, y - cur_y);

	// check for obstructing persons
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i] == person)  // these persons aren't going to obstruct themselves
			continue;
		if (s_persons[i]->layer != layer)  // ignore persons not on same layer
			continue;
		base = get_person_base(s_persons[i]);
		if (do_rects_intersect(my_base, base)) {
			is_obstructed = true;
			if (out_obstructing_person) *out_obstructing_person = s_persons[i];
			break;
		}
	}

	// no obstructing person, check map-defined obstructions
	obsmap = get_map_layer_obsmap(layer);
	if (test_obsmap_rect(obsmap, my_base))
		is_obstructed = true;
	
	// check for obstructing tiles; constrain search to immediate vicinity of sprite base
	tileset = get_map_tileset();
	get_tile_size(tileset, &tile_w, &tile_h);
	area.x1 = my_base.x1 / tile_w;
	area.y1 = my_base.y1 / tile_h;
	area.x2 = area.x1 + (my_base.x2 - my_base.x1) / tile_w + 2;
	area.y2 = area.y1 + (my_base.y2 - my_base.y1) / tile_h + 2;
	for (i_x = area.x1; i_x < area.x2; ++i_x) for (i_y = area.y1; i_y < area.y2; ++i_y) {
		base = translate_rect(my_base, -(i_x * tile_w), -(i_y * tile_h));
		obsmap = get_tile_obsmap(tileset, get_map_tile(i_x, i_y, layer));
		if (obsmap != NULL && test_obsmap_rect(obsmap, base)) {
			is_obstructed = true;
			if (out_tile_index) *out_tile_index = get_map_tile(i_x, i_y, layer);
			break;
		}
	}
	
	return is_obstructed;
}

rect_t
get_person_base(const person_t* person)
{
	rect_t base_rect;
	int    base_x, base_y;
	float  x, y;

	base_rect = person->sprite->base;
	get_person_xy(person, &x, &y, true);
	base_x = x - (base_rect.x1 + (base_rect.x2 - base_rect.x1) / 2);
	base_y = y - (base_rect.y1 + (base_rect.y2 - base_rect.y1) / 2);
	base_rect.x1 += base_x; base_rect.x2 += base_x;
	base_rect.y1 += base_y; base_rect.y2 += base_y;
	return base_rect;
}

const char*
get_person_name(const person_t* person)
{
	return person->name;
}

float
get_person_speed(const person_t* person)
{
	return sqrt(person->speed_x * person->speed_y);
}

void
get_person_xy(const person_t* person, float* out_x, float* out_y, bool normalize)
{
	rect_t map_rect;
	
	if (normalize) {
		map_rect = get_map_bounds();
		*out_x = fmod(fmod(person->x, map_rect.x2) + map_rect.x2, map_rect.x2);
		*out_y = fmod(fmod(person->y, map_rect.y2) + map_rect.y2, map_rect.y2);
	}
	else {
		*out_x = person->x;
		*out_y = person->y;
	}
}

void
get_person_xyz(const person_t* person, float* out_x, float* out_y, int* out_layer, bool normalize)
{
	rect_t map_rect;

	*out_layer = person->layer;
	if (normalize) {
		map_rect = get_map_bounds();
		*out_x = fmod(fmod(person->x, map_rect.x2) + map_rect.x2, map_rect.x2);
		*out_y = fmod(fmod(person->y, map_rect.y2) + map_rect.y2, map_rect.y2);
	}
	else {
		*out_x = person->x;
		*out_y = person->y;
	}
}

bool
set_person_script(person_t* person, int type, const lstring_t* script)
{
	int         script_id;
	const char* script_name;

	script_name = type == PERSON_SCRIPT_ON_CREATE ? "[person create script]"
		: type == PERSON_SCRIPT_ON_DESTROY ? "[person destroy script]"
		: type == PERSON_SCRIPT_ON_TOUCH ? "[person touch script]"
		: type == PERSON_SCRIPT_ON_TALK ? "[person talk script]"
		: type == PERSON_SCRIPT_GENERATOR ? "[command generator]"
		: NULL;
	if (script_name == NULL) return false;
	script_id = compile_script(script, script_name);
	free_script(person->scripts[type]);
	person->scripts[type] = script_id;
	return true;
}

void
set_person_speed(person_t* person, float speed)
{
	person->speed_x = speed;
	person->speed_y = speed;
}

void
set_person_xyz(person_t* person, int x, int y, int layer)
{
	person->x = x;
	person->y = y;
	person->layer = layer;
	sort_persons();
}

bool
call_person_script(const person_t* person, int type, bool use_default)
{
	const person_t* last_person;

	last_person = s_current_person;
	s_current_person = person;
	if (use_default)
		run_script(s_def_scripts[type], false);
	run_script(person->scripts[type], false);
	s_current_person = last_person;
	return true;
}

void
command_person(person_t* person, int command)
{
	float     new_x, new_y;
	person_t* person_to_touch;
	
	new_x = person->x; new_y = person->y;
	switch (command) {
	case COMMAND_ANIMATE:
		if (person->anim_frames > 0 && --person->anim_frames == 0) {
			++person->frame;
			person->anim_frames = get_sprite_frame_delay(person->sprite, person->direction, person->frame);
		}
		break;
	case COMMAND_FACE_NORTH:
		set_person_direction(person, "north");
		break;
	case COMMAND_FACE_NORTHEAST:
		set_person_direction(person, "northeast");
		break;
	case COMMAND_FACE_EAST:
		set_person_direction(person, "east");
		break;
	case COMMAND_FACE_SOUTHEAST:
		set_person_direction(person, "southeast");
		break;
	case COMMAND_FACE_SOUTH:
		set_person_direction(person, "south");
		break;
	case COMMAND_FACE_SOUTHWEST:
		set_person_direction(person, "southwest");
		break;
	case COMMAND_FACE_WEST:
		set_person_direction(person, "west");
		break;
	case COMMAND_FACE_NORTHWEST:
		set_person_direction(person, "northwest");
		break;
	case COMMAND_MOVE_NORTH:
		new_y = person->y - person->speed_y;
		break;
	case COMMAND_MOVE_EAST:
		new_x = person->x + person->speed_x;
		break;
	case COMMAND_MOVE_SOUTH:
		new_y = person->y + person->speed_y;
		break;
	case COMMAND_MOVE_WEST:
		new_x = person->x - person->speed_x;
		break;
	}
	if (new_x != person->x || new_y != person->y) {
		// person is trying to move, make sure the path is clear of obstructions
		if (!is_person_obstructed_at(person, new_x, new_y, &person_to_touch, NULL)) {
			command_person(person, COMMAND_ANIMATE);
			person->x = new_x; person->y = new_y;
			person->revert_frames = person->revert_delay;
			sort_persons();
		}
		else {
			// if not, and we collided with a person, call that person's touch script
			if (person_to_touch != NULL)
				call_person_script(person_to_touch, PERSON_SCRIPT_ON_TOUCH, true);
		}
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
render_persons(int layer, int cam_x, int cam_y)
{
	spriteset_t* sprite;
	float        x, y;
	int          i;

	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->layer != layer)
			continue;
		sprite = s_persons[i]->sprite;
		get_person_xy(s_persons[i], &x, &y, true);
		x -= cam_x - s_persons[i]->x_offset;
		y -= cam_y - s_persons[i]->y_offset;
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
			call_person_script(person, PERSON_SCRIPT_ON_CREATE, true);
		}
		else {
			call_person_script(person, PERSON_SCRIPT_ON_DESTROY, true);
			free_person(person);
			--s_num_persons;
			for (j = i; j < s_num_persons; ++j) s_persons[j] = s_persons[j + 1];
			--i;
		}
	}
	sort_persons();
}

void
talk_person(const person_t* person)
{
	rect_t    map_rect;
	person_t* target_person;
	float     talk_x, talk_y;

	if (s_is_talking)
		return;
	map_rect = get_map_bounds();
	
	// check if anyone else is within earshot
	get_person_xy(person, &talk_x, &talk_y, true);
	if (strstr(person->direction, "north") != NULL) talk_y -= s_talk_distance;
	if (strstr(person->direction, "east") != NULL) talk_x += s_talk_distance;
	if (strstr(person->direction, "south") != NULL) talk_y += s_talk_distance;
	if (strstr(person->direction, "west") != NULL) talk_x -= s_talk_distance;
	is_person_obstructed_at(person, talk_x, talk_y, &target_person, NULL);
	if (target_person != NULL) {
		s_is_talking = true;
		call_person_script(target_person, PERSON_SCRIPT_ON_TALK, true);
		s_is_talking = false;
	}
}

void
update_persons(void)
{
	int       command;
	person_t* person;
	
	int i, j;

	for (i = 0; i < s_num_persons; ++i) {
		person = s_persons[i];
		if (person->revert_delay > 0 && --person->revert_frames <= 0)
			person->frame = 0;
		if (person->num_commands == 0)
			call_person_script(person, PERSON_SCRIPT_GENERATOR, true);
		if (person->num_commands > 0) {
			command = person->commands[0];
			--person->num_commands;
			for (j = 0; j < person->num_commands; ++j)
				person->commands[j] = person->commands[j + 1];
			command_person(person, command);
		}
	}
}

void
init_person_api(void)
{
	memset(s_def_scripts, 0, PERSON_SCRIPT_MAX * sizeof(int));
	
	register_api_func(g_duktape, NULL, "CreatePerson", js_CreatePerson);
	register_api_func(g_duktape, NULL, "DestroyPerson", js_DestroyPerson);
	register_api_func(g_duktape, NULL, "IsCommandQueueEmpty", js_IsCommandQueueEmpty);
	register_api_func(g_duktape, NULL, "IsPersonObstructed", js_IsPersonObstructed);
	register_api_func(g_duktape, NULL, "DoesPersonExist", js_DoesPersonExist);
	register_api_func(g_duktape, NULL, "GetCurrentPerson", js_GetCurrentPerson);
	register_api_func(g_duktape, NULL, "GetObstructingPerson", js_GetObstructingPerson);
	register_api_func(g_duktape, NULL, "GetObstructingTile", js_GetObstructingTile);
	register_api_func(g_duktape, NULL, "GetPersonDirection", js_GetPersonDirection);
	register_api_func(g_duktape, NULL, "GetPersonFrameRevert", js_GetPersonFrameRevert);
	register_api_func(g_duktape, NULL, "GetPersonLayer", js_GetPersonLayer);
	register_api_func(g_duktape, NULL, "GetPersonList", js_GetPersonList);
	register_api_func(g_duktape, NULL, "GetPersonSpeed", js_GetPersonSpeed);
	register_api_func(g_duktape, NULL, "GetPersonX", js_GetPersonX);
	register_api_func(g_duktape, NULL, "GetPersonY", js_GetPersonY);
	register_api_func(g_duktape, NULL, "GetTalkDistance", js_GetTalkDistance);
	register_api_func(g_duktape, NULL, "SetDefaultPersonScript", js_SetDefaultPersonScript);
	register_api_func(g_duktape, NULL, "SetPersonDirection", js_SetPersonDirection);
	register_api_func(g_duktape, NULL, "SetPersonFrameRevert", js_SetPersonFrameRevert);
	register_api_func(g_duktape, NULL, "SetPersonLayer", js_SetPersonLayer);
	register_api_func(g_duktape, NULL, "SetPersonScript", js_SetPersonScript);
	register_api_func(g_duktape, NULL, "SetPersonSpeed", js_SetPersonSpeed);
	register_api_func(g_duktape, NULL, "SetPersonSpriteset", js_SetPersonSpriteset);
	register_api_func(g_duktape, NULL, "SetPersonX", js_SetPersonX);
	register_api_func(g_duktape, NULL, "SetPersonY", js_SetPersonY);
	register_api_func(g_duktape, NULL, "SetTalkDistance", js_SetTalkDistance);
	register_api_func(g_duktape, NULL, "CallDefaultPersonScript", js_CallDefaultPersonScript);
	register_api_func(g_duktape, NULL, "CallPersonScript", js_CallPersonScript);
	register_api_func(g_duktape, NULL, "ClearPersonCommands", js_ClearPersonCommands);
	register_api_func(g_duktape, NULL, "QueuePersonCommand", js_QueuePersonCommand);

	// movement script specifier constants
	register_api_const(g_duktape, "SCRIPT_ON_CREATE", PERSON_SCRIPT_ON_CREATE);
	register_api_const(g_duktape, "SCRIPT_ON_DESTROY", PERSON_SCRIPT_ON_DESTROY);
	register_api_const(g_duktape, "SCRIPT_ON_ACTIVATE_TOUCH", PERSON_SCRIPT_ON_TOUCH);
	register_api_const(g_duktape, "SCRIPT_ON_ACTIVATE_TALK", PERSON_SCRIPT_ON_TALK);
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

static int
compare_persons(const void* a, const void* b)
{
	person_t* p1 = *(person_t**)a;
	person_t* p2 = *(person_t**)b;

	return (p1->y + p1->y_offset) - (p2->y + p2->y_offset);
}

static void
free_person(person_t* person)
{
	int i;

	for (i = 0; i < PERSON_SCRIPT_MAX; ++i)
		free_script(person->scripts[i]);
	free_spriteset(person->sprite);
	free(person->name);
	free(person->direction);
	free(person);
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

static void
set_person_spriteset(person_t* person, spriteset_t* spriteset)
{
	free_spriteset(person->sprite);
	ref_spriteset(spriteset);
	person->sprite = spriteset;
	person->anim_frames = get_sprite_frame_delay(person->sprite, person->direction, 0);
	person->frame = 0;
}

static void
sort_persons(void)
{
	qsort(s_persons, s_num_persons, sizeof(person_t*), compare_persons);
}

static duk_ret_t
js_CreatePerson(duk_context* ctx)
{
	bool        destroy_with_map;
	const char* name;
	const char* sprite_file;

	name = duk_require_string(ctx, 0);
	sprite_file = duk_require_string(ctx, 1);
	destroy_with_map = duk_require_boolean(ctx, 2);
	create_person(name, sprite_file, !destroy_with_map);
	return 0;
}

static duk_ret_t
js_DestroyPerson(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "DestroyPerson(): Person '%s' doesn't exist", name);
	destroy_person(person);
	return 0;
}

static duk_ret_t
js_IsCommandQueueEmpty(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "IsCommandQueueEmpty(): Person '%s' doesn't exist", name);
	duk_push_boolean(ctx, person->num_commands <= 0);
	return 1;
}

static duk_ret_t
js_DoesPersonExist(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	duk_push_boolean(ctx, find_person(name) != NULL);
	return 1;
}

static duk_ret_t
js_IsPersonObstructed(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "IsPersonObstructed(): Person '%s' doesn't exist", name);
	duk_push_boolean(ctx, is_person_obstructed_at(person, x, y, NULL, NULL));
	return 1;
}

static duk_ret_t
js_GetCurrentPerson(duk_context* ctx)
{
	if (s_current_person == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "GetCurrentPerson(): Must be called from a person script");
	duk_push_string(ctx, s_current_person->name);
	return 1;
}

static duk_ret_t
js_GetObstructingPerson(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	person_t* obs_person = NULL;
	person_t* person;

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetObstructingPerson(): Map engine must be running");
	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetObstructingPerson(): Person '%s' doesn't exist", name);
	is_person_obstructed_at(person, x, y, &obs_person, NULL);
	duk_push_string(ctx, obs_person != NULL ? get_person_name(obs_person) : "");
	return 1;
}

static duk_ret_t
js_GetObstructingTile(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);
	int y = duk_require_int(ctx, 2);

	person_t* person;
	int       tile_index;

	if (!g_map_running)
		duk_error(ctx, DUK_ERR_ERROR, "GetObstructingTile(): Map engine must be running");
	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetObstructingTile(): Person '%s' doesn't exist", name);
	is_person_obstructed_at(person, x, y, NULL, &tile_index);
	duk_push_int(ctx, tile_index);
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
js_GetPersonFrameRevert(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonFrameRevert(): Person '%s' doesn't exist", name);
	duk_push_int(ctx, person->revert_delay);
	return 1;
}

static duk_ret_t
js_GetPersonLayer(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	
	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonLayer(): Person '%s' doesn't exist", name);
	duk_push_int(ctx, person->layer);
	return 1;
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
js_GetPersonSpeed(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*  person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonSpeed(): Person '%s' doesn't exist", name);
	duk_push_number(ctx, get_person_speed(person));
	return 1;
}

static duk_ret_t
js_GetPersonX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	
	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonX(): Person '%s' doesn't exist", name);
	duk_push_number(ctx, person->x);
	return 1;
}

static duk_ret_t
js_GetPersonY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "GetPersonY(): Person '%s' doesn't exist", name);
	duk_push_number(ctx, person->y);
	return 1;
}

static duk_ret_t
js_GetTalkDistance(duk_context* ctx)
{
	duk_push_int(ctx, s_talk_distance);
	return 1;
}

static duk_ret_t
js_SetDefaultPersonScript(duk_context* ctx)
{
	int type = duk_require_int(ctx, 0);
	lstring_t* script = duk_require_lstring_t(ctx, 2);

	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error(ctx, DUK_ERR_ERROR, "SetDefaultPersonScript(): Invalid script type constant");
	free_script(s_def_scripts[type]);
	s_def_scripts[type] = compile_script(script, "[default person script]");
	free_lstring(script);
	return 0;
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
js_SetPersonFrameRevert(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int frames = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "SetPersonFrameRevert(): Person '%s' doesn't exist", name);
	if (frames < 0)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "SetPersonFrameRevert(): Negative delay not allowed (caller passed %i)", frames);
	person->revert_delay = frames;
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
	
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error(ctx, DUK_ERR_ERROR, "SetPersonScript(): Invalid script type constant");
	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "SetPersonScript(): Person '%s' doesn't exist", name);
	set_person_script(person, type, script);
	free_lstring(script);
	return 0;
}

static duk_ret_t
js_SetPersonSpeed(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	float speed = duk_require_number(ctx, 1);

	person_t*  person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "SetPersonSpeed(): Person '%s' doesn't exist", name);
	set_person_speed(person, speed);
	return 0;
}

static duk_ret_t
js_SetPersonSpriteset(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*    person;
	spriteset_t* spriteset;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "SetPersonScript(): Person '%s' doesn't exist", name);
	duk_get_prop_string(ctx, 1, "\xFF" "ptr"); spriteset = duk_get_pointer(ctx, -1); duk_pop(ctx);
	set_person_spriteset(person, spriteset);
	return 0;
}

static duk_ret_t
js_SetPersonX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
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

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "SetPersonY(): Person '%s' doesn't exist", name);
	person->y = y;
	return 0;
}

static duk_ret_t
js_SetTalkDistance(duk_context* ctx)
{
	int pixels = duk_require_int(ctx, 0);

	if (pixels < 0)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "SetTalkDistance(): Negative distance not allowed (caller passed %i)", pixels);
	s_talk_distance = pixels;
	return 0;
}

static duk_ret_t
js_CallDefaultPersonScript(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int type = duk_require_int(ctx, 1);

	const person_t* last_person;
	person_t*       person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "CallDefaultPersonScript(): Person '%s' doesn't exist", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error(ctx, DUK_ERR_ERROR, "CallDefaultPersonScript(): Invalid script type constant");
	last_person = s_current_person;
	s_current_person = person;
	run_script(s_def_scripts[type], false);
	s_current_person = last_person;
	return 0;
}

static duk_ret_t
js_CallPersonScript(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int type = duk_require_int(ctx, 1);
	
	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "CallPersonScript(): Person '%s' doesn't exist", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error(ctx, DUK_ERR_ERROR, "CallPersonScript(): Invalid script type constant");
	call_person_script(person, type, false);
	return 0;
}

static duk_ret_t
js_ClearPersonCommands(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "ClearPersonCommands(): Person '%s' doesn't exist", name);
	person->num_commands = 0;
	free(person->commands);
	person->commands = NULL;
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
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "QueuePersonCommand(): Person '%s' doesn't exist", name);
	}
}
