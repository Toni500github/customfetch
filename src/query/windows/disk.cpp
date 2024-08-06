#include <string>
#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

Disk::Disk(const std::string_view path, std::vector<std::string_view>& paths) {
    debug("Constructing");
    m_disk_infos.typefs = "NTFS";
    m_disk_infos.free_amount = 101.34f;
    m_disk_infos.total_amount = 480.01f;
    m_disk_infos.used_amount = m_disk_infos.total_amount - m_disk_infos.free_amount;
}

std::string& Disk::typefs() {
    return m_disk_infos.typefs;
}

float& Disk::free_amount() {
    return m_disk_infos.free_amount;
}

float& Disk::used_amount() {
    return m_disk_infos.used_amount;
}

float& Disk::total_amount() {
    return m_disk_infos.total_amount;
}

#endif // CF_WINDOWS
