#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

User::User() {
    debug("Constructing User");
}

std::string User::name() {
    return std::string(getenv("USERNAME"));
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

std::string User::wm_name() { return MAGIC_LINE; }
std::string User::de_name() { return MAGIC_LINE; }

std::string User::term_name() { return "CMD"; }
std::string User::term_version() { return UNKNOWN; }


#endif
