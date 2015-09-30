#include "cell.h"
#include "utility.h"

void*
fslurp(const char* filename, size_t *out_size)
{
	void* buffer;
	FILE* file = NULL;

	if (!(file = fopen(filename, "rb")))
		return false;
	*out_size = (fseek(file, 0, SEEK_END), ftell(file));
	if (!(buffer = malloc(*out_size))) goto on_error;
	fseek(file, 0, SEEK_SET);
	if (fread(buffer, 1, *out_size, file) != *out_size)
		goto on_error;
	fclose(file);
	return buffer;

on_error:
	return NULL;
}

bool
fspew(const void* buffer, size_t size, const char* filename)
{
	FILE* file = NULL;

	if (!(file = fopen(filename, "wb")))
		return false;
	fwrite(buffer, size, 1, file);
	fclose(file);
	return true;
}

bool
wildcmp(const char* filename, const char* pattern)
{
	const char* cp = NULL;
	const char* mp = NULL;
	bool        is_match = 0;

	// check filename against the specified filter string
	while (*filename != '\0' && *pattern != '*') {
		if (*pattern != *filename && *pattern != '?')
			return false;
		++pattern;
		++filename;
	}
	while (*filename != '\0') {
		if (*pattern == '*') {
			if (*++pattern == '\0') return true;
			mp = pattern;
			cp = filename + 1;
		}
		else if (*pattern == *filename || *pattern == '?') {
			pattern++;
			filename++;
		}
		else {
			pattern = mp;
			filename = cp++;
		}
	}
	while (*pattern == '*')
		pattern++;
	return *pattern == '\0';
}
