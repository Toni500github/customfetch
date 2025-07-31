/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

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

#if (defined(__MACOS__) || defined(__MACH__) || defined(TARGET_OS_MAC) || defined(TARGET_OS_OSX) || \
     __is_target_vendor(apple) || __is_target_os(darwin) || __is_target_os(MacOSX))
# define CF_MACOS 1
#else
# define CF_MACOS 0
#endif

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__CYGWIN__) || \
     defined(__MINGW32__) || defined(__MINGW64__))
# define CF_WINDOWS 1
#else
# define CF_WINDOWS 0
#endif

#if !(CF_LINUX || CF_ANDROID || CF_MACOS) || CF_WINDOWS
# warning \
    "Platform currently may not be supported, only Linux, Android and MacOS. Please feel free to report any compilation errors"
#endif

#endif  // _PLATFORM_H_
