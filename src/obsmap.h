#ifndef MINISPHERE__OBSMAP_H__INCLUDED
#define MINISPHERE__OBSMAP_H__INCLUDED

typedef struct obsmap obsmap_t;

obsmap_t* new_obsmap       (void);
void      free_obsmap      (obsmap_t* obsmap);
bool      add_obsmap_line  (obsmap_t* obsmap, rect_t line);
bool      test_obsmap_line (const obsmap_t* obsmap, rect_t line);
bool      test_obsmap_rect (const obsmap_t* obsmap, rect_t rect);

#endif // MINISPHERE__OBSMAP_H__INCLUDED
