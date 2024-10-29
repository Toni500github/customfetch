/*
 * Copyright 2024 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/types.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "fmt/format.h"
#include "query.hpp"
#include "util.hpp"

using namespace Query;

static std::string get_from_text(std::string& line)
{
    std::string amount = line.substr(line.find(':') + 1);
    strip(amount);
    return amount;
}

static CPU::CPU_t get_cpu_infos()
{
    CPU::CPU_t ret;
    debug("calling in CPU {}", __PRETTY_FUNCTION__);
    constexpr std::string_view cpuinfo_path = "/proc/cpuinfo";
    std::ifstream              file(cpuinfo_path.data());
    if (!file.is_open())
    {
        error("Could not open {}", cpuinfo_path);
        return ret;
    }

    std::string line;
    float       cpu_mhz = -1;
    while (std::getline(file, line))
    {
        if (hasStart(line, "model name"))
            ret.name = get_from_text(line);

        if (hasStart(line, "processor"))
            ret.nproc = get_from_text(line);

        if (hasStart(line, "cpu MHz"))
        {
            double tmp = std::stof(get_from_text(line));
            if (tmp > cpu_mhz)
                cpu_mhz = tmp;
        }
    }

    // sometimes /proc/cpuinfo at model name
    // the name will contain the min freq
    // happens on intel cpus especially
    const size_t pos = ret.name.rfind('@');
    if (pos != std::string::npos)
        ret.name.erase(pos - 1);

    cpu_mhz /= 1000;
    ret.freq_max_cpuinfo = cpu_mhz;

    // add 1 to the nproc
    ret.nproc = fmt::to_string(std::stoi(ret.nproc) + 1);

    const std::string freq_dir = "/sys/devices/system/cpu/cpu0/cpufreq";
    if (std::filesystem::exists(freq_dir))
    {
        std::ifstream cpu_bios_limit_f(freq_dir  + "/bios_limit");
        std::ifstream cpu_scaling_cur_f(freq_dir + "/scaling_cur_freq");
        std::ifstream cpu_scaling_max_f(freq_dir + "/scaling_max_freq");
        std::ifstream cpu_scaling_min_f(freq_dir + "/scaling_min_freq");

        std::string freq_bios_limit, freq_cpu_scaling_cur, freq_cpu_scaling_max, freq_cpu_scaling_min;

        std::getline(cpu_bios_limit_f, freq_bios_limit);
        std::getline(cpu_scaling_cur_f, freq_cpu_scaling_cur);
        std::getline(cpu_scaling_max_f, freq_cpu_scaling_max);
        std::getline(cpu_scaling_min_f, freq_cpu_scaling_min);

        ret.freq_bios_limit = freq_bios_limit.empty() ? 0 : (std::stof(freq_bios_limit) / 1000000);
        ret.freq_cur        = freq_cpu_scaling_cur.empty() ? 0 : (std::stof(freq_cpu_scaling_cur) / 1000000);
        ret.freq_max        = freq_cpu_scaling_max.empty() ? 0 : (std::stof(freq_cpu_scaling_max) / 1000000);
        ret.freq_min        = freq_cpu_scaling_min.empty() ? 0 : (std::stof(freq_cpu_scaling_min) / 1000000);
    }

    return ret;
}

CPU::CPU() noexcept
{
    if (!m_bInit)
    {
        m_cpu_infos = get_cpu_infos();
        m_bInit     = true;
    }
}

std::string& CPU::name() noexcept
{ return m_cpu_infos.name; }

std::string& CPU::nproc() noexcept
{ return m_cpu_infos.nproc; }

double& CPU::freq_bios_limit() noexcept
{ return m_cpu_infos.freq_bios_limit; }

double& CPU::freq_cur() noexcept
{ return m_cpu_infos.freq_cur; }

double& CPU::freq_max() noexcept
{ return (m_cpu_infos.freq_max <= 0) ? m_cpu_infos.freq_max_cpuinfo : m_cpu_infos.freq_max; }

double& CPU::freq_min() noexcept
{ return m_cpu_infos.freq_min; }
