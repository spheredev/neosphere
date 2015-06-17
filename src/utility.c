#include "minisphere.h"

#include "utility.h"

const char*
syspath(const char* filename)
{
	static char retval[SPHERE_PATH_MAX];

	retval[SPHERE_PATH_MAX - 1] = '\0';
	snprintf(retval, SPHERE_PATH_MAX - 1, "~sys/%s", filename);
	return retval;
}
