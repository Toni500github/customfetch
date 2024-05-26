#include "query.hpp"
#include "util.hpp"
#include "pci.ids.hpp"

#include <fstream>
#include <sys/types.h>

using namespace Query;

std::string GPU::name() {
    std::string sys_vendor_path = "/sys/class/drm/card0/device/vendor";
    std::string sys_device_path = "/sys/class/drm/card0/device/device";
    //fmt::format("/sys/class/drm/card{}/", id);
    
    /*while (true) {
        if (!std::filesystem::exists(sys_path))
            continue;
    }*/

    std::ifstream vendor_file(sys_vendor_path);
    if(!vendor_file.is_open()) {
        error("Could not open {}", sys_vendor_path);
        return UNKNOWN;
    }

    std::ifstream device_file(sys_device_path);
    if(!device_file.is_open()) {
        error("Could not open {}", sys_device_path);
        return UNKNOWN;
    }

    std::string vendor_s;
    while(vendor_file >> vendor_s);

    std::string id_s;
    while(device_file >> id_s);

    return binarySearchPCIArray(vendor_s, id_s);
}

std::string GPU::vendor() {
    std::string sys_path = "/sys/class/drm/card0/device/vendor";
    std::ifstream file(sys_path);
    if(!file.is_open()) {
        error("Could not open {}", sys_path);
        return UNKNOWN;
    }

    std::string id_s;
    while(file >> id_s);

    return vendor_from_id(all_ids, id_s);
}
