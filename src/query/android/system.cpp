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

#include <sys/stat.h>
#include <unistd.h>

#include <ctime>
#include <array>
#include <string_view>

#include "../linux/utils/packages.hpp"
#include "query.hpp"
#include "util.hpp"

using namespace Query;

static System::System_t get_system_infos()
{
    System::System_t ret;

    ret.os_name             = "Android";
    ret.os_id               = "android";
    ret.os_version_id       = get_android_property("ro.build.version.release");
    ret.os_version_codename = get_android_property("ro.build.version.codename");
    ret.os_pretty_name      = "Android " + ret.os_version_codename + " " + ret.os_version_id;

    constexpr std::array<std::string_view, 8> vendors_prop_names = {
        "ro.product.marketname",   "ro.vendor.product.display", "ro.config.devicename", "ro.config.marketing_name",
        "ro.product.vendor.model", "ro.product.oppo_model",     "ro.oppo.market.name",  "ro.product.brand"
    };
    for (const std::string_view name : vendors_prop_names)
    {
        if (ret.host_modelname.empty() || ret.host_modelname == UNKNOWN)
            ret.host_modelname = get_android_property(name);
        else
            break;
    }
    ret.host_vendor  = get_android_property("ro.product.manufacturer");
    ret.host_version = get_android_property("ro.product.model");

    if (access("/system/bin/init", F_OK) == 0)
    {
        ret.os_initsys_name = "init";
        ret.os_initsys_version.clear();
    }

    return ret;
}

static unsigned long get_uptime()
{
    struct std::timespec uptime;
    if (clock_gettime(CLOCK_BOOTTIME, &uptime) != 0)
        return 0;

    return (uint64_t)uptime.tv_sec + (uint64_t)uptime.tv_nsec / 1000000;
}

System::System()
{
    CHECK_INIT(m_bInit);

    if (uname(&m_uname_infos) != 0)
        die("uname() failed: {}\nCould not get system infos", strerror(errno));

    m_uptime = get_uptime();
    m_system_infos = get_system_infos();
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
{
    static bool done = false;
    if (!done)
    {
        m_system_infos.pkgs_installed = get_all_pkgs(config);

        done = true;
    }

    return m_system_infos.pkgs_installed;
}

#endif
