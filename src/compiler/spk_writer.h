#ifndef CELL__SPK_WRITER_H__INCLUDED
#define CELL__SPK_WRITER_H__INCLUDED

#include "fs.h"

typedef struct spk_writer spk_writer_t;

spk_writer_t* spk_create   (const char* filename);
void          spk_close    (spk_writer_t* writer);
bool          spk_add_file (spk_writer_t* writer, fs_t* fs, const char* filename, const char* spk_pathname);

#endif // CELL__SPK_WRITER_H__INCLUDED
