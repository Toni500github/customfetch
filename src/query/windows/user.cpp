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

std::string User::shell_version(const std::string_view shell_name) {
    return "";
}

std::string User::wm_name(const std::string_view term_name) { return MAGIC_LINE; }
std::string User::de_name(const std::string_view term_name) { return MAGIC_LINE; }

std::string User::term_name() { return "CMD"; }
std::string User::term_version(const std::string_view term_name) { return UNKNOWN; }


#endif
