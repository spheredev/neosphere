#ifndef SSJ__FILE_LINE_H__INCLUDED
#define SSJ__FILE_LINE_H__INCLUDED

typedef struct fileline fileline_t;

fileline_t* new_fileline      (const char* filename, int line_no);
void        free_fileline     (fileline_t* obj);
const char* fileline_filename (fileline_t* obj);
int         fileline_line_no  (fileline_t* obj);

#endif
