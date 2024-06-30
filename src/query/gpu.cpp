#include "query.hpp"
#include "util.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <sys/types.h>

using namespace Query;

GPU::GPU(u_short id) {
    const u_short max_iter = 10;
    u_short id_iter = id;
    std::string sys_path;
    while(id_iter <= max_iter) {
        sys_path = "/sys/class/drm/card" + fmt::to_string(id_iter);
        if (std::filesystem::exists(sys_path))
            break;
        else
            id_iter++;
    }

    if (id_iter >= max_iter) {
        error("Failed to parse GPU infos on the path /sys/class/drm/");
        return;
    }

    /* Read the vendor ID, in hex. */
    m_vendor_id_s = read_by_syspath(sys_path + "/device/vendor");
    
    /* Read the device ID, in hex. */
    m_device_id_s = read_by_syspath(sys_path + "/device/device");

    /* Convert vendor and device IDs */
    std::istringstream vendor_id_converter(m_vendor_id_s);
    vendor_id_converter >> std::hex >> m_vendor_id;

    std::istringstream device_id_converter(m_device_id_s);
    device_id_converter >> std::hex >> m_device_id;
}

std::string GPU::name() {
    std::string name = binarySearchPCIArray(m_vendor_id_s, m_device_id_s);
    auto first_bracket = name.find_first_of('[');
    auto last_bracket = name.find_last_of(']');
    
    // remove the chips name "TU106 [GeForce GTX 1650]"
    // This should work for AMD and Intel too.
    if (first_bracket != std::string::npos && last_bracket != std::string::npos)
        name = name.substr(first_bracket + 1, last_bracket - first_bracket - 1);

    //name = this->vendor() + ' ' + name;

    replace_str(name, "NVIDIA Corporation", "NVIDIA");
    replace_str(name, "Advanced Micro Devices Inc.", "AMD");
    replace_str(name, "Intel Corporation", "Intel");

    return name;
}

std::string GPU::vendor() {
    return binarySearchPCIArray(m_vendor_id_s);
}
