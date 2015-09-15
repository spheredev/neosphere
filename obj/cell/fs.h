#ifndef CELL__FS_H__INCLUDED
#define CELL__FS_H__INCLUDED

extern int  fs_install (const char* pattern, const char* src_path, const char* dest_path, bool recursive);
extern bool fs_mkdir   (const char* path);

extern void init_basics_api (void);

#endif // CELL__FS_H__INCLUDED
