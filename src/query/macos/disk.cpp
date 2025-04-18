/*
 * Copyright 2025 Toni500git
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

#include "platform.hpp"
#if CF_MACOS

#include <cstring>
#include <sys/types.h>

#include "query.hpp"
#include "util.hpp"

using namespace Query;

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

Disk::Disk(const std::string& path, systemInfo_t& queried_paths, parse_args_t& parse_args,
         const bool auto_module)
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

    if (auto_module)
    {
        const int size = getfsstat(NULL, 0, MNT_WAIT);
        if (size <= 0) 
            die(_("Failed to get Disk infos"));

        struct statfs* buf = reinterpret_cast<struct statfs*>(malloc(sizeof(*buf) * (unsigned)size));
        if (getfsstat(buf, (int) (sizeof(*buf) * (unsigned) size), MNT_NOWAIT) <= 0)
            die(_("Failed to get Disk infos"));

        for (struct statfs* fs = buf; fs < buf + size; ++fs)
        {
            if (strcmp(fs->f_mntonname, "/") != 0 && !hasStart(fs->f_mntfromname, "/dev/"))
                continue;

            m_disk_infos.types_disk = get_disk_type(fs->f_flags);
            if (!(parse_args.config.auto_disks_types & m_disk_infos.types_disk))
                continue;

            if (!parse_args.config.auto_disks_show_dupl)
            {
                const auto& it = std::find(m_queried_devices.begin(), m_queried_devices.end(), fs->f_mntfromname);
                if (it != m_queried_devices.end())
                    continue;

                m_queried_devices.push_back(fs->f_mntfromname);
            }

            parse_args.no_more_reset = false;
            m_disks_formats.push_back(
                parse(format_auto_query_string(parse_args.config.auto_disks_fmt, fs), parse_args)
            );
        }

        free(buf);
        return;
    }

    struct statfs fs;
    if (statfs(path.c_str(), &fs) != 0)
    {
        perror("statvfs");
        error(_("Failed to get disk info at {}"), path);
        return;
    }

    if (path != fs.f_mntonname && path != fs.f_mntfromname)
        return;

    m_disk_infos.typefs       = fs.f_fstypename;
    m_disk_infos.device       = fs.f_mntfromname;
    m_disk_infos.mountdir     = fs.f_mntonname;

    m_disk_infos.total_amount = static_cast<double>(fs.f_blocks * fs.f_bsize);    
    m_disk_infos.free_amount  = static_cast<double>(fs.f_bfree  * fs.f_bsize);    
    m_disk_infos.used_amount  = m_disk_infos.total_amount - m_disk_infos.free_amount;

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

#endif // CF_MACOS
