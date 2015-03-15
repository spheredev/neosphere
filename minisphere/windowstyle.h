typedef struct windowstyle windowstyle_t;

extern windowstyle_t* load_windowstyle     (const char* path);
extern void           free_windowstyle     (windowstyle_t* windowstyle);
extern void           init_windowstyle_api (void);
extern void           draw_window          (windowstyle_t* winstyle, ALLEGRO_COLOR mask, int x, int y, int width, int height);
