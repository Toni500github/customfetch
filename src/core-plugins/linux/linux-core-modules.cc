#include <dlfcn.h>
#include "linux-core-modules.hh"
#include "common.hpp"
#include "util.hpp"

APICALL EXPORT MOD_INIT(void *handle)
{
    if (!handle)
    {
        error("Exiting because !handle");
        return;
    }

    LOAD_LIB_SYMBOL(handle, void, cfRegisterModule, const module_t& module);

    // *** $<os>
    os_release = fopen("/etc/os-release", "r");

    module_t os_name_module = { "name", {}, os_name };
    module_t os_pretty_name_module = { "pretty_name", {}, os_pretty_name };
    module_t os_name_id_module = { "name_id", {}, os_name_id };

    module_t os_module = { "os", {
        std::move(os_name_module),
        std::move(os_pretty_name_module),
        std::move(os_name_id_module),
    }, NULL};

    //fclose(os_release);
    cfRegisterModule(os_module);

    // *** $<system>
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
