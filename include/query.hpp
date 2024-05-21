#ifndef QUERY_HPP
#define QUERY_HPP
#include <string>
#include <sys/utsname.h>

using std::string;

class QuerySystem {
public:
    QuerySystem();
    string SystemName();
    string GPUName();
    struct utsname osInfo;
    /* ... */
};

inline QuerySystem query_sys;

#endif
