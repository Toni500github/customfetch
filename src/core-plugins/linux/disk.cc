#include <mntent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include <cstdio>

#include "common.hpp"
#include "core-modules.hh"
#include "util.hpp"

enum
{
    DISK_VOLUME_TYPE_HIDDEN    = 1 << 2,
    DISK_VOLUME_TYPE_REGULAR   = 1 << 3,
    DISK_VOLUME_TYPE_EXTERNAL  = 1 << 4,
    DISK_VOLUME_TYPE_READ_ONLY = 1 << 5,
};

// https://github.com/fastfetch-cli/fastfetch/blob/dev/src/detection/disk/disk_linux.c
static bool is_physical_device(const mntent* device)
{
#if !CF_ANDROID  // On Android, `/dev` is not accessible, so that the following checks always fail

    // Always show the root path
    if (strcmp(device->mnt_dir, "/") == 0)
        return true;

    if (strcmp(device->mnt_fsname, "none") == 0)
        return false;

    // DrvFs is a filesystem plugin to WSL that was designed to support interop between WSL and the Windows filesystem.
    if (strcmp(device->mnt_type, "9p") == 0)
        return std::string_view(device->mnt_opts).find("aname=drvfs") != std::string_view::npos;

    // ZFS pool
    if (strcmp(device->mnt_type, "zfs") == 0)
        return true;

    // Pseudo filesystems don't have a device in /dev
    if (!hasStart(device->mnt_fsname, "/dev/"))
        return false;

    // #731
    if (strcmp(device->mnt_type, "bcachefs") == 0)
        return true;

    if (hasStart(device->mnt_fsname + 5, "loop") ||  // Ignore loop devices
        hasStart(device->mnt_fsname + 5, "ram") ||   // Ignore ram devices
        hasStart(device->mnt_fsname + 5, "fd")       // Ignore fd devices
    )
        return false;

    struct stat deviceStat;
    if (stat(device->mnt_fsname, &deviceStat) != 0)
        return false;

    // Ignore all devices that are not block devices
    if (!S_ISBLK(deviceStat.st_mode))
        return false;

#else

    // Pseudo filesystems don't have a device in /dev
    if (!hasStart(device->mnt_fsname, "/dev/"))
        return false;

    if (hasStart(device->mnt_fsname + 5, "loop") ||  // Ignore loop devices
        hasStart(device->mnt_fsname + 5, "ram") ||   // Ignore ram devices
        hasStart(device->mnt_fsname + 5, "fd")       // Ignore fd devices
    )
        return false;

    // https://source.android.com/docs/core/ota/apex?hl=zh-cn
    if (hasStart(device->mnt_dir, "/apex/"))
        return false;

#endif  // !CF_ANDROID

    return true;
}

static bool is_removable(const mntent* device)
{
    if (!hasStart(device->mnt_fsname, "/dev/"))
        return false;

    //                                                                          like device->mnt_fsname.substr(5);
    std::string sys_block_partition{ fmt::format("/sys/class/block/{}", (device->mnt_fsname + "/dev/"_len)) };
    // check if it's like /dev/sda1
    if (sys_block_partition.back() >= '0' && sys_block_partition.back() <= '9')
        sys_block_partition.pop_back();

    return read_by_syspath(sys_block_partition + "/removable") == "1";
}

static int get_disk_type(const mntent* device)
{
#if CF_LINUX
    int ret = 0;

    if (hasStart(device->mnt_dir, "/boot") || hasStart(device->mnt_dir, "/efi"))
        ret = DISK_VOLUME_TYPE_HIDDEN;
    else if (is_removable(device))
        ret = DISK_VOLUME_TYPE_EXTERNAL;
    else
        ret = DISK_VOLUME_TYPE_REGULAR;

    if (hasmntopt(device, MNTOPT_RO))
        ret |= DISK_VOLUME_TYPE_READ_ONLY;

    return ret;
#else  // CF_ANDROID
    if (strcmp(device->mnt_dir, "/") == 0 || strcmp(device->mnt_dir, "/storage/emulated") == 0)
        return DISK_VOLUME_TYPE_REGULAR;

    if (hasStart(device->mnt_dir, "/mnt/media_rw/"))
        return DISK_VOLUME_TYPE_EXTERNAL;

    return DISK_VOLUME_TYPE_HIDDEN;
#endif
}

static struct mntent* get_disk_info(const callbackInfo_t* callbackInfo)
{
    if (callbackInfo->moduleArgs->name != "disk" ||
        (callbackInfo->moduleArgs->name == "disk" && callbackInfo->moduleArgs->value.empty()))
        die("Module disk doesn't have an argmument to the path/device to query");

    const std::string& path = callbackInfo->moduleArgs->value;
    if (access(path.c_str(), F_OK) != 0 || !mountsFile)
        return NULL;

    struct mntent* pDevice;
    while ((pDevice = getmntent(mountsFile)))
    {
        debug("pDevice->mnt_dir = {:<50} && pDevice->mnt_fsname = {}", pDevice->mnt_dir, pDevice->mnt_fsname);
        if (path == pDevice->mnt_dir || path == pDevice->mnt_fsname)
        {
            rewind(mountsFile);
            return pDevice;
        }
    }

    rewind(mountsFile);
    return NULL;
}

static bool get_disk_usage_info(const callbackInfo_t* callbackInfo, struct statvfs* fs)
{
    struct mntent*     pDevice  = get_disk_info(callbackInfo);
    const std::string& path     = callbackInfo->moduleArgs->value;
    const std::string& statpath = (hasStart(path, "/dev") && pDevice) ? pDevice->mnt_dir : path;

    return (statvfs(statpath.c_str(), fs) == 0);
}

// don't get confused by the name pls
MODFUNC(disk_fsname)
{ return get_disk_info(callbackInfo)->mnt_type; }

MODFUNC(disk_device)
{ return get_disk_info(callbackInfo)->mnt_fsname; }

MODFUNC(disk_mountdir)
{ return get_disk_info(callbackInfo)->mnt_dir; }

MODFUNC(disk_types)
{
    const int   types = get_disk_type(get_disk_info(callbackInfo));
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
    struct statvfs fs;
    if (!get_disk_usage_info(callbackInfo, &fs))
        return 0;

    return static_cast<double>(fs.f_blocks * fs.f_frsize);
}

double disk_free(const callbackInfo_t* callbackInfo)
{
    struct statvfs fs;
    if (!get_disk_usage_info(callbackInfo, &fs))
        return 0;

    return static_cast<double>(fs.f_bfree * fs.f_frsize);
}

double disk_used(const callbackInfo_t *callbackInfo)
{ 
    return disk_total(callbackInfo) - disk_free(callbackInfo);
}
