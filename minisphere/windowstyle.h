typedef struct windowstyle windowstyle_t;

enum wstyle_bg
{
	WSTYLE_BG_TILE,
	WSTYLE_BG_STRETCH,
	WSTYLE_BG_GRADIENT,
	WSTYLE_BG_TILE_GRADIENT,
	WSTYLE_BG_STRETCH_GRADIENT
};

struct windowstyle
{
	int             bg_style;
	ALLEGRO_BITMAP* bitmaps[9];
};

extern windowstyle_t* load_windowstyle     (const char* path);
extern void           free_windowstyle     (windowstyle_t* windowstyle);
extern void           init_windowstyle_api (void);
