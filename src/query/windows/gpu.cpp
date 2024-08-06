#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

GPU::GPU(std::uint16_t& id, std::vector<std::uint16_t>& queried_gpus) {
    debug("Constructing GPU");
    m_gpu_infos.name = UNKNOWN;
    m_gpu_infos.vendor = UNKNOWN;
}

std::string& GPU::name() {
    return m_gpu_infos.name;
}

std::string& GPU::vendor() {
    return m_gpu_infos.vendor;
}

#endif
