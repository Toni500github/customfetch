#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

#define FMT_HEADER_ONLY 1
#include "common.hpp"
#include "fmt/format.h"
#include "linux-core-modules.hh"
#include "switch_fnv1a.hpp"
#include "util.hpp"

static const char* read_value(const char* name, size_t n)
{
    if (!os_release)
        return UNKNOWN;
    rewind(os_release);  // Reset file pointer to start

    char*  buf  = strdup(UNKNOWN);  // Default value
    char*  line = NULL;
    size_t len  = 0;
    while (getline(&line, &len, os_release) != -1)
    {
        if (strncmp(line, name, n) != 0)
            continue;

        // Find the first quote after the key
        char* start = strchr(line + n, '"');
        if (!start)
            continue;
        start++;

        // Find the closing quote
        char* end = strrchr(start, '"');
        if (!end)
            continue;

        free(buf);
        buf = strndup(start, end - start);
        break;
    }

    free(line);
    return buf;
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

modfunc os_name()
{ return read_value("NAME=", "NAME="_len); }

modfunc os_pretty_name()
{ return read_value("PRETTY_NAME=", "PRETTY_NAME="_len); }

modfunc os_name_id()
{ return read_value("ID=", "ID="_len); }

modfunc os_version_id()
{ return read_value("VERSION_ID=", "VERSION_ID="_len); }

modfunc os_version_codename()
{ return read_value("VERSION_CODENAME=", "VERSION_CODENAME="_len); }

modfunc os_uptime()
{ return fmt::to_string(get_uptime()); }

modfunc os_kernel_name()
{ return g_uname_infos.sysname; }

modfunc os_kernel_version()
{ return g_uname_infos.release; }

modfunc os_hostname()
{ return g_uname_infos.nodename; }

modfunc os_initsys_name()
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

modfunc os_initsys_version()
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
