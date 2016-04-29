// the declaration below has been deliberately hoisted outside of the
// include guard; as there is a mutual dependency between the map engine
// and persons manager, the engine won't compile otherwise.
typedef struct person person_t;

#ifndef MINISPHERE__PERSONS_H__INCLUDED
#define MINISPHERE__PERSONS_H__INCLUDED

#include "map_engine.h"
#include "spriteset.h"

typedef
enum command_op
{
	COMMAND_WAIT,
	COMMAND_ANIMATE,
	COMMAND_FACE_NORTH,
	COMMAND_FACE_NORTHEAST,
	COMMAND_FACE_EAST,
	COMMAND_FACE_SOUTHEAST,
	COMMAND_FACE_SOUTH,
	COMMAND_FACE_SOUTHWEST,
	COMMAND_FACE_WEST,
	COMMAND_FACE_NORTHWEST,
	COMMAND_MOVE_NORTH,
	COMMAND_MOVE_NORTHEAST,
	COMMAND_MOVE_EAST,
	COMMAND_MOVE_SOUTHEAST,
	COMMAND_MOVE_SOUTH,
	COMMAND_MOVE_SOUTHWEST,
	COMMAND_MOVE_WEST,
	COMMAND_MOVE_NORTHWEST,
	COMMAND_RUN_SCRIPT
} command_op_t;

typedef
enum person_script
{
	PERSON_SCRIPT_ON_CREATE,
	PERSON_SCRIPT_ON_DESTROY,
	PERSON_SCRIPT_ON_TOUCH,
	PERSON_SCRIPT_ON_TALK,
	PERSON_SCRIPT_GENERATOR,
	PERSON_SCRIPT_MAX
} person_script_t;

void         initialize_persons_manager (void);
void         shutdown_persons_manager   (void);
person_t*    create_person              (const char* name, spriteset_t* spriteset, bool is_persistent, script_t* create_script);
void         destroy_person             (person_t* person);
bool         has_person_moved           (const person_t* person);
bool         is_person_busy             (const person_t* person);
bool         is_person_following        (const person_t* person, const person_t* leader);
bool         is_person_ignored          (const person_t* person, const person_t* by_person);
bool         is_person_obstructed_at    (const person_t* person, double x, double y, person_t** out_obstructing_person, int* out_tile_index);
double       get_person_angle           (const person_t* person);
rect_t       get_person_base            (const person_t* person);
color_t      get_person_mask            (const person_t* person);
const char*  get_person_name            (const person_t* person);
void         get_person_scale           (const person_t*, double* out_scale_x, double* out_scale_y);
void         get_person_speed           (const person_t* person, double* out_x_speed, double* out_y_speed);
spriteset_t* get_person_spriteset       (person_t* person);
void         get_person_xy              (const person_t* person, double* out_x, double* out_y, bool normalize);
void         get_person_xyz             (const person_t* person, double* out_x, double* out_y, int* out_layer, bool want_normalize);
void         set_person_angle           (person_t* person, double theta);
void         set_person_mask            (person_t* person, color_t mask);
void         set_person_scale           (person_t*, double scale_x, double scale_y);
void         set_person_script          (person_t* person, int type, script_t* script);
void         set_person_speed           (person_t* person, double x_speed, double y_speed);
void         set_person_spriteset       (person_t* person, spriteset_t* spriteset);
void         set_person_xyz             (person_t* person, double x, double y, int layer);
bool         call_person_script         (const person_t* person, int type, bool use_default);
bool         compile_person_script      (person_t* person, int type, const lstring_t* codestring);
person_t*    find_person                (const char* name);
bool         queue_person_command       (person_t* person, int command, bool is_immediate);
bool         queue_person_script        (person_t* person, script_t* script, bool is_immediate);
void         reset_persons              (bool keep_existing);
void         render_persons             (int layer, bool is_flipped, int cam_x, int cam_y);
void         talk_person                (const person_t* person);
void         update_persons             (void);

void init_persons_api (void);

#endif // MINISPHERE__PERSONS_H__INCLUDED
