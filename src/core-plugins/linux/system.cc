#include <sys/utsname.h>

#include <filesystem>
#include <string>
#include <string_view>

#include "common.hpp"
#include "linux-core-modules.hh"
#include "util.hpp"

/* The handler that we'll use for our module, Handlers return const std::string (WILL be changed to const char
 * pointers). */
modfunc host()
{
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    std::string board_name    = "(unknown)";
    std::string board_version = "(unknown)";
    std::string board_vendor  = "(unknown)";

    if (std::filesystem::exists(syspath + "/board_name"))
    {
        board_name    = read_by_syspath(syspath + "/board_name");
        board_version = read_by_syspath(syspath + "/board_version");
        board_vendor  = read_by_syspath(syspath + "/board_vendor");

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

modfunc host_name()
{
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    if (std::filesystem::exists(syspath + "/board_name"))
        return read_by_syspath(syspath + "/board_name");
    else if (std::filesystem::exists(syspath + "/product_name"))
        return read_by_syspath(syspath + "/product_name");

    return UNKNOWN;
}

modfunc host_version()
{
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    if (std::filesystem::exists(syspath + "/board_name"))
        return read_by_syspath(syspath + "/board_version");
    else if (std::filesystem::exists(syspath + "/product_name"))
        return read_by_syspath(syspath + "/product_version");

    return UNKNOWN;
}

modfunc host_vendor()
{
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    std::string board_vendor{ UNKNOWN };

    if (std::filesystem::exists(syspath + "/board_name"))
    {
        board_vendor = read_by_syspath(syspath + "/board_vendor");
    }
    else if (std::filesystem::exists(syspath + "/product_name"))
    {
        const std::string& board_name = read_by_syspath(syspath + "/product_name");
        if (hasStart(board_name, "Standard PC"))
            board_vendor = "KVM/QEMU";
    }

    return board_vendor;
}

modfunc arch()
{
    return g_uname_infos.machine;
}
