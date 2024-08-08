#include <algorithm>
#include <cstdio>

#include "query.hpp"
#include "util.hpp"

using namespace Query;

Disk::Disk(const std::string_view path, std::vector<std::string_view>& paths)
{
    debug("Constructing {}", __func__);

    if (std::find(paths.begin(), paths.end(), path) == paths.end())
        paths.push_back(path);
    else
        return;

    if (statvfs(path.data(), &m_statvfs) != 0)
    {
        error("stat() failed: ");
        perror("");
        error("Failed to get disk info");
        memset(&m_statvfs, 0, sizeof(struct statvfs));
    }

    m_disk_infos.total_amount = static_cast<float>(m_statvfs.f_blocks * m_statvfs.f_frsize);
    m_disk_infos.used_amount  =
        static_cast<float>((m_statvfs.f_blocks * m_statvfs.f_frsize) - 
                           (m_statvfs.f_bavail * m_statvfs.f_frsize));
    
    m_disk_infos.free_amount = static_cast<float>(m_statvfs.f_bfree * m_statvfs.f_frsize);

    FILE* mountsFile = setmntent("/proc/mounts", "r");
    if (mountsFile == NULL)
        die("setmntent() failed. Could not get disk info");

    struct mntent* pDevice;
    while ((pDevice = getmntent(mountsFile)))
    {
        debug("pDevice->mnt_dir = {}", pDevice->mnt_dir);
        if (path == pDevice->mnt_dir)
        {
            m_disk_infos.typefs = pDevice->mnt_type;
            break;
        }
    }

    if (mountsFile)
        fclose(mountsFile);
}

// clang-format off
float& Disk::total_amount()
{ return m_disk_infos.total_amount; }

float& Disk::used_amount()
{ return m_disk_infos.used_amount; }

float& Disk::free_amount()
{ return m_disk_infos.free_amount; }

std::string& Disk::typefs()
{ return m_disk_infos.typefs; }
