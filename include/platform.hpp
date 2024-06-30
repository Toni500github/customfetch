#ifndef PLATFORM_H
#define PLATFORM_H

#if (defined(unix) || defined(__unix) || defined(__unix__))
# define CF_UNIX 1
#endif

#if (defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__))
#define CF_WINDOWS 1
typedef unsigned short int u_short; // they don't have sys/types.h XDDD
#endif

#endif // PLATFORM_H
