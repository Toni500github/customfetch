/*
 * Copyright 2025 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "platform.hpp"
#if CF_LINUX || CF_MACOS

#include <dlfcn.h>
#include <unistd.h>

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

#if __has_include(<sys/socket.h>) && __has_include(<wayland-client.h>)
#include <sys/socket.h>
#include <wayland-client.h>
#endif

// #if __has_include(<sys/socket.h>) && __has_include(<X11/Xlib.h>)
// # include <X11/Xlib.h>
// #endif

#include "query.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"
#include "utils/dewm.hpp"
#include "utils/term.hpp"

using namespace Query;

static std::string get_de_name()
{
    std::string ret = parse_de_env();
    debug("get_de_name = {}", ret);
    if (hasStart(ret, "X-"))
        ret.erase(0,2);

    return ret;
}

static std::string get_wm_name(std::string& wm_path_exec)
{
    std::string path, proc_name, wm_name;
    const uid_t uid = getuid();

    for (auto const& dir_entry : std::filesystem::directory_iterator{ "/proc/" })
    {
        if (!std::isdigit((dir_entry.path().string().at(6))))  // /proc/5
            continue;

        path = dir_entry.path() / "loginuid";
        std::ifstream f_uid(path, std::ios::binary);
        std::string   s_uid;
        std::getline(f_uid, s_uid);
        if (std::stoul(s_uid) != uid)
            continue;

        path = dir_entry.path() / "cmdline";
        std::ifstream f_cmdline(path, std::ios::binary);
        std::getline(f_cmdline, proc_name);

        size_t pos = 0;
        if ((pos = proc_name.find('\0')) != std::string::npos)
            proc_name.erase(pos);

        if ((pos = proc_name.rfind('/')) != std::string::npos)
            proc_name.erase(0, pos + 1);

        debug("WM proc_name = {}", proc_name);

        if ((wm_name = prettify_wm_name(proc_name)) == MAGIC_LINE)
            continue;

        char buf[PATH_MAX];
        wm_path_exec = realpath((dir_entry.path().string() + "/exe").c_str(), buf);
        break;
    }

    debug("wm_name = {}", wm_name);
    if (wm_name.empty())
        return MAGIC_LINE;

    return wm_name;
}

static std::string get_de_version(const std::string_view de_name)
{
    switch (fnv1a16::hash(str_tolower(de_name.data())))
    {
        case "mate"_fnv1a16:     return get_mate_version();
        case "cinnamon"_fnv1a16: return get_cinnamon_version();

        case "kde"_fnv1a16: return get_kwin_version();

        case "xfce"_fnv1a16:
        case "xfce4"_fnv1a16: return get_xfce4_version();

        case "gnome"_fnv1a16:
        case "gnome-shell"_fnv1a16:
        {
            std::string ret;
            read_exec({ "gnome-shell", "--version" }, ret);
            ret.erase(0, ret.rfind(' '));
            return ret;
        }
        default:
        {
            std::string ret;
            read_exec({ de_name.data(), "--version" }, ret);
            ret.erase(0, ret.rfind(' '));
            return ret;
        }
    }
}

static std::string get_wm_wayland_name(std::string& wm_path_exec)
{
#if __has_include(<sys/socket.h>) && __has_include(<wayland-client.h>)
    LOAD_LIBRARY("libwayland-client.so", return get_wm_name(wm_path_exec);)

    LOAD_LIB_SYMBOL(wl_display*, wl_display_connect, const char* name)
    LOAD_LIB_SYMBOL(void, wl_display_disconnect, wl_display* display)
    LOAD_LIB_SYMBOL(int, wl_display_get_fd, wl_display* display)

    std::string ret = MAGIC_LINE;

    struct wl_display* display = wl_display_connect(NULL);

    struct ucred ucred;
    socklen_t    len = sizeof(struct ucred);
    if (getsockopt(wl_display_get_fd(display), SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1)
        return MAGIC_LINE;

    std::ifstream f(fmt::format("/proc/{}/comm", ucred.pid), std::ios::in);
    f >> ret;
    wl_display_disconnect(display);

    char buf[PATH_MAX];    
    wm_path_exec = realpath(fmt::format("/proc/{}/exe", ucred.pid).c_str(), buf);

    UNLOAD_LIBRARY()

    return prettify_wm_name(ret);
#else
    return get_wm_name(wm_path_exec);
#endif
}

static std::string get_shell_version(const std::string_view shell_name)
{
    std::string ret;

    if (shell_name == "nu")
        ret = read_shell_exec("nu -c \"version | get version\"");
    else
        ret = read_shell_exec(fmt::format("{} -c 'echo \"${}_VERSION\"'", shell_name, str_toupper(shell_name.data())));

    strip(ret);
    return ret;
}

static std::string get_shell_name(const std::string_view shell_path)
{
    return shell_path.substr(shell_path.rfind('/') + 1).data();
}

#if CF_LINUX
static std::string get_term_name(std::string& term_ver, const std::string_view osname)
{
    // customfetch -> shell -> terminal
    const pid_t   ppid = getppid();
    std::ifstream ppid_f(fmt::format("/proc/{}/status", ppid), std::ios::in);
    std::string   line, term_pid{"0"};
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

    std::ifstream f("/proc/" + term_pid + "/comm", std::ios::in);
    std::string   term_name;
    std::getline(f, term_name);

    // st (suckless terminal)
    if (term_name == "exe")
        term_name = "st";

    // either gnome-console or "gnome-terminal-"
    // I hope this is not super stupid
    if (hasStart(term_name, "gnome-console"))
        term_name.erase("gnome-console"_len + 1);
    else if (hasStart(term_name, "gnome-terminal"))
        term_name.erase("gnome-terminal"_len + 1);

    
    // let's try to get the real terminal name
    // on NixOS, instead of returning the -wrapped name.
    // tested on gnome-console, kitty, st and alacritty
    // hope now NixOS users will know the terminal they got, along the version if possible
    if (osname.find("NixOS") != osname.npos ||
        (hasEnding(term_name, "wrapped") && which("nix") != UNKNOWN))
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
        {
            term_ver = tmp_name.substr(pos+1);
            tmp_name.erase(pos);  // gnome-console  EZ
        }

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
#elif CF_MACOS
#include <sys/proc_info.h>
#include <libproc.h>
static std::string get_term_name(std::string& term_ver, const std::string_view osname)
{
    // customfetch -> shell -> terminal
    struct proc_bsdinfo info;
    if (proc_pidinfo(getppid(), PROC_PIDTBSDINFO, 0, &info, sizeof(info)) <= 0)
        return UNKNOWN;

    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
    if (!proc_pidpath(info.pbi_ppid, pathbuf, sizeof(pathbuf)) <= 0)
        return UNKNOWN;

    std::string path{pathbuf};
    if ((size_t pos = path.rfind('/')) != path.npos)
        path.erase(0, pos+1);

    return path;
}
#endif

static std::string get_term_version(const std::string_view term_name)
{
    if (term_name.empty())
        return UNKNOWN;

    bool remove_term_name = true;
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
        
        case "xterm"_fnv1a16:
            get_term_version_exec(term_name, ret, true); break;

        default:
            get_term_version_exec(term_name, ret);
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

User::User() noexcept
{
    CHECK_INIT(m_bInit);

    if (m_pPwd = getpwuid(getuid()), !m_pPwd)
        die(_("getpwent failed: {}\nCould not get user infos"), std::strerror(errno));
}

// clang-format off
std::string User::name() noexcept
{ return m_pPwd->pw_name; }

std::string User::shell_path() noexcept
{ return m_pPwd->pw_shell; }

// clang-format on
// Be ready to loose some brain cells from now on
std::string& User::shell_name() noexcept
{
    static bool done = false;
    if (!done)
    {
        m_users_infos.shell_name = get_shell_name(this->shell_path());
        done                     = true;
    }

    return m_users_infos.shell_name;
}

std::string& User::shell_version(const std::string_view shell_name)
{
    if (m_users_infos.shell_name.empty())
    {
        m_users_infos.shell_version = UNKNOWN;
        return m_users_infos.shell_version;
    }

    static bool done = false;
    if (!done)
    {
        m_users_infos.shell_version = get_shell_version(shell_name);
        done                        = true;
    }

    return m_users_infos.shell_version;
}

std::string& User::wm_name(bool dont_query_dewm, const std::string_view term_name)
{
    if (dont_query_dewm || hasStart(term_name, "/dev") || CF_MACOS)
    {
        m_users_infos.wm_name = MAGIC_LINE;
        return m_users_infos.wm_name;
    }

    static bool done = false;
    debug("CALLING {} || done = {} && de_name = {} && wm_name = {}", __func__, done, m_users_infos.de_name,
          m_users_infos.wm_name);

    if (!done)
    {
        const char* env = std::getenv("WAYLAND_DISPLAY");
        if (env != nullptr && env[0] != '\0')
            m_users_infos.wm_name = get_wm_wayland_name(m_users_infos.m_wm_path);
        else
            m_users_infos.wm_name = get_wm_name(m_users_infos.m_wm_path);

        if (m_users_infos.de_name == m_users_infos.wm_name)
            m_users_infos.de_name = MAGIC_LINE;

        done = true;
    }

    return m_users_infos.wm_name;
}

std::string& User::wm_version(bool dont_query_dewm, const std::string_view term_name)
{
    if (dont_query_dewm || hasStart(term_name, "/dev") || CF_MACOS)
    {
        m_users_infos.wm_name = MAGIC_LINE;
        return m_users_infos.wm_name;
    }
    
    static bool done = false;
    if (!done)
    {
        m_users_infos.wm_version.clear();
        if (m_users_infos.wm_name == "Xfwm4" && get_fast_xfwm4_version(m_users_infos.wm_version, m_users_infos.m_wm_path))
        {
            done = true;
            goto _return;
        }

        if (m_users_infos.wm_name == "dwm")
            read_exec({m_users_infos.m_wm_path.c_str(), "-v"}, m_users_infos.wm_version, true);
        else
            read_exec({m_users_infos.m_wm_path.c_str(), "--version"}, m_users_infos.wm_version);

        if (m_users_infos.wm_name == "Xfwm4")
            m_users_infos.wm_version.erase(0, "\tThis is xfwm4 version "_len); // saying only "xfwm4 4.18.2 etc." no?
        else
            m_users_infos.wm_version.erase(0, m_users_infos.wm_name.length() + 1);

        const size_t pos = m_users_infos.wm_version.find(' ');
        if (pos != std::string::npos)
            m_users_infos.wm_version.erase(pos);

        done = true;
    }

    _return:
    return m_users_infos.wm_version;
}

std::string& User::de_name(bool dont_query_dewm, const std::string_view term_name, const std::string_view wm_name)
{
    // first let's see if we are not in a tty or if the user doesn't want to
    // if so don't even try to get the DE or WM names
    // they waste times
    if (dont_query_dewm || hasStart(term_name, "/dev") || CF_MACOS)
    {
        m_users_infos.de_name = MAGIC_LINE;
        return m_users_infos.de_name;
    }

    static bool done = false;
    debug("CALLING {} || done = {} && de_name = {} && wm_name = {}", __func__, done, m_users_infos.de_name,
          m_users_infos.wm_name);

    if (!done)
    {
        if ((m_users_infos.de_name != MAGIC_LINE && wm_name != MAGIC_LINE) &&
            m_users_infos.de_name == wm_name)
        {
            m_users_infos.de_name = MAGIC_LINE;
            done = true;
            return m_users_infos.de_name;
        }

        m_users_infos.de_name = get_de_name();
        if (m_users_infos.de_name == m_users_infos.wm_name)
            m_users_infos.de_name = MAGIC_LINE;

        done = true;
    }

    return m_users_infos.de_name;
}

std::string& User::de_version(const std::string_view de_name)
{
    if (m_bDont_query_dewm || de_name == UNKNOWN || de_name == MAGIC_LINE || de_name.empty() || CF_MACOS)
    {
        m_users_infos.de_version = UNKNOWN;
        return m_users_infos.de_version;
    }

    static bool done = false;
    if (!done)
    {
        m_users_infos.de_version = get_de_version(str_tolower(de_name.data()));
        done                     = true;
    }

    return m_users_infos.de_version;
}

std::string& User::term_name()
{
    static bool done = false;
    if (done || is_live_mode)
        return m_users_infos.term_name;

    Query::System query_sys;
    m_users_infos.term_name = get_term_name(m_users_infos.term_version, query_sys.os_name());
    if (hasStart(str_tolower(m_users_infos.term_name), "login") || hasStart(m_users_infos.term_name, "init") ||
        hasStart(m_users_infos.term_name, "(init)"))
    {
        m_users_infos.term_name    = ttyname(STDIN_FILENO);
        m_users_infos.term_version = "NO VERSIONS ABOSULETY";  // lets not make it unknown
        m_bDont_query_dewm         = true;
    }

    done = true;

    return m_users_infos.term_name;
}

std::string& User::term_version(const std::string_view term_name)
{
    static bool done = false;
    if (done || is_live_mode)
        return m_users_infos.term_version;

    if (m_users_infos.term_version == "NO VERSIONS ABOSULETY")
    {
        m_users_infos.term_version.clear();
        goto done;
    }
    else if (m_users_infos.term_version != MAGIC_LINE)
        goto done;

    m_users_infos.term_version = get_term_version(term_name);
done:
    done = true;

    return m_users_infos.term_version;
}

#endif // CF_LINUX
