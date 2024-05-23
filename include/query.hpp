#ifndef QUERY_HPP
#define QUERY_HPP

#include "util.hpp"
#include <sys/utsname.h>

class QuerySystem {
public:
    QuerySystem();
    string_view SystemName();
    string_view GPUName();
    string OS_Name();
    struct utsname osInfo;
    /* ... */
};

inline QuerySystem query_sys;

#endif
