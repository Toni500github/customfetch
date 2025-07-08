#include <dlfcn.h>
#include <mntent.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

#include "core-modules.hh"
#include "cufetch/cufetch.hh"
#include "config.hpp"
#include "fmt/format.h"
#include "util.hpp"

#if CF_LINUX
# include "linux/utils/packages.hpp"
#endif

using unused = const callbackInfo_t*;

std::string amount(const double amount, const moduleArgs_t* moduleArgs)
{
    constexpr std::array<std::string_view, 32> sorted_valid_prefixes = { "B",   "EB", "EiB", "GB", "GiB", "kB",
                                                                         "KiB", "MB", "MiB", "PB", "PiB", "TB",
                                                                         "TiB", "YB", "YiB", "ZB", "ZiB" };

    if (!moduleArgs->next || moduleArgs->next->value.empty())
    {
        byte_units_t amount_unit = auto_divide_bytes(amount, 1024);
        return fmt::format("{:.2f} {}", amount_unit.num_bytes, amount_unit.unit);
    }

    const std::string& prefix = moduleArgs->next->value;
    if (std::binary_search(sorted_valid_prefixes.begin(), sorted_valid_prefixes.end(), prefix))
        return fmt::format("{:.5f}", divide_bytes(amount, prefix).num_bytes);
    return "0";
}

static std::string get_auto_uptime(const std::uint16_t days, const std::uint16_t hours, const std::uint16_t mins,
                                   const std::uint16_t secs, const Config& config)
{
    if (days == 0 && hours == 0 && mins == 0)
        return fmt::format("{}{}", secs, config.uptime_s_fmt);

    std::string ret;

    if (days > 0)
        ret += fmt::format("{}{}, ", days, config.uptime_d_fmt);

    if (hours > 0)
        ret += fmt::format("{}{}, ", hours, config.uptime_h_fmt);

    if (mins > 0)
        ret += fmt::format("{}{}, ", mins, config.uptime_m_fmt);

    ret.erase(ret.length() - 2);  // the last ", "

    return ret;
}

static std::string get_colors_symbol(const callbackInfo_t* callback, bool is_light)
{
    const moduleArgs_t* symbolArg;
    for (symbolArg = callback->moduleArgs; symbolArg && symbolArg->name != "symbol";
         symbolArg = symbolArg->next)
        ;
    if (symbolArg->value.empty())
        die(
            _("color symbol palette argument module is empty.\n"
              "Must be used like 'colors_symbol(`symbol for printing the color palette`)'"));

    if (is_light)
        return parse(
            fmt::format("${{\033[90m}} {0} ${{\033[91m}} {0} ${{\033[92m}} {0} ${{\033[93m}} {0} ${{\033[94m}} "
                        "{0} ${{\033[95m}} {0} ${{\033[96m}} {0} ${{\033[97m}} {0} ${{0}}",
                        symbolArg->value),
            callback->parse_args);
    else
        return parse(
            fmt::format("${{\033[30m}} {0} ${{\033[31m}} {0} ${{\033[32m}} {0} ${{\033[33m}} {0} ${{\033[34m}} "
                        "{0} ${{\033[35m}} {0} ${{\033[36m}} {0} ${{\033[37m}} {0} ${{0}}",
                        symbolArg->value),
            callback->parse_args);
}

MODFUNC(disk_fmt)
{
    const callbackInfo_t *callback = callbackInfo;
    const double used  = disk_used(callback);
    const double total = disk_total(callback);
    const std::string& perc =
                get_and_color_percentage(used, total, callback->parse_args, false);

    // clang-format off
    std::string result {fmt::format("{} / {} {}",
                        amount(used, callback->moduleArgs),
                        amount(total, callback->moduleArgs), 
                        parse("${0}(" + perc + ")", callback->parse_args))
                        };
    // clang-format on
    const std::string& fsname = disk_fsname(callback);
    if (fsname != MAGIC_LINE)
        result += " - " + fsname;
    
    const std::string& types = disk_types(callback);
    if (!types.empty())
        result += " [" + types + "]";

    return result;
}

