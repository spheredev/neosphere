#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "map_engine.h"
#include "obsmap.h"
#include "spriteset.h"

#include "persons.h"

struct person
{
	unsigned int    id;
	char*           name;
	int             anim_frames;
	char*           direction;
	int             follow_distance;
	int             frame;
	bool            ignore_all_persons;
	bool            ignore_all_tiles;
	bool            is_persistent;
	bool            is_visible;
	int             layer;
	person_t*       leader;
	color_t         mask;
	int             mv_x, mv_y;
	int             revert_delay;
	int             revert_frames;
	double          scale_x;
	double          scale_y;
	script_t*       scripts[PERSON_SCRIPT_MAX];
	double          speed_x, speed_y;
	spriteset_t*    sprite;
	double          theta;
	double          x, y;
	int             x_offset, y_offset;
	int             max_commands;
	int             max_history;
	int             num_commands;
	int             num_ignores;
	struct command  *commands;
	char*           *ignores;
	struct step     *steps;
};

struct step
{
	double x, y;
};

struct command
{
	int       type;
	bool      is_immediate;
	script_t* script;
};

static duk_ret_t js_CreatePerson                 (duk_context* ctx);
static duk_ret_t js_DestroyPerson                (duk_context* ctx);
static duk_ret_t js_IsCommandQueueEmpty          (duk_context* ctx);
static duk_ret_t js_IsIgnoringPersonObstructions (duk_context* ctx);
static duk_ret_t js_IsIgnoringTileObstructions   (duk_context* ctx);
static duk_ret_t js_IsPersonVisible              (duk_context* ctx);
static duk_ret_t js_IsPersonObstructed           (duk_context* ctx);
static duk_ret_t js_IsPersonVisible              (duk_context* ctx);
static duk_ret_t js_DoesPersonExist              (duk_context* ctx);
static duk_ret_t js_GetCurrentPerson             (duk_context* ctx);
static duk_ret_t js_GetObstructingPerson         (duk_context* ctx);
static duk_ret_t js_GetObstructingTile           (duk_context* ctx);
static duk_ret_t js_GetPersonAngle               (duk_context* ctx);
static duk_ret_t js_GetPersonBase                (duk_context* ctx);
static duk_ret_t js_GetPersonData                (duk_context* ctx);
static duk_ret_t js_GetPersonDirection           (duk_context* ctx);
static duk_ret_t js_GetPersonFollowDistance      (duk_context* ctx);
static duk_ret_t js_GetPersonFollowers           (duk_context* ctx);
static duk_ret_t js_GetPersonFrame               (duk_context* ctx);
static duk_ret_t js_GetPersonFrameNext           (duk_context* ctx);
static duk_ret_t js_GetPersonFrameRevert         (duk_context* ctx);
static duk_ret_t js_GetPersonIgnoreList          (duk_context* ctx);
static duk_ret_t js_GetPersonLayer               (duk_context* ctx);
static duk_ret_t js_GetPersonLeader              (duk_context* ctx);
static duk_ret_t js_GetPersonList                (duk_context* ctx);
static duk_ret_t js_GetPersonMask                (duk_context* ctx);
static duk_ret_t js_GetPersonOffsetX             (duk_context* ctx);
static duk_ret_t js_GetPersonOffsetY             (duk_context* ctx);
static duk_ret_t js_GetPersonSpeedX              (duk_context* ctx);
static duk_ret_t js_GetPersonSpeedY              (duk_context* ctx);
static duk_ret_t js_GetPersonSpriteset           (duk_context* ctx);
static duk_ret_t js_GetPersonValue               (duk_context* ctx);
static duk_ret_t js_GetPersonX                   (duk_context* ctx);
static duk_ret_t js_GetPersonY                   (duk_context* ctx);
static duk_ret_t js_GetPersonXFloat              (duk_context* ctx);
static duk_ret_t js_GetPersonYFloat              (duk_context* ctx);
static duk_ret_t js_GetTalkDistance              (duk_context* ctx);
static duk_ret_t js_SetDefaultPersonScript       (duk_context* ctx);
static duk_ret_t js_SetPersonAngle               (duk_context* ctx);
static duk_ret_t js_SetPersonData                (duk_context* ctx);
static duk_ret_t js_SetPersonDirection           (duk_context* ctx);
static duk_ret_t js_SetPersonFollowDistance      (duk_context* ctx);
static duk_ret_t js_SetPersonFrame               (duk_context* ctx);
static duk_ret_t js_SetPersonFrameNext           (duk_context* ctx);
static duk_ret_t js_SetPersonFrameRevert         (duk_context* ctx);
static duk_ret_t js_SetPersonIgnoreList          (duk_context* ctx);
static duk_ret_t js_SetPersonLayer               (duk_context* ctx);
static duk_ret_t js_SetPersonMask                (duk_context* ctx);
static duk_ret_t js_SetPersonOffsetX             (duk_context* ctx);
static duk_ret_t js_SetPersonOffsetY             (duk_context* ctx);
static duk_ret_t js_SetPersonScaleAbsolute       (duk_context* ctx);
static duk_ret_t js_SetPersonScaleFactor         (duk_context* ctx);
static duk_ret_t js_SetPersonScript              (duk_context* ctx);
static duk_ret_t js_SetPersonSpeed               (duk_context* ctx);
static duk_ret_t js_SetPersonSpeedXY             (duk_context* ctx);
static duk_ret_t js_SetPersonSpriteset           (duk_context* ctx);
static duk_ret_t js_SetPersonValue               (duk_context* ctx);
static duk_ret_t js_SetPersonVisible             (duk_context* ctx);
static duk_ret_t js_SetPersonX                   (duk_context* ctx);
static duk_ret_t js_SetPersonXYFloat             (duk_context* ctx);
static duk_ret_t js_SetPersonY                   (duk_context* ctx);
static duk_ret_t js_SetTalkDistance              (duk_context* ctx);
static duk_ret_t js_CallDefaultPersonScript      (duk_context* ctx);
static duk_ret_t js_CallPersonScript             (duk_context* ctx);
static duk_ret_t js_ClearPersonCommands          (duk_context* ctx);
static duk_ret_t js_FollowPerson                 (duk_context* ctx);
static duk_ret_t js_IgnorePersonObstructions     (duk_context* ctx);
static duk_ret_t js_IgnoreTileObstructions       (duk_context* ctx);
static duk_ret_t js_QueuePersonCommand           (duk_context* ctx);
static duk_ret_t js_QueuePersonScript            (duk_context* ctx);

static bool does_person_exist    (unsigned int person_id);
static void set_person_direction (person_t* person, const char* direction);
static void set_person_name      (person_t* person, const char* name);
static void command_person       (person_t* person, int command);
static int  compare_persons      (const void* a, const void* b);
static bool enlarge_step_history (person_t* person, int new_size);
static bool follow_person        (person_t* person, person_t* leader, int distance);
static void free_person          (person_t* person);
static void record_step          (person_t* person);
static void sort_persons         (void);
static void update_person        (person_t* person);

