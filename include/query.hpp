#ifndef QUERY_HPP
#define QUERY_HPP

#include "util.hpp"

#include <array>
#include <fstream>
#include <memory>
#include <vector>
#include <pwd.h>
#include <unordered_map>
#include <variant>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

extern "C" {
#include <pci/pci.h>
}

#define smart_pci_access_ptr std::unique_ptr<pci_access, decltype(&pci_cleanup)>

#define systemInfo_t std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::string, size_t>>>
#define VARIANT std::variant<std::string, size_t>

namespace Query {

class System {
public:
    System();
    std::string kernel_name();
    std::string kernel_version();
    std::string hostname();
    std::string arch();
    std::string username();
    std::string os_pretty_name();
    std::string os_name();
    long        uptime();

private:
    struct utsname m_uname_infos;
    struct sysinfo m_sysInfos;
    struct passwd *m_pPwd;
};

class CPU {
public:
    //CPU();
    std::string name();
    std::string vendor();
};

class GPU {
public:
    GPU(smart_pci_access_ptr& pac, u_short id = 0);
    //std::string vendor_id();
    std::string name();
    std::string vendor();

private:
    uint16_t    m_vendor_id;
    uint16_t    m_device_id;

    pci_access *m_pPac;
};

class RAM {
public:
    RAM();
    size_t total_amount();
    size_t free_amount();
    size_t used_amount();
private:
    std::array<size_t, 3> m_memory_infos;
};

};

// Parse input, in-place, with data from systemInfo.
// Documentation on formatting is in the default config.toml file.
// pureOutput is set to the string, but without the brackets.
std::string parse(std::string& input, systemInfo_t &systemInfo, std::unique_ptr<std::string> &pureOutput, std::string reset_fgcolor);

// Set module values to a systemInfo_t map.
// If the name of said module matches any module name, it will be added
// else, error out.
void addModuleValues(systemInfo_t &sysInfo, std::string &moduleName);
void addValueFromModule(systemInfo_t &sysInfo, std::string &moduleName, std::string &moduleValueName);

//inline Query::System query_system;
//inline Query::CPU query_cpu;
//inline Query::GPU query_gpu;
//inline Query::RAM query_ram;
inline smart_pci_access_ptr pac(pci_alloc(), pci_cleanup);

#endif
