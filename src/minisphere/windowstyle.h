#ifndef MINISPHERE__WINDOWSTYLE_H__INCLUDED
#define MINISPHERE__WINDOWSTYLE_H__INCLUDED

#include "color.h"

typedef struct windowstyle windowstyle_t;

windowstyle_t* load_windowstyle (const char* filename);
windowstyle_t* ref_windowstyle  (windowstyle_t* winstyle);
void           free_windowstyle (windowstyle_t* windowstyle);
void           draw_window      (windowstyle_t* winstyle, color_t mask, int x, int y, int width, int height);

#endif // MINISPHERE__WINDOWSTYLE_H__INCLUDED
