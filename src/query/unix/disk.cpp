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

#include <mntent.h>
#include <cstdio>
#include <filesystem>

#include "query.hpp"
#include "util.hpp"
#include "parse.hpp"

using namespace Query;

Disk::Disk(const std::string& path, systemInfo_t& queried_paths)
{
    if (queried_paths.find(path) != queried_paths.end())
    {
        m_disk_infos.device       = getInfoFromName(queried_paths, path, "device");
        m_disk_infos.mountdir     = getInfoFromName(queried_paths, path, "mountdir");
        m_disk_infos.typefs       = getInfoFromName(queried_paths, path, "typefs");
        m_disk_infos.total_amount = std::stod(getInfoFromName(queried_paths, path, "total_amount"));
        m_disk_infos.used_amount  = std::stod(getInfoFromName(queried_paths, path, "used_amount"));
        m_disk_infos.free_amount  = std::stod(getInfoFromName(queried_paths, path, "free_amount"));
        return;
    }

    if (!std::filesystem::exists(path))
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
        perror(_("setmntent"));
        error(_("setmntent() failed. Could not get disk info"));
        return;
    }

    struct mntent* pDevice;
    while ((pDevice = getmntent(mountsFile)))
    {
        debug("pDevice->mnt_dir = {} && pDevice->mnt_fsname = {}", pDevice->mnt_dir, pDevice->mnt_fsname);
        if (path == pDevice->mnt_dir || path == pDevice->mnt_fsname)
        {
            m_disk_infos.typefs   = pDevice->mnt_type;
            m_disk_infos.device   = pDevice->mnt_fsname;
            m_disk_infos.mountdir = pDevice->mnt_dir;
            break;
        }
    }

    const std::string& statpath = (hasStart(path, "/dev") ? pDevice->mnt_dir : path);

    if (statvfs(statpath.c_str(), &m_statvfs) != 0)
    {
        perror("statvfs");
        error(_("Failed to get disk info"));
        return;
    }

    m_disk_infos.total_amount = static_cast<double>(m_statvfs.f_blocks * m_statvfs.f_frsize);    
    m_disk_infos.free_amount  = static_cast<double>(m_statvfs.f_bfree  * m_statvfs.f_frsize);    
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

std::string& Disk::typefs() noexcept
{ return m_disk_infos.typefs; }

std::string& Disk::mountdir() noexcept
{ return m_disk_infos.mountdir; }

std::string& Disk::device() noexcept
{ return m_disk_infos.device; }
