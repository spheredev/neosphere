#ifndef CELL__POSIX_H__INCLUDED
#define CELL__POSIX_H__INCLUDED

#ifdef _MSC_VER
#define strcasecmp stricmp
#define snprintf _snprintf
#define strtok_r strtok_s
#endif

#endif // CELL__POSIX_H__INCLUDED
