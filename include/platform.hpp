#ifndef PLATFORM_H
#define PLATFORM_H

#if (defined(unix) || defined(__unix) || defined(__unix__))
# define CF_UNIX 1
#endif

#if (defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__))
#define CF_WINDOWS 1
// https://github.com/msys2/MINGW-packages/issues/4999#issuecomment-1530791650
char *strndup(const char* src, size_t size) {
  size_t len = strnlen(src, size);
  len = len < size ? len : size;
  char * dst = malloc(len + 1);
  if (!dst)
    return NULL;
  memcpy(dst, src, len);
  dst[len] = '\0';
  return result;
}
typedef unsigned short int u_short; // they don't have sys/types.h XDDD
#endif

#endif // PLATFORM_H
