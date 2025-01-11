#ifndef _PLATFORM_H
#define _PLATFORM_H

#if (defined(__ANDROID__) || defined(ANDROID_API) || ANDROID_APP)
# define CF_ANDROID 1
#else
# define CF_ANDROID 0
#endif

#if (defined(unix) || defined(__unix) || defined(__unix__)) && !CF_ANDROID
# define CF_UNIX 1
#else
# define CF_UNIX 0
#endif

#if (defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__))
# define CF_WINDOWS 1
#else
# define CF_WINDOWS 0
#endif

#if !(CF_UNIX || CF_ANDROID) || CF_WINDOWS
# error "Platform currently not supported, only Unix and Android"
#endif

#endif // _PLATFORM_H
