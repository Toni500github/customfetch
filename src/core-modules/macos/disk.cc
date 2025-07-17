#include "platform.hpp"
#if CF_MACOS

#include <sys/mount.h>
#include <sys/param.h>
#include <sys/types.h>

#include <string>

#include "core-modules.hh"
#include "cufetch/common.hh"
#include "fmt/format.h"
#include "util.hpp"

static std::string format_auto_query_string(std::string str, const struct statfs* fs)
{
    replace_str(str, "%1", fs->f_mntonname);
    replace_str(str, "%2", fs->f_mntfromname);
    replace_str(str, "%3", fs->f_fstypename);

    replace_str(str, "%4", fmt::format("$<disk({}).total>", fs->f_mntonname));
    replace_str(str, "%5", fmt::format("$<disk({}).free>", fs->f_mntonname));
    replace_str(str, "%6", fmt::format("$<disk({}).used>", fs->f_mntonname));
    replace_str(str, "%7", fmt::format("$<disk({}).used_perc>", fs->f_mntonname));
    replace_str(str, "%8", fmt::format("$<disk({}).free_perc>", fs->f_mntonname));

    return str;
}

static int get_disk_type(const int flags)
{
    int type = 0;
    if (flags & MNT_DONTBROWSE)
        type = DISK_VOLUME_TYPE_HIDDEN;
    else if (flags & MNT_REMOVABLE || !(flags & MNT_LOCAL))
        type = DISK_VOLUME_TYPE_EXTERNAL;
    else
        type = DISK_VOLUME_TYPE_REGULAR;

    if (flags & MNT_RDONLY)
        type |= DISK_VOLUME_TYPE_READ_ONLY;

    return type;
}

static bool get_disk_info(const callbackInfo_t* callbackInfo, struct statfs* fs)
{
    if (callbackInfo->moduleArgs->name != "disk" ||
        (callbackInfo->moduleArgs->name == "disk" && callbackInfo->moduleArgs->value.empty()))
        die("Module disk doesn't have an argmument to the path/device to query");

    const std::string& path = callbackInfo->moduleArgs->value;
    return (statfs(path.c_str(), fs) != 0);
}

MODFUNC(disk_fsname)
{
    struct statfs fs;
    if (!get_disk_info(callbackInfo, &fs))
        return MAGIC_LINE;

    return fs.f_fstypename;
}

MODFUNC(disk_device)
{
    struct statfs fs;
    if (!get_disk_info(callbackInfo, &fs))
        return MAGIC_LINE;

    return fs.f_mntfromname;
}

MODFUNC(disk_mountdir)
{
    struct statfs fs;
    if (!get_disk_info(callbackInfo, &fs))
        return MAGIC_LINE;

    return fs.f_mntonname;
}

MODFUNC(auto_disk)
{ return MAGIC_LINE; }

MODFUNC(disk_types)
{
    struct statfs fs;
    if (!get_disk_info(callbackInfo, &fs))
        return MAGIC_LINE;

    const int   types = get_disk_type(fs.f_flags);
    std::string str;
    if (types & DISK_VOLUME_TYPE_EXTERNAL)
        str += "External, ";
    if (types & DISK_VOLUME_TYPE_HIDDEN)
        str += "Hidden, ";
    if (types & DISK_VOLUME_TYPE_READ_ONLY)
        str += "Read-only, ";

    if (!str.empty())
        str.erase(str.length() - 2);

    return str;
}

double disk_total(const callbackInfo_t* callbackInfo)
{
    struct statfs fs;
    if (!get_disk_info(callbackInfo, &fs))
        return 0;

    return static_cast<double>(fs.f_blocks * fs.f_bsize);
}

double disk_free(const callbackInfo_t* callbackInfo)
{
    struct statfs fs;
    if (!get_disk_info(callbackInfo, &fs))
        return 0;

    return static_cast<double>(fs.f_bfree * fs.f_bsize);
}

double disk_used(const callbackInfo_t *callbackInfo)
{ 
    return disk_total(callbackInfo) - disk_free(callbackInfo);
}

#endif
