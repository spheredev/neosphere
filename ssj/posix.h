#ifndef SSJ__POSIX_H__INCLUDED
#define SSJ__POSIX_H__INCLUDED

#if defined(_MSC_VER)
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define strcasecmp stricmp
#define strtok_r   strtok_s

#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#endif

#endif // SSJ__POSIX_H__INCLUDED
