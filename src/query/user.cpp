#include "query.hpp"
#include "util.hpp"
#include "switch_fnv1a.hpp"

using namespace Query;

User::User() {
    uid_t uid = geteuid();

    if (m_pPwd = getpwuid(uid), !m_pPwd)
        die("getpwent failed: {}\nCould not get user infos", errno);
}

std::string User::name() {
    return m_pPwd->pw_name;
}

std::string User::shell() {
    std::string shell = this->shell_path();
    shell.erase(0, shell.find_last_of('/')+1);
    
    return shell;
}

std::string User::shell_path() {
    return m_pPwd->pw_shell;
}

std::string User::shell_version() {
    std::string shell = this->shell();
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
    
    debug("ret = {}", ret);
    strip(ret);
    return ret;
}