static const person_t*   s_current_person = NULL;
static script_t*         s_def_scripts[PERSON_SCRIPT_MAX];
static int               s_talk_distance  = 8;
static int               s_max_persons    = 0;
static unsigned int      s_next_person_id = 0;
static int               s_num_persons    = 0;
static person_t*         *s_persons       = NULL;

void
initialize_persons_manager(void)
{
	memset(s_def_scripts, 0, PERSON_SCRIPT_MAX * sizeof(int));
	s_num_persons = s_max_persons = 0;
	s_persons = NULL;
	s_talk_distance = 8;
	s_current_person = NULL;
}

void
shutdown_persons_manager(void)
{
	int i;
	
	for (i = 0; i < s_num_persons; ++i)
		free_person(s_persons[i]);
	free(s_persons);
}

person_t*
create_person(const char* name, const char* sprite_file, bool is_persistent)
{
	point3_t  map_origin = get_map_origin();
	char*     path;
	person_t* person;

	if (++s_num_persons > s_max_persons) {
		s_max_persons = s_num_persons * 2;
		s_persons = realloc(s_persons, s_max_persons * sizeof(person_t*));
	}
	person = s_persons[s_num_persons - 1] = calloc(1, sizeof(person_t));
	person->id = s_next_person_id++;
	set_person_name(person, name);
	path = get_asset_path(sprite_file, "spritesets", false);
	person->sprite = load_spriteset(path);
	free(path);
	set_person_direction(person, person->sprite->poses[0].name->cstr);
	person->is_persistent = is_persistent;
	person->is_visible = true;
	person->x = map_origin.x;
	person->y = map_origin.y;
	person->layer = map_origin.z;
	person->speed_x = 1.0;
	person->speed_y = 1.0;
	person->anim_frames = get_sprite_frame_delay(person->sprite, person->direction, 0);
	person->mask = rgba(255, 255, 255, 255);
	person->scale_x = person->scale_y = 1.0;
	sort_persons();
	return person;
}

void
destroy_person(person_t* person)
{
	int i, j;

	// call person's destroy script before renouncing leadership
	// the destroy script may want to reassign followers (they will be orphaned otherwise), so
	// the order is important.
	call_person_script(person, PERSON_SCRIPT_ON_DESTROY, true);
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->leader == person) s_persons[i]->leader = NULL;
	}

	// remove person from the persons list
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i] == person) {
			for (j = i; j < s_num_persons - 1; ++j) s_persons[j] = s_persons[j + 1];
			--s_num_persons; --i;
		}
	}
	
	free_person(person);
	sort_persons();
}

bool
has_person_moved(const person_t* person)
{
	return person->mv_x != 0 || person->mv_y != 0;
}

bool
is_person_busy(const person_t* person)
{
	return person->num_commands > 0;
}

bool
is_person_following(const person_t* person, const person_t* leader)
{
	const person_t* node;

	node = person;
	while (node = node->leader)
		if (node == leader) return true;
	return false;
}

bool
is_person_ignored(const person_t* person, const person_t* by_person)
{
	// note: commutative; if either person ignores the other, the function should return true

	int i;

	if (by_person->ignore_all_persons || person->ignore_all_persons)
		return true;
	for (i = 0; i < by_person->num_ignores; ++i)
		if (strcmp(by_person->ignores[i], person->name) == 0) return true;
	for (i = 0; i < person->num_ignores; ++i)
		if (strcmp(person->ignores[i], by_person->name) == 0) return true;
	return false;
}

bool
is_person_obstructed_at(const person_t* person, double x, double y, person_t** out_obstructing_person, int* out_tile_index)
{
	rect_t           area;
	rect_t           base, my_base;
	double           cur_x, cur_y;
	bool             is_obstructed = false;
	int              layer;
	const obsmap_t*  obsmap;
	int              tile_w, tile_h;
	const tileset_t* tileset;

	int i, i_x, i_y;
	
	normalize_map_entity_xy(&x, &y, person->layer);
	get_person_xyz(person, &cur_x, &cur_y, &layer, true);
	my_base = translate_rect(get_person_base(person), x - cur_x, y - cur_y);
	if (out_obstructing_person) *out_obstructing_person = NULL;
	if (out_tile_index) *out_tile_index = -1;

	// check for obstructing persons
	if (!person->ignore_all_persons) {
		for (i = 0; i < s_num_persons; ++i) {
			if (s_persons[i] == person)  // these persons aren't going to obstruct themselves!
				continue;
			if (s_persons[i]->layer != layer) continue;  // ignore persons not on the same layer
			if (is_person_following(s_persons[i], person)) continue;  // ignore own followers
			if (is_person_ignored(person, s_persons[i])) continue;
			base = get_person_base(s_persons[i]);
			if (do_rects_intersect(my_base, base)) {
				is_obstructed = true;
				if (out_obstructing_person) *out_obstructing_person = s_persons[i];
				break;
			}
		}
	}

	// no obstructing person, check map-defined obstructions
	obsmap = get_map_layer_obsmap(layer);
	if (test_obsmap_rect(obsmap, my_base))
		is_obstructed = true;
	
	// check for obstructing tiles; constrain search to immediate vicinity of sprite base
	if (!person->ignore_all_tiles) {
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
	}
	
	return is_obstructed;
}

double
get_person_angle(const person_t* person)
{
	return person->theta;
}

rect_t
get_person_base(const person_t* person)
{
	rect_t base_rect;
	int    base_x, base_y;
	double x, y;

	base_rect = zoom_rect(person->sprite->base, person->scale_x, person->scale_y);
	get_person_xy(person, &x, &y, true);
	base_x = x - (base_rect.x1 + (base_rect.x2 - base_rect.x1) / 2);
	base_y = y - (base_rect.y1 + (base_rect.y2 - base_rect.y1) / 2);
	base_rect.x1 += base_x; base_rect.x2 += base_x;
	base_rect.y1 += base_y; base_rect.y2 += base_y;
	return base_rect;
}

color_t
get_person_mask(const person_t* person)
{
	return person->mask;
}

const char*
get_person_name(const person_t* person)
{
	return person->name;
}

void
get_person_scale(const person_t* person, double* out_scale_x, double* out_scale_y)
{
	*out_scale_x = person->scale_x;
	*out_scale_y = person->scale_y;
}

void
get_person_speed(const person_t* person, double* out_x_speed, double* out_y_speed)
{
	if (out_x_speed) *out_x_speed = person->speed_x;
	if (out_y_speed) *out_y_speed = person->speed_y;
}

