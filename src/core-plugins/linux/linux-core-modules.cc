#include <dlfcn.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <string_view>
#include <utility>
#include "linux-core-modules.hh"
#include "common.hpp"
#include "fmt/format.h"
#include "util.hpp"

using unused = const std::string&;

APICALL EXPORT MOD_INIT(void *handle)
{
    // ------------ INIT STUFF ------------
    if (!handle)
    {
        error("Exiting because !handle");
        return;
    }

    LOAD_LIB_SYMBOL(handle, void, cfRegisterModule, const module_t& module);

    if (uname(&g_uname_infos) != 0)
        die(_("uname() failed: {}\nCould not get system infos"), strerror(errno));

    if (g_pwd = getpwuid(getuid()), !g_pwd)
        die(_("getpwent failed: {}\nCould not get user infos"), std::strerror(errno));

    term_pid = get_terminal_pid();
    term_name = get_terminal_name();
    if (hasStart(str_tolower(term_name), "login") || hasStart(term_name, "init") || hasStart(term_name, "(init)"))
    {
        is_tty = true;
        term_name = ttyname(STDIN_FILENO);
    }
    os_release = fopen("/etc/os-release", "r");
    cpuinfo = fopen("/proc/cpuinfo", "r");
    meminfo = fopen("/proc/meminfo", "r");

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
    }, [](unused) {return os_kernel_name() + ' ' + os_kernel_version();}};

    module_t os_initsys_name_module = {"name", {}, os_initsys_name};
    module_t os_initsys_version_module = {"version", {}, os_initsys_version};
    module_t os_initsys_module = {"initsys", {
        std::move(os_initsys_name_module),
        std::move(os_initsys_version_module),
    }, [](unused) {return os_initsys_name() + ' ' + os_initsys_version();}};

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
    constexpr std::array<std::string_view, 32> sorted_valid_prefixes = { "B",   "EB", "EiB", "GB", "GiB", "kB",
                                                                         "KiB", "MB", "MiB", "PB", "PiB", "TB",
                                                                         "TiB", "YB", "YiB", "ZB", "ZiB" };
    const auto& return_devided_bytes = [&](const double& amount, const std::string& module) -> double {
        const std::string& prefix = module.substr(module.find('-') + 1);
        if (std::binary_search(sorted_valid_prefixes.begin(), sorted_valid_prefixes.end(), prefix))
            return devide_bytes(amount, prefix).num_bytes;

        return 0;
    };
    module_t ram_free_module{"free", {}, NULL};
}
