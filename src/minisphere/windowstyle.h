#include "color.h"

typedef struct windowstyle windowstyle_t;

windowstyle_t* load_windowstyle (const char* filename);
windowstyle_t* ref_windowstyle  (windowstyle_t* winstyle);
void           free_windowstyle (windowstyle_t* windowstyle);
void           draw_window      (windowstyle_t* winstyle, color_t mask, int x, int y, int width, int height);

void init_windowstyle_api        (void);
void duk_push_sphere_windowstyle (duk_context* ctx, windowstyle_t* winstyle);