spriteset_t*
get_person_spriteset(person_t* person)
{
	return person->sprite;
}

void
get_person_xy(const person_t* person, double* out_x, double* out_y, bool want_normalize)
{
	*out_x = person->x;
	*out_y = person->y;
	if (want_normalize)
		normalize_map_entity_xy(out_x, out_y, person->layer);
}

void
get_person_xyz(const person_t* person, double* out_x, double* out_y, int* out_layer, bool want_normalize)
{
	*out_x = person->x;
	*out_y = person->y;
	*out_layer = person->layer;
	if (want_normalize)
		normalize_map_entity_xy(out_x, out_y, *out_layer);
}

void
set_person_angle(person_t* person, double theta)
{
	person->theta = theta;
}

void
set_person_mask(person_t* person, color_t mask)
{
	person->mask = mask;
}

void
set_person_scale(person_t* person, double scale_x, double scale_y)
{
	person->scale_x = scale_x;
	person->scale_y = scale_y;
}

void
set_person_script(person_t* person, int type, script_t* script)
{
	free_script(person->scripts[type]);
	person->scripts[type] = script;
}

void
set_person_speed(person_t* person, double x_speed, double y_speed)
{
	person->speed_x = x_speed;
	person->speed_y = y_speed;
}

void
set_person_spriteset(person_t* person, spriteset_t* spriteset)
{
	spriteset_t* old_spriteset;
	
	old_spriteset = person->sprite;
	person->sprite = ref_spriteset(spriteset);
	person->anim_frames = get_sprite_frame_delay(person->sprite, person->direction, 0);
	person->frame = 0;
	free_spriteset(old_spriteset);
}

void
set_person_xyz(person_t* person, double x, double y, int layer)
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

