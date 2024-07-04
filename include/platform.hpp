#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstddef>
#include <cstdlib>

#if (defined(unix) || defined(__unix) || defined(__unix__))
#define CF_UNIX 1
typedef unsigned int uint;
#endif

#if (defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__))
#define CF_WINDOWS 1

// they don't have sys/types.h XDDD
typedef unsigned short u_short; 
typedef unsigned int uint;
#endif // CF_WINDOWS

#endif // PLATFORM_H
