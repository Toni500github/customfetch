#ifndef _PLATFORM_H
#define _PLATFORM_H

#if (defined(__ANDROID__) || defined(ANDROID_API) || ANDROID_APP)
# define CF_ANDROID 1
#else
# define CF_ANDROID 0
#endif

#if (defined(__linux) || defined(__linux__) || defined(__gnu_linux__)) && !CF_ANDROID
# define CF_LINUX 1
#else
# define CF_LINUX 0
#endif

#if (defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__))
# define CF_WINDOWS 1
#else
# define CF_WINDOWS 0
#endif

#if !(CF_LINUX || CF_ANDROID) || CF_WINDOWS
# warning "Platform currently may not be supported, only Linux and Android"
#endif

#endif // _PLATFORM_H
