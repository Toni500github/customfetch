#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

RAM::RAM() {
    debug("Constructing RAM");
    m_memory_infos.free_amount = 10442;
    m_memory_infos.total_amount = 15882;
    m_memory_infos.used_amount = m_memory_infos.total_amount - m_memory_infos.free_amount;
    m_memory_infos.swap_free_amount = 512;
    m_memory_infos.total_amount = 512;
}

float& RAM::free_amount() {
    return m_memory_infos.free_amount;
}

float& RAM::total_amount() {
    return m_memory_infos.total_amount;
}

float& RAM::used_amount() {
    return m_memory_infos.used_amount;
}

float& RAM::swap_free_amount() {
    return m_memory_infos.swap_free_amount;
}

float& RAM::swap_total_amount() {
    return m_memory_infos.swap_total_amount;
}

#endif
