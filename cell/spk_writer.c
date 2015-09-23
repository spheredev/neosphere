#include "cell.h"

#include "spk_writer.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

struct spk_writer
{
	FILE* file;
};

spk_writer_t*
spk_create(const char* filename)
{
	const uint16_t SPK_VERSION = 1;

	spk_writer_t* writer;

	writer = calloc(1, sizeof(spk_writer_t));
	if (!(writer->file = fopen(filename, "wb")))
		return NULL;
	fputs(".spk", writer->file);
	fwrite(&SPK_VERSION, sizeof(uint16_t), 1, writer->file);
	
	return writer;
}
