#include "spriteset.h"

typedef struct person person_t;

struct person
{
	char*        name;
	char*        direction;
	int          layer;
	int          frame;
	float        speed;
	spriteset_t* sprite;
	float        x, y;
};

extern void      init_person_api (void);
extern person_t* find_person     (const char* name);
extern void      render_persons  (int cam_x, int cam_y);
