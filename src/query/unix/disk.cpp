#include <mntent.h>
#include <algorithm>
#include <cstdio>
#include <filesystem>

#include "query.hpp"
#include "util.hpp"

using namespace Query;

Disk::Disk(const std::string_view path, std::vector<std::string>& paths)
{
    if (std::find(paths.begin(), paths.end(), path) == paths.end())
        paths.push_back(path.data());
    else
        return;

    if (!std::filesystem::exists(path))
    {
        // if user is using disk.disk or disk.fs
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
        error("setmntent() failed. Could not get disk info");
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

    const std::string_view statpath = (hasStart(path, "/dev") ? pDevice->mnt_dir : path);

    if (statvfs(statpath.data(), &m_statvfs) != 0)
    {
        perror("statvfs");
        error("Failed to get disk info");
        return;
    }

    m_disk_infos.total_amount = static_cast<double>(m_statvfs.f_blocks * m_statvfs.f_frsize);    
    m_disk_infos.free_amount  = static_cast<double>(m_statvfs.f_bfree  * m_statvfs.f_frsize);    
    m_disk_infos.used_amount  = m_disk_infos.total_amount - m_disk_infos.free_amount;

    endmntent(mountsFile);

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
