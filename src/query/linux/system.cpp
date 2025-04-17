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

#include "platform.hpp"
#if CF_LINUX

#include <linux/limits.h>
#include <unistd.h>

#include <ctime>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#include "config.hpp"
#include "query.hpp"
#include "util.hpp"
#include "switch_fnv1a.hpp"
#include "utils/packages.hpp"

using namespace Query;

static void get_host_paths(System::System_t& paths)
{
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    if (std::filesystem::exists(syspath + "/board_name"))
    {
        paths.host_modelname = read_by_syspath(syspath + "/board_name");
        paths.host_version   = read_by_syspath(syspath + "/board_version");
        paths.host_vendor    = read_by_syspath(syspath + "/board_vendor");

        if (paths.host_vendor == "Micro-Star International Co., Ltd.")
            paths.host_vendor = "MSI";
    }

    else if (std::filesystem::exists(syspath + "/product_name"))
    {
        paths.host_modelname = read_by_syspath(syspath + "/product_name");
        if (hasStart(paths.host_modelname, "Standard PC"))
        {
            // everyone does it like "KVM/QEMU Standard PC (...) (host_version)" so why not
            paths.host_vendor  = "KVM/QEMU";
            paths.host_version = std::string_view('(' + read_by_syspath(syspath + "/product_version") + ')').data();
        }
        else
            paths.host_version = read_by_syspath(syspath + "/product_version");
    }
}

static System::System_t get_system_infos_lsb_releases()
{
    System::System_t ret;

    debug("calling in System {}", __PRETTY_FUNCTION__);
    std::string lsb_release_path;
    constexpr std::array<std::string_view, 3> lsb_paths = { "/etc/lsb-release", "/usr/lib/lsb-release" };
    for (const std::string_view path : lsb_paths)
    {
        if (std::filesystem::exists(path))
        {
            lsb_release_path = path;
            break;
        }
    }

    std::ifstream os_release_file(lsb_release_path, std::ios::in);
    if (!os_release_file.is_open())
    {
        error(_("Failed to get OS infos"), lsb_release_path);
        return ret;
    }

    // get OS /etc/lsb-release infos
    std::uint16_t iter_index = 0;
    std::string   line;
    while (std::getline(os_release_file, line) && iter_index < 3)
    {
        if (hasStart(line, "DISTRIB_DESCRIPTION="))
            getFileValue(iter_index, line, ret.os_pretty_name, "DISTRIB_DESCRIPTION="_len);

        else if (hasStart(line, "DISTRIB_ID="))
            getFileValue(iter_index, line, ret.os_id, "DISTRIB_ID="_len);

        else if (hasStart(line, "DISTRIB_CODENAME="))
            getFileValue(iter_index, line, ret.os_version_codename, "DISTRIB_CODENAME="_len);
    }

    return ret;
}

static System::System_t get_system_infos_os_releases()
{
    System::System_t ret;

    debug("calling in System {}", __PRETTY_FUNCTION__);
    std::string os_release_path;
    constexpr std::array<std::string_view, 3> os_paths = { "/etc/os-release", "/usr/lib/os-release", "/usr/share/os-release" };
    for (const std::string_view path : os_paths)
    {
        if (std::filesystem::exists(path))
        {
            os_release_path = path;
            break;
        }
    }

    std::ifstream os_release_file(os_release_path, std::ios::in);
    if (!os_release_file.is_open())
    {
        //error(_("Could not open '{}'\nFailed to get OS infos"), os_release_path);
        return ret;
    }

    // get OS /etc/os-release infos
    std::uint16_t iter_index = 0;
    std::string   line;
    while (std::getline(os_release_file, line) && iter_index < 5)
    {
        if (hasStart(line, "PRETTY_NAME="))
            getFileValue(iter_index, line, ret.os_pretty_name, "PRETTY_NAME="_len);

        else if (hasStart(line, "NAME="))
            getFileValue(iter_index, line, ret.os_name, "NAME="_len);

        else if (hasStart(line, "ID="))
            getFileValue(iter_index, line, ret.os_id, "ID="_len);

        else if (hasStart(line, "VERSION_ID="))
            getFileValue(iter_index, line, ret.os_version_id, "VERSION_ID="_len);

        else if (hasStart(line, "VERSION_CODENAME="))
            getFileValue(iter_index, line, ret.os_version_codename, "VERSION_CODENAME="_len);
    }

    return ret;
}

