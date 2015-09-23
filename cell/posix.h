#ifndef CELL__POSIX_H__INCLUDED
#define CELL__POSIX_H__INCLUDED

#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define strcasecmp stricmp
#define snprintf _snprintf
#define strtok_r strtok_s
#endif

#endif // CELL__POSIX_H__INCLUDED
