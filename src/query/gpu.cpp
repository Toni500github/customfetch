#include "query.hpp"
#include "util.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <sys/types.h>

using namespace Query;

static std::string read_drm_by_path(const std::string& path) {
    std::ifstream f_drm(path);
    if (!f_drm.is_open())
        return UNKNOWN;

    std::string ret;
    std::getline(f_drm, ret);
    return ret;
}

std::string GPU::vendor_id() {
    std::string sys_path;
    u_short id = 0;
    while (true) {
        if (!std::filesystem::exists(fmt::format("/sys/class/drm/card{}/device/vendor", id))) {
            if (id > 3) break;
        } else {
            sys_path = fmt::format("/sys/class/drm/card{}/device/vendor", id);
            break;
        }
        id++;
        continue;
    }

    return read_drm_by_path(sys_path);
}

std::string GPU::name(const std::string &vendor_id) {
    std::string sys_path;
    u_short id = 0;
    while (true) {
        if (!std::filesystem::exists(fmt::format("/sys/class/drm/card{}/device/device", id))) {
            if (id > 3) break;
        } else {
            sys_path = fmt::format("/sys/class/drm/card{}/device/device", id);
            break;
        }
        id++;
        continue;
    }

    std::string id_s = read_drm_by_path(sys_path);
    std::string ret = fmt::format("{} {}", this->vendor(vendor_id), binarySearchPCIArray(vendor_id, id_s));

    ret = replace_str(ret, "Advanced Micro Devices, Inc.", "AMD");
    ret = replace_str(ret, "NVIDIA Corporation", "NVIDIA");
    return ret;
}

std::string GPU::vendor(const std::string &vendor_id) {
    return binarySearchPCIArray(vendor_id);
}
