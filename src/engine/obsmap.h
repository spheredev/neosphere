#ifndef MINISPHERE__OBSMAP_H__INCLUDED
#define MINISPHERE__OBSMAP_H__INCLUDED

typedef struct obsmap obsmap_t;

obsmap_t* obsmap_new       (void);
void      obsmap_free      (obsmap_t* obsmap);
bool      obsmap_add_line  (obsmap_t* obsmap, rect_t line);
bool      obsmap_test_line (const obsmap_t* obsmap, rect_t line);
bool      obsmap_test_rect (const obsmap_t* obsmap, rect_t rect);

#endif // MINISPHERE__OBSMAP_H__INCLUDED
