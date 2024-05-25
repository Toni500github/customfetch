#include "query.hpp"
#include "util.hpp"
#include "pci.ids.hpp"

#include <fstream>
#include <sys/types.h>

using namespace Query;

std::string GPU::name() {
    static u_short id = 0;
    std::string sys_path = "/sys/class/drm/card0/device/device";
    //fmt::format("/sys/class/drm/card{}/", id);
    
    /*while (true) {
        if (!std::filesystem::exists(sys_path))
            continue;
    }*/

    std::ifstream file(sys_path);
    if(!file.is_open()) {
        error("Could not open {}", sys_path);
        return UNKNOWN;
    }

    std::string id_s;
    while(file >> id_s);

    return name_from_id(all_ids, id_s);
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
