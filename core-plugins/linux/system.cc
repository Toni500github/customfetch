/* This is an example for a mod you could install in .config/mods

Mods are essentially custom(fetch) modules that you can implement yourself and register using libcufetch!
They are to be compiled as shared libraries **with no name mangling!!**, with one start function. Scroll down for more details on the start function.

To compile this, just run `g++ -I../include -shared -fPIC mod-library.cc -o mod-library.so`. To use it, you'll need to put it in your customfetch/mods config directory.
*/

#include <dlfcn.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>
#include <string_view>

#include "common.hpp"

#define BOLD_COLOR(x) (fmt::emphasis::bold | fmt::fg(x))

/* ret_type = type of what the function returns
 * func     = the function name
 * ...      = the arguments in a function if any
 */
#define LOAD_LIB_SYMBOL(ret_type, func, ...)   \
    typedef ret_type (*func##_t)(__VA_ARGS__); \
    func##_t func = reinterpret_cast<func##_t>(dlsym(handle, #func));

#define UNLOAD_LIBRARY() dlclose(handle);

std::string read_by_syspath(const std::string_view path)
{
    std::ifstream f(path.data());
    if (!f.is_open())
    {
        error(_("Failed to open {}"), path);

        return "(unknown)";
    }

    std::string result;
    std::getline(f, result);

    if (!result.empty() && result.back() == '\n')
        result.pop_back();

    return result;
}

/* The handler that we'll use for our module, Handlers return const std::string (WILL be changed to const char pointers). */
const std::string host() {
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    std::string board_name;
    std::string board_version;
    std::string board_vendor;

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

    std::string board_name;

    if (std::filesystem::exists(syspath + "/board_name"))
    {
        board_name = read_by_syspath(syspath + "/board_name");
    }
    else if (std::filesystem::exists(syspath + "/product_name"))
    {
        board_name = read_by_syspath(syspath + "/product_name");
    }

    return board_name;
}

extern "C" void start(void *handle) {
    if (!handle) {
        std::cout << "Exiting because !handle" << std::endl;
        return;
    }

    LOAD_LIB_SYMBOL(void, cfRegisterModule, const module_t &module);

    module_t host_name_module = {"name", {}, host_name};
    module_t host_module = {"host", { std::move(host_name_module) }, host};
    
    /* Only for compatibility */
    module_t host_name_module_compat = { "host_name", {}, host_name };

    module_t system_module = { "system", { std::move(host_module), std::move(host_name_module_compat) }, NULL };

    cfRegisterModule(system_module);
}
