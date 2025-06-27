#include <dlfcn.h>
#include <mntent.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "common.hpp"
#include "config.hpp"
#include "core-modules.hh"
#include "cufetch.hh"
#include "fmt/format.h"
#include "util.hpp"

using unused = const callbackInfo_t*;

const std::string amount(const double amount, const moduleArgs_t* moduleArgs)
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

void core_plugins_start()
{
    // ------------ INIT STUFF ------------
    if (uname(&g_uname_infos) != 0)
        die(_("uname() failed: {}\nCould not get system infos"), strerror(errno));

    if (g_pwd = getpwuid(getuid()), !g_pwd)
        die(_("getpwent failed: {}\nCould not get user infos"), std::strerror(errno));

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
    module_t os_name_pretty_module = {"pretty", {}, os_pretty_name};
    module_t os_name_id_module = {"id", {}, os_name_id};
    module_t os_name_module = { "name", {
        std::move(os_name_pretty_module),
        std::move(os_name_id_module)
    }, os_name };

    module_t os_uptime_module = {"uptime", {}, os_uptime};
    module_t os_hostname_module = {"hostname", {}, os_hostname};

    module_t os_kernel_name_module = {"name", {}, os_kernel_name};
    module_t os_kernel_version_module = {"version", {}, os_kernel_version};
    module_t os_kernel_module = {"kernel", {
        std::move(os_kernel_name_module),
        std::move(os_kernel_version_module)
    }, [](unused) {return os_kernel_name() + " " + os_kernel_version();}};

    module_t os_initsys_name_module = {"name", {}, os_initsys_name};
    module_t os_initsys_version_module = {"version", {}, os_initsys_version};
    module_t os_initsys_module = {"initsys", {
        std::move(os_initsys_name_module),
        std::move(os_initsys_version_module),
    }, [](unused) {return os_initsys_name() + " " + os_initsys_version();}};

    /* Only for compatibility */
    module_t os_pretty_name_module_compat = { "pretty_name", {}, os_pretty_name };
    module_t os_name_id_module_compat = { "name_id", {}, os_name_id };
    module_t os_kernel_name_module_compat = {"kernel_name", {}, os_kernel_name};
    module_t os_kernel_version_module_compat = {"kernel_version", {}, os_kernel_version};
    module_t os_initsys_name_module_compat = {"initsys_name", {}, os_initsys_name};
    module_t os_initsys_version_module_compat = {"initsys_version", {}, os_initsys_version};

    // $<os>
    module_t os_module = { "os", {
        std::move(os_name_module),
        std::move(os_uptime_module),
        std::move(os_kernel_module),
        std::move(os_hostname_module),
        std::move(os_initsys_module),

        std::move(os_pretty_name_module_compat),
        std::move(os_name_id_module_compat),
        std::move(os_kernel_name_module_compat),
        std::move(os_kernel_version_module_compat),
        std::move(os_initsys_name_module_compat),
        std::move(os_initsys_version_module_compat),
    }, NULL};

    //fclose(os_release);
    cfRegisterModule(os_module);

    // $<system>
    module_t host_name_module = {"name", {}, host_name};
    module_t host_version_module = {"version", {}, host_version};
    module_t host_vendor_module = {"vendor", {}, host_vendor};
    module_t host_module = {"host", { std::move(host_name_module), std::move(host_version_module), std::move(host_vendor_module) }, host};

    /* Only for compatibility */
    module_t host_name_module_compat = { "host_name", {}, host_name };
    module_t host_version_module_compat = {"host_version", {}, host_version};
    module_t host_vendor_module_compat = {"host_vendor", {}, host_vendor};

    module_t arch_module = {"arch", {}, arch};

    module_t system_module = { "system", { 
        std::move(host_module),
        std::move(host_name_module_compat), std::move(host_version_module_compat), std::move(host_vendor_module_compat),
        std::move(arch_module),
    }, NULL };

    cfRegisterModule(system_module);

    // $<cpu>
    module_t cpu_name_module   = {"name", {}, cpu_name};
    module_t cpu_nproc_module  = {"nproc" , {}, cpu_nproc};

    module_t cpu_freq_cur_module = {"current", {}, cpu_freq_cur};
    module_t cpu_freq_max_module = {"max", {}, cpu_freq_max};
    module_t cpu_freq_min_module = {"min", {}, cpu_freq_min};
    module_t cpu_freq_bios_module = {"bios_limit", {}, cpu_freq_bios};
    module_t cpu_freq_module = {"freq", {
        std::move(cpu_freq_cur_module),
        std::move(cpu_freq_max_module),
        std::move(cpu_freq_min_module),
        std::move(cpu_freq_bios_module),
    }, cpu_freq_max};

    module_t cpu_temp_C_module = {"C", {}, [](unused) {return fmt::format("{:.2f}°C", cpu_temp());}};
    module_t cpu_temp_F_module = {"F", {}, [](unused) {return fmt::format("{:.2f}°F", cpu_temp() * 1.8 + 34);}};
    module_t cpu_temp_K_module = {"K", {}, [](unused) {return fmt::format("{:.2f}°K", cpu_temp() + 273.15);}};
    module_t cpu_temp_module = {"temp", {
        std::move(cpu_temp_C_module),
        std::move(cpu_temp_F_module),
        std::move(cpu_temp_K_module),
    }, [](unused) {return fmt::format("{:.2f}°C", cpu_temp());}};

    /* Only for compatibility */
    module_t cpu_freq_cur_module_compat = {"freq_cur", {}, cpu_freq_cur};
    module_t cpu_freq_max_module_compat = {"freq_max", {}, cpu_freq_max};
    module_t cpu_freq_min_module_compat = {"freq_min", {}, cpu_freq_min};
    module_t cpu_freq_bios_module_compat = {"freq_bios_limit", {}, cpu_freq_bios};
    module_t cpu_temp_C_module_compat = {"temp_C", {}, [](unused) {return fmt::format("{:.2f}", cpu_temp());}};
    module_t cpu_temp_F_module_compat = {"temp_F", {}, [](unused) {return fmt::format("{:.2f}", cpu_temp() * 1.8 + 34);}};
    module_t cpu_temp_K_module_compat = {"temp_K", {}, [](unused) {return fmt::format("{:.2f}", cpu_temp() + 273.15);}};

    module_t cpu_module = {"cpu", {
        std::move(cpu_name_module),
        std::move(cpu_nproc_module),
        std::move(cpu_freq_module),
        std::move(cpu_temp_module),

        std::move(cpu_freq_cur_module_compat),
        std::move(cpu_freq_max_module_compat),
        std::move(cpu_freq_min_module_compat),
        std::move(cpu_freq_bios_module_compat),
        std::move(cpu_temp_C_module_compat),
        std::move(cpu_temp_F_module_compat),
        std::move(cpu_temp_K_module_compat),
    }, [](unused) {
            return fmt::format("{} ({}) @ {} GHz", cpu_name(), cpu_nproc(), cpu_freq_max());
        }};

    cfRegisterModule(cpu_module);

    // $<user>
    module_t user_name_module = {"name", {}, user_name};

    module_t user_shell_path_module = {"path", {}, user_shell_path};
    module_t user_shell_name_module = {"name", {}, user_shell_name};
    module_t user_shell_version_module = {"version", {}, user_shell_version};
    module_t user_shell_module = {"shell", {
        std::move(user_shell_name_module),
        std::move(user_shell_path_module),
        std::move(user_shell_version_module),
    }, [](unused) {return user_shell_name() + " " + user_shell_version();}};

    module_t user_term_name_module = {"name", {}, user_term_name};
    module_t user_term_version_module = {"version", {}, user_shell_version};
    module_t user_term_module = {"terminal", {
        std::move(user_term_version_module),
        std::move(user_term_name_module)
    }, [](unused) {return user_term_name() + " " + user_term_version();}};

    module_t user_wm_name_module = {"name", {}, user_wm_name};
    module_t user_wm_version_module = {"version", {}, user_wm_version};
    module_t user_wm_module = {"wm", {
        std::move(user_wm_version_module),
        std::move(user_wm_name_module)
    }, [](unused) {return user_wm_name() + " " + user_wm_version();}};

    module_t user_de_name_module = {"name", {}, user_de_name};
    module_t user_de_version_module = {"version", {}, user_de_version};
    module_t user_de_module = {"de", {
        std::move(user_de_version_module),
        std::move(user_de_name_module)
    }, [](unused) {return user_de_name() + " " + user_de_version();}};

    /* Only for compatibility */
    module_t user_shell_path_module_compat = {"shell_path", {}, user_shell_path};
    module_t user_shell_name_module_compat = {"shell_name", {}, user_shell_name};
    module_t user_shell_version_module_compat = {"shell_version", {}, user_shell_version};
    module_t user_term_name_module_compat = {"terminal_name", {}, user_term_name};
    module_t user_term_version_module_compat = {"terminal_version", {}, user_shell_version};
    module_t user_wm_name_module_compat = {"wm_name", {}, user_wm_name};
    module_t user_wm_version_module_compat = {"wm_version", {}, user_wm_version};
    module_t user_de_name_module_compat = {"de_name", {}, user_de_name};
    module_t user_de_version_module_compat = {"de_version", {}, user_de_version};

    module_t user_module = {"user", {
        std::move(user_name_module),
        std::move(user_shell_module),
        std::move(user_term_module),
        std::move(user_wm_module),
        std::move(user_de_module),

        std::move(user_shell_name_module_compat),
        std::move(user_shell_path_module_compat),
        std::move(user_shell_version_module_compat),
        std::move(user_term_version_module_compat),
        std::move(user_term_name_module_compat),
        std::move(user_wm_name_module_compat),
        std::move(user_wm_version_module_compat),
        std::move(user_de_name_module_compat),
        std::move(user_de_version_module_compat),
    }, NULL};

    cfRegisterModule(user_module);

    // $<ram>
    module_t ram_free_module  = {"free",  {}, [](const callbackInfo_t *callback) { return amount(ram_free() * 1024,  callback->moduleArgs);  }};
    module_t ram_used_module  = {"used",  {}, [](const callbackInfo_t *callback) { return amount(ram_used() * 1024,  callback->moduleArgs);  }};
    module_t ram_total_module = {"total", {}, [](const callbackInfo_t *callback) { return amount(ram_total() * 1024, callback->moduleArgs); }};

    module_t ram_module = {"ram", {
        std::move(ram_free_module),
        std::move(ram_used_module),
        std::move(ram_total_module)
    }, NULL};
    cfRegisterModule(ram_module);

    // $<swap>
    module_t swap_free_module  = {"free",  {}, [](const callbackInfo_t *callback) { return amount(swap_free() * 1024,  callback->moduleArgs); }};
    module_t swap_used_module  = {"used",  {}, [](const callbackInfo_t *callback) { return amount(swap_used() * 1024,  callback->moduleArgs); }};
    module_t swap_total_module = {"total", {}, [](const callbackInfo_t *callback) { return amount(swap_total() * 1024, callback->moduleArgs); }};

    module_t swap_module = {"swap", {
        std::move(swap_free_module),
        std::move(swap_used_module),
        std::move(swap_total_module)
    }, NULL};
    cfRegisterModule(swap_module);

    // $<disk>
    module_t disk_fsname_module = {"fs", {}, disk_fsname};
    module_t disk_device_module = {"device", {}, disk_device};
    module_t disk_mountdir_module = {"mountdir", {}, disk_mountdir};
    module_t disk_types_module = {"types", {}, disk_types};
    module_t disk_free_module  = {"free",  {}, [](const callbackInfo_t *callback) { return amount(disk_free(callback),  callback->moduleArgs); }};
    module_t disk_used_module  = {"used",  {}, [](const callbackInfo_t *callback) { return amount(disk_used(callback),  callback->moduleArgs); }};
    module_t disk_total_module = {"total", {}, [](const callbackInfo_t *callback) { return amount(disk_total(callback), callback->moduleArgs); }};

    module_t disk_module = {"disk", {
        std::move(disk_fsname_module),
        std::move(disk_device_module),
        std::move(disk_mountdir_module),
        std::move(disk_types_module),
        std::move(disk_free_module),
        std::move(disk_used_module),
        std::move(disk_total_module),
    }, NULL};
    cfRegisterModule(disk_module);

    // $<battery>
    module_t battery_modelname_module = {"name", {}, battery_modelname};
    module_t battery_status_module = {"status", {}, battery_status};
    module_t battery_capacity_level_module = {"capacity_level", {}, battery_capacity_level};
    module_t battery_technology_module = {"technology", {}, battery_technology};
    module_t battery_vendor_module = {"manufacturer", {}, battery_vendor};
    module_t battery_perc_module = {"perc", {}, battery_perc};

    module_t battery_temp_C_module = {"C", {}, [](unused) {return fmt::format("{:.2f}°C", battery_temp());}};
    module_t battery_temp_F_module = {"F", {}, [](unused) {return fmt::format("{:.2f}°F", battery_temp() * 1.8 + 34);}};
    module_t battery_temp_K_module = {"K", {}, [](unused) {return fmt::format("{:.2f}°K", battery_temp() + 273.15);}};
    module_t battery_temp_module = {"temp", {
        std::move(battery_temp_C_module),
        std::move(battery_temp_F_module),
        std::move(battery_temp_K_module),
    }, [](unused) {return fmt::format("{:.2f}°C", battery_temp());}};

    /* Only for compatibility */
    module_t battery_temp_C_module_compat = {"temp_C", {}, [](unused) {return fmt::format("{:.2f}", battery_temp());}};
    module_t battery_temp_F_module_compat = {"temp_F", {}, [](unused) {return fmt::format("{:.2f}", battery_temp() * 1.8 + 34);}};
    module_t battery_temp_K_module_compat = {"temp_K", {}, [](unused) {return fmt::format("{:.2f}", battery_temp() + 273.15);}};

    module_t battery_module = {"battery", {
        std::move(battery_modelname_module),
        std::move(battery_status_module),
        std::move(battery_capacity_level_module),
        std::move(battery_technology_module),
        std::move(battery_vendor_module),
        std::move(battery_perc_module),
        std::move(battery_temp_module),

        std::move(battery_temp_C_module_compat),
        std::move(battery_temp_F_module_compat),
        std::move(battery_temp_K_module_compat),
    }, NULL};
    cfRegisterModule(battery_module);

    // $<gpu>
    module_t gpu_name_module = {"name", {}, gpu_name};
    module_t gpu_vendor_short_module = {"short", {}, [](const callbackInfo_t *callback) {return shorten_vendor_name(gpu_vendor(callback));}};
    module_t gpu_vendor_module = {"vendor", {std::move(gpu_vendor_short_module)}, gpu_vendor};

    module_t gpu_module = {"gpu", {
        std::move(gpu_name_module),
        std::move(gpu_vendor_module)
    }, [](const callbackInfo_t *callback) {return shorten_vendor_name(gpu_vendor(callback)) + " " + gpu_name(callback);}};
    cfRegisterModule(gpu_module);

    // $<title>
    module_t title_sep_module = { "sep", {}, [](const callbackInfo_t* callback) {
                                     const size_t title_len =
                                         std::string_view(user_name() + "@" + os_hostname()).length();

                                     std::string str;
                                     str.reserve(callback->config.title_sep.length() * title_len);
                                     for (size_t i = 0; i < title_len; i++)
                                         str += callback->config.title_sep;

                                     return str;
                                 } };
    module_t title_module = { "title", { std::move(title_sep_module) }, [](const callbackInfo_t* callback) {
                                 return parse("${auto2}$<user.name>${0}@${auto2}$<os.hostname>", callback->modulesInfo,
                                              callback->config);
                             } };
    cfRegisterModule(title_module);

    // $<colors>
    module_t colros_symbol_module = {
        "symbol",
        {},
        [](const callbackInfo_t* callback) {
            const moduleArgs_t* symbolArg;
            for (symbolArg = callback->moduleArgs; symbolArg && symbolArg->name != "symbol";
                 symbolArg = symbolArg->next)
                ;
            if (symbolArg->value.empty())
                die(
                    _("color symbol palette argument module is empty.\n"
                      "Must be used like 'colors_symbol(`symbol for printing the color palette`)'"));

            if (symbolArg->prev->name == "light")
                return parse(
                    fmt::format("${{\033[90m}} {0} ${{\033[91m}} {0} ${{\033[92m}} {0} ${{\033[93m}} {0} ${{\033[94m}} "
                                "{0} ${{\033[95m}} {0} ${{\033[96m}} {0} ${{\033[97m}} {0} ${{0}}",
                                symbolArg->value),
                    callback->modulesInfo, callback->config);
            else
                return parse(
                    fmt::format("${{\033[30m}} {0} ${{\033[31m}} {0} ${{\033[32m}} {0} ${{\033[33m}} {0} ${{\033[34m}} "
                                "{0} ${{\033[35m}} {0} ${{\033[36m}} {0} ${{\033[37m}} {0} ${{0}}",
                                symbolArg->value),
                    callback->modulesInfo, callback->config);
        }
    };
    module_t colors_light_module = { "light", { std::move(colros_symbol_module) }, [](const callbackInfo_t* callback) {
                                        return parse(
                                            "${\033[100m}   ${\033[101m}   ${\033[102m}   ${\033[103m}   ${\033[104m}  "
                                            " ${\033[105m}   ${\033[106m}   ${\033[107m}   ${0}",
                                            callback->modulesInfo, callback->config);
                                    } };
    module_t colors_module = { "colors",
                               { std::move(colros_symbol_module), std::move(colors_light_module) },
                               [](const callbackInfo_t* callback) {
                                   return parse(
                                       "${\033[40m}   ${\033[41m}   ${\033[42m}   ${\033[43m}   ${\033[44m}   "
                                       "${\033[45m}   ${\033[46m}   ${\033[47m}   ${0}",
                                       callback->modulesInfo, callback->config);
                               } };
    cfRegisterModule(colors_module);
}