bool
compile_person_script(person_t* person, int type, const lstring_t* codestring)
{
	char*       full_name;
	const char* person_name;
	script_t*   script;
	const char* script_name;

	person_name = get_person_name(person);
	script_name = type == PERSON_SCRIPT_ON_CREATE ? "create script"
		: type == PERSON_SCRIPT_ON_DESTROY ? "destroy script"
		: type == PERSON_SCRIPT_ON_TOUCH ? "touch script"
		: type == PERSON_SCRIPT_ON_TALK ? "talk script"
		: type == PERSON_SCRIPT_GENERATOR ? "command generator"
		: NULL;
	if (script_name == NULL) return false;
	if ((full_name = malloc(strlen(person_name) + strlen(script_name) + 11)) == NULL)
		return false;
	sprintf(full_name, "[%s : %s]", person_name, script_name);
	script = compile_script(codestring, full_name);
	set_person_script(person, type, script);
	free(full_name);
	return true;
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

bool
follow_person(person_t* person, person_t* leader, int distance)
{
	const person_t* node;

	// prevent circular follower chains from forming
	if (leader != NULL) {
		node = leader;
		do
			if (node == person) return false;
		while (node = node->leader);
	}
	
	// add the person as a follower (or sever existing link if leader==NULL)
	if (leader != NULL) {
		if (!enlarge_step_history(leader, distance))
			return false;
		person->leader = leader;
		person->follow_distance = distance;
	}
	person->leader = leader;
	return true;
}

bool
queue_person_command(person_t* person, int command, bool is_immediate)
{
	++person->num_commands;
	if (person->num_commands > person->max_commands) {
		if (!(person->commands = realloc(person->commands, person->num_commands * sizeof(struct command))))
			return false;
		person->max_commands = person->num_commands;
	}
	person->commands[person->num_commands - 1].type = command;
	person->commands[person->num_commands - 1].is_immediate = is_immediate;
	person->commands[person->num_commands - 1].script = NULL;
	return true;
}

bool
queue_person_script(person_t* person, lstring_t* script, bool is_immediate)
{
	lstring_t* script_name;
	
	++person->num_commands;
	if (person->num_commands > person->max_commands) {
		person->max_commands = person->num_commands * 2;
		if (!(person->commands = realloc(person->commands, person->max_commands * sizeof(struct command))))
			return false;
	}
	if (!(script_name = new_lstring("[%s : queued script]", person->name)))
		return false;
	person->commands[person->num_commands - 1].type = COMMAND_RUN_SCRIPT;
	person->commands[person->num_commands - 1].is_immediate = is_immediate;
	person->commands[person->num_commands - 1].script = compile_script(script, lstring_cstr(script_name));
	free_lstring(script_name);
	return true;
}

void
render_persons(int layer, bool is_flipped, int cam_x, int cam_y)
{
	person_t*    person;
	spriteset_t* sprite;
	int          w, h;
	double       x, y;
	int          i;

	for (i = 0; i < s_num_persons; ++i) {
		person = s_persons[i];
		if (!person->is_visible || person->layer != layer)
			continue;
		sprite = person->sprite;
		get_sprite_size(sprite, &w, &h);
		get_person_xy(person, &x, &y, true);
		x -= cam_x - person->x_offset;
		y -= cam_y - person->y_offset;
		draw_sprite(sprite, person->mask, is_flipped, person->theta, person->scale_x, person->scale_y,
			person->direction, x, y, person->frame);
	}
}

void
reset_persons(bool keep_existing)
{
	point3_t  map_origin;
	person_t* person;
	
	int i, j;

	map_origin = get_map_origin();
	for (i = 0; i < s_num_persons; ++i) {
		person = s_persons[i];
		if (!keep_existing)
			person->num_commands = 0;
		if (person->is_persistent || keep_existing) {
			person->x = map_origin.x;
			person->y = map_origin.y;
			person->layer = map_origin.z;
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
	double    talk_x, talk_y;

	map_rect = get_map_bounds();
	
	// check if anyone else is within earshot
	get_person_xy(person, &talk_x, &talk_y, true);
	if (strstr(person->direction, "north") != NULL) talk_y -= s_talk_distance;
	if (strstr(person->direction, "east") != NULL) talk_x += s_talk_distance;
	if (strstr(person->direction, "south") != NULL) talk_y += s_talk_distance;
	if (strstr(person->direction, "west") != NULL) talk_x -= s_talk_distance;
	is_person_obstructed_at(person, talk_x, talk_y, &target_person, NULL);
	
	// if so, call their talk script
	if (target_person != NULL)
		call_person_script(target_person, PERSON_SCRIPT_ON_TALK, true);
}

void
update_persons(void)
{
	bool is_sort_needed = false;
	
	int i;

	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->leader != NULL)
			continue;  // skip followers for now
		update_person(s_persons[i]);
		is_sort_needed |= has_person_moved(s_persons[i]);
	}
	if (is_sort_needed) sort_persons();
}

static bool
does_person_exist(unsigned int person_id)
{
	int i;

	for (i = 0; i < s_num_persons; ++i)
		if (person_id == s_persons[i]->id) return true;
	return false;
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
command_person(person_t* person, int command)
{
	double    new_x, new_y;
	person_t* person_to_touch;

	new_x = person->x; new_y = person->y;
	switch (command) {
	case COMMAND_ANIMATE:
		person->revert_frames = person->revert_delay;
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
			if (new_x != person->x)
				person->mv_x = new_x > person->x ? 1 : -1;
			if (new_y != person->y)
				person->mv_y = new_y > person->y ? 1 : -1;
			person->x = new_x; person->y = new_y;
		}
		else {
			// if not, and we collided with a person, call that person's touch script
			if (person_to_touch != NULL)
				call_person_script(person_to_touch, PERSON_SCRIPT_ON_TOUCH, true);
		}
	}
}

static int
compare_persons(const void* a, const void* b)
{
	person_t* p1 = *(person_t**)a;
	person_t* p2 = *(person_t**)b;

	int y_delta;

	y_delta = (p1->y + p1->y_offset) - (p2->y + p2->y_offset);
	if (y_delta != 0)
		return y_delta;
	else if (is_person_following(p1, p2))
		return -1;
	else if (is_person_following(p2, p1))
		return 1;
	else
		return p1->id - p2->id;
}

static void
free_person(person_t* person)
{
	int i;

	free(person->steps);
	for (i = 0; i < PERSON_SCRIPT_MAX; ++i)
		free_script(person->scripts[i]);
	free_spriteset(person->sprite);
	free(person->name);
	free(person->direction);
	free(person);
}

static void
record_step(person_t* person)
{
	struct step* p_step;

	if (person->max_history <= 0)
		return;
	memmove(&person->steps[1], &person->steps[0], (person->max_history - 1) * sizeof(struct step));
	p_step = &person->steps[0];
	p_step->x = person->x;
	p_step->y = person->y;
}

static bool
enlarge_step_history(person_t* person, int new_size)
{
	struct step *new_steps;

	int i;
	
	if (new_size > person->max_history) {
		if (!(new_steps = realloc(person->steps, new_size * sizeof(struct step))))
			return false;

		// when enlarging the history buffer, we fill new slots with pastmost values
		// (kind of like sign extension)
		for (i = person->max_history; i < new_size; ++i) {
			new_steps[i].x = person->steps != NULL
				? person->steps[person->max_history - 1].x
				: person->x;
			new_steps[i].y = person->steps != NULL
				? person->steps[person->max_history - 1].y
				: person->y;
		}
		person->steps = new_steps;
		person->max_history = new_size;
	}
	
	return true;
}

static void
sort_persons(void)
{
	qsort(s_persons, s_num_persons, sizeof(person_t*), compare_persons);
}

static void
update_person(person_t* person)
{
	struct command  command;
	double          delta_x, delta_y;
	int             facing;
	bool            is_finished;
	const person_t* last_person;
	unsigned int    person_id;
	struct step     step;
	int             vector;

	int i;

	person_id = person->id;
	person->mv_x = 0; person->mv_y = 0;
	if (person->revert_frames > 0 && --person->revert_frames <= 0)
		person->frame = 0;
	if (person->leader == NULL) {  // no leader; use command queue
		// call the command generator if the queue is empty
		if (person->num_commands == 0)
			call_person_script(person, PERSON_SCRIPT_GENERATOR, true);

		// run through the queue, stopping after the first non-immediate command
		is_finished = person->num_commands == 0;
		while (!is_finished) {
			command = person->commands[0];
			--person->num_commands;
			for (i = 0; i < person->num_commands; ++i)
				person->commands[i] = person->commands[i + 1];
			last_person = s_current_person;
			s_current_person = person;
			if (command.type != COMMAND_RUN_SCRIPT)
				command_person(person, command.type);
			else
				run_script(command.script, false);
			s_current_person = last_person;
			free_script(command.script);
			is_finished = !does_person_exist(person_id)  // stop if person was destroyed
				|| !command.is_immediate || person->num_commands == 0;
		}
	}
	else {  // leader set; follow the leader!
		step = person->leader->steps[person->follow_distance - 1];
		delta_x = step.x - person->x;
		delta_y = step.y - person->y;
		if (fabs(delta_x) > person->speed_x)
			command_person(person, delta_x > 0 ? COMMAND_MOVE_EAST : COMMAND_MOVE_WEST);
		if (fabs(delta_y) > person->speed_y)
			command_person(person, delta_y > 0 ? COMMAND_MOVE_SOUTH : COMMAND_MOVE_NORTH);
		vector = person->mv_x + person->mv_y * 3;
		facing = vector == -3 ? COMMAND_FACE_NORTH
			: vector == -2 ? COMMAND_FACE_NORTHEAST
			: vector == 1 ? COMMAND_FACE_EAST
			: vector == 4 ? COMMAND_FACE_SOUTHEAST
			: vector == 3 ? COMMAND_FACE_SOUTH
			: vector == 2 ? COMMAND_FACE_SOUTHWEST
			: vector == -1 ? COMMAND_FACE_WEST
			: vector == -4 ? COMMAND_FACE_NORTHWEST
			: COMMAND_WAIT;
		command_person(person, facing);
	}

	// if the person's position changed, record it in their step history
	if (has_person_moved(person)) {
		record_step(person);
	}

	// recursively update follower chain
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->leader != person)
			continue;
		update_person(s_persons[i]);
	}
}

