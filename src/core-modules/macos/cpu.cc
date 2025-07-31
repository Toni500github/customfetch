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
#if CF_MACOS

#include <sys/sysctl.h>
#include <unistd.h>

#include <cstdint>
#include <ratio>
#include <string>

#include "core-modules.hh"

static bool get_sysctl(int name[2], void* ret, size_t* oldlenp)
{
    return (sysctl(name, 2, ret, oldlenp, NULL, 0) == 0);
}

static bool get_sysctl(const char* name, void* ret, size_t* oldlenp)
{
    return (sysctlbyname(name, ret, oldlenp, NULL, 0) == 0);
}

float cpu_temp() { return 0; }

MODFUNC(cpu_name)
{
    char   buf[1024];
    size_t len = sizeof(buf);
    get_sysctl("machdep.cpu.brand_string", &buf, &len);
    return buf;
}

MODFUNC(cpu_nproc)
{
    char   buf[1024];
    size_t len = sizeof(buf);
    if (!get_sysctl("hw.logicalcpu_max", &buf, &len))
        get_sysctl("hw.ncpu", &buf, &len);
    return buf;
}

MODFUNC(cpu_freq_cur)
{
    std::uint64_t freq   = 0;
    size_t        length = sizeof(freq);
    if (!get_sysctl("hw.cpufrequency", &freq, &length))
        get_sysctl((int[2]){ CTL_HW, HW_CPU_FREQ }, &freq, &length);

    return fmt::to_string(static_cast<double>(freq) / std::giga().num);
}

MODFUNC(cpu_freq_min)
{
    std::uint64_t freq   = 0;
    size_t        length = sizeof(freq);
    get_sysctl("hw.cpufrequency_min", &freq, &length);

    return fmt::to_string(static_cast<double>(freq) / std::giga().num);
}

MODFUNC(cpu_freq_max)
{
    std::uint64_t freq   = 0;
    size_t        length = sizeof(freq);
    get_sysctl("hw.cpufrequency_max", &freq, &length);

    return fmt::to_string(static_cast<double>(freq) / std::giga().num);
}

MODFUNC(cpu_freq_bios) { return MAGIC_LINE; }

#endif
