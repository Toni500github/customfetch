#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

GPU::GPU(u_short id) {
    debug("Constructing GPU");
}

std::string GPU::name() {
    return UNKNOWN;
}

std::string GPU::vendor() {
    return UNKNOWN;
}

#endif
