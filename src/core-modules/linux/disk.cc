#include "platform.hpp"
#if CF_LINUX || CF_ANDROID

#include <mntent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include <cstdio>

#include "cufetch/common.hh"
#include "core-modules.hh"
#include "cufetch/config.hh"
#include "switch_fnv1a.hpp"
#include "util.hpp"

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

static std::string format_auto_query_string(std::string str, const struct mntent* device)
{
    replace_str(str, "%1", device->mnt_dir);
    replace_str(str, "%2", device->mnt_fsname);
    replace_str(str, "%3", device->mnt_type);

    replace_str(str, "%4", fmt::format("$<disk({}).total>", device->mnt_dir));
    replace_str(str, "%5", fmt::format("$<disk({}).free>", device->mnt_dir));
    replace_str(str, "%6", fmt::format("$<disk({}).used>", device->mnt_dir));
    replace_str(str, "%7", fmt::format("$<disk({}).used_perc>", device->mnt_dir));
    replace_str(str, "%8", fmt::format("$<disk({}).free_perc>", device->mnt_dir));

    return str;
}

static struct mntent* get_disk_info(const callbackInfo_t* callbackInfo)
{
    if (callbackInfo->moduleArgs->name != "disk" ||
        (callbackInfo->moduleArgs->name == "disk" && callbackInfo->moduleArgs->value.empty()))
        die("Module disk doesn't have an argmument to the path/device to query");

    const std::string& path = callbackInfo->moduleArgs->value;
    if (access(path.c_str(), F_OK) != 0 || !mountsFile)
        die("Failed to query disk at path: '{}", path);

    struct mntent* pDevice;
    while ((pDevice = getmntent(mountsFile)))
    {
        debug("pDevice->mnt_dir = {:<50} && pDevice->mnt_fsname = {}", pDevice->mnt_dir, pDevice->mnt_fsname);
        if (path == pDevice->mnt_dir || path == pDevice->mnt_fsname)
            break;
    }

    rewind(mountsFile);
    return pDevice;
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

MODFUNC(auto_disk)
{
    static std::vector<std::string> queried_devices;
    const ConfigBase& config = callbackInfo->parse_args.config;
    const std::string& auto_disks_fmt = config.getValue<std::string>("auto.disk.fmt", "${auto}Disk (%1): $<disk(%1)>");
    int auto_disks_types = 0;
    for (const std::string& str : config.getValueArrayStr("auto.disk.display-types", {"external", "regular", "read-only"}))
    {
        switch (fnv1a16::hash(str))
        {
            case "removable"_fnv1a16: // deprecated
            case "external"_fnv1a16:
                auto_disks_types |= DISK_VOLUME_TYPE_EXTERNAL; break;
            case "regular"_fnv1a16:
                auto_disks_types |= DISK_VOLUME_TYPE_REGULAR; break;
            case "read-only"_fnv1a16:
                auto_disks_types |= DISK_VOLUME_TYPE_READ_ONLY; break;
            case "hidden"_fnv1a16:
                auto_disks_types |= DISK_VOLUME_TYPE_HIDDEN; break;
        }
    }

    long old_position = ftell(mountsFile);
    if (old_position == -1L)
        die("Failed to get initial file position");

    struct mntent* pDevice;
    while ((pDevice = getmntent(mountsFile)))
    {
        if (!is_physical_device(pDevice))
            continue;

        if (!(auto_disks_types & get_disk_type(pDevice)))
            continue;

        old_position = ftell(mountsFile);
        if (old_position == -1L)
            break;

        debug("AUTO: pDevice->mnt_dir = {} && pDevice->mnt_fsname = {}", pDevice->mnt_dir, pDevice->mnt_fsname);
        callbackInfo->parse_args.no_more_reset = false;
        callbackInfo->parse_args.tmp_layout.push_back(parse(format_auto_query_string(auto_disks_fmt, pDevice), callbackInfo->parse_args));
        if (fseek(mountsFile, old_position, SEEK_SET) == -1)
            die("Failed to seek back to saved position");
    }
    return "";
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

#endif