void
init_persons_api(void)
{
	duk_push_global_stash(g_duktape);
	duk_push_object(g_duktape); duk_put_prop_string(g_duktape, -2, "person_data");
	duk_pop(g_duktape);

	register_api_func(g_duktape, NULL, "CreatePerson", js_CreatePerson);
	register_api_func(g_duktape, NULL, "DestroyPerson", js_DestroyPerson);
	register_api_func(g_duktape, NULL, "IsCommandQueueEmpty", js_IsCommandQueueEmpty);
	register_api_func(g_duktape, NULL, "IsIgnoringPersonObstructions", js_IsIgnoringPersonObstructions);
	register_api_func(g_duktape, NULL, "IsIgnoringTileObstructions", js_IsIgnoringTileObstructions);
	register_api_func(g_duktape, NULL, "IsPersonObstructed", js_IsPersonObstructed);
	register_api_func(g_duktape, NULL, "IsPersonVisible", js_IsPersonVisible);
	register_api_func(g_duktape, NULL, "DoesPersonExist", js_DoesPersonExist);
	register_api_func(g_duktape, NULL, "GetCurrentPerson", js_GetCurrentPerson);
	register_api_func(g_duktape, NULL, "GetObstructingPerson", js_GetObstructingPerson);
	register_api_func(g_duktape, NULL, "GetObstructingTile", js_GetObstructingTile);
	register_api_func(g_duktape, NULL, "GetPersonAngle", js_GetPersonAngle);
	register_api_func(g_duktape, NULL, "GetPersonBase", js_GetPersonBase);
	register_api_func(g_duktape, NULL, "GetPersonData", js_GetPersonData);
	register_api_func(g_duktape, NULL, "GetPersonDirection", js_GetPersonDirection);
	register_api_func(g_duktape, NULL, "GetPersonFollowDistance", js_GetPersonFollowDistance);
	register_api_func(g_duktape, NULL, "GetPersonFollowers", js_GetPersonFollowers);
	register_api_func(g_duktape, NULL, "GetPersonFrame", js_GetPersonFrame);
	register_api_func(g_duktape, NULL, "GetPersonFrameNext", js_GetPersonFrameNext);
	register_api_func(g_duktape, NULL, "GetPersonFrameRevert", js_GetPersonFrameRevert);
	register_api_func(g_duktape, NULL, "GetPersonIgnoreList", js_GetPersonIgnoreList);
	register_api_func(g_duktape, NULL, "GetPersonLayer", js_GetPersonLayer);
	register_api_func(g_duktape, NULL, "GetPersonLeader", js_GetPersonLeader);
	register_api_func(g_duktape, NULL, "GetPersonList", js_GetPersonList);
	register_api_func(g_duktape, NULL, "GetPersonMask", js_GetPersonMask);
	register_api_func(g_duktape, NULL, "GetPersonOffsetX", js_GetPersonOffsetX);
	register_api_func(g_duktape, NULL, "GetPersonOffsetY", js_GetPersonOffsetY);
	register_api_func(g_duktape, NULL, "GetPersonSpeedX", js_GetPersonSpeedX);
	register_api_func(g_duktape, NULL, "GetPersonSpeedY", js_GetPersonSpeedY);
	register_api_func(g_duktape, NULL, "GetPersonSpriteset", js_GetPersonSpriteset);
	register_api_func(g_duktape, NULL, "GetPersonValue", js_GetPersonValue);
	register_api_func(g_duktape, NULL, "GetPersonX", js_GetPersonX);
	register_api_func(g_duktape, NULL, "GetPersonXFloat", js_GetPersonXFloat);
	register_api_func(g_duktape, NULL, "GetPersonY", js_GetPersonY);
	register_api_func(g_duktape, NULL, "GetPersonYFloat", js_GetPersonYFloat);
	register_api_func(g_duktape, NULL, "GetTalkDistance", js_GetTalkDistance);
	register_api_func(g_duktape, NULL, "SetDefaultPersonScript", js_SetDefaultPersonScript);
	register_api_func(g_duktape, NULL, "SetPersonAngle", js_SetPersonAngle);
	register_api_func(g_duktape, NULL, "SetPersonData", js_SetPersonData);
	register_api_func(g_duktape, NULL, "SetPersonDirection", js_SetPersonDirection);
	register_api_func(g_duktape, NULL, "SetPersonFollowDistance", js_SetPersonFollowDistance);
	register_api_func(g_duktape, NULL, "SetPersonFrame", js_SetPersonFrame);
	register_api_func(g_duktape, NULL, "SetPersonFrameNext", js_SetPersonFrameNext);
	register_api_func(g_duktape, NULL, "SetPersonFrameRevert", js_SetPersonFrameRevert);
	register_api_func(g_duktape, NULL, "SetPersonIgnoreList", js_SetPersonIgnoreList);
	register_api_func(g_duktape, NULL, "SetPersonLayer", js_SetPersonLayer);
	register_api_func(g_duktape, NULL, "SetPersonMask", js_SetPersonMask);
	register_api_func(g_duktape, NULL, "SetPersonOffsetX", js_SetPersonOffsetX);
	register_api_func(g_duktape, NULL, "SetPersonOffsetY", js_SetPersonOffsetY);
	register_api_func(g_duktape, NULL, "SetPersonScaleAbsolute", js_SetPersonScaleAbsolute);
	register_api_func(g_duktape, NULL, "SetPersonScaleFactor", js_SetPersonScaleFactor);
	register_api_func(g_duktape, NULL, "SetPersonScript", js_SetPersonScript);
	register_api_func(g_duktape, NULL, "SetPersonSpeed", js_SetPersonSpeed);
	register_api_func(g_duktape, NULL, "SetPersonSpeedXY", js_SetPersonSpeedXY);
	register_api_func(g_duktape, NULL, "SetPersonSpriteset", js_SetPersonSpriteset);
	register_api_func(g_duktape, NULL, "SetPersonValue", js_SetPersonValue);
	register_api_func(g_duktape, NULL, "SetPersonVisible", js_SetPersonVisible);
	register_api_func(g_duktape, NULL, "SetPersonX", js_SetPersonX);
	register_api_func(g_duktape, NULL, "SetPersonXYFloat", js_SetPersonXYFloat);
	register_api_func(g_duktape, NULL, "SetPersonY", js_SetPersonY);
	register_api_func(g_duktape, NULL, "SetTalkDistance", js_SetTalkDistance);
	register_api_func(g_duktape, NULL, "CallDefaultPersonScript", js_CallDefaultPersonScript);
	register_api_func(g_duktape, NULL, "CallPersonScript", js_CallPersonScript);
	register_api_func(g_duktape, NULL, "ClearPersonCommands", js_ClearPersonCommands);
	register_api_func(g_duktape, NULL, "FollowPerson", js_FollowPerson);
	register_api_func(g_duktape, NULL, "IgnorePersonObstructions", js_IgnorePersonObstructions);
	register_api_func(g_duktape, NULL, "IgnoreTileObstructions", js_IgnoreTileObstructions);
	register_api_func(g_duktape, NULL, "QueuePersonCommand", js_QueuePersonCommand);
	register_api_func(g_duktape, NULL, "QueuePersonScript", js_QueuePersonScript);

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
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "person_data");
	duk_push_object(ctx); duk_put_prop_string(ctx, -2, name);
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
js_DestroyPerson(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "DestroyPerson(): Person '%s' doesn't exist", name);
	destroy_person(person);
	return 0;
}

static duk_ret_t
js_IsCommandQueueEmpty(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "IsCommandQueueEmpty(): Person '%s' doesn't exist", name);
	duk_push_boolean(ctx, person->num_commands <= 0);
	return 1;
}

static duk_ret_t
js_IsIgnoringPersonObstructions(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "IsIgnoringPersonObstructions(): Person '%s' doesn't exist", name);
	duk_push_boolean(ctx, person->ignore_all_persons);
	return 1;
}

static duk_ret_t
js_IsIgnoringTileObstructions(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "IsIgnoringTileObstructions(): Person '%s' doesn't exist", name);
	duk_push_boolean(ctx, person->ignore_all_tiles);
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
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "IsPersonObstructed(): Person '%s' doesn't exist", name);
	duk_push_boolean(ctx, is_person_obstructed_at(person, x, y, NULL, NULL));
	return 1;
}

