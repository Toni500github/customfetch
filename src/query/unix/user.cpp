#include <unistd.h>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "query.hpp"
#include "util.hpp"
#include "utils/dewm.hpp"
#include "switch_fnv1a.hpp"

using namespace Query;

enum {
    WM_NAME = 0,
    DE_NAME,
    SH_VERSION,
    SH_NAME,
    TERM_NAME,
};

static std::string _get_de_name() {
    return parse_de_env();
}

static std::string _get_wm_name() {
    std::string path, proc_name, wm_name;
    uid_t uid = getuid();

    for (auto const& dir_entry : std::filesystem::directory_iterator{"/proc/"}) {
        if (!std::isdigit((dir_entry.path().string().at(6))))
            continue;

        path = dir_entry.path().string() + "/loginuid";
        std::ifstream f_uid(path, std::ios::binary);
        std::string s_uid;
        std::getline(f_uid, s_uid);
        if (std::stoul(s_uid) != uid)
            continue;
        
        path = dir_entry.path().string() + "/cmdline";
        std::ifstream f(path, std::ios::binary);
        std::getline(f, proc_name);
        debug("proc_name = {}", proc_name);
        
        size_t pos = 0;
        if ((pos = proc_name.find('\0')) != std::string::npos)
            proc_name.erase(pos);
        
        if ((pos = proc_name.rfind('/')) != std::string::npos)
            proc_name.erase(pos);
        
        if ((wm_name = prettify_wm_name(proc_name)) == MAGIC_LINE)
            continue;

        break;
    }
    
    debug("wm_name = {}", wm_name);
    return wm_name;
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

// don't mind, TODO make it work
static std::string _get_term_name() {
    std::string ret;
    auto ppid = getppid();
    
    std::ifstream f(fmt::format("/proc/{}/comm", ppid), std::ios::in);
    std::string name;
    f >> name;
    f.close();
    
    // st (suckless terminal)
    if (name == "exe")
        ret = "st";
    else 
        ret = name;

    return ret;
}

static std::array<std::string, 6> get_users_infos(const std::string_view shell_path) {
    std::array<std::string, 6> ret;
    std::fill(ret.begin(), ret.end(), UNKNOWN);

    ret.at(SH_NAME) = _get_shell_name(shell_path);
    ret.at(SH_VERSION) = _get_shell_version(ret.at(SH_NAME));
    ret.at(DE_NAME) = _get_de_name();
    ret.at(WM_NAME) = _get_wm_name();
    ret.at(TERM_NAME) = _get_term_name();

    if (ret.at(DE_NAME) == ret.at(WM_NAME))
        ret.at(DE_NAME) = MAGIC_LINE;

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

std::string User::term_name() {
    return m_users_infos.at(TERM_NAME);
}
