#include <cstdint>
#include <filesystem>
#include <string>

#include "core-modules.hh"
#include "fmt/format.h"
#include "util.hpp"

static std::string get_name(const std::string_view m_vendor_id_s, const std::string_view m_device_id_s)
{
    const std::string& name = binarySearchPCIArray(m_vendor_id_s, m_device_id_s);
    debug("GPU binarySearchPCIArray name = {}", name);
    const size_t first_bracket = name.find('[');
    const size_t last_bracket  = name.rfind(']');

    // remove the chips name "TU106 [GeForce GTX 1650]"
    // This should work for AMD and Intel too.
    if (first_bracket != std::string::npos && last_bracket != std::string::npos)
        return name.substr(first_bracket + 1, last_bracket - first_bracket - 1);

    return name;
}

static std::string get_vendor(const std::string_view m_vendor_id_s)
{ return binarySearchPCIArray(m_vendor_id_s); }

static std::string get_gpu_syspath(const std::string& id)
{
    const std::uint16_t max_iter = 10;
    std::uint16_t       id_iter  = std::stoi(id);
    std::string         sys_path;
    int                 i = 0;
    for (; i <= max_iter; i++)
    {
        sys_path = "/sys/class/drm/card" + fmt::to_string(id_iter);
        if (std::filesystem::exists(sys_path + "/device/device") &&
            std::filesystem::exists(sys_path + "/device/vendor"))
            return sys_path;
        else
            id_iter++;
    }

    error(_("Failed to parse GPU infos on the path /sys/class/drm/"));
    return UNKNOWN;
}

MODFUNC(gpu_name)
{
    const std::string& id = (callbackInfo && callbackInfo->moduleArgs->name.length() > 3)
        ? callbackInfo->moduleArgs->name.substr(3)
        : "0";
    const std::string& sys_path = get_gpu_syspath(id);
    return get_name(read_by_syspath(sys_path + "/device/vendor"), read_by_syspath(sys_path + "/device/device"));
}

MODFUNC(gpu_vendor)
{
    const std::string& id = (callbackInfo && callbackInfo->moduleArgs->name.length() > 3)
        ? callbackInfo->moduleArgs->name.substr(3)
        : "0";
    const std::string& sys_path = get_gpu_syspath(id);
    return get_vendor(read_by_syspath(sys_path + "/device/vendor"));
}
