#include "query.hpp"
#include "util.hpp"

#include <filesystem>
#include <pci/pci.h>
#include <sstream>
#include <string>
#include <sys/types.h>

enum {
    NAME = 0,
    VENDOR
};

using namespace Query;

static std::string _get_vendor(uint16_t& m_vendor_id, pci_access *m_pPac) {
    char devbuf[256];

    pci_lookup_name(m_pPac, devbuf, sizeof(devbuf), PCI_LOOKUP_VENDOR, m_vendor_id);

    return (devbuf[0] != '\0') ? devbuf : UNKNOWN;
}

static std::string _get_name(uint16_t& m_vendor_id, uint16_t& m_device_id, pci_access *m_pPac, std::string vendor_str) {
    char devbuf[256];

    pci_lookup_name(m_pPac, devbuf, sizeof(devbuf), PCI_LOOKUP_DEVICE, m_vendor_id, m_device_id);

    std::string name(devbuf);
    auto first_bracket = name.find_first_of('[');
    auto last_bracket = name.find_last_of(']');
    
    // remove the chips name "TU106 [GeForce GTX 1650]"
    // This should work for AMD and Intel too.
    if (first_bracket != std::string::npos && last_bracket != std::string::npos)
        name = name.substr(first_bracket + 1, last_bracket - first_bracket - 1);

    name = vendor_str + ' ' + name;

    replace_str(name, "NVIDIA Corporation", "NVIDIA");
    replace_str(name, "Advanced Micro Devices Inc.", "AMD");
    replace_str(name, "Intel Corporation", "Intel");

    return name.empty() ? UNKNOWN : name;
}

static std::array<std::string, 2> get_gpu_infos(uint16_t& m_vendor_id, uint16_t& m_device_id, pci_access *m_pPac) {
    debug("calling GPU {}", __func__);
    std::array<std::string, 2> ret;
    
    ret[VENDOR] = _get_vendor(m_vendor_id, m_pPac);
    ret[NAME] = _get_name(m_vendor_id, m_device_id, m_pPac, ret[VENDOR]);
    
    return ret;
}

GPU::GPU(smart_pci_access_ptr &pac, u_short id) : m_pPac(pac.get()) {
    debug("Constructing {}", __func__);
    if (!m_bInit) {
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
        std::string vendor_id_string = read_by_syspath(sys_path + "/device/vendor");
        
        /* Read the device ID, in hex. */
        std::string device_id_string = read_by_syspath(sys_path + "/device/device");

        /* Convert vendor and device IDs */
        std::istringstream vendor_id_converter(vendor_id_string);
        vendor_id_converter >> std::hex >> m_vendor_id;

        std::istringstream device_id_converter(device_id_string);
        device_id_converter >> std::hex >> m_device_id;
        m_gpu_infos = get_gpu_infos(m_vendor_id, m_device_id, m_pPac);

        m_bInit = true;
    }
}

std::string GPU::name() {
    return m_gpu_infos.at(NAME);
}

std::string GPU::vendor() {
    return m_gpu_infos.at(VENDOR);
}
