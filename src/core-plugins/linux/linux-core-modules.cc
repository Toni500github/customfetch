#include <dlfcn.h>
#include "linux-core-modules.hh"
#include "common.hpp"

APICALL EXPORT MOD_INIT(void *handle)
{
    if (!handle)
    {
        error("Exiting because !handle");
        return;
    }

    LOAD_LIB_SYMBOL(handle, void, cfRegisterModule, const module_t& module);

    if (uname(&g_uname_infos) != 0)
        die(_("uname() failed: {}\nCould not get system infos"), strerror(errno));

    os_release = fopen("/etc/os-release", "r");

    // $<os.name>
    module_t os_name_pretty_module = {"pretty", {}, os_pretty_name};
    module_t os_name_id_module = {"id", {}, os_name_id};
    module_t os_name_module = { "name", {
        std::move(os_name_pretty_module),
        std::move(os_name_id_module)
    }, os_name };

    // $<os.uptime>
    module_t os_uptime_module = {"uptime", {}, os_uptime};
    
    // $<os.hostname>
    module_t os_hostname_module = {"hostname", {}, os_hostname};

    // $<os.kernel>
    module_t os_kernel_name_module = {"name", {}, os_kernel_name};
    module_t os_kernel_version_module = {"version", {}, os_kernel_version};
    module_t os_kernel_module = {"kernel", {
        std::move(os_kernel_name_module),
        std::move(os_kernel_version_module)
    }, []() {return os_kernel_name() + ' ' + os_kernel_version();}};

    // $<os.initsys>
    module_t os_initsys_name_module = {"name", {}, os_initsys_name};
    module_t os_initsys_version_module = {"version", {}, os_initsys_version};
    module_t os_initsys_module = {"initsys", {
        std::move(os_initsys_name_module),
        std::move(os_initsys_version_module),
    }, []() {return os_initsys_name() + ' ' + os_initsys_version();}};

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
}
