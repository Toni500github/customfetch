#include "query.hpp"
#include "util.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <pci/pci.h>
#include <sstream>
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

GPU::GPU(smart_pci_access_ptr &pac) : pac(pac.get()) {
    /* Read the vendor ID, in hex. */
    std::string sys_vendor_path = "/sys/class/drm/card0/device/vendor";

    std::ifstream file(sys_vendor_path);
    if(!file.is_open())
        die("Could not open {}", sys_vendor_path);

    std::string vendor_id_string;
    while(file >> vendor_id_string);

    /* Read the device ID, in hex. */
    std::string sys_device_path = "/sys/class/drm/card0/device/device";

    std::ifstream device_file(sys_device_path);
    if(!device_file.is_open())
        die("Could not open {}", sys_device_path);

    std::string device_id_string;
    while(device_file >> device_id_string);

    /* Convert vendor and device IDs */
    std::istringstream vendor_id_converter(vendor_id_string);
    vendor_id_converter >> std::hex >> vendor_id;

    std::istringstream device_id_converter(device_id_string);
    device_id_converter >> std::hex >> device_id;
}

std::string GPU::name() {
    char devbuf[128];

    pci_lookup_name(pac, devbuf, sizeof(devbuf), PCI_LOOKUP_DEVICE, vendor_id, device_id);

    std::string name(devbuf);
    auto first_bracket = name.find_first_of('[');
    auto last_bracket = name.find_last_of(']');
    
    // remove the chips name "TU106 [GeForce GTX 1650]"
    // This should work for AMD and Intel too.
    if (first_bracket != std::string::npos && last_bracket != std::string::npos)
        name = name.substr(first_bracket+1, last_bracket - first_bracket - 1);

    return name;
}

std::string GPU::vendor() {
    char devbuf[128];

    pci_lookup_name(pac, devbuf, sizeof(devbuf), PCI_LOOKUP_VENDOR, vendor_id);

    std::string name(devbuf);

    // Might not work with libpci, maybe it changed some names?
    replace_str(name, "NVIDIA Corporation", "NVIDIA");
    replace_str(name, "Advanced Micro Devices Inc.", "AMD");
    replace_str(name, "Intel Corporation", "Intel");

    return name;
}
