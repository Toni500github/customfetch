#ifndef QUERY_HPP
#define QUERY_HPP

#include "util.hpp"
#include <sys/sysinfo.h>
#include <memory>
#include <fstream>
#include <sys/utsname.h>

namespace Query {

class System {
public:
    System();
    std::string_view kernel_name();
    std::string_view kernel_version();
    std::string_view hostname();
    std::string_view arch();
    std::string OS_pretty_name();
    long uptime();
private: 
    // private just for the sake of (something idk?) since there are lazy access functions
    struct utsname uname_infos;
    struct sysinfo sysInfos;
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

};

inline Query::System query_system;
inline Query::CPU query_cpu;
inline Query::GPU query_gpu;
inline Query::RAM query_ram;

#endif
