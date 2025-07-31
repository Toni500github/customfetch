/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "platform.hpp"
#if CF_MACOS

#include <sys/mount.h>
#include <sys/param.h>
#include <sys/types.h>

#include <string>

#include "core-modules.hh"
#include "fmt/format.h"
#include "libcufetch/common.hh"
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
