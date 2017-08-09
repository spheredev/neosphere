#ifndef MINISPHERE__WINDOWSTYLE_H__INCLUDED
#define MINISPHERE__WINDOWSTYLE_H__INCLUDED

#include "color.h"

typedef struct windowstyle windowstyle_t;

windowstyle_t* winstyle_load  (const char* filename);
windowstyle_t* winstyle_ref   (windowstyle_t* winstyle);
void           winstyle_unref (windowstyle_t* windowstyle);
void           winstyle_draw  (windowstyle_t* winstyle, color_t mask, int x, int y, int width, int height);

#endif // MINISPHERE__WINDOWSTYLE_H__INCLUDED
