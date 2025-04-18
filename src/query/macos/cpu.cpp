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

#include <ratio>
#include <string>
#include <sys/sysctl.h>
#include <unistd.h>

#include "query.hpp"
#include "util.hpp"

using namespace Query;

static bool get_sysctl(int name[2], void *ret, size_t *oldlenp)
{
    return (sysctl(name, 2, ret, oldlenp, NULL, 0) == 0);
}

static bool get_sysctl(const char *name, void *ret, size_t *oldlenp)
{
    return (sysctlbyname(name, ret, oldlenp, NULL, 0) == 0);
}

static CPU::CPU_t get_cpu_infos()
{
    CPU::CPU_t ret;
    debug("calling in CPU {}", __PRETTY_FUNCTION__);
    char buf[1024]; 
    size_t len = sizeof(buf);

    get_sysctl("machdep.cpu.brand_string", &buf, &len);
    ret.name = buf;

    if (!(get_sysctl("machdep.cpu.vendor", &buf, &len)) && hasStart(ret.name, "Apple"))
        ret.vendor = "Apple";
    else
        ret.vendor = buf;

    if (!get_sysctl("hw.logicalcpu_max", &buf, &len))
        get_sysctl("hw.ncpu", &buf, &len);
    ret.nproc = buf;

    uint64_t freq_cur = 0, freq_max = 0, freq_min;
    size_t length = sizeof(freq_cur);
    get_sysctl("hw.cpufrequency_max", &freq_max, &length);
    get_sysctl("hw.cpufrequency_min", &freq_min, &length);

    if (!get_sysctl("hw.cpufrequency", &freq_cur, &length))
        get_sysctl((int[]){ CTL_HW, HW_CPU_FREQ }, &freq_cur, &length);

    ret.freq_cur = static_cast<double>(freq_cur) / std::giga().num;
    ret.freq_max = static_cast<double>(freq_max) / std::giga().num;
    ret.freq_min = static_cast<double>(freq_min) / std::giga().num;
    return ret;
}

CPU::CPU() noexcept
{
    CHECK_INIT(!m_bInit)
    {
        m_cpu_infos = get_cpu_infos();
        m_bInit     = true;
    }
}

// clang-format off
std::string& CPU::name() noexcept
{ return m_cpu_infos.name; }

std::string& CPU::nproc() noexcept
{ return m_cpu_infos.nproc; }

std::string& CPU::vendor() noexcept
{ return m_cpu_infos.vendor; }

std::string& CPU::modelname() noexcept
{ return m_cpu_infos.modelname; }

double& CPU::temp() noexcept
{ return m_cpu_infos.temp; }

double& CPU::freq_bios_limit() noexcept
{ return m_cpu_infos.freq_bios_limit; }

double& CPU::freq_cur() noexcept
{ return m_cpu_infos.freq_cur; }

double& CPU::freq_max() noexcept
{ return (m_cpu_infos.freq_max <= 0) ? m_cpu_infos.freq_max_cpuinfo : m_cpu_infos.freq_max; }

double& CPU::freq_min() noexcept
{ return m_cpu_infos.freq_min; }

#endif // CF_MACOS
