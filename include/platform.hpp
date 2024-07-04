#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>
#include <cstdlib>

#if (defined(unix) || defined(__unix) || defined(__unix__))
# define CF_UNIX 1
#endif

#if (defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__))
# define CF_WINDOWS 1
#endif

#if !(defined(CF_WINDOWS) || defined(CF_UNIX))
# error "Platform currently not supported, only Unix and Windows currently"
#endif

#endif // PLATFORM_H