static duk_ret_t
js_IsPersonVisible(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonVisible(): Person '%s' doesn't exist", name);
	duk_push_boolean(ctx, person->is_visible);
	return 1;
}

static duk_ret_t
js_GetCurrentPerson(duk_context* ctx)
{
	if (s_current_person == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetCurrentPerson(): Must be called from a person script");
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

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetObstructingPerson(): Map engine must be running");
	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetObstructingPerson(): Person '%s' doesn't exist", name);
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

	if (!is_map_engine_running())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetObstructingTile(): Map engine must be running");
	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetObstructingTile(): Person '%s' doesn't exist", name);
	is_person_obstructed_at(person, x, y, NULL, &tile_index);
	duk_push_int(ctx, tile_index);
	return 1;
}

static duk_ret_t
js_GetPersonAngle(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonAngle(): Person '%s' doesn't exist", name);
	duk_push_number(ctx, get_person_angle(person));
	return 1;
}

static duk_ret_t
js_GetPersonBase(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	rect_t    base;
	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonDirection(): Person '%s' doesn't exist", name);
	base = get_sprite_base(get_person_spriteset(person));
	duk_push_object(ctx);
	duk_push_int(ctx, base.x1); duk_put_prop_string(ctx, -2, "x1");
	duk_push_int(ctx, base.y1); duk_put_prop_string(ctx, -2, "y1");
	duk_push_int(ctx, base.x2); duk_put_prop_string(ctx, -2, "x2");
	duk_push_int(ctx, base.y2); duk_put_prop_string(ctx, -2, "y2");
	return 1;
}

static duk_ret_t
js_GetPersonData(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	int          num_directions;
	int          num_frames;
	person_t*    person;
	spriteset_t* spriteset;
	int          width, height;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonData(): Person '%s' doesn't exist", name);
	spriteset = person->sprite;
	get_sprite_size(spriteset, &width, &height);
	get_spriteset_info(spriteset, NULL, &num_directions);
	get_spriteset_pose_info(spriteset, person->direction, &num_frames);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "person_data");
	duk_get_prop_string(ctx, -1, name);
	duk_push_int(ctx, num_frames); duk_put_prop_string(ctx, -2, "num_frames");
	duk_push_int(ctx, num_directions); duk_put_prop_string(ctx, -2, "num_directions");
	duk_push_int(ctx, width); duk_put_prop_string(ctx, -2, "width");
	duk_push_int(ctx, height); duk_put_prop_string(ctx, -2, "height");
	duk_push_string(ctx, person->leader ? person->leader->name : ""); duk_put_prop_string(ctx, -2, "leader");
	duk_remove(ctx, -2); duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_GetPersonDirection(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	
	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonDirection(): Person '%s' doesn't exist", name);
	duk_push_string(ctx, person->direction);
	return 1;
}

static duk_ret_t
js_GetPersonFollowDistance(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonFollowDistance(): Person '%s' doesn't exist", name);
	if (person->leader == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "GetPersonFollowDistance(): Person '%s' is not following anyone", name);
	duk_push_int(ctx, person->follow_distance);
	return 1;
}

static duk_ret_t
js_GetPersonFollowers(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	duk_uarridx_t index = 0;
	person_t*     person;

	int i;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonFollowers(): Person '%s' doesn't exist", name);
	duk_push_array(ctx);
	for (i = 0; i < s_num_persons; ++i) {
		if (s_persons[i]->leader == person) {
			duk_push_string(ctx, s_persons[i]->name);
			duk_put_prop_index(ctx, -2, index++);
		}
	}
	return 1;
}

static duk_ret_t
js_GetPersonFrame(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	int       num_frames;
	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonFrame(): Person '%s' doesn't exist", name);
	get_spriteset_pose_info(person->sprite, person->direction, &num_frames);
	duk_push_int(ctx, person->frame % num_frames);
	return 1;
}

static duk_ret_t
js_GetPersonFrameNext(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonFrame(): Person '%s' doesn't exist", name);
	duk_push_int(ctx, person->anim_frames);
	return 1;
}

static duk_ret_t
js_GetPersonFrameRevert(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonFrameRevert(): Person '%s' doesn't exist", name);
	duk_push_int(ctx, person->revert_delay);
	return 1;
}

static duk_ret_t
js_GetPersonIgnoreList(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	int i;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonIgnoreList(): Person '%s' doesn't exist", name);
	duk_push_array(ctx);
	for (i = 0; i < person->num_ignores; ++i) {
		duk_push_string(ctx, person->ignores[i]);
		duk_put_prop_index(ctx, -2, i);
	}
	return 1;
}

static duk_ret_t
js_GetPersonLayer(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	
	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonLayer(): Person '%s' doesn't exist", name);
	duk_push_int(ctx, person->layer);
	return 1;
}

static duk_ret_t
js_GetPersonLeader(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonLeader(): Person '%s' doesn't exist", name);
	duk_push_string(ctx, person->leader != NULL ? person->leader->name : "");
	return 1;
}

static duk_ret_t
js_GetPersonMask(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonMask(): Person '%s' doesn't exist", name);
	duk_push_sphere_color(ctx, get_person_mask(person));
	return 1;
}

static duk_ret_t
js_GetPersonOffsetX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonOffsetX(): Person '%s' doesn't exist", name);
	duk_push_int(ctx, person->x_offset);
	return 1;
}

static duk_ret_t
js_GetPersonOffsetY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonOffsetY(): Person '%s' doesn't exist", name);
	duk_push_int(ctx, person->y_offset);
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
js_GetPersonSpriteset(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	spriteset_t* new_spriteset;
	person_t*    person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonSpriteset(): Person '%s' doesn't exist", name);
	if ((new_spriteset = clone_spriteset(get_person_spriteset(person))) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetPersonSpriteset(): Failed to create new spriteset");
	duk_push_sphere_spriteset(ctx, new_spriteset);
	free_spriteset(new_spriteset);
	return 1;
}

static duk_ret_t
js_GetPersonSpeedX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*  person;
	double     x_speed;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonSpeedX(): Person '%s' doesn't exist", name);
	get_person_speed(person, &x_speed, NULL);
	duk_push_number(ctx, x_speed);
	return 1;
}

static duk_ret_t
js_GetPersonSpeedY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*  person;
	double     y_speed;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonSpeedY(): Person '%s' doesn't exist", name);
	get_person_speed(person, NULL, &y_speed);
	duk_push_number(ctx, y_speed);
	return 1;
}

