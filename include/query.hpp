#ifndef QUERY_HPP
#define QUERY_HPP

#include "util.hpp"
#include <sys/sysinfo.h>
#include <memory>
#include <fstream>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>

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
    //GPU();
    std::string vendor_id();
    std::string name(const std::string &vendor_id);
    std::string vendor(const std::string &vendor_id);

};

class RAM {
public:
    size_t total_amount();
    size_t free_amount();
    size_t used_amount();
};

}; // namespace Query

inline Query::System query_system;
inline Query::CPU query_cpu;
inline Query::GPU query_gpu;
inline Query::RAM query_ram;

#endif
