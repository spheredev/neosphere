#ifndef SSJ__FILE_LINE_H__INCLUDED
#define SSJ__FILE_LINE_H__INCLUDED

typedef struct file_line file_line_t;

file_line_t* new_fileline       (const char* filename, int line_no);
void         free_fileline      (file_line_t* obj);
const char*  file_line_filename (file_line_t* obj);
int          file_line_lineno   (file_line_t* obj);

#endif
