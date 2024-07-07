#include <unistd.h>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
//#include <sys/socket.h>
//#include <wayland-client.h>
#include <proc/readproc.h>

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
    TERM_VERISON
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

/*static std::string _get_wm_wayland_name() {
    std::string ret = MAGIC_LINE;

    struct wl_display *display = wl_display_connect(NULL);

    struct ucred ucred;
    socklen_t len = sizeof(struct ucred);
    if (getsockopt(wl_display_get_fd(display), SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1)
        return MAGIC_LINE;
    
    std::ifstream f(fmt::format("/proc/{}/comm", ucred.pid), std::ios::in);
    f >> ret;
    wl_display_disconnect(display);

    return ret;
}*/

static std::string _get_shell_version(const std::string_view shell_name) {
    std::string shell = shell_name.data();
    std::string ret = UNKNOWN;

    switch (fnv1a32::hash(shell)) {
        case "nu"_fnv1a32:
            ret = shell_exec("nu -c \"version | get version\""); break;
        
        case "bash"_fnv1a32:
        case "osh"_fnv1a32:
        case "zsh"_fnv1a32: 
        default:
            ret = shell_exec(fmt::format("{} -c 'echo \"${}_VERSION\"'", shell, str_toupper(shell)));
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
    // ./cufetch -> shell -> terminal
    // https://ubuntuforums.org/showthread.php?t=2372923&p=13693160#post13693160
    pid_t ppid = getppid();
    proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_info));
    PROCTAB *pt_ptr = openproc(PROC_FILLSTATUS | PROC_PID, &ppid);
    if (readproc(pt_ptr, &proc_info) == NULL)
        return MAGIC_LINE;
    
    std::ifstream f(fmt::format("/proc/{}/comm", proc_info.ppid), std::ios::in);
    std::string name;
    f >> name;
    f.close();
    
    // st (suckless terminal)
    if (name == "exe")
        ret = "st";
    else 
        ret = name;

    closeproc(pt_ptr);

    return ret;
}

static std::string _get_term_version(std::string_view term_name) {
    std::string ret;
    if (hasStart(term_name, "kitty"))
        term_name = "kitten";

    if (hasStart(term_name, "st"))
        read_exec({term_name.data(), "-v"}, ret, true);
    
    else if (hasStart(term_name, "kitty"))
        read_exec({"kitten", "--version"}, ret);

    else // tell your terminal to NOT RETURN ERROR WHEN ASKING FOR ITS VERSION (looking at you st)
        read_exec({term_name.data(), "--version"}, ret);

    debug("ret = {}", ret);
    
    if (ret.empty())
        return UNKNOWN;

    size_t pos = 0;
    ret.erase(0, term_name.length()+1);
    if ((pos = ret.find(' ')) != std::string::npos)
        ret.erase(pos);

    debug("ret after = {}", ret);
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
    ret.at(TERM_VERISON) = _get_term_version(ret.at(TERM_NAME));

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

std::string User::term_version() {
    return m_users_infos.at(TERM_VERISON);
}
