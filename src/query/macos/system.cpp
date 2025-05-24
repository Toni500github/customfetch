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
#if CF_MACOS

#include <sys/sysctl.h>
#include <sys/time.h>

#include <string_view>

#include "util.hpp"
#include "query.hpp"
#include "rapidxml-1.13/rapidxml.hpp"
#include "switch_fnv1a.hpp"

using namespace Query;

static std::string get_codename(const std::string_view os_version_id)
{
    std::string major;
    std::string minor{UNKNOWN};
    size_t pos1 = os_version_id.find('.');
    if (pos1 != os_version_id.npos)
    {
        major = os_version_id.substr(0, pos1);
        size_t pos2 = os_version_id.find('.', pos1+1);
        if (pos2 != os_version_id.npos)
           minor = os_version_id.substr(pos1+1, pos2);
    }

    switch (fnv1a16::hash(major))
    {
        case "15"_fnv1a16: return "Sequoia";
        case "14"_fnv1a16: return "Sonoma";
        case "13"_fnv1a16: return "Ventura";
        case "12"_fnv1a16: return "Monterey";
        case "11"_fnv1a16: return "Big Sur";
        case "10"_fnv1a16:
        {
            switch (fnv1a16::hash(minor))
            {
                case "16"_fnv1a16: return "Big Sur";
                case "15"_fnv1a16: return "Catalina";
                case "14"_fnv1a16: return "Mojave";
                case "13"_fnv1a16: return "High Sierra";
                case "12"_fnv1a16: return "Sierra";
                case "11"_fnv1a16: return "El Capitan";
                case "10"_fnv1a16: return "Yosemite";
                case "9"_fnv1a16: return "Mavericks";
                case "8"_fnv1a16: return "Mountain Lion";
                case "7"_fnv1a16: return "Lion";
                case "6"_fnv1a16: return "Snow Leopard";
                case "5"_fnv1a16: return "Leopard";
                case "4"_fnv1a16: return "Tiger";
                case "3"_fnv1a16: return "Panther";
                case "2"_fnv1a16: return "Jaguar";
                case "1"_fnv1a16: return "Puma";
                case "0"_fnv1a16: return "Cheetah";
            }
        }
    }

    return UNKNOWN;
}

static System::System_t get_os_infos()
{
    System::System_t ret;
    std::ifstream f("/System/Library/CoreServices/SystemVersion.plist", std::ios::in);
    if (!f.is_open())
        die("Couldn't get MacOS base infos");

    std::string buffer(std::istreambuf_iterator<char>{f}, std::istreambuf_iterator<char>{});
    buffer.push_back('\0');

    rapidxml::xml_document<> doc;
    doc.parse<0>(&buffer[0]);
    rapidxml::xml_node<>* root_node = doc.first_node("plist")->first_node("dict")->first_node("key");

    for (; root_node; root_node = root_node->next_sibling())
    {
        const std::string_view key = root_node->value(); // <key>ProductName</key>
        root_node = root_node->next_sibling();
        const std::string_view value = root_node->value(); // <string>macOS</string>
        if (key == "ProductName")
            ret.os_name = value;
        else if (key == "ProductUserVisibleVersion")
            ret.os_version_id = value;
    }
    ret.os_pretty_name = ret.os_name + " " + ret.os_version_id;

    ret.os_version_codename = get_codename(ret.os_version_id);
    if (ret.os_version_codename != UNKNOWN)
        ret.os_pretty_name += " (" + ret.os_version_codename + ")";

    return ret;
}

System::System()
{
    CHECK_INIT(m_bInit);

    if (uname(&m_uname_infos) != 0)
        die(_("uname() failed: {}\nCould not get system infos"), strerror(errno));

    struct timeval boot_time;
    size_t size = sizeof(boot_time);
    int name[] = {CTL_KERN, KERN_BOOTTIME};
    if(sysctl(name, 2, &boot_time, &size, NULL, 0) != 0)
        die(_("failed to get uptime"));

    m_uptime = time(NULL) - boot_time.tv_sec;
    m_system_infos = get_os_infos();
}

// clang-format off
std::string System::kernel_name() noexcept
{ return m_uname_infos.sysname; }

std::string System::kernel_version() noexcept
{ return m_uname_infos.release; }

std::string System::hostname() noexcept
{ return m_uname_infos.nodename; }

std::string System::arch() noexcept
{ return m_uname_infos.machine; }

unsigned long& System::uptime() noexcept
{ return m_uptime; }

std::string& System::os_pretty_name() noexcept
{ return m_system_infos.os_pretty_name; }

std::string& System::os_name() noexcept
{ return m_system_infos.os_name; }

std::string& System::os_id() noexcept
{ return m_system_infos.os_id; }

std::string& System::os_versionid() noexcept
{ return m_system_infos.os_version_id; }

std::string& System::os_version_codename() noexcept
{ return m_system_infos.os_version_codename; }

std::string& System::host_modelname() noexcept
{ return m_system_infos.host_modelname; }

std::string& System::host_vendor() noexcept
{ return m_system_infos.host_vendor; }

std::string& System::host_version() noexcept
{ return m_system_infos.host_version; }

std::string& System::os_initsys_name()
{ return m_system_infos.os_initsys_name; }

std::string& System::os_initsys_version()
{ return m_system_infos.os_initsys_version; }

std::string& System::pkgs_installed(const Config& config)
{ return m_system_infos.pkgs_installed; }

#endif // !CF_MACOS
