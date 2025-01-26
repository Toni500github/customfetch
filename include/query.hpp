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

#ifndef _QUERY_HPP
#define _QUERY_HPP

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <variant>

#include "config.hpp"
#include "util.hpp"

extern "C" {
#include <mntent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
}

// Special variable for storing info modules values
using systemInfo_t =
    std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::string, size_t, double>>>;
// used in systemInfo_t most of the time
using variant = std::variant<std::string, size_t, double>;

#if !ANDROID_APP
#define CHECK_INIT(x) if (x)
#else
#define CHECK_INIT(x) if (true)
#endif

namespace Query
{

class System
{
public:
    struct System_t
    {
        std::string os_pretty_name{ UNKNOWN };
        std::string os_name{ UNKNOWN };
        std::string os_id{ UNKNOWN };
        std::string os_version_id{ UNKNOWN };
        std::string os_version_codename{ UNKNOWN };
        std::string os_initsys_name{ UNKNOWN };
        std::string os_initsys_version{ UNKNOWN };

        std::string host_modelname{ UNKNOWN };
        std::string host_version{ UNKNOWN };
        std::string host_vendor{ UNKNOWN };

        std::string pkgs_installed{ UNKNOWN };
    };

    System();

    std::string  kernel_name() noexcept;
    std::string  kernel_version() noexcept;
    std::string  hostname() noexcept;
    std::string  arch() noexcept;
    std::string& os_pretty_name() noexcept;
    std::string& os_name() noexcept;
    std::string& os_id() noexcept;
    std::string& os_initsys_name();
    std::string& os_initsys_version();
    std::string& os_versionid() noexcept;
    std::string& os_version_codename() noexcept;
    long&        uptime() noexcept;

    // motherboard (host)
    std::string& host_modelname() noexcept;
    std::string& host_vendor() noexcept;
    std::string& host_version() noexcept;

    std::string& pkgs_installed(const Config& config);

private:
    static System_t       m_system_infos;
    static bool           m_bInit;
    static struct utsname m_uname_infos;
    static struct sysinfo m_sysInfos;
};

class User
{
public:
    struct User_t
    {
        std::string shell_path{ MAGIC_LINE };
        std::string shell_name{ MAGIC_LINE };
        std::string shell_version{ UNKNOWN };
        std::string wm_name{ MAGIC_LINE };
        std::string wm_version{ UNKNOWN };
        std::string de_name{ MAGIC_LINE };
        std::string de_version{ UNKNOWN };
        std::string term_name{ MAGIC_LINE };
        std::string term_version{ MAGIC_LINE };
        // private:
        std::string m_wm_path;
    };

    User() noexcept;

    std::string  name() noexcept;
    std::string  shell_path() noexcept;
    std::string& shell_name() noexcept;
    std::string& shell_version(const std::string_view shell_name);
    std::string& wm_name(bool dont_query_dewm, const std::string_view term_name);
    std::string& wm_version(bool dont_query_dewm, const std::string_view term_name);
    std::string& de_name(bool dont_query_dewm, const std::string_view term_name, const std::string_view wm_name);
    std::string& de_version(const std::string_view de_name);
    std::string& term_name();
    std::string& term_version(const std::string_view term_name);

    static bool m_bDont_query_dewm;

private:
    static bool           m_bInit;
    static User_t         m_users_infos;
    static struct passwd* m_pPwd;
};

class Theme
{
public:
    struct Theme_t
    {
        std::string gtk_theme_name{ MAGIC_LINE };
        std::string gtk_icon_theme{ MAGIC_LINE };
        std::string gtk_font{ MAGIC_LINE };
        std::string cursor{ MAGIC_LINE };
        std::string cursor_size{ UNKNOWN };
    };

    Theme(const std::uint8_t ver, systemInfo_t& queried_themes, const std::string& theme_name_version,
          const Config& config, const bool gsettings_only = false);

    Theme(systemInfo_t& queried_themes, const Config& config, const bool gsettings_only = false);

