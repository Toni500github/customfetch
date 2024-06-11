#include "query.hpp"
#include "util.hpp"

#include <filesystem>
#include <fstream>
#include <pci/pci.h>
#include <sstream>
#include <string>
#include <sys/types.h>

using namespace Query;

static std::string read_drm_by_path(const std::string& path) {
    std::ifstream f_drm(path);
    if (!f_drm.is_open()) {
        error("Could not open {}: Failed to get GPU infos", path);
        return UNKNOWN;
    }
    std::string ret;
    std::getline(f_drm, ret);
    return ret;
}

GPU::GPU(smart_pci_access_ptr &pac, u_short id) : m_pPac(pac.get()) {
    u_short id_iter = id;
    std::string sys_path;
    while(id_iter <= 5) {
        sys_path = "/sys/class/drm/card" + fmt::to_string(id_iter);
        if (std::filesystem::exists(sys_path))
            break;
        else
            id_iter++;
    }

    if (id_iter >= 5) {
        error("Failed to parse GPU infos on the path /sys/class/drm/");
        return;
    }

    /* Read the vendor ID, in hex. */
    std::string vendor_id_string = read_drm_by_path(sys_path + "/device/vendor");
    
    /* Read the device ID, in hex. */
    std::string device_id_string = read_drm_by_path(sys_path + "/device/device");

    /* Convert vendor and device IDs */
    std::istringstream vendor_id_converter(vendor_id_string);
    vendor_id_converter >> std::hex >> m_vendor_id;

    std::istringstream device_id_converter(device_id_string);
    device_id_converter >> std::hex >> m_device_id;
}

std::string GPU::name() {
    char devbuf[256];

    pci_lookup_name(m_pPac, devbuf, sizeof(devbuf), PCI_LOOKUP_DEVICE, m_vendor_id, m_device_id);

    std::string name(devbuf);
    auto first_bracket = name.find_first_of('[');
    auto last_bracket = name.find_last_of(']');
    
    // remove the chips name "TU106 [GeForce GTX 1650]"
    // This should work for AMD and Intel too.
    if (first_bracket != std::string::npos && last_bracket != std::string::npos)
        name = name.substr(first_bracket + 1, last_bracket - first_bracket - 1);

    name = this->vendor() + ' ' + name;

    replace_str(name, "NVIDIA Corporation", "NVIDIA");
    replace_str(name, "Advanced Micro Devices Inc.", "AMD");
    replace_str(name, "Intel Corporation", "Intel");

    return name;
}

std::string GPU::vendor() {
    char devbuf[256];

    pci_lookup_name(m_pPac, devbuf, sizeof(devbuf), PCI_LOOKUP_VENDOR, m_vendor_id);

    return devbuf;
}
