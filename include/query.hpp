#ifndef QUERY_HPP
#define QUERY_HPP

#include "util.hpp"
#include "config.hpp"

#include <array>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <variant>
#include <string>
#include <memory>

extern "C" {
#include <pwd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <pci/pci.h>
}

using smart_pci_access_ptr = std::unique_ptr<pci_access, decltype(&pci_cleanup)>;
using systemInfo_t = std::unordered_map<std::string, std::unordered_map<std::string, std::variant<std::string, size_t, float>>>;
using variant = std::variant<std::string, size_t, float>;


namespace Query {

class System {
public:
    System();
    std::string kernel_name();
    std::string kernel_version();
    std::string hostname();
    std::string arch();
    std::string os_pretty_name();
    std::string os_name();
    std::string os_id();
    long        uptime();
    
    // motherboard (host)
    std::string host_modelname();
    std::string host_vendor();
    std::string host_version();

private:
    std::array<std::string, 4> m_os_release_vars;
    struct utsname m_uname_infos;
    struct sysinfo m_sysInfos;
};

class User {
public:
    User();
    std::string name();
    std::string shell();
    std::string shell_path();
    std::string shell_version();

private:
    struct passwd *m_pPwd;
};

class CPU {
public:
    CPU();
    std::string name();
    std::string nproc();

    float freq_max();
    float freq_min();
    float freq_cur();
    float freq_bios_limit();

private:
    std::array<float, 4> m_cpu_infos_t;
    std::array<std::string, 3> m_cpu_infos_str;
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

class Disk {
public:
    Disk(const std::string_view path);
    float total_amount();
    float free_amount();
    float used_amount();
    std::string typefs();

private:
    struct statvfs m_statvfs;
    std::string m_typefs;
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

//inline Query::System query_system;
//inline Query::CPU query_cpu;
//inline Query::GPU query_gpu;
//inline Query::RAM query_ram;
inline smart_pci_access_ptr pac(pci_alloc(), pci_cleanup);

#endif
