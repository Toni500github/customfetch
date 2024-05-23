#ifndef QUERY_HPP
#define QUERY_HPP

#include "util.hpp"
#include <sys/utsname.h>

namespace Query {

class System {
public:
    System();
    string_view name();
    string_view GPUName();
    string OS_Name();
    struct utsname sysInfos;
};

};

inline Query::System query_system;

#endif
