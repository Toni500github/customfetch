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

#include "platform.hpp"
#if CF_ANDROID

#include "core-modules.hh"
#include "util.hpp"

// clang-format off
MODFUNC(os_name)
{ return "Android"; }

MODFUNC(os_pretty_name)
{ return "Android " + os_version_codename(NULL) + " " + os_version_id(NULL); }

MODFUNC(os_name_id)
{ return "android"; }

MODFUNC(os_version_id)
{ return get_android_property("ro.build.version.release"); }

MODFUNC(os_version_codename)
{ return get_android_property("ro.build.version.codename"); }

MODFUNC(os_kernel_name)
{ return g_uname_infos.sysname; }

MODFUNC(os_kernel_version)
{ return g_uname_infos.release; }

MODFUNC(os_hostname)
{ return g_uname_infos.nodename; }

MODFUNC(os_initsys_name)
{ return "init"; }

MODFUNC(os_initsys_version)
{ return ""; }

unsigned long os_uptime()
{
    struct std::timespec uptime;
    if (clock_gettime(CLOCK_BOOTTIME, &uptime) != 0)
        return 0;

    return (uint64_t)uptime.tv_sec + (uint64_t)uptime.tv_nsec / 1000000;
}

#endif
