#ifndef QUERY_HPP
#define QUERY_HPP

#include "util.hpp"
#include <sys/sysinfo.h>
#include <memory>
#include <fstream>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>

extern "C" {
    #include <pci/pci.h>
}

#define smart_pci_access_ptr std::unique_ptr<pci_access, decltype(&pci_cleanup)>

namespace Query {

    class System {
    public:
        System();
        std::string kernel_name();
        std::string kernel_version();
        std::string hostname();
        std::string arch();
        std::string username();
        std::string os_name();
        long uptime();
    
private: 
        // private just for the sake of (something idk?) since there are lazy access functions
        struct utsname uname_infos;
        struct sysinfo sysInfos;
        struct passwd *pwd;
    };

    class CPU {
    public:
        //CPU();
        std::string name();
        std::string vendor();
    };

    class GPU {
    public:
        GPU(smart_pci_access_ptr &pac);
        //std::string vendor_id();
        std::string name();
        std::string vendor();
    private:
        uint16_t vendor_id;
        uint16_t device_id;

        pci_access *pac;
    
};

    class RAM {
    public:
        size_t total_amount();
        size_t free_amount();
        size_t used_amount();
    };

};

//inline Query::System query_system;
//inline Query::CPU query_cpu;
//inline Query::GPU query_gpu;
//inline Query::RAM query_ram;
inline smart_pci_access_ptr pac(pci_alloc(), pci_cleanup);

#endif
