#ifndef QUERY_HPP
#define QUERY_HPP

#include "util.hpp"
#include "config.hpp"
#include "parse.hpp"

#include <array>
#include <fstream>
#include <vector>
#include <pwd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

extern "C" {
#include <pci/pci.h>
}

using smart_pci_access_ptr = std::unique_ptr<pci_access, decltype(&pci_cleanup)>;

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
    std::string os_id();
    long        uptime();

private:
    std::array<std::string, 8> m_os_release_vars;
    struct utsname m_uname_infos;
    struct sysinfo m_sysInfos;
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
