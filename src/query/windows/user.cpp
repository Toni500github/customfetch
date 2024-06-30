#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

User::User() {
    debug("Constructing User");
}

std::string User::name() {
    return UNKNOWN;
}

std::string User::shell() {
    return "batch";
}

std::string User::shell_path() {
    return UNKNOWN;
}

std::string User::shell_version() {
    return UNKNOWN;
}

#endif
