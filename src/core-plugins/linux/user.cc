#include <unistd.h>
#include <fstream>
#include "common.hpp"
#include "fmt/format.h"
#include "linux-core-modules.hh"
#include "switch_fnv1a.hpp"
#include "utils/term.hpp"
#include "util.hpp"

// clang-format off
static std::string get_term_name_env(bool get_default = false)
{
    if (getenv("SSH_TTY") != NULL)
        return getenv("SSH_TTY");

    if (getenv("KITTY_PID") != NULL              ||
        getenv("KITTY_INSTALLATION_DIR") != NULL ||
        getenv("KITTY_PUBLIC_KEY") != NULL       ||
        getenv("KITTY_WINDOW_ID") != NULL)
        return "kitty";
    
    if (getenv("ALACRITTY_SOCKET")      != NULL ||
        getenv("ALACRITTY_LOG")         != NULL ||
        getenv("ALACRITTY_WINDOW_ID")   != NULL)
        return "alacritty";

    if (getenv("TERMUX_VERSION")             != NULL ||
        getenv("TERMUX_MAIN_PACKAGE_FORMAT") != NULL)
        return "com.termux";

    if(getenv("KONSOLE_VERSION") != NULL)
        return "konsole";

    if (getenv("GNOME_TERMINAL_SCREEN")  != NULL ||
        getenv("GNOME_TERMINAL_SERVICE") != NULL) 
        return "gnome-terminal";

    if (get_default)
    {
        char *env = getenv("TERM_PROGRAM");
        if (env != NULL)
        {
            if (hasStart(env, "Apple"))
                return "Apple Terminal";

            return env;
        }

        env = getenv("TERM");
        if (env != NULL)
            return env;
    }

    return UNKNOWN;
}
// clang-format on


modfunc user_name()
{ return g_pwd->pw_name; }

modfunc user_shell_path()
{ return g_pwd->pw_shell; }

modfunc user_shell_name()
{
    return user_shell_path().substr(user_shell_path().rfind('/') + 1);
}

modfunc user_shell_version()
{
    const std::string& shell_name = user_shell_name();
    std::string ret;

    if (shell_name == "nu")
        ret = read_shell_exec("nu -c \"version | get version\"");
    else
        ret = read_shell_exec(fmt::format("{} -c 'echo \"${}_VERSION\"'", shell_name, str_toupper(shell_name.data())));

    strip(ret);
    return ret;
}

std::string get_terminal_pid()
{
    // customfetch -> shell -> terminal
    const pid_t   ppid = getppid();
    std::ifstream ppid_f(fmt::format("/proc/{}/status", ppid), std::ios::in);
    std::string   line, term_pid{ "0" };
    while (std::getline(ppid_f, line))
    {
        if (hasStart(line, "PPid:"))
        {
            term_pid = line.substr("PPid:"_len);
            strip(term_pid);
            break;
        }
    }
    debug("term_pid = {}", term_pid);

    if (std::stoi(term_pid) < 1)
        return MAGIC_LINE;

    return term_pid;
}

std::string get_terminal_name()
{
    if (term_pid == MAGIC_LINE)
        return MAGIC_LINE;

    std::ifstream f("/proc/" + term_pid + "/comm", std::ios::in);
    std::string   term_name;
    if (f.is_open())
        std::getline(f, term_name);
    else
        term_name = get_term_name_env(true);

    return term_name;
}

modfunc user_term_name()
{
    // st (suckless terminal)
    if (term_name == "exe")
        term_name = "st";
    // either gnome-console or "gnome-terminal-"
    // I hope this is not super stupid
    else if (hasStart(term_name, "gnome-console"))
        term_name.erase("gnome-console"_len + 1);
    else if (hasStart(term_name, "gnome-terminal"))
        term_name.erase("gnome-terminal"_len + 1);

    const std::string& osname = os_name();
    // let's try to get the real terminal name
    // on NixOS, instead of returning the -wrapped name.
    // tested on gnome-console, kitty, st and alacritty
    // hope now NixOS users will know the terminal they got, along the version if possible
    if (osname.find("NixOS") != osname.npos || (hasEnding(term_name, "wrapped") && which("nix") != UNKNOWN))
    {
        // /nix/store/sha256string-gnome-console-0.31.0/bin/.kgx-wrapped
        char        buf[PATH_MAX];
        std::string tmp_name = realpath(("/proc/" + term_pid + "/exe").c_str(), buf);

        size_t pos;
        if ((pos = tmp_name.find('-')) != std::string::npos)
            tmp_name.erase(0, pos + 1);  // gnome-console-0.31.0/bin/.kgx-wrapped

        if ((pos = tmp_name.find('/')) != std::string::npos)
            tmp_name.erase(pos);  // gnome-console-0.31.0

        if ((pos = tmp_name.rfind('-')) != std::string::npos)
            tmp_name.erase(pos);  // gnome-console  EZ

        term_name = tmp_name;
    }

    // sometimes may happen that the terminal name from /comm
    // at the end has some letfover characters from /cmdline
    if (!std::isalnum(term_name.back()))
    {
        size_t i = term_name.size();
        while (i > 0)
        {
            char ch = term_name[i - 1];
            // stop when we find an a num or alpha char
            // example  with "gnome-terminal-"
            if (std::isalnum(static_cast<unsigned char>(ch)))
                break;
            term_name.erase(--i, 1);
        }
    }

    return term_name;
}

modfunc user_term_version()
{
    const std::string& term_name = user_term_name();
    if (term_name.empty())
        return UNKNOWN;

    bool        remove_term_name = true;
    std::string ret;

    switch (fnv1a16::hash(str_tolower(term_name.data())))
    {
        case "st"_fnv1a16:
            if (fast_detect_st_ver(ret))
                remove_term_name = false;
            break;

        case "konsole"_fnv1a16:
            if (fast_detect_konsole_ver(ret))
                remove_term_name = false;
            break;

        case "xterm"_fnv1a16: get_term_version_exec(term_name, ret, true); break;

        default: get_term_version_exec(term_name, ret);
    }

    debug("get_term_version ret = {}", ret);

    if (ret.empty())
        return UNKNOWN;

    if (hasStart(ret, "# GNOME"))
    {
        if (hasStart(ret, "# GNOME Console "))
            ret.erase(0, "# GNOME Console"_len);
        else if (hasStart(ret, "# GNOME Terminal "))
            ret.erase(0, "# GNOME Terminal "_len);
        debug("gnome ret = {}", ret);
        remove_term_name = false;
    }
    // Xterm(388)
    else if (term_name == "xterm")
    {
        ret.erase(0, term_name.length() + 1);  // 388)
        ret.pop_back();                        // 388
        return ret;
    }

    if (remove_term_name)
        ret.erase(0, term_name.length() + 1);

    const size_t pos = ret.find(' ');
    if (pos != std::string::npos)
        ret.erase(pos);

    debug("get_term_version ret after = {}", ret);
    return ret;
}
