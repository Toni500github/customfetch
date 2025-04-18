/*
 * Copyright 2025 Toni500git
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

#include "platform.hpp"
#if CF_ANDROID

#include <cctype>
#include <string>
#include <vector>

#include "json.h"
#include "query.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

using namespace Query;

static Battery::Battery_t get_battery_infos_termux()
{
    Battery::Battery_t infos;
    std::string        result, _;
    if (!read_exec({ "/data/data/com.termux/files/usr/libexec/termux-api", "BatteryStatus" }, result))
        return infos;

    const auto& doc = json::jobject::parse(result);

    infos.status   = doc["plugged"].as_string();
    infos.perc     = doc["percentage"];
    infos.temp     = doc["temperature"];

    switch (fnv1a16::hash(infos.status))
    {
        case "PLUGGED_AC"_fnv1a16:       infos.status = "AC Connected, "; break;
        case "PLUGGED_USB"_fnv1a16:      infos.status = "USB Connected, "; break;
        case "PLUGGED_WIRELESS"_fnv1a16: infos.status = "Wireless Connected, "; break;
        default:                         infos.status.clear();
    }

    // CHARGING or DISCHARGING
    std::string charge_status{ str_tolower(doc["status"].as_string()) };
    charge_status.at(0) = toupper(charge_status.at(0));
    infos.status += charge_status;

    return infos;
}

static Battery::Battery_t get_battery_infos_dumpsys()
{
    Battery::Battery_t infos;
    std::string        result, _;
    if (!read_exec({ "/system/bin/dumpsys", "battery" }, result))
        return infos;

    const std::vector<std::string>& vec = split(result, '\n');
    if (vec.at(0) != "Current Battery Service state:")
        return infos;

    double level = 0, scale = 0;
    for (size_t i = 1; i < vec.size(); ++i)
    {
        const size_t       pos   = vec.at(i).rfind(':');
        const std::string& key   = vec.at(i).substr(2, pos);
        const std::string& value = vec.at(i).substr(pos + 2);

        switch (fnv1a16::hash(key))
        {
            case "level"_fnv1a16: level = std::stod(value); break;
            case "scale"_fnv1a16: scale = std::stod(value); break;

            case "AC powered"_fnv1a16:
            case "USB powered"_fnv1a16:
            case "Dock powered"_fnv1a16:
            case "Wireless powered"_fnv1a16:
                if (value == "true")
                    infos.status = key;
                break;

            case "temperature"_fnv1a16: infos.temp = std::stod(value) / 10; break;
            case "technology"_fnv1a16:  infos.technology = value;
        }
    }

    if (level > 0 && scale > 0)
        infos.perc = level * 100 / scale;

    return infos;
}

Battery::Battery()
{
    CHECK_INIT(m_bInit);

// can't execute commands in android app
// also this is a widget, you can see the percentage in your topbar
#if !ANDROID_APP
    m_battery_infos = get_battery_infos_termux();
    if (m_battery_infos.status == MAGIC_LINE || m_battery_infos.temp <= 0)
        m_battery_infos = get_battery_infos_dumpsys();
#endif
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

#endif // CF_ANDROID
