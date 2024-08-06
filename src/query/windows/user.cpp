#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

User::User() {
    debug("Constructing User");
    m_users_infos.shell_name = "batch";
    m_users_infos.shell_version = UNKNOWN;
    m_users_infos.de_version = MAGIC_LINE;
}

std::string User::name() {
    return getenv("USERNAME");
}

std::string& User::shell_name() {
    return m_users_infos.shell_name;
}

std::string User::shell_path() {
    return "";
}

std::string& User::shell_version(const std::string_view shell_name) {
    return m_users_infos.shell_version;
}

std::string& User::wm_name(bool dont_query_dewm, const std::string_view term_name) { return m_users_infos.wm_name; }
std::string& User::de_name(bool dont_query_dewm, const std::string_view term_name, const std::string_view wm_name) { return m_users_infos.de_name; }
std::string& User::de_version(const std::string_view de_name) { return m_users_infos.de_version; }
std::string& User::term_name() { return m_users_infos.term_name; }
std::string& User::term_version(const std::string_view term_name) { return  m_users_infos.term_version; }


#endif
