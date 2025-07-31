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

#include <regex>  // -100000000 runtime/compile-time
#include <string>
#include <string_view>

#include "core-modules.hh"
#include "libcufetch/common.hh"
#include "switch_fnv1a.hpp"
#include "util.hpp"

static bool get_sysctl(const char* name, void* ret, size_t* oldlenp)
{
    return (sysctlbyname(name, ret, oldlenp, NULL, 0) == 0);
}

// https://github.com/fastfetch-cli/fastfetch/blob/a734f18fd56014f5c0b9fb388727b778e2bc05d1/src/detection/host/host_mac.c#L4
const std::string get_host_from_family(const std::string_view host_family)
{
    // Macbook Pro: https://support.apple.com/en-us/HT201300
    // Macbook Air: https://support.apple.com/en-us/HT201862
    // Mac mini:    https://support.apple.com/en-us/HT201894
    // iMac:        https://support.apple.com/en-us/HT201634
    // Mac Pro:     https://support.apple.com/en-us/HT202888
    // Mac Studio:  https://support.apple.com/en-us/HT213073

    if (hasStart(host_family, "MacBookPro"))
    {
        const std::string_view version = host_family.substr("MacBookPro"_len);
        switch (fnv1a16::hash(version.data()))
        {
            case "18,3"_fnv1a16:
            case "18,4"_fnv1a16: return "MacBook Pro (14-inch, 2021)";
            case "18,1"_fnv1a16:
            case "18,2"_fnv1a16: return "MacBook Pro (16-inch, 2021)";
            case "17,1"_fnv1a16: return "MacBook Pro (13-inch, M1, 2020)";
            case "16,3"_fnv1a16: return "MacBook Pro (13-inch, 2020, Two Thunderbolt 3 ports)";
            case "16,2"_fnv1a16: return "MacBook Pro (13-inch, 2020, Four Thunderbolt 3 ports)";
            case "16,4"_fnv1a16:
            case "16,1"_fnv1a16: return "MacBook Pro (16-inch, 2019)";
            case "15,4"_fnv1a16: return "MacBook Pro (13-inch, 2019, Two Thunderbolt 3 ports)";
            case "15,3"_fnv1a16: return "MacBook Pro (15-inch, 2019)";
            case "15,2"_fnv1a16: return "MacBook Pro (13-inch, 2018/2019, Four Thunderbolt 3 ports)";
            case "15,1"_fnv1a16: return "MacBook Pro (15-inch, 2018/2019)";
            case "14,3"_fnv1a16: return "MacBook Pro (15-inch, 2017)";
            case "14,2"_fnv1a16: return "MacBook Pro (13-inch, 2017, Four Thunderbolt 3 ports)";
            case "14,1"_fnv1a16: return "MacBook Pro (13-inch, 2017, Two Thunderbolt 3 ports)";
            case "13,3"_fnv1a16: return "MacBook Pro (15-inch, 2016)";
            case "13,2"_fnv1a16: return "MacBook Pro (13-inch, 2016, Four Thunderbolt 3 ports)";
            case "13,1"_fnv1a16: return "MacBook Pro (13-inch, 2016, Two Thunderbolt 3 ports)";
            case "12,1"_fnv1a16: return "MacBook Pro (Retina, 13-inch, Early 2015)";
            case "11,4"_fnv1a16:
            case "11,5"_fnv1a16: return "MacBook Pro (Retina, 15-inch, Mid 2015)";
            case "11,2"_fnv1a16:
            case "11,3"_fnv1a16: return "MacBook Pro (Retina, 15-inch, Late 2013/Mid 2014)";
            case "11,1"_fnv1a16: return "MacBook Pro (Retina, 13-inch, Late 2013/Mid 2014)";
            case "10,2"_fnv1a16: return "MacBook Pro (Retina, 13-inch, Late 2012/Early 2013)";
            case "10,1"_fnv1a16: return "MacBook Pro (Retina, 15-inch, Mid 2012/Early 2013)";
            case "9,2"_fnv1a16:  return "MacBook Pro (13-inch, Mid 2012)";
            case "9,1"_fnv1a16:  return "MacBook Pro (15-inch, Mid 2012)";
            case "8,3"_fnv1a16:  return "MacBook Pro (17-inch, 2011)";
            case "8,2"_fnv1a16:  return "MacBook Pro (15-inch, 2011)";
            case "8,1"_fnv1a16:  return "MacBook Pro (13-inch, 2011)";
            case "7,1"_fnv1a16:  return "MacBook Pro (13-inch, Mid 2010)";
            case "6,2"_fnv1a16:  return "MacBook Pro (15-inch, Mid 2010)";
            case "6,1"_fnv1a16:  return "MacBook Pro (17-inch, Mid 2010)";
            case "5,5"_fnv1a16:  return "MacBook Pro (13-inch, Mid 2009)";
            case "5,3"_fnv1a16:  return "MacBook Pro (15-inch, Mid 2009)";
            case "5,2"_fnv1a16:  return "MacBook Pro (17-inch, Mid/Early 2009)";
            case "5,1"_fnv1a16:  return "MacBook Pro (15-inch, Late 2008)";
            case "4,1"_fnv1a16:  return "MacBook Pro (17/15-inch, Early 2008)";
        }
    }
    else if (hasStart(host_family, "MacBookAir"))
    {
        const std::string_view version = host_family.substr("MacBookAir"_len);
        switch (fnv1a16::hash(version.data()))
        {
            case "10,1"_fnv1a16: return "MacBook Air (M1, 2020)";
            case "9,1"_fnv1a16:  return "MacBook Air (Retina, 13-inch, 2020)";
            case "8,2"_fnv1a16:  return "MacBook Air (Retina, 13-inch, 2019)";
            case "8,1"_fnv1a16:  return "MacBook Air (Retina, 13-inch, 2018)";
            case "7,2"_fnv1a16:  return "MacBook Air (13-inch, Early 2015/2017)";
            case "7,1"_fnv1a16:  return "MacBook Air (11-inch, Early 2015)";
            case "6,2"_fnv1a16:  return "MacBook Air (13-inch, Mid 2013/Early 2014)";
            case "6,1"_fnv1a16:  return "MacBook Air (11-inch, Mid 2013/Early 2014)";
            case "5,2"_fnv1a16:  return "MacBook Air (13-inch, Mid 2012)";
            case "5,1"_fnv1a16:  return "MacBook Air (11-inch, Mid 2012)";
            case "4,2"_fnv1a16:  return "MacBook Air (13-inch, Mid 2011)";
            case "4,1"_fnv1a16:  return "MacBook Air (11-inch, Mid 2011)";
            case "3,2"_fnv1a16:  return "MacBook Air (13-inch, Late 2010)";
            case "3,1"_fnv1a16:  return "MacBook Air (11-inch, Late 2010)";
            case "2,1"_fnv1a16:  return "MacBook Air (Mid 2009)";
        }
    }
    else if (hasStart(host_family, "Macmini"))
    {
        const std::string_view version = host_family.substr("Macmini"_len);
        switch (fnv1a16::hash(version.data()))
        {
            case "9,1"_fnv1a16: return "Mac mini (M1, 2020)";
            case "8,1"_fnv1a16: return "Mac mini (2018)";
            case "7,1"_fnv1a16: return "Mac mini (Mid 2014)";
            case "6,1"_fnv1a16:
            case "6,2"_fnv1a16: return "Mac mini (Late 2012)";
            case "5,1"_fnv1a16:
            case "5,2"_fnv1a16: return "Mac mini (Mid 2011)";
            case "4,1"_fnv1a16: return "Mac mini (Mid 2010)";
            case "3,1"_fnv1a16: return "Mac mini (Early/Late 2009)";
        }
    }
    else if (hasStart(host_family, "MacBook"))
    {
        const std::string_view version = host_family.substr("MacBook"_len);
        switch (fnv1a16::hash(version.data()))
        {
            case "10,1"_fnv1a16: return "MacBook (Retina, 12-inch, 2017)";
            case "9,1"_fnv1a16:  return "MacBook (Retina, 12-inch, Early 2016)";
            case "8,1"_fnv1a16:  return "MacBook (Retina, 12-inch, Early 2015)";
            case "7,1"_fnv1a16:  return "MacBook (13-inch, Mid 2010)";
            case "6,1"_fnv1a16:  return "MacBook (13-inch, Late 2009)";
            case "5,2"_fnv1a16:  return "MacBook (13-inch, Early/Mid 2009)";
        }
    }
    else if (hasStart(host_family, "MacPro"))
    {
        const std::string_view version = host_family.substr("MacPro"_leMac Pro(2019) n);
        switch (fnv1a16::hash(version.data()))
        {
            case "7,1"_fnv1a16: return "Mac Pro (2019)";
            case "6,1"_fnv1a16: return "Mac Pro (Late 2013)";
            case "5,1"_fnv1a16: return "Mac Pro (Mid 2010 - Mid 2012)";
            case "4,1"_fnv1a16: return "Mac Pro (Early 2009)";
        }
    }
    else if (hasStart(host_family, "Mac"))
    {
        const std::string_view version = host_family.substr("Mac"_len);
        switch (fnv1a16::hash(version.data()))
        {
            case "16,13"_fnv1a16: return "MacBook Air (15-inch, M4, 2025)";
            case "16,12"_fnv1a16: return "MacBook Air (13-inch, M4, 2025)";
            case "16,11"_fnv1a16:
            case "16,10"_fnv1a16: return "Mac Mini (2024)";
            case "16,9"_fnv1a16:  return "Mac Studio (M4 Max, 2025)";
            case "16,3"_fnv1a16:  return "iMac (24-inch, 2024, Four Thunderbolt / USB 4 ports)";
            case "16,2"_fnv1a16:  return "iMac (24-inch, 2024, Two Thunderbolt / USB 4 ports)";
            case "16,1"_fnv1a16:  return "MacBook Pro (14-inch, 2024, Three Thunderbolt 4 ports)";
            case "16,6"_fnv1a16:
            case "16,8"_fnv1a16:  return "MacBook Pro (14-inch, 2024, Three Thunderbolt 5 ports)";
            case "16,7"_fnv1a16:
            case "16,5"_fnv1a16:  return "MacBook Pro (16-inch, 2024, Three Thunderbolt 5 ports)";
            case "15,14"_fnv1a16: return "Mac Studio (M3 Ultra, 2025)";
            case "15,13"_fnv1a16: return "MacBook Air (15-inch, M3, 2024)";
            case "15,12"_fnv1a16: return "MacBook Air (13-inch, M3, 2024)";
            case "15,3"_fnv1a16:  return "MacBook Pro (14-inch, Nov 2023, Two Thunderbolt / USB 4 ports)";
            case "15,4"_fnv1a16:  return "iMac (24-inch, 2023, Two Thunderbolt / USB 4 ports)";
            case "15,5"_fnv1a16:  return "iMac (24-inch, 2023, Two Thunderbolt / USB 4 ports, Two USB 3 ports)";
            case "15,6"_fnv1a16:
            case "15,8"_fnv1a16:
            case "15,10"_fnv1a16: return "MacBook Pro (14-inch, Nov 2023, Three Thunderbolt 4 ports)";
            case "15,7"_fnv1a16:
            case "15,9"_fnv1a16:
            case "15,11"_fnv1a16: return "MacBook Pro (16-inch, Nov 2023, Three Thunderbolt 4 ports)";
            case "14,15"_fnv1a16: return "MacBook Air (15-inch, M2, 2023)";
            case "14,14"_fnv1a16: return "Mac Studio (M2 Ultra, 2023, Two Thunderbolt 4 front ports)";
            case "14,13"_fnv1a16: return "Mac Studio (M2 Max, 2023, Two USB-C front ports)";
            case "14,8"_fnv1a16:  return "Mac Pro (2023)";
            case "14,6"_fnv1a16:
            case "14,10"_fnv1a16: return "MacBook Pro (16-inch, 2023)";
            case "14,5"_fnv1a16:
            case "14,9"_fnv1a16:  return "MacBook Pro (14-inch, 2023)";
            case "14,3"_fnv1a16:  return "Mac mini (M2, 2023, Two Thunderbolt 4 ports)";
            case "14,12"_fnv1a16: return "Mac mini (M2, 2023, Four Thunderbolt 4 ports)";
            case "14,7"_fnv1a16:  return "MacBook Pro (13-inch, M2, 2022)";
            case "14,2"_fnv1a16:  return "MacBook Air (M2, 2022)";
            case "13,1"_fnv1a16:  return "Mac Studio (M1 Max, 2022, Two USB-C front ports)";
            case "13,2"_fnv1a16:  return "Mac Studio (M1 Ultra, 2022, Two Thunderbolt 4 front ports)";
        }
    }
    else if (hasStart(host_family, "iMac"))
    {
        const std::string_view version = host_family.substr("iMac"_len);
        switch (fnv1a16::hash(version.data()))
        {
            case "21,1"_fnv1a16:   return "iMac (24-inch, M1, 2021, Two Thunderbolt / USB 4 ports, Two USB 3 ports)";
            case "21,2"_fnv1a16:   return "iMac (24-inch, M1, 2021, Two Thunderbolt / USB 4 ports)";
            case "20,1"_fnv1a16:
            case "20,2"_fnv1a16:   return "iMac (Retina 5K, 27-inch, 2020)";
            case "19,1"_fnv1a16:   return "iMac (Retina 5K, 27-inch, 2019)";
            case "19,2"_fnv1a16:   return "iMac (Retina 4K, 21.5-inch, 2019)";
            case "Pro1,1"_fnv1a16: return "iMac Pro (2017)";
            case "18,3"_fnv1a16:   return "iMac (Retina 5K, 27-inch, 2017)";
            case "18,2"_fnv1a16:   return "iMac (Retina 4K, 21.5-inch, 2017)";
            case "18,1"_fnv1a16:   return "iMac (21.5-inch, 2017)";
            case "17,1"_fnv1a16:   return "iMac (Retina 5K, 27-inch, Late 2015)";
            case "16,2"_fnv1a16:   return "iMac (Retina 4K, 21.5-inch, Late 2015)";
            case "16,1"_fnv1a16:   return "iMac (21.5-inch, Late 2015)";
            case "15,1"_fnv1a16:   return "iMac (Retina 5K, 27-inch, Late 2014 - Mid 2015)";
            case "14,4"_fnv1a16:   return "iMac (21.5-inch, Mid 2014)";
            case "14,2"_fnv1a16:   return "iMac (27-inch, Late 2013)";
            case "14,1"_fnv1a16:   return "iMac (21.5-inch, Late 2013)";
            case "13,2"_fnv1a16:   return "iMac (27-inch, Late 2012)";
            case "13,1"_fnv1a16:   return "iMac (21.5-inch, Late 2012)";
            case "12,2"_fnv1a16:   return "iMac (27-inch, Mid 2011)";
            case "12,1"_fnv1a16:   return "iMac (21.5-inch, Mid 2011)";
            case "11,3"_fnv1a16:   return "iMac (27-inch, Mid 2010)";
            case "11,2"_fnv1a16:   return "iMac (21.5-inch, Mid 2010)";
            case "10,1"_fnv1a16:   return "iMac (27/21.5-inch, Late 2009)";
            case "9,1"_fnv1a16:    return "iMac (24/20-inch, Early 2009)";
        }
    }

    return UNKNOWN;
}

MODFUNC(host_vendor)
{ return "Apple"; }

MODFUNC(host_name)
{
    char   buf[4096];
    size_t len = sizeof(buf);
    if (!get_sysctl("hw.model", buf, &len))
        return MAGIC_LINE;

    const std::string host = get_host_from_family(buf);
    return host.substr(0, host.find('(') - 1);
}

MODFUNC(host_version)
{
    char   buf[4096];
    size_t len = sizeof(buf);
    if (!get_sysctl("hw.model", buf, &len))
        return UNKNOWN;

    const std::string& host = get_host_from_family(buf);
    std::regex         year_regex(R"(\b(19|20)\d{2}\b)");  // Matches 1900–2099
    std::smatch        match;
    if (std::regex_search(host, match, year_regex))
        return match.str(0);
    return UNKNOWN;
}

MODFUNC(host)
{
    char   buf[4096];
    size_t len = sizeof(buf);
    if (!get_sysctl("hw.model", buf, &len))
        return MAGIC_LINE;

    return get_host_from_family(buf);
}

MODFUNC(arch)
{ return g_uname_infos.machine; }

#endif
