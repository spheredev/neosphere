#include "color.h"

typedef struct windowstyle windowstyle_t;

extern windowstyle_t* load_windowstyle (const char* filename, bool is_sfs_compliant);
extern windowstyle_t* ref_windowstyle  (windowstyle_t* winstyle);
extern void           free_windowstyle (windowstyle_t* windowstyle);
extern void           draw_window      (windowstyle_t* winstyle, color_t mask, int x, int y, int width, int height);

extern void init_windowstyle_api        (void);
extern void duk_push_sphere_windowstyle (duk_context* ctx, windowstyle_t* winstyle);
