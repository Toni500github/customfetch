#include "platform.hpp"
#if CF_MACOS

#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>

#include <cstring>
#include <string>

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

#endif
