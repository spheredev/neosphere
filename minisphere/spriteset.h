typedef struct spriteset       spriteset_t;
typedef struct spriteset_pose  spriteset_pose_t;
typedef struct spriteset_frame spriteset_frame_t;

struct spriteset_frame
{
	int image_idx;
	int delay;
};

struct spriteset_pose
{
	int               num_frames;
	char*             name;
	spriteset_frame_t *frames;
};

struct spriteset
{
	int              num_images;
	int              num_poses;
	ALLEGRO_BITMAP*  *bitmaps;
	spriteset_pose_t *poses;
};

extern spriteset_t* open_spriteset     (const char* path);
extern void         close_spriteset    (spriteset_t* spriteset);
extern void         init_spriteset_api (duk_context* ctx);
