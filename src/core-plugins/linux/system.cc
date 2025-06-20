#include <dlfcn.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdio.h>
#include <sys/utsname.h>
#include <string>
#include <fstream>
#include <string_view>

#include "common.hpp"
#include "util.hpp"

/* The handler that we'll use for our module, Handlers return const std::string (WILL be changed to const char pointers). */
const std::string host() {
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    std::string board_name = "(unknown)";
    std::string board_version = "(unknown)";
    std::string board_vendor = "(unknown)";

    if (std::filesystem::exists(syspath + "/board_name"))
    {
        board_name      = read_by_syspath(syspath + "/board_name");
        board_version   = read_by_syspath(syspath + "/board_version");
        board_vendor    = read_by_syspath(syspath + "/board_vendor");

        if (board_vendor == "Micro-Star International Co., Ltd.")
            board_vendor = "MSI";
    }
    else if (std::filesystem::exists(syspath + "/product_name"))
    {
        board_name = read_by_syspath(syspath + "/product_name");

        static constexpr std::string_view standard_pc_name = "Standard PC";
        if (board_name.substr(0, standard_pc_name.size()) == standard_pc_name)
        {
            // everyone does it like "KVM/QEMU Standard PC (...) (host_version)" so why not
            board_vendor  = "KVM/QEMU";
            board_version = std::string_view('(' + read_by_syspath(syspath + "/product_version") + ')').data();
        }
        else
            board_version = read_by_syspath(syspath + "/product_version");
    }

    return board_vendor + ' ' + board_name + ' ' + board_version;
}

const std::string host_name() {
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    if (std::filesystem::exists(syspath + "/board_name"))
        return read_by_syspath(syspath + "/board_name");
    else if (std::filesystem::exists(syspath + "/product_name"))
        return read_by_syspath(syspath + "/product_name");

    return UNKNOWN;
}

const std::string host_version() {
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    std::string board_version = "(unknown)";

    if (std::filesystem::exists(syspath + "/board_name"))
    {
        board_version   = read_by_syspath(syspath + "/board_version");
    }
    else if (std::filesystem::exists(syspath + "/product_name"))
    {
        board_version = read_by_syspath(syspath + "/product_version");
    }

    return board_version;
}

const std::string host_vendor() {
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    std::string board_vendor = "(unknown)";

    if (std::filesystem::exists(syspath + "/board_name"))
    {
        board_vendor   = read_by_syspath(syspath + "/board_vendor");
    }
    else if (std::filesystem::exists(syspath + "/product_name"))
    {
        const std::string &board_name = read_by_syspath(syspath + "/product_name");
        static constexpr std::string_view standard_pc_name = "Standard PC";
        if (board_name.substr(0, standard_pc_name.size()) == standard_pc_name) {
            board_vendor = "KVM/QEMU";
        }
    }

    return board_vendor;
}

const std::string arch() {
    utsname sysinfo;

    if (uname(&sysinfo) != 0) {
        die(_("uname() failed: {}\nCould not get system infos"), strerror(errno));
    }

    return sysinfo.machine;
}

extern "C" void start(void *handle) {
    if (!handle) {
        std::cout << "Exiting because !handle" << std::endl;
        return;
    }

    LOAD_LIB_SYMBOL(handle, void, cfRegisterModule, const module_t &module);

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
