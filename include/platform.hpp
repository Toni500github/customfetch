#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstddef>
#include <cstdlib>

#if (defined(unix) || defined(__unix) || defined(__unix__))
# define CF_UNIX 1
#endif

#if !(defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__))
#define CF_WINDOWS 1
typedef unsigned short int u_short; // they don't have sys/types.h XDDD
// https://github.com/msys2/MINGW-packages/issues/4999#issuecomment-1530791650
char *strndup(const char* src, size_t size) {
  size_t len = strnlen(src, size);
  len = len < size ? len : size;
  char *dst = (char*)malloc(len + 1);
  if (!dst)
    return nullptr;
  memcpy(dst, src, len);
  dst[len] = '\0';
  return dst;
}
#endif

#endif // PLATFORM_H
