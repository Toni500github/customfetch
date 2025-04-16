/*
 * Copyright 2024 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Copyright (c) 2021-2023 Linus Dierheimer
 * Copyright (c) 2022-2024 Carter Li
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "platform.hpp"
#if CF_LINUX || CF_ANDROID

#include <mntent.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include <string_view>

#include "config.hpp"
#include "query.hpp"
#include "util.hpp"
#include "parse.hpp"

using namespace Query;

// https://github.com/fastfetch-cli/fastfetch/blob/dev/src/detection/disk/disk_linux.c
static bool is_physical_device(const mntent* device)
{
    #if !CF_ANDROID // On Android, `/dev` is not accessible, so that the following checks always fail

    //Always show the root path
    if (strcmp(device->mnt_dir, "/") == 0)
        return true;

    if (strcmp(device->mnt_fsname, "none") == 0)
        return false;

    //DrvFs is a filesystem plugin to WSL that was designed to support interop between WSL and the Windows filesystem.
    if (strcmp(device->mnt_type, "9p") == 0)
        return std::string_view(device->mnt_opts).find("aname=drvfs") != std::string_view::npos;

    //ZFS pool
    if (strcmp(device->mnt_type, "zfs") == 0)
        return true;

    //Pseudo filesystems don't have a device in /dev
    if (!hasStart(device->mnt_fsname, "/dev/"))
        return false;

    //#731
    if (strcmp(device->mnt_type, "bcachefs") == 0)
        return true;

    if(
        hasStart(device->mnt_fsname + 5, "loop") || //Ignore loop devices
        hasStart(device->mnt_fsname + 5, "ram")  || //Ignore ram devices
        hasStart(device->mnt_fsname + 5, "fd")      //Ignore fd devices
    ) return false;

    struct stat deviceStat;
    if (stat(device->mnt_fsname, &deviceStat) != 0)
        return false;

    //Ignore all devices that are not block devices
    if (!S_ISBLK(deviceStat.st_mode))
        return false;

    #else

    //Pseudo filesystems don't have a device in /dev
    if (!hasStart(device->mnt_fsname, "/dev/"))
        return false;

    if(
        hasStart(device->mnt_fsname + 5, "loop") || //Ignore loop devices
        hasStart(device->mnt_fsname + 5, "ram")  || //Ignore ram devices
        hasStart(device->mnt_fsname + 5, "fd")      //Ignore fd devices
    ) return false;

    // https://source.android.com/docs/core/ota/apex?hl=zh-cn
    if (hasStart(device->mnt_dir, "/apex/"))
        return false;

    #endif // !CF_ANDROID

    return true;
}

static bool is_removable(const mntent* device)
{
    if (!hasStart(device->mnt_fsname, "/dev/"))
        return false;

    //                                                                          like str.substr(5);
    std::string sys_block_partition {fmt::format("/sys/class/block/{}", (device->mnt_fsname + "/dev/"_len))};
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
#else // CF_ANDROID
    if (strcmp(device->mnt_dir, "/") == 0 || strcmp(device->mnt_dir, "/storage/emulated") == 0)
        return DISK_VOLUME_TYPE_REGULAR;

    if (hasStart(device->mnt_dir, "/mnt/media_rw/"))
        return DISK_VOLUME_TYPE_EXTERNAL;

    return DISK_VOLUME_TYPE_HIDDEN;
#endif
}

static std::string format_auto_query_string(std::string str, const struct mntent *device)
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

Disk::Disk(const std::string& path, systemInfo_t& queried_paths, parse_args_t& parse_args, const bool auto_module)
{
    if (queried_paths.find(path) != queried_paths.end() && !is_live_mode)
    {
        m_disk_infos.device       = getInfoFromName(queried_paths, path, "device");
        m_disk_infos.mountdir     = getInfoFromName(queried_paths, path, "mountdir");
        m_disk_infos.typefs       = getInfoFromName(queried_paths, path, "typefs");
        m_disk_infos.total_amount = std::stod(getInfoFromName(queried_paths, path, "total_amount"));
        m_disk_infos.used_amount  = std::stod(getInfoFromName(queried_paths, path, "used_amount"));
        m_disk_infos.free_amount  = std::stod(getInfoFromName(queried_paths, path, "free_amount"));
        return;
    }

    if (access(path.data(), F_OK) != 0 && !auto_module)
    {
        // if user is using $<disk(path)> or $<disk(path).fs>
        // then let's just "try" to remove it
        m_disk_infos.typefs = MAGIC_LINE;
        m_disk_infos.device = MAGIC_LINE;
        m_disk_infos.mountdir = MAGIC_LINE;
        return;
    }

    FILE* mountsFile = setmntent("/proc/mounts", "r");
    if (mountsFile == NULL)
    {
        perror("setmntent");
        error(_("setmntent() failed. Could not get disk info"));
        return;
    }

    if (auto_module)
    {
        struct mntent* pDevice;
        while ((pDevice = getmntent(mountsFile)))
        {
            if (!is_physical_device(pDevice))
                continue;

            m_disk_infos.types_disk = get_disk_type(pDevice);
            if (!(parse_args.config.auto_disks_types & m_disk_infos.types_disk))
                continue;

            if (!parse_args.config.auto_disks_show_dupl)
            {
                const auto& it = std::find(m_queried_devices.begin(), m_queried_devices.end(), pDevice->mnt_fsname);
                if (it != m_queried_devices.end())
                    continue;

                m_queried_devices.push_back(pDevice->mnt_fsname);
            }

            parse_args.no_more_reset = false;
            debug("AUTO: pDevice->mnt_dir = {} && pDevice->mnt_fsname = {}", pDevice->mnt_dir, pDevice->mnt_fsname);
            m_disks_formats.push_back(
                parse(format_auto_query_string(parse_args.config.auto_disks_fmt, pDevice), parse_args)
            );
        }

        endmntent(mountsFile);
        return;
    }

    struct mntent* pDevice;
    while ((pDevice = getmntent(mountsFile)))
    {
        debug("pDevice->mnt_dir = {} && pDevice->mnt_fsname = {}", pDevice->mnt_dir, pDevice->mnt_fsname);
        if (path == pDevice->mnt_dir || path == pDevice->mnt_fsname)
        {
            m_disk_infos.types_disk = get_disk_type(pDevice);
            if (!(parse_args.config.auto_disks_types & m_disk_infos.types_disk))
                continue;

            m_disk_infos.typefs   = pDevice->mnt_type;
            m_disk_infos.device   = pDevice->mnt_fsname;
            m_disk_infos.mountdir = pDevice->mnt_dir;
            break;
        }
    }

    const std::string& statpath = (hasStart(path, "/dev") && pDevice) ? pDevice->mnt_dir : path;

    struct statvfs fs;
    if (statvfs(statpath.c_str(), &fs) != 0)
    {
        perror("statvfs");
        error(_("Failed to get disk info at {}"), statpath);
        return;
    }

    m_disk_infos.total_amount = static_cast<double>(fs.f_blocks * fs.f_frsize);    
    m_disk_infos.free_amount  = static_cast<double>(fs.f_bfree  * fs.f_frsize);    
    m_disk_infos.used_amount  = m_disk_infos.total_amount - m_disk_infos.free_amount;

    endmntent(mountsFile);
    queried_paths.insert(
        {path, {
            {"total_amount", variant(m_disk_infos.total_amount)},
            {"used_amount",  variant(m_disk_infos.used_amount)},
            {"free_amount",  variant(m_disk_infos.free_amount)},
            {"typefs",       variant(m_disk_infos.typefs)},
            {"mountdir",     variant(m_disk_infos.mountdir)},
            {"device",       variant(m_disk_infos.device)}
        }}
    );
}

// clang-format off
double& Disk::total_amount() noexcept
{ return m_disk_infos.total_amount; }

double& Disk::used_amount() noexcept 
{ return m_disk_infos.used_amount; }

double& Disk::free_amount() noexcept
{ return m_disk_infos.free_amount; }

int& Disk::types_disk() noexcept
{ return m_disk_infos.types_disk; }

std::string& Disk::typefs() noexcept
{ return m_disk_infos.typefs; }

std::string& Disk::mountdir() noexcept
{ return m_disk_infos.mountdir; }

std::string& Disk::device() noexcept
{ return m_disk_infos.device; }

#endif // CF_LINUX || CF_ANDROID
