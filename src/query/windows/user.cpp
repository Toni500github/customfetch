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

std::string User::shell_name() {
    return "batch";
}

std::string User::shell_path() {
    return "";
}

std::string User::shell_version() {
    return "";
}

std::string wm_name() { return MAGIC_LINE; }
std::string de_name() { return MAGIC_LINE; }

std::string term_name() { return "CMD"; }
std::string term_version() { return UNKNOWN; }


#endif
