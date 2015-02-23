#include "spriteset.h"

typedef struct person person_t;

extern void      init_person_api (void);
extern void      get_person_xy   (const person_t* person, float* out_x, float* out_y, int map_width, int map_height);
extern void      command_person  (person_t* person, int command);
extern person_t* find_person     (const char* name);
extern void      render_persons  (int cam_x, int cam_y);
extern void      update_persons  (void);

enum person_cmd
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
	COMMAND_MOVE_NORTHWEST
};
