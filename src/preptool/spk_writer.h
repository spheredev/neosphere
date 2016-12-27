#ifndef MAJIN__SPK_WRITER_H__INCLUDED
#define MAJIN__SPK_WRITER_H__INCLUDED

typedef struct spk_writer spk_writer_t;

spk_writer_t* spk_create   (const char* filename);
void          spk_close    (spk_writer_t* writer);
bool          spk_add_file (spk_writer_t* writer, const char* filename, const char* spk_pathname);

#endif // MAJIN__SPK_WRITER_H__INCLUDED
