#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>

#define FMT_HEADER_ONLY 1
#include "common.hpp"
#include "core-modules.hh"
#include "fmt/format.h"
#include "switch_fnv1a.hpp"
#include "util.hpp"

static std::string read_value(const std::string_view name)
{
    if (!os_release)
        return UNKNOWN;

    rewind(os_release);

    std::string result{ UNKNOWN };
    char*       line = nullptr;
    size_t      len  = 0;

    while (getline(&line, &len, os_release) != -1)
    {
        if (name.length() > len || strncmp(line, name.data(), name.length()) != 0)
            continue;

        char* start = strchr(line + name.length(), '"'); /* Get first occurence of " */
        if (start)
            start++; /* Get after the " */
        else
            start = line + name.length(); /* No ", get the start. */

        char* end = strrchr(start, '"'); /* Get last occurence of " */
        if (!end)
            end = line + strlen(line) - 1; /* Set to the end of the string -- no newline. (I heard Windows has a
                                              different newline sequence.. *sigh*) */

        result.assign(start, end - start);
        break;
    }

    free(line);
    return result;
}

static unsigned long get_uptime()
{
    const std::string& buf = read_by_syspath("/proc/uptime");
    if (buf != UNKNOWN)
        return std::stoul(buf.substr(0, buf.find('.')));  // 19065.18 190952.06

    struct std::timespec uptime;
    if (clock_gettime(CLOCK_BOOTTIME, &uptime) != 0)
        return 0;

    return (unsigned long)uptime.tv_sec * 1000 + (unsigned long)uptime.tv_nsec / 1000000;
}

MODFUNC(os_name)
{ return read_value("NAME="); }

MODFUNC(os_pretty_name)
{ return read_value("PRETTY_NAME="); }

MODFUNC(os_name_id)
{ return read_value("ID="); }

MODFUNC(os_version_id)
{ return read_value("VERSION_ID="); }

MODFUNC(os_version_codename)
{ return read_value("VERSION_CODENAME="); }

MODFUNC(os_uptime)
{ return fmt::to_string(get_uptime()); }

MODFUNC(os_kernel_name)
{ return g_uname_infos.sysname; }

MODFUNC(os_kernel_version)
{ return g_uname_infos.release; }

MODFUNC(os_hostname)
{ return g_uname_infos.nodename; }

MODFUNC(os_initsys_name)
{
    // there's no way PID 1 doesn't exist.
    // This will always succeed (because we are on linux)
    std::ifstream f_initsys("/proc/1/comm", std::ios::binary);
    if (!f_initsys.is_open())
        die(_("/proc/1/comm doesn't exist! (what?)"));

    std::string initsys;
    std::getline(f_initsys, initsys);
    size_t pos = 0;

    if ((pos = initsys.find('\0')) != std::string::npos)
        initsys.erase(pos);

    if ((pos = initsys.rfind('/')) != std::string::npos)
        initsys.erase(0, pos + 1);

    return initsys;
}

MODFUNC(os_initsys_version)
{
    std::string os_initsys_version;
    std::string path;
    char        buf[PATH_MAX];
    if (realpath(which("init").c_str(), buf))
        path = buf;

    std::ifstream f(path, std::ios::in);
    std::string   line;

    const std::string& name = str_tolower(os_initsys_name());
    switch (fnv1a16::hash(name))
    {
        case "systemd"_fnv1a16:
        case "systemctl"_fnv1a16:
        {
            while (read_binary_file(f, line))
            {
                if (hasEnding(line, "running in %ssystem mode (%s)"))
                {
                    os_initsys_version = line.substr("systemd "_len);
                    os_initsys_version.erase(os_initsys_version.find(' '));
                    break;
                }
            }
        }
        break;
        case "openrc"_fnv1a16:
        {
            std::string tmp;
            while (read_binary_file(f, line))
            {
                if (line == "RC_VERSION")
                {
                    os_initsys_version = tmp;
                    break;
                }
                tmp = line;
            }
        }
        break;
    }

    return os_initsys_version;
}