    std::string  gtk_theme() noexcept;
    std::string  gtk_icon_theme() noexcept;
    std::string  gtk_font() noexcept;
    std::string& cursor() noexcept;
    std::string& cursor_size() noexcept;

private:
    User           query_user;
    std::string    m_wmde_name;
    std::string    m_theme_name;
    systemInfo_t&  m_queried_themes;
    static Theme_t m_theme_infos;
};

class CPU
{
public:
    struct CPU_t
    {
        std::string name{ UNKNOWN };
        std::string nproc{ UNKNOWN };

        double freq_max        = 0;
        double freq_min        = 0;
        double freq_cur        = 0;
        double freq_bios_limit = 0;
        double temp            = 0;

        // private:
        double freq_max_cpuinfo = 0;
        // only in Android
        std::string modelname;
        std::string vendor;
    };

    CPU() noexcept;

    std::string& name() noexcept;
    std::string& nproc() noexcept;

    // only in Android
    std::string& vendor() noexcept;
    std::string& modelname() noexcept;

    double& freq_max() noexcept;
    double& freq_min() noexcept;
    double& freq_cur() noexcept;
    double& freq_bios_limit() noexcept;
    double& temp() noexcept;

private:
    static bool  m_bInit;
    static CPU_t m_cpu_infos;
};

class GPU
{
public:
    struct GPU_t
    {
        std::string name{ UNKNOWN };
        std::string vendor{ UNKNOWN };
    };

    GPU(const std::string& id, systemInfo_t& queried_gpus);

    std::string& name() noexcept;
    std::string& vendor() noexcept;

private:
    uint16_t    m_vendor_id;
    uint16_t    m_device_id;
    std::string m_vendor_id_s;
    std::string m_device_id_s;

    static GPU_t m_gpu_infos;
};

class Battery
{
public:
    struct Battery_t
    {
        std::string modelname{ MAGIC_LINE };
        std::string vendor{ MAGIC_LINE };
        std::string status{ MAGIC_LINE };
        std::string technology{ UNKNOWN };
        std::string capacity_level{ UNKNOWN };
        double      temp{ 0 };
        double      capacity{ 0 };
    };

    Battery();

    std::string& modelname() noexcept;
    std::string& vendor() noexcept;
    std::string& status() noexcept;
    std::string& technology() noexcept;
    std::string& capacity_level() noexcept;
    double&      capacity() noexcept;
    double&      temp() noexcept;

private:
    static bool      m_bInit;
    static Battery_t m_battery_infos;
};

class Disk
{
public:
    struct Disk_t
    {
        double      total_amount = 0;
        double      free_amount  = 0;
        double      used_amount  = 0;
        std::string typefs;
        std::string device;
        std::string mountdir;
    };

    Disk(const std::string& path, systemInfo_t& queried_paths);

    double&      total_amount() noexcept;
    double&      free_amount() noexcept;
    double&      used_amount() noexcept;
    std::string& typefs() noexcept;
    std::string& device() noexcept;
    std::string& mountdir() noexcept;

private:
    static struct statvfs m_statvfs;
    static Disk_t         m_disk_infos;
};

class RAM
{
public:
    struct RAM_t
    {
        double total_amount      = 0;
        double free_amount       = 0;
        double used_amount       = 0;
        double swap_free_amount  = 0;
        double swap_used_amount  = 0;
        double swap_total_amount = 0;
    };

    RAM() noexcept;

    double& total_amount() noexcept;
    double& free_amount() noexcept;
    double& used_amount() noexcept;
    double& swap_free_amount() noexcept;
    double& swap_used_amount() noexcept;
    double& swap_total_amount() noexcept;

private:
    static bool  m_bInit;
    static RAM_t m_memory_infos;
};

}  // namespace Query

// inline Query::System query_system;
// inline Query::CPU query_cpu;
// inline Query::GPU query_gpu;
// inline Query::RAM query_ram;

#endif
