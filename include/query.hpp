#ifndef QUERY_HPP
#define QUERY_HPP

#include "util.hpp"
#include <memory>
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
    struct utsname sysInfos;
};

class CPU {
public:
    //CPU();
    std::string_view name();
    std::string_view vendor();
};

class GPU {
public:
    //GPU();
    std::string name();
    std::string vendor();
};

};

inline Query::System query_system;
inline Query::CPU query_cpu;
inline Query::GPU query_gpu;

#endif
