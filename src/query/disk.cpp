#include <cstdio>
#include "query.hpp"
#include "util.hpp"

using namespace Query;

Disk::Disk(const std::string_view path) {
    if (!m_bInit) {
        if (statvfs(path.data(), &m_statvfs) != 0) {
            error("stat() failed: ");
            perror("");
            error("Failed to get disk info");
            memset(&m_statvfs, 0, sizeof(struct statvfs));
        }
        
        FILE *mountsFile = setmntent("/proc/mounts", "r");
        if (mountsFile == NULL)
            error("setmntent() failed. Could not get disk info");
        
        struct mntent *pDevice;
        while((pDevice = getmntent(mountsFile)))
        {
            debug("pDevice->mnt_dir = {}", pDevice->mnt_dir);
            if (path != pDevice->mnt_dir)
                continue;

            m_typefs = pDevice->mnt_type;
            break;
        }

        m_bInit = true;
    }
}

float Disk::total_amount() {
    return ((static_cast<float>(m_statvfs.f_blocks * m_statvfs.f_frsize) / 1024) / 1024) / 1024;
}

float Disk::used_amount() {
    return ((static_cast<float>(
        (m_statvfs.f_blocks * m_statvfs.f_frsize) - 
        (m_statvfs.f_bavail * m_statvfs.f_frsize)
        ) / 1024) / 1024) / 1024;
}

float Disk::free_amount() {
    return ((static_cast<float>(m_statvfs.f_bfree * m_statvfs.f_frsize) / 1024) / 1024) / 1024;
}

std::string Disk::typefs() {
    return m_typefs;
}
