#include "platform.hpp"
#if CF_ANDROID

#include <linux/limits.h>
#include <string>

#include "core-modules.hh"
#include "util.hpp"

MODFUNC(user_name)
{ return g_pwd->pw_name; }

MODFUNC(user_shell_path)
{
    char buf[PATH_MAX];
    return realpath(fmt::format("/proc/{}/exe", getppid()).c_str(), buf);
}

MODFUNC(user_shell_name)
{
    return user_shell_path(callbackInfo).substr(user_shell_path(callbackInfo).rfind('/') + 1);
}

MODFUNC(user_shell_version)
{
    const std::string& shell_name = user_shell_name(callbackInfo);
    std::string ret;

    if (shell_name == "nu")
        ret = read_shell_exec("nu -c \"version | get version\"");
    else
        ret = read_shell_exec(fmt::format("{} -c 'echo \"${}_VERSION\"'", shell_name, str_toupper(shell_name.data())));

    strip(ret);
    return ret;
}

MODFUNC(user_term_name)
{ return "Termux"; }

MODFUNC(user_term_version)
{ return getenv("TERMUX_VERSION"); }

MODFUNC(user_wm_name)
{ return MAGIC_LINE; }

MODFUNC(user_wm_version)
{ return MAGIC_LINE; }

MODFUNC(user_de_name)
{ return MAGIC_LINE; }

MODFUNC(user_de_version)
{ return MAGIC_LINE; }

#endif