static unsigned long get_uptime()
{
    const std::string& buf = read_by_syspath("/proc/uptime");
    if (buf != UNKNOWN)
        return std::stoul(buf.substr(0,buf.find('.'))); // 19065.18 190952.06

    struct std::timespec uptime;
    if (clock_gettime(CLOCK_BOOTTIME, &uptime) != 0)
        return 0;

    return (unsigned long)uptime.tv_sec * 1000 + (unsigned long)uptime.tv_nsec / 1000000;
}

System::System()
{
    CHECK_INIT(!m_bInit)
    {
        if (uname(&m_uname_infos) != 0)
            die(_("uname() failed: {}\nCould not get system infos"), strerror(errno));

        m_uptime = get_uptime();
        m_system_infos = get_system_infos_os_releases();
        if (m_system_infos.os_name == UNKNOWN || m_system_infos.os_pretty_name == UNKNOWN)
            m_system_infos = get_system_infos_lsb_releases();

        get_host_paths(m_system_infos);
    }
    m_bInit = true;
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

// clang-format on
std::string& System::os_initsys_name()
{
    static bool done = false;
    if (done && !is_live_mode)
        return m_system_infos.os_initsys_name;
    
    // there's no way PID 1 doesn't exist.
    // This will always succeed (because we are on linux)
    std::ifstream f_initsys("/proc/1/comm", std::ios::binary);
    if (!f_initsys.is_open())
        die(_("/proc/1/comm doesn't exist! (what?)"));

    std::string initsys;
    std::getline(f_initsys, initsys);
    size_t pos = 0;

    if ((pos = initsys.find('\0')) != std::string::npos)
        initsys.erase(pos);

    if ((pos = initsys.rfind('/')) != std::string::npos)
        initsys.erase(0, pos + 1);

    m_system_infos.os_initsys_name = initsys;

    done = true;

    return m_system_infos.os_initsys_name;
}

std::string& System::os_initsys_version()
{
    static bool done = false;
    if (done && !is_live_mode)
        return m_system_infos.os_initsys_version;

    std::string path;
    char buf[PATH_MAX];
    if (realpath(which("init").c_str(), buf))
        path = buf;

    std::ifstream f(path, std::ios::in);
    std::string line;

    const std::string& name = str_tolower(this->os_initsys_name());
    switch (fnv1a16::hash(name))
    {
        case "systemd"_fnv1a16:
        case "systemctl"_fnv1a16:
        {
            while (read_binary_file(f, line))
            {
                if (hasEnding(line, "running in %ssystem mode (%s)"))
                {
                    m_system_infos.os_initsys_version = line.substr("systemd "_len);
                    m_system_infos.os_initsys_version.erase(m_system_infos.os_initsys_version.find(' '));
                    break;
                }
            }
        }
        break;
        case "openrc"_fnv1a16:
        {
            std::string tmp;
            while(read_binary_file(f, line))
            {
                if (line == "RC_VERSION")
                {
                    m_system_infos.os_initsys_version = tmp;
                    break;
                }
                tmp = line;
            }
        }
        break;
    }
    done = true;
    return m_system_infos.os_initsys_version;
}

std::string& System::pkgs_installed(const Config& config)
{
    static bool done = false;
    if (!done || is_live_mode)
    {
        m_system_infos.pkgs_installed = get_all_pkgs(config);
        done = true;
    }

    return m_system_infos.pkgs_installed;
}

#endif
