#include "platform.hpp"
#if CF_ANDROID

#include "core-modules.hh"
#include "util.hpp"

MODFUNC(os_name)
{ return "Android"; }

MODFUNC(os_pretty_name)
{ return "Android " + os_version_codename(NULL) + " " + os_version_id(NULL); }

MODFUNC(os_name_id)
{ return "android"; }

MODFUNC(os_version_id)
{ return get_android_property("ro.build.version.release"); }

MODFUNC(os_version_codename)
{ return get_android_property("ro.build.version.codename"); }

MODFUNC(os_kernel_name)
{ return g_uname_infos.sysname; }

MODFUNC(os_kernel_version)
{ return g_uname_infos.release; }

MODFUNC(os_hostname)
{ return g_uname_infos.nodename; }

MODFUNC(os_initsys_name)
{ return "init"; }

MODFUNC(os_initsys_version)
{ return ""; }

unsigned long os_uptime()
{
    struct std::timespec uptime;
    if (clock_gettime(CLOCK_BOOTTIME, &uptime) != 0)
        return 0;

    return (uint64_t)uptime.tv_sec + (uint64_t)uptime.tv_nsec / 1000000;
}

#endif