static duk_ret_t
js_GetPersonValue(duk_context* ctx)
{
	duk_require_type_mask(ctx, 1, DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER);
	const char* name = duk_require_string(ctx, 0);
	const char* key = duk_to_string(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonValue(): Person '%s' doesn't exist", name);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "person_data");
	if (!duk_get_prop_string(ctx, -1, name)) {
		duk_pop(ctx);
		duk_push_object(ctx);
		duk_put_prop_string(ctx, -2, name);
		duk_get_prop_string(ctx, -1, name);
	}
	duk_get_prop_string(ctx, -1, key);
	duk_remove(ctx, -2); duk_remove(ctx, -2); duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_GetPersonX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	
	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonX(): Person '%s' doesn't exist", name);
	duk_push_int(ctx, person->x);
	return 1;
}

static duk_ret_t
js_GetPersonXFloat(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonXFloat(): Person '%s' doesn't exist", name);
	duk_push_number(ctx, person->x);
	return 1;
}

static duk_ret_t
js_GetPersonY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonY(): Person '%s' doesn't exist", name);
	duk_push_int(ctx, person->y);
	return 1;
}

static duk_ret_t
js_GetPersonYFloat(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "GetPersonYFloat(): Person '%s' doesn't exist", name);
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
	const char* script_name = (type == PERSON_SCRIPT_ON_CREATE) ? "[default create person script]"
		: (type == PERSON_SCRIPT_ON_DESTROY) ? "[default destroy person script]"
		: (type == PERSON_SCRIPT_ON_TALK) ? "[default talk script]"
		: (type == PERSON_SCRIPT_ON_TOUCH) ? "[default touch person script]"
		: (type == PERSON_SCRIPT_GENERATOR) ? "[default command generator]"
		: NULL;
	script_t* script = duk_require_sphere_script(ctx, 1, script_name);

	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetDefaultPersonScript(): Invalid script type constant");
	free_script(s_def_scripts[type]);
	s_def_scripts[type] = script;
	return 0;
}

