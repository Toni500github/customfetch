#include <dlfcn.h>
#include <unistd.h>

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>

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

using namespace Query;

static std::string _get_de_name()
{
    const std::string& ret = parse_de_env();
    debug("get_de_name = {}", ret);
    return ret;
}

static std::string _get_wm_name()
{
    std::string path, proc_name, wm_name;
    uid_t       uid = getuid();

    for (auto const& dir_entry : std::filesystem::directory_iterator{"/proc/"})
    {
        if (!std::isdigit((dir_entry.path().string().at(6))))  // /proc/50
            continue;

        path = dir_entry.path().string() + "/loginuid";
        std::ifstream f_uid(path, std::ios::binary);
        std::string   s_uid;
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
    if (wm_name.empty())
        return MAGIC_LINE;

    return wm_name;
}

static std::string get_de_version(const std::string_view de_name)
{
    switch (fnv1a32::hash(de_name.data()))
    {
        case "mate"_fnv1a32:     return get_mate_version();
        case "cinnamon"_fnv1a32: return get_cinnamon_version();
        case "gnome"_fnv1a32:
        {
            std::string ret;
            read_exec({ "gnome-shell", "--version" }, ret);
            ret.erase(0, ret.rfind(' '));
            return ret;
        }
        case "xfce"_fnv1a32:
        case "xfce4"_fnv1a32:
        {
            std::string ret;
            read_exec({ "xfce4-session", "--version" }, ret);
            ret.erase(0, "xfce4-session"_len + 1);
            ret.erase(ret.find(' '));
            return ret;
        }
    }
}

static std::string _get_wm_wayland_name()
{
#if __has_include(<sys/socket.h>) && __has_include(<wayland-client.h>)
    LOAD_LIBRARY("libwayland-client.so", return MAGIC_LINE;)

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

    UNLOAD_LIBRARY()

    return ret;
#endif
}

/*static std::string get_wm_x11_name() {
#if __has_include(<sys/socket.h>) && __has_include(<X11/Xlib.h>)
    LOAD_LIBRARY("libX11.so", return MAGIC_LINE;)
    dlerror();
    LOAD_LIB_SYMBOL(Display *, XOpenDisplay, char*)
    LOAD_LIB_SYMBOL(int, XCloseDisplay, Display *)

    Display *display = XOpenDisplay(NULL);
    if (display == NULL)
        return MAGIC_LINE;

    debug("Connection x11 = {}", ConnectionNumber(display));
    std::string ret = MAGIC_LINE;

    struct ucred ucred;
    socklen_t len = sizeof(struct ucred);
    if (getsockopt(ConnectionNumber(display), SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1)
        return UNKNOWN;

    std::ifstream f(fmt::format("/proc/{}/comm", ucred.pid), std::ios::in);
    f >> ret;
    XCloseDisplay(display);

    return ret;
#endif
}*/

static std::string _get_shell_version(const std::string_view shell_name) noexcept
{
    std::string ret;

    if (shell_name == "nu")
        ret = shell_exec("nu -c \"version | get version\"");
    else
        ret = shell_exec(fmt::format("{} -c 'echo \"${}_VERSION\"'", shell_name, str_toupper(shell_name.data())));

    strip(ret);
    return ret;
}

static std::string _get_shell_name(const std::string_view shell_path) noexcept
{
    std::string ret = shell_path.substr(shell_path.rfind('/') + 1).data();

    return ret;
}

static std::string _get_term_name() noexcept
{
    // cufetch -> shell -> terminal
    pid_t         ppid = getppid();
    std::ifstream ppid_f(fmt::format("/proc/{}/status", ppid), std::ios::in);
    std::string   line, term_pid;
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

    std::ifstream f("/proc/" + term_pid + "/comm", std::ios::in);
    std::string   name;
    std::getline(f, name);

    // st (suckless terminal)
    if (name == "exe")
        name = "st";
    // NixOS
    else if (hasEnding(name, "wrapped"))
    {
         // /nix/store/random_stuff-kitty-0.31.0/bin/.kitty_wrapped
        std::ifstream cmdline_f("/proc/" + term_pid + "/cmdline", std::ios::binary);
        std::string tmp_name;
        std::getline(cmdline_f, tmp_name);
        
        size_t pos;
        if ((pos = tmp_name.rfind('-')) != std::string::npos)
            tmp_name.erase(pos);  // /nix/store/random_stuff-kitty 
        
        // another one
        if ((pos = tmp_name.rfind('-')) != std::string::npos)
            tmp_name.erase(0,pos); // kitty  EZ
        
        name = tmp_name;
    }

    return name;
}

static std::string _get_term_version(std::string_view term_name)
{
    if (term_name.empty())
        return UNKNOWN;

    std::string ret;
    if (hasStart(term_name, "kitty"))
        term_name = "kitten";

    if (hasStart(term_name, "st"))
        read_exec({ term_name.data(), "-v" }, ret, true);
    else  // tell your terminal to NOT RETURN ERROR WHEN ASKING FOR ITS VERSION (looking at you st)
        read_exec({ term_name.data(), "--version" }, ret);

    debug("ret = {}", ret);

    if (ret.empty())
        return UNKNOWN;

    size_t pos = 0;
    ret.erase(0, term_name.length() + 1);
    if ((pos = ret.find(' ')) != std::string::npos)
        ret.erase(pos);

    debug("ret after = {}", ret);
    return ret;
}

User::User()
{
    debug("Constructing {}", __func__);
    if (!m_bInit)
    {
        uid_t uid = geteuid();

        if (m_pPwd = getpwuid(uid), !m_pPwd)
            die("getpwent failed: {}\nCould not get user infos", errno);

        m_bInit = true;
    }
}

// clang-format off
std::string User::name() noexcept 
{ return m_pPwd->pw_name; }

std::string User::shell_path() noexcept
{ return m_pPwd->pw_shell; }

// clang-format on
std::string User::shell_name() noexcept
{
    static bool done = false;
    if (!done)
    {
        m_users_infos.shell_name = _get_shell_name(this->shell_path());
        done                     = true;
    }

    return m_users_infos.shell_name;
}

std::string User::shell_version(const std::string_view shell_name) noexcept
{
    if (m_users_infos.shell_name.empty())
        return UNKNOWN;

    static bool done = false;
    if (!done)
    {
        m_users_infos.shell_version = _get_shell_version(shell_name);
        done                        = true;
    }

    return m_users_infos.shell_version;
}

std::string User::wm_name(bool dont_query_dewm, const std::string_view term_name)
{
    if (dont_query_dewm || hasStart(term_name, "/dev"))
        return MAGIC_LINE;

    static bool done = false;
    debug("CALLING {} || done = {} && de_name = {} && wm_name = {}", __func__, done, m_users_infos.de_name,
          m_users_infos.wm_name);

    if (!done)
    {
        const char* env = std::getenv("WAYLAND_DISPLAY");
        if (env != nullptr && env[0] != '\0')
            m_users_infos.wm_name = _get_wm_wayland_name();
        else
            m_users_infos.wm_name = _get_wm_name();

        if (m_users_infos.de_name == m_users_infos.wm_name)
        {
            m_users_infos.de_name = MAGIC_LINE;
            m_bCut_de             = true;
        }

        done = true;
    }

    return m_users_infos.wm_name;
}

std::string User::de_name(bool dont_query_dewm, const std::string_view term_name, const std::string_view wm_name)
{
    // first let's see if we are not in a tty or if the user doesn't want to
    // if so don't even try to get the DE or WM names
    // they waste times
    if (dont_query_dewm || hasStart(term_name, "/dev") || m_bCut_de)
        return MAGIC_LINE;

    static bool done = false;
    debug("CALLING {} || done = {} && de_name = {} && wm_name = {}", __func__, done, m_users_infos.de_name,
          m_users_infos.wm_name);

    if (!done)
    {
        if (m_users_infos.de_name != MAGIC_LINE && wm_name != MAGIC_LINE && m_users_infos.de_name == wm_name)
        {
            m_users_infos.de_name = MAGIC_LINE;
            // cry about it
            goto ret;
        }

        m_users_infos.de_name = _get_de_name();
        if (m_users_infos.de_name == m_users_infos.wm_name)
        {
            m_users_infos.de_name = MAGIC_LINE;
            m_bCut_de             = true;
        }

    ret:
        done = true;
    }

    return m_users_infos.de_name;
}

std::string User::de_version(const std::string_view de_name)
{
    if (m_bDont_query_dewm || de_name == UNKNOWN || de_name == MAGIC_LINE || de_name.empty())
        return UNKNOWN;

    static bool done = false;
    if (!done)
    {
        m_users_infos.de_version = get_de_version(str_tolower(de_name.data()));
        done                     = true;
    }

    return m_users_infos.de_version;
}

std::string User::term_name()
{
    static bool done = false;
    if (!done)
    {
        m_users_infos.term_name = _get_term_name();
        if (hasStart(str_tolower(m_users_infos.term_name), "login") || hasStart(m_users_infos.term_name, "init") ||
            hasStart(m_users_infos.term_name, "(init)"))
        {
            m_users_infos.term_name    = ttyname(STDIN_FILENO);
            m_users_infos.term_version = "NO VERSIONS ABOSULETY";  // lets not make it unknown
            m_bDont_query_dewm         = true;
        }

        done = true;
    }
    return m_users_infos.term_name;
}

std::string User::term_version(const std::string_view term_name)
{
    static bool done = false;
    if (!done)
    {
        if (m_users_infos.term_version == "NO VERSIONS ABOSULETY")
        {
            done = true;
            m_users_infos.term_version.clear();
            return m_users_infos.term_version;
        }

        m_users_infos.term_version = _get_term_version(term_name);
        done                       = true;
    }
    return m_users_infos.term_version;
}
