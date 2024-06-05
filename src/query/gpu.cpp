#include "query.hpp"
#include "util.hpp"
#include "pci.ids.hpp"

#include <fstream>
#include <sys/types.h>

using namespace Query;

std::string GPU::vendor_id() {
    std::string sys_path = "/sys/class/drm/card0/device/vendor";
    std::ifstream file(sys_path);
    if(!file.is_open()) {
        error("Could not open {}", sys_path);
        return UNKNOWN;
    }

    std::string id_s;
    while(file >> id_s);

    return id_s;
}

std::string GPU::name(const std::string &vendor_id) {
    std::string sys_device_path = "/sys/class/drm/card0/device/device";
    //fmt::format("/sys/class/drm/card{}/", id);
    
    /*while (true) {
        if (!std::filesystem::exists(sys_path))
            continue;
    }*/

    std::ifstream device_file(sys_device_path);
    if(!device_file.is_open()) {
        error("Could not open {}", sys_device_path);
        return UNKNOWN;
    }

    std::string id_s;
    while(device_file >> id_s);

    return binarySearchPCIArray(vendor_id, id_s);
}

std::string GPU::vendor(const std::string &vendor_id) {
    return binarySearchPCIArray(vendor_id);
}