static duk_ret_t
js_SetPersonAngle(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	double theta = duk_require_number(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonAngle(): Person '%s' doesn't exist", name);
	set_person_angle(person, theta);
	return 0;
}

static duk_ret_t
js_SetPersonData(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	duk_require_object_coercible(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonData(): Person '%s' doesn't exist", name);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "person_data");
	duk_dup(ctx, 1); duk_put_prop_string(ctx, -2, name);
	return 0;
}

static duk_ret_t
js_SetPersonDirection(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	const char* new_dir = duk_require_string(ctx, 1);

	person_t*   person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonDirection(): Person '%s' doesn't exist", name);
	set_person_direction(person, new_dir);
	return 0;
}

static duk_ret_t
js_SetPersonFollowDistance(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int distance = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonFollowDistance(): Person '%s' doesn't exist", name);
	if (person->leader == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "SetPersonFollowDistance(): Person '%s' is not following anyone", name);
	if (distance <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetPersonFollowDistance(): Distance must be greater than zero (%i)", distance);
	if (!enlarge_step_history(person->leader, distance))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetPersonFollowDistance(): Failed to enlarge leader's tracking buffer (internal error)");
	person->follow_distance = distance;
	return 0;
}

static duk_ret_t
js_SetPersonFrame(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int frame_index = duk_require_int(ctx, 1);

	int       num_frames;
	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonFrame(): Person '%s' doesn't exist", name);
	get_spriteset_pose_info(person->sprite, person->direction, &num_frames);
	person->frame = (frame_index % num_frames + num_frames) % num_frames;
	person->anim_frames = get_sprite_frame_delay(person->sprite, person->direction, person->frame);
	person->revert_frames = person->revert_delay;
	return 0;
}

static duk_ret_t
js_SetPersonFrameNext(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int frames = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonFrameRevert(): Person '%s' doesn't exist", name);
	if (frames < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetPersonFrameNext(): Negative delay not allowed (%i)", frames);
	person->anim_frames = frames;
	person->revert_frames = person->revert_delay;
	return 0;
}

static duk_ret_t
js_SetPersonFrameRevert(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int frames = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonFrameRevert(): Person '%s' doesn't exist", name);
	if (frames < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetPersonFrameRevert(): Negative delay not allowed (%i)", frames);
	person->revert_delay = frames;
	person->revert_frames = person->revert_delay;
	return 0;
}

static duk_ret_t
js_SetPersonIgnoreList(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	duk_require_object_coercible(ctx, 1);

	size_t    list_size;
	person_t* person;

	int i;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonIgnoreList(): Person '%s' doesn't exist", name);
	if (!duk_is_array(ctx, 1))
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetPersonIgnoreList(): ignore_list argument must be an array");
	list_size = duk_get_length(ctx, 1);
	if (list_size > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetPersonIgnoreList(): List too large (internal error)");
	for (i = 0; i < person->num_ignores; ++i) {
		free(person->ignores[i]);
	}
	person->ignores = realloc(person->ignores, list_size * sizeof(char*));
	person->num_ignores = (int)list_size;
	for (i = 0; i < (int)list_size; ++i) {
		duk_get_prop_index(ctx, 1, (duk_uarridx_t)i);
		person->ignores[i] = strdup(duk_require_string(ctx, -1));
		duk_pop(ctx);
	}
	return 0;
}

static duk_ret_t
js_SetPersonLayer(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int layer = duk_require_map_layer(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonLayer(): Person '%s' doesn't exist", name);
	person->layer = layer;
	return 0;
}

static duk_ret_t
js_SetPersonMask(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	color_t mask = duk_require_sphere_color(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonMask(): Person '%s' doesn't exist", name);
	set_person_mask(person, mask);
	return 0;
}

static duk_ret_t
js_SetPersonOffsetX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int offset = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonOffsetX(): Person '%s' doesn't exist", name);
	person->x_offset = offset;
	return 0;
}

static duk_ret_t
js_SetPersonOffsetY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int offset = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonOffsetY(): Person '%s' doesn't exist", name);
	person->y_offset = offset;
	return 0;
}

static duk_ret_t
js_SetPersonScaleAbsolute(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int width = duk_require_int(ctx, 1);
	int height = duk_require_int(ctx, 2);

	person_t* person;
	int       sprite_w, sprite_h;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonScaleAbsolute(): Person '%s' doesn't exist", name);
	if (width < 0 || height < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetPersonScaleAbsolute(): Scale dimensions cannot be negative ({ w: %i, h: %i })", width, height);
	get_sprite_size(get_person_spriteset(person), &sprite_w, &sprite_h);
	set_person_scale(person, width / sprite_w, height / sprite_h);
	return 0;
}

static duk_ret_t
js_SetPersonScaleFactor(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	double scale_x = duk_require_number(ctx, 1);
	double scale_y = duk_require_number(ctx, 2);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonScaleFactor(): Person '%s' doesn't exist", name);
	if (scale_x < 0.0 || scale_y < 0.0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetPersonScaleFactor(): Scaling factors cannot be negative ({ scale_x: %f, scale_y: %f })", scale_x, scale_y);
	set_person_scale(person, scale_x, scale_y);
	return 0;
}

static duk_ret_t
js_SetPersonScript(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int type = duk_require_int(ctx, 1);
	
	lstring_t* codestring;
	person_t*  person;
	script_t*  script;
	
	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonScript(): Person '%s' doesn't exist", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetPersonScript(): Invalid script type constant");
	if (duk_is_string(ctx, 2)) {
		codestring = duk_require_lstring_t(ctx, 2);
		compile_person_script(person, type, codestring);
		free_lstring(codestring);
	}
	else {
		script = duk_require_sphere_script(ctx, 2, "[person script]");
		set_person_script(person, type, script);
	}
	return 0;
}

static duk_ret_t
js_SetPersonSpeed(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	double speed = duk_require_number(ctx, 1);

	person_t*  person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonSpeed(): Person '%s' doesn't exist", name);
	set_person_speed(person, speed, speed);
	return 0;
}

static duk_ret_t
js_SetPersonSpeedXY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	double x_speed = duk_require_number(ctx, 1);
	double y_speed = duk_require_number(ctx, 2);

	person_t*  person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonSpeed(): Person '%s' doesn't exist", name);
	set_person_speed(person, x_speed, y_speed);
	return 0;
}

static duk_ret_t
js_SetPersonSpriteset(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	spriteset_t* spriteset = duk_require_sphere_spriteset(ctx, 1);

	spriteset_t* new_spriteset;
	person_t*    person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonSpriteset(): Person '%s' doesn't exist", name);
	if ((new_spriteset = clone_spriteset(spriteset)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "SetPersonSpriteset(): Failed to create new spriteset");
	set_person_spriteset(person, new_spriteset);
	free_spriteset(new_spriteset);
	return 0;
}

static duk_ret_t
js_SetPersonValue(duk_context* ctx)
{
	duk_require_type_mask(ctx, 1, DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER);
	const char* name = duk_require_string(ctx, 0);
	const char* key = duk_to_string(ctx, 1);
	duk_require_valid_index(ctx, 2);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonValue(): Person '%s' doesn't exist", name);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "person_data");
	if (!duk_get_prop_string(ctx, -1, name)) {
		duk_pop(ctx);
		duk_push_object(ctx);
		duk_put_prop_string(ctx, -2, name);
		duk_get_prop_string(ctx, -1, name);
	}
	duk_dup(ctx, 2); duk_put_prop_string(ctx, -2, key);
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
js_SetPersonVisible(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	bool is_visible = duk_require_boolean(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonVisible(): Person '%s' doesn't exist", name);
	person->is_visible = is_visible;
	return 0;
}

static duk_ret_t
js_SetPersonX(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int x = duk_require_int(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonX(): Person '%s' doesn't exist", name);
	person->x = x;
	return 0;
}

static duk_ret_t
js_SetPersonXYFloat(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	double x = duk_require_number(ctx, 1);
	double y = duk_require_number(ctx, 2);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonXYFloat(): Person '%s' doesn't exist", name);
	person->x = x; person->y = y;
	return 0;
}

static duk_ret_t
js_SetPersonY(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int y = duk_require_int(ctx, 1);
	
	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "SetPersonY(): Person '%s' doesn't exist", name);
	person->y = y;
	return 0;
}

static duk_ret_t
js_SetTalkDistance(duk_context* ctx)
{
	int pixels = duk_require_int(ctx, 0);

	if (pixels < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetTalkDistance(): Negative distance not allowed (caller passed %i)", pixels);
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
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "CallDefaultPersonScript(): Person '%s' doesn't exist", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CallDefaultPersonScript(): Invalid script type constant");
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
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "CallPersonScript(): Person '%s' doesn't exist", name);
	if (type < 0 || type >= PERSON_SCRIPT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "CallPersonScript(): Invalid script type constant");
	call_person_script(person, type, false);
	return 0;
}

static duk_ret_t
js_ClearPersonCommands(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "ClearPersonCommands(): Person '%s' doesn't exist", name);
	person->num_commands = 0;
	return 0;
}

static duk_ret_t
js_FollowPerson(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	const char* leader_name = duk_is_null(ctx, 1) ? "" : duk_require_string(ctx, 1);
	int distance = leader_name[0] != '\0' ? duk_require_int(ctx, 2) : 0;

	person_t* leader = NULL;
	person_t* person;

	if (!(person = find_person(name)))
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "FollowPerson(): Person '%s' doesn't exist", name);
	if (!(leader_name[0] == '\0' || (leader = find_person(leader_name))))
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "FollowPerson(): Person '%s' doesn't exist", leader_name);
	if (distance <= 0 && leader_name[0] != '\0')
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "FollowPerson(): Distance must be greater than zero (%i)", distance);
	if (!follow_person(person, leader, distance))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "FollowPerson(): Circular chain is not allowed");
	return 0;
}

static duk_ret_t
js_IgnorePersonObstructions(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	bool is_ignoring = duk_require_boolean(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "IgnorePersonObstructions(): Person '%s' doesn't exist", name);
	person->ignore_all_persons = is_ignoring;
	return 0;
}

static duk_ret_t
js_IgnoreTileObstructions(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	bool is_ignoring = duk_require_boolean(ctx, 1);

	person_t* person;

	if ((person = find_person(name)) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "IgnoreTileObstructions(): Person '%s' doesn't exist", name);
	person->ignore_all_tiles = is_ignoring;
	return 0;
}

static duk_ret_t
js_QueuePersonCommand(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	int command = duk_require_int(ctx, 1);
	bool is_immediate = duk_require_boolean(ctx, 2);

	person_t* person;

	if (!(person = find_person(name)))
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "QueuePersonCommand(): Person '%s' doesn't exist", name);
	if (command < 0 || command >= COMMAND_RUN_SCRIPT)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "QueuePersonCommand(): Invalid command type constant");
	if (!queue_person_command(person, command, is_immediate))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "QueuePersonCommand(): Failed to enlarge person's command queue (internal error)");
	return 0;
}

static duk_ret_t
js_QueuePersonScript(duk_context* ctx)
{
	const char* name = duk_require_string(ctx, 0);
	lstring_t* script = duk_require_lstring_t(ctx, 1);
	bool is_immediate = duk_require_boolean(ctx, 2);

	person_t* person;

	if (!(person = find_person(name)))
		duk_error_ni(ctx, -1, DUK_ERR_REFERENCE_ERROR, "QueuePersonScript(): Person '%s' doesn't exist", name);
	if (!queue_person_script(person, script, is_immediate))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "QueuePersonScript(): Failed to enqueue script (internal error)");
	free_lstring(script);
	return 0;
}
