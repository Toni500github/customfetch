#ifndef _QUERY_HPP
#define _QUERY_HPP

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

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

using systemInfo_t =
    std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::string, size_t, float>>>;
using variant = std::variant<std::string, size_t, float>;

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
        std::string shell_name{ UNKNOWN };
        std::string shell_version{ UNKNOWN };
        std::string wm_name{ MAGIC_LINE };
        std::string de_name{ MAGIC_LINE };
        std::string de_version{ UNKNOWN };
        std::string term_name{ MAGIC_LINE };
        std::string term_version{ MAGIC_LINE };
    };

    User() noexcept;

    std::string  name() noexcept;
    std::string  shell_path() noexcept;
    std::string& shell_name() noexcept;
    std::string& shell_version(const std::string_view shell_name);
    std::string& wm_name(bool dont_query_dewm, const std::string_view term_name);
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

    Theme(const std::uint8_t ver, systemInfo_t& queried_themes, std::vector<std::string>& queried_themes_names,
          const std::string& theme_name_version, const Config &config);

    Theme(systemInfo_t& queried_themes, const Config &config);

    std::string  gtk_theme() noexcept;
    std::string  gtk_icon_theme() noexcept;
    std::string  gtk_font() noexcept;
    std::string& cursor() noexcept;
    std::string& cursor_size() noexcept;

private:
    User              query_user;
    static Theme_t    m_theme_infos;
    systemInfo_t&     m_queried_themes;
    const std::string m_theme_name_version;
    std::string       m_wmde_name;
    const Config&     m_Config;
};

class CPU
{
public:
    struct CPU_t
    {
        std::string name{ UNKNOWN };
        std::string nproc{ UNKNOWN };

        float freq_max        = 0;
        float freq_min        = 0;
        float freq_cur        = 0;
        float freq_bios_limit = 0;

        // private:
        float freq_max_cpuinfo = 0;
    };

    CPU() noexcept;

    std::string& name() noexcept;
    std::string& nproc() noexcept;
    float&       freq_max() noexcept;
    float&       freq_min() noexcept;
    float&       freq_cur() noexcept;
    float&       freq_bios_limit() noexcept;

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

    GPU(const std::uint16_t id, std::vector<std::uint16_t>& queried_gpus);

    std::string& name() noexcept;
    std::string& vendor() noexcept;

private:
    uint16_t    m_vendor_id;
    uint16_t    m_device_id;
    std::string m_vendor_id_s;
    std::string m_device_id_s;

    static GPU_t m_gpu_infos;
};

class Disk
{
public:
    struct Disk_t
    {
        float       total_amount = 0;
        float       free_amount  = 0;
        float       used_amount  = 0;
        std::string typefs;
        std::string device;
        std::string mountdir;
    };

    Disk(const std::string_view path, std::vector<std::string_view>& paths);

    float&       total_amount() noexcept;
    float&       free_amount() noexcept;
    float&       used_amount() noexcept;
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
        float total_amount      = 0;
        float free_amount       = 0;
        float used_amount       = 0;
        float swap_free_amount  = 0;
        float swap_used_amount  = 0;
        float swap_total_amount = 0;
    };

    RAM() noexcept;

    float& total_amount() noexcept;
    float& free_amount() noexcept;
    float& used_amount() noexcept;
    float& swap_free_amount() noexcept;
    float& swap_used_amount() noexcept;
    float& swap_total_amount() noexcept;

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