void core_plugins_start(const Config& config)
{
    // ------------ INIT STUFF ------------
    const size_t uptime_secs  = os_uptime();
    const size_t uptime_mins  = uptime_secs / (60);
    const size_t uptime_hours = uptime_secs / (60 * 60);
    const size_t uptime_days  = uptime_secs / (60 * 60 * 24);

#if CF_LINUX
    if (uname(&g_uname_infos) != 0)
        die(_("uname() failed: {}\nCould not get system infos"), std::strerror(errno));

    if (g_pwd = getpwuid(getuid()), !g_pwd)
        die(_("getpwent failed: {}\nCould not get user infos"), std::strerror(errno));
#endif

    term_pid  = get_terminal_pid();
    term_name = get_terminal_name();
    if (hasStart(str_tolower(term_name), "login") || hasStart(term_name, "init") || hasStart(term_name, "(init)"))
    {
        is_tty    = true;
        term_name = ttyname(STDIN_FILENO);
    }
    os_release = fopen("/etc/os-release", "r");
    cpuinfo    = fopen("/proc/cpuinfo", "r");
    meminfo    = fopen("/proc/meminfo", "r");
    mountsFile = setmntent("/proc/mounts", "r");

    // ------------ MODULES REGISTERING ------------
    module_t os_name_pretty_module = {"pretty", "OS pretty name [Ubuntu 22.04.4 LTS; Arch Linux]", {}, os_pretty_name};
    module_t os_name_id_module = {"id", "OS id name [ubuntu, arch]", {}, os_name_id};
    module_t os_name_module = { "name", "OS basic name [Ubuntu]", {
        std::move(os_name_pretty_module),
        std::move(os_name_id_module)
    }, os_name };

    module_t os_uptime_s_module = {"secs", "uptime of the system in seconds [45]", {}, [=](unused) {return fmt::to_string(uptime_secs % 60);}};
    module_t os_uptime_m_module = {"mins", "uptime of the system in minutes [12]", {}, [=](unused) {return fmt::to_string(uptime_mins % 60);}};
    module_t os_uptime_h_module = {"hours", "uptime of the system in hours [34]", {}, [=](unused) {return fmt::to_string(uptime_hours % 24);}};
    module_t os_uptime_d_module = {"days", "uptime of the system in days [2]", {}, [=](unused) {return fmt::to_string(uptime_days);}};
    module_t os_uptime_module = {"uptime", "(auto) uptime of the system [36 mins, 3 hours, 23 days]", {
        std::move(os_uptime_s_module),
        std::move(os_uptime_m_module),
        std::move(os_uptime_h_module),
        std::move(os_uptime_d_module),
    }, [=](unused) { return get_auto_uptime(uptime_days, uptime_hours % 24, uptime_mins % 60,
                                                   uptime_secs % 60, config); }};

    module_t os_hostname_module = {"hostname", "hostname of the OS [myMainPC]", {}, os_hostname};

    module_t os_kernel_name_module = {"name", "kernel name [Linux]", {}, os_kernel_name};
    module_t os_kernel_version_module = {"version", "kernel version [6.9.3-zen1-1-zen]", {}, os_kernel_version};
    module_t os_kernel_module = {"kernel", "kernel name and version [Linux 6.9.3-zen1-1-zen]", {
        std::move(os_kernel_name_module),
        std::move(os_kernel_version_module)
    }, [](unused) {return os_kernel_name() + " " + os_kernel_version();}};

    module_t os_initsys_name_module = {"name", "Init system name [systemd]", {}, os_initsys_name};
    module_t os_initsys_version_module = {"version", "Init system version [256.5-1-arch]", {}, os_initsys_version};
    module_t os_initsys_module = {"initsys", "Init system name and version [systemd 256.5-1-arch]", {
        std::move(os_initsys_name_module),
        std::move(os_initsys_version_module),
    }, [](unused) {return os_initsys_name() + " " + os_initsys_version();}};

    module_t os_pkgs_module = {"pkgs", "Count of system packages", {}, [&](unused){ return get_all_pkgs(config); }};

    // $<os>
    module_t os_module = { "os", "OS modules", {
        std::move(os_name_module),
        std::move(os_uptime_module),
        std::move(os_kernel_module),
        std::move(os_hostname_module),
        std::move(os_initsys_module),
        std::move(os_pkgs_module),
    }, NULL};
    cfRegisterModule(os_module);

    // $<system>
    module_t host_name_module = {"name", "Host (aka. Motherboard) model name [PRO B550M-P GEN3 (MS-7D95)]", {}, host_name};
    module_t host_version_module = {"version", "Host (aka. Motherboard) model version [1.0]", {}, host_version};
    module_t host_vendor_module = {"vendor", "Host (aka. Motherboard) model vendor [Micro-Star International Co., Ltd.]", {}, host_vendor};
    module_t host_module = {"host", "Host (aka. Motherboard) model name with vendor and version [MSI PRO B550M-P GEN3 (MS-7D95) 1.0]", { 
        std::move(host_name_module), 
        std::move(host_version_module), 
        std::move(host_vendor_module) },
    host};

    module_t arch_module = {"arch", "the architecture of the machine [x86_64, aarch64]", {}, arch};

    module_t system_module = { "system", "System modules", { 
        std::move(host_module),
        std::move(arch_module),
    }, NULL };
    cfRegisterModule(system_module);

    // $<cpu>
    module_t cpu_name_module   = {"name", "CPU model name [AMD Ryzen 5 5500]", {}, cpu_name};
    module_t cpu_nproc_module  = {"nproc" , "CPU number of virtual processors [12]", {}, cpu_nproc};

    module_t cpu_freq_cur_module = {"current", "CPU current frequency (in GHz) [3.42]", {}, cpu_freq_cur};
    module_t cpu_freq_max_module = {"max", "CPU maximum frequency (in GHz) [4.90]", {}, cpu_freq_max};
    module_t cpu_freq_min_module = {"min", "CPU minimum frequency (in GHz) [2.45]", {}, cpu_freq_min};
    module_t cpu_freq_bios_module = {"bios_limit", "CPU frequency limited by bios (in GHz) [4.32]", {}, cpu_freq_bios};
    module_t cpu_freq_module = {"freq", "CPU frequency info (GHz)", {
        std::move(cpu_freq_cur_module),
        std::move(cpu_freq_max_module),
        std::move(cpu_freq_min_module),
        std::move(cpu_freq_bios_module),
    }, cpu_freq_max};

    module_t cpu_temp_C_module = {"C", "CPU temperature in Celsius [40.62]", {}, [](unused) {return fmt::format("{:.2f}°C", cpu_temp());}};
    module_t cpu_temp_F_module = {"F", "CPU temperature in Fahrenheit [105.12]", {}, [](unused) {return fmt::format("{:.2f}°F", cpu_temp() * 1.8 + 34);}};
    module_t cpu_temp_K_module = {"K", "CPU temperature in Kelvin [313.77]", {}, [](unused) {return fmt::format("{:.2f}°K", cpu_temp() + 273.15);}};
    module_t cpu_temp_module = {"temp", "CPU temperature (by the chosen unit) [40.62]", {
        std::move(cpu_temp_C_module),
        std::move(cpu_temp_F_module),
        std::move(cpu_temp_K_module),
    }, [](unused) {return fmt::format("{:.2f}°C", cpu_temp());}};

    module_t cpu_module = {"cpu", "CPU model name with number of virtual processors and max freq [AMD Ryzen 5 5500 (12) @ 4.90 GHz]",{
        std::move(cpu_name_module),
        std::move(cpu_nproc_module),
        std::move(cpu_freq_module),
        std::move(cpu_temp_module),
    }, [](unused) {
            return fmt::format("{} ({}) @ {} GHz", cpu_name(), cpu_nproc(), cpu_freq_max());
        }};
    cfRegisterModule(cpu_module);

    // $<user>
    module_t user_name_module = {"name", "name you are currently logged in (not real name) [toni69]", {}, user_name};

    module_t user_shell_path_module = {"path", "login shell (with path) [/bin/zsh]", {}, user_shell_path};
    module_t user_shell_name_module = {"name", "login shell [zsh]", {}, user_shell_name};
    module_t user_shell_version_module = {"version", "login shell version (may be not correct) [5.9]", {}, user_shell_version};
    module_t user_shell_module = {"shell", "login shell name and version [zsh 5.9]", {
        std::move(user_shell_name_module),
        std::move(user_shell_path_module),
        std::move(user_shell_version_module),
    }, [](unused) {return user_shell_name() + " " + user_shell_version();}};

    module_t user_term_name_module = {"name", "terminal name [alacritty]", {}, user_term_name};
    module_t user_term_version_module = {"version", "terminal version [0.13.2]", {}, user_shell_version};
    module_t user_term_module = {"terminal", "terminal name and version [alacritty 0.13.2]", {
        std::move(user_term_version_module),
        std::move(user_term_name_module)
    }, [](unused) {return user_term_name() + " " + user_term_version();}};

    module_t user_wm_name_module = {"name", "Window Manager current session name [dwm; xfwm4]", {}, user_wm_name};
    module_t user_wm_version_module = {"version", "Window Manager version (may not work correctly) [6.2; 4.18.0]", {}, user_wm_version};
    module_t user_wm_module = {"wm", "Window Manager current session name and version", {
        std::move(user_wm_version_module),
        std::move(user_wm_name_module)
    }, [](unused) {return user_wm_name() + " " + user_wm_version();}};

    module_t user_de_name_module = {"name", "Desktop Environment current session name [Plasma]", {}, user_de_name};
    module_t user_de_version_module = {"version", "Desktop Environment version (if available)", {}, user_de_version};
    module_t user_de_module = {"de", "Desktop Environment current session name and version", {
        std::move(user_de_version_module),
        std::move(user_de_name_module)
    }, [](unused) {return user_de_name() + " " + user_de_version();}};

    module_t user_module = {"user", "User modules", {
        std::move(user_name_module),
        std::move(user_shell_module),
        std::move(user_term_module),
        std::move(user_wm_module),
        std::move(user_de_module),
    }, NULL};
    cfRegisterModule(user_module);

    // $<ram>
    module_t ram_free_perc_module  = {"perc", "percentage of available amount of RAM in total [82.31%]", {}, [](const callbackInfo_t *callback) {return get_and_color_percentage(ram_free(), ram_total(), callback->parse_args, true);}};
    module_t ram_used_perc_module  = {"perc", "percentage of used amount of RAM in total [17.69%]", {}, [](const callbackInfo_t *callback) {return get_and_color_percentage(ram_used(), ram_total(), callback->parse_args, false);}};
    module_t ram_free_module  = {"free", "available amount of RAM (auto) [10.46 GiB]", {std::move(ram_free_perc_module)}, [](const callbackInfo_t *callback) { return amount(ram_free() * 1024,  callback->moduleArgs);  }};
    module_t ram_used_module  = {"used", "used amount of RAM (auto) [2.81 GiB]", {std::move(ram_used_perc_module)}, [](const callbackInfo_t *callback) { return amount(ram_used() * 1024,  callback->moduleArgs);  }};
    module_t ram_total_module = {"total", "total amount of RAM (auto) [15.88 GiB]", {}, [](const callbackInfo_t *callback) { return amount(ram_total() * 1024, callback->moduleArgs); }};
        
    module_t ram_module = {"ram", "used and total amount of RAM (auto) with used percentage [2.81 GiB / 15.88 GiB (5.34%)]", {
        std::move(ram_free_module),
        std::move(ram_used_module),
        std::move(ram_total_module)
    }, NULL};
    cfRegisterModule(ram_module);

    // $<swap>
    module_t swap_free_perc_module  = {"perc", "percentage of available amount of the swapfile in total [6.71%]", {}, [](const callbackInfo_t *callback) {return get_and_color_percentage(swap_free(), swap_total(), callback->parse_args, true);}};
    module_t swap_used_perc_module  = {"perc", "percentage of used amount of the swapfile in total [93.29%]", {}, [](const callbackInfo_t *callback) {return get_and_color_percentage(swap_used(), swap_total(), callback->parse_args, false);}};
    module_t swap_free_module  = {"free", "available amount of the swapfile (auto) [34.32 MiB]", {std::move(swap_free_perc_module)}, [](const callbackInfo_t *callback) { return amount(swap_free() * 1024,  callback->moduleArgs);  }};
    module_t swap_used_module  = {"used", "used amount of the swapfile (auto) [477.68 MiB]", {std::move(swap_used_perc_module)}, [](const callbackInfo_t *callback) { return amount(swap_used() * 1024,  callback->moduleArgs);  }};
    module_t swap_total_module = {"total", "total amount of the swapfile (auto) [512.00 MiB]", {}, [](const callbackInfo_t *callback) { return amount(swap_total() * 1024, callback->moduleArgs); }};

    module_t swap_module = {"swap", "used and total amount of the swapfile (auto) with used percentage [477.68 MiB / 512.00 MiB (88.45%)]", {
        std::move(swap_free_module),
        std::move(swap_used_module),
        std::move(swap_total_module)
    }, NULL};
    cfRegisterModule(swap_module);

    // $<disk>
    module_t disk_fsname_module = {"fs", "type of filesystem [ext4]", {}, disk_fsname};
    module_t disk_device_module = {"device", "path to device [/dev/sda5]", {}, disk_device};
    module_t disk_mountdir_module = {"mountdir", "path to the device mount point [/]", {}, disk_mountdir};
    module_t disk_types_module = {"types", "an array of type options (pretty format) [Regular, External]", {}, disk_types};

    module_t disk_free_perc_module  = {"perc", "percentage of available amount of the disk in total [17.82%]", {}, [](const callbackInfo_t *callback) {return get_and_color_percentage(disk_free(callback), disk_total(callback), callback->parse_args, true);}};
    module_t disk_used_perc_module  = {"perc", "percentage of used amount of the disk in total [82.18%]", {}, [](const callbackInfo_t *callback) {return get_and_color_percentage(disk_used(callback), disk_total(callback), callback->parse_args, false);}};
    module_t disk_free_module  = {"free", "available amount of disk space (auto) [438.08 GiB]", {std::move(disk_free_perc_module)}, [](const callbackInfo_t *callback) { return amount(disk_free(callback),  callback->moduleArgs);  }};
    module_t disk_used_module  = {"used", "used amount of disk space (auto) [360.02 GiB]", {std::move(disk_used_perc_module)}, [](const callbackInfo_t *callback) { return amount(disk_used(callback),  callback->moduleArgs);  }};
    module_t disk_total_module = {"total", "total amount of disk space (auto) [100.08 GiB]", {}, [](const callbackInfo_t *callback) { return amount(disk_total(callback), callback->moduleArgs); }};

    module_t disk_module = {"disk", "used and total amount of disk space (auto) with type of filesystem and used percentage [379.83 GiB / 438.08 GiB (86.70%) - ext4]", {
        std::move(disk_fsname_module),
        std::move(disk_device_module),
        std::move(disk_mountdir_module),
        std::move(disk_types_module),
        std::move(disk_free_module),
        std::move(disk_used_module),
        std::move(disk_total_module),
    }, disk_fmt};
    cfRegisterModule(disk_module);

    // $<battery>
    module_t battery_modelname_module = {"name", "battery model name", {}, battery_modelname};
    module_t battery_status_module = {"status", "battery current status [Discharging, AC Connected]", {}, battery_status};
    module_t battery_capacity_level_module = {"capacity_level", "battery capacity level [Normal, Critical]", {}, battery_capacity_level};
    module_t battery_technology_module = {"technology", "battery technology [Li-lion]", {}, battery_technology};
    module_t battery_vendor_module = {"manufacturer", "battery manufacturer name", {}, battery_vendor};
    module_t battery_perc_module = {"perc", "battery current percentage", {}, battery_perc};

    module_t battery_temp_C_module = {"C", "battery temperature in Celsius [e.g. 37.12°C]", {}, [](unused) {return fmt::format("{:.2f}°C", battery_temp());}};
    module_t battery_temp_F_module = {"F", "battery temperature in Fahrenheit [e.g. 98.81°F]", {}, [](unused) {return fmt::format("{:.2f}°F", battery_temp() * 1.8 + 34);}};
    module_t battery_temp_K_module = {"K", "battery temperature in Kelvin [e.g. 310.27°K]", {}, [](unused) {return fmt::format("{:.2f}°K", battery_temp() + 273.15);}};
    module_t battery_temp_module = {"temp", "battery temperature (by the chosen unit)", {
        std::move(battery_temp_C_module),
        std::move(battery_temp_F_module),
        std::move(battery_temp_K_module),
    }, [](unused) {return fmt::format("{:.2f}°C", battery_temp());}};

    module_t battery_module = {"battery", "battery current percentage and status [50.00% [Discharging]]", {
        std::move(battery_modelname_module),
        std::move(battery_status_module),
        std::move(battery_capacity_level_module),
        std::move(battery_technology_module),
        std::move(battery_vendor_module),
        std::move(battery_perc_module),
        std::move(battery_temp_module),
    }, NULL};
    cfRegisterModule(battery_module);

    // $<gpu>
    module_t gpu_name_module = {"name", "GPU model name [GeForce GTX 1650]", {}, gpu_name};
    module_t gpu_vendor_short_module = {"short", "GPU short vendor name [NVIDIA]", {}, [](const callbackInfo_t *callback) {
        return shorten_vendor_name(gpu_vendor(callback));
    }};
    module_t gpu_vendor_module = {"vendor", "GPU vendor name [NVIDIA Corporation]", {
        std::move(gpu_vendor_short_module)
    }, gpu_vendor};
    module_t gpu_module = {"gpu", "GPU shorter vendor name and model name [NVIDIA GeForce GTX 1650]", {
        std::move(gpu_name_module),
        std::move(gpu_vendor_module)
    }, [](const callbackInfo_t *callback) {return shorten_vendor_name(gpu_vendor(callback)) + " " + gpu_name(callback);}};
    cfRegisterModule(gpu_module);

    // $<auto>
    module_t auto_disk_module = {"disk", "Query all disks based on auto.disk.display-types", {}, auto_disk};
    module_t auto_module = {"auto", "", {std::move(auto_disk_module)}, NULL};
    cfRegisterModule(auto_module);

    // $<title>
    module_t title_sep_module = { "sep", "separator between the title and the system infos (with the title length) [--------]", {}, [&](unused) {
                                     const size_t title_len =
                                         std::string_view(user_name() + "@" + os_hostname()).length();

                                     std::string str;
                                     str.reserve(config.title_sep.length() * title_len);
                                     for (size_t i = 0; i < title_len; i++)
                                         str += config.title_sep;

                                     return str;
                                 } };
    module_t title_module = { "title", "user and hostname colored with ${auto2} [toni@arch2]", { std::move(title_sep_module) }, [](const callbackInfo_t* callback) {
                                 return parse("${auto2}$<user.name>${0}@${auto2}$<os.hostname>", callback->parse_args);
                             } };
    cfRegisterModule(title_module);

    // $<colors>
    module_t colors_light_symbol_module = { "symbol", "light color palette with specific symbol", {}, [](const callbackInfo_t* callback) { return get_colors_symbol(callback, true); } };
    module_t colors_symbol_module = { "symbol", "color palette with specific symbol", {}, [](const callbackInfo_t* callback) { return get_colors_symbol(callback, false); }};
    module_t colors_light_module = { "light", "light color palette with background spaces", { std::move(colors_light_symbol_module) }, [](const callbackInfo_t* callback) {
                                        return parse(
                                            "${\033[100m}   ${\033[101m}   ${\033[102m}   ${\033[103m}   ${\033[104m}  "
                                            " ${\033[105m}   ${\033[106m}   ${\033[107m}   ${0}", callback->parse_args);
                                    } };
    module_t colors_module = { "colors", "color palette with background spaces",
                               { std::move(colors_symbol_module), std::move(colors_light_module) },
                               [](const callbackInfo_t* callback) {
                                   return parse(
                                       "${\033[40m}   ${\033[41m}   ${\033[42m}   ${\033[43m}   ${\033[44m}   "
                                       "${\033[45m}   ${\033[46m}   ${\033[47m}   ${0}",
                                       callback->parse_args);
                               } };
    cfRegisterModule(colors_module);
}

void core_plugins_finish()
{
    if (mountsFile) fclose(mountsFile);
    if (os_release) fclose(os_release);
    if (meminfo)    fclose(meminfo);
    if (cpuinfo)    fclose(cpuinfo);
}
