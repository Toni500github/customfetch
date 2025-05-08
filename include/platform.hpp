#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#ifndef __clang__
# define __is_target_vendor(x) 0
# define __is_target_os(x) 0
# define __is_target_environment(x) 0
#endif

#if (defined(__ANDROID__) || defined(ANDROID_API) || __is_target_environment(Android))
# define CF_ANDROID 1
#else
# define CF_ANDROID 0
#endif

#if (defined(__linux) || defined(__linux__) || defined(__gnu_linux__)) && !CF_ANDROID
# define CF_LINUX 1
#else
# define CF_LINUX 0
#endif

#if (defined(__MACOS__) || defined(__MACH__) || defined(TARGET_OS_MAC) || defined(TARGET_OS_OSX) || __is_target_vendor(apple) || __is_target_os(darwin) || __is_target_os(MacOSX))
# define CF_MACOS 1
#else
# define CF_MACOS 0
#endif

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__))
# define CF_WINDOWS 1
#else
# define CF_WINDOWS 0
#endif

#if !(CF_LINUX || CF_ANDROID || CF_MACOS) || CF_WINDOWS
# warning "Platform currently may not be supported, only Linux, Android and MacOS. Please feel free to report any compilation errors"
#endif

#endif // _PLATFORM_H_
