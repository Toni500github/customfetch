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
#if CF_LINUX

#include <cstdlib>
#include <filesystem>
#include <string>

#include "query.hpp"
#include "util.hpp"

using namespace Query;

static void read_strip_syspath(std::string& str, const std::string_view path)
{
    str = read_by_syspath(path);
    debug("str = {} || path = {}", str, path);

    // optimization
    if (str.back() == '\n')
        str.pop_back();
    else if (str != UNKNOWN)
        strip(str);
}

static Battery::Battery_t get_battery_infos()
{
    Battery::Battery_t infos;
    if (!std::filesystem::exists("/sys/class/power_supply/"))
        return infos;

    for (const auto& dir_entry : std::filesystem::directory_iterator{ "/sys/class/power_supply/" })
    {
        const std::string& path = dir_entry.path().string() + "/";
        debug("battery path = {}", path);
        std::string tmp;

        read_strip_syspath(tmp, path + "type");
        if (tmp == UNKNOWN || tmp != "Battery")
            continue;

        read_strip_syspath(tmp, path + "scope");
        if (tmp == "Device")
            continue;

        debug("battery found yeappyy");
        read_strip_syspath(tmp, path + "capacity");
        if (tmp != UNKNOWN)
            infos.perc = std::stod(tmp);
        else
            continue;

        read_strip_syspath(tmp, path + "temp");
        if (tmp != UNKNOWN)
            infos.temp = std::stod(tmp) / 10;

        read_strip_syspath(tmp, path + "manufacturer");
        if (tmp != UNKNOWN)
            infos.vendor = tmp;

        read_strip_syspath(tmp, path + "model_name");
        if (tmp != UNKNOWN)
            infos.modelname = tmp;

        read_strip_syspath(tmp, path + "technology");
        if (tmp != UNKNOWN)
            infos.technology = tmp;

        read_strip_syspath(tmp, path + "status");
        if (tmp != UNKNOWN)
            infos.status = tmp;

        read_strip_syspath(tmp, path + "capacity_level");
        if (tmp != UNKNOWN)
            infos.capacity_level = tmp;
    }

    return infos;
}

Battery::Battery()
{
    CHECK_INIT(m_bInit);

    m_battery_infos = get_battery_infos();
}

// clang-format off
std::string& Battery::modelname() noexcept
{ return m_battery_infos.modelname; }

std::string& Battery::status() noexcept
{ return m_battery_infos.status; }

std::string& Battery::vendor() noexcept
{ return m_battery_infos.vendor; }

std::string& Battery::technology() noexcept
{ return m_battery_infos.technology; }

std::string& Battery::capacity_level() noexcept
{ return m_battery_infos.capacity_level; }

double& Battery::perc() noexcept
{ return m_battery_infos.perc; }

double& Battery::temp() noexcept
{ return m_battery_infos.temp; }

#endif // CF_LINUX
