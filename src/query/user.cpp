#include <cstdlib>
#include "query.hpp"
#include "util.hpp"
#include "switch_fnv1a.hpp"

using namespace Query;

enum {
    WM_NAME = 0,
    DE_NAME,
    SH_VERSION,
    SH_NAME
};

std::string _get_wm_name() {
    const char *env = std::getenv("DESKTOP_SESSION");
    if (env == NULL) {
        env = std::getenv("XDG_CURRENT_DESKTOP");
        if (env == NULL) {
            error("Couldn't get WM/DE name, failed to get $DESKTOP_SESSION and $XDG_CURRENT_DESKTOP");
            return UNKNOWN;
        } else {
            return env;
        }
    }

    return env;
}

static std::string _get_shell_version(const std::string_view shell_name) {
    std::string shell = shell_name.data();
    std::string ret = UNKNOWN;

    switch (fnv1a32::hash(shell)) {
        case "bash"_fnv1a32:
        case "osh"_fnv1a32:
        case "zsh"_fnv1a32: 
            ret = shell_exec(fmt::format("{} -c 'echo \"${}_VERSION\"'", shell, str_toupper(shell))); break;

        case "nu"_fnv1a32:
            ret = shell_exec("nu -c \"version | get version\""); break;

        default:
            ret = shell_exec(fmt::format("{} --version", shell));
            //ret.erase(0, ret.find(shell));
    }
    
    strip(ret);
    return ret;
}

static std::string _get_shell_name(const std::string_view shell_path) {
    std::string ret = shell_path.substr(shell_path.rfind('/')+1).data();

    return ret;
}

static std::array<std::string, 6> get_users_infos(const std::string_view shell_path) {
    std::array<std::string, 6> ret;
    std::fill(ret.begin(), ret.end(), UNKNOWN);

    ret.at(SH_NAME) = _get_shell_name(shell_path);
    ret.at(SH_VERSION) = _get_shell_version(ret.at(SH_NAME));
    ret.at(WM_NAME) = _get_wm_name();
    ret.at(DE_NAME) = ret.at(WM_NAME);

    return ret;
}

User::User() {
    debug("Constructing {}", __func__);
    if (!m_bInit) {
        uid_t uid = geteuid();

        if (m_pPwd = getpwuid(uid), !m_pPwd)
            die("getpwent failed: {}\nCould not get user infos", errno);

        m_users_infos = get_users_infos(this->shell_path());

        m_bInit = true;
    }
}

std::string User::name() {
    return m_pPwd->pw_name;
}

std::string User::shell_path() {
    return m_pPwd->pw_shell;
}

std::string User::shell_name() {
    return m_users_infos.at(SH_NAME);
}

std::string User::shell_version() {
    return m_users_infos.at(SH_VERSION);
}

std::string User::wm_name() {
    return m_users_infos.at(WM_NAME);
}

std::string User::de_name() {
    return m_users_infos.at(DE_NAME);
}
