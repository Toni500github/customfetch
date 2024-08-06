#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

CPU::CPU() {
    debug("Constructing {}", __func__);
    m_cpu_infos.name = UNKNOWN;
    m_cpu_infos.nproc = "12";
    m_cpu_infos.freq_cur = 3.41f;
    m_cpu_infos.freq_max = 4.411f;
    m_cpu_infos.freq_min = 1.01f;
    m_cpu_infos.freq_bios_limit = 4.411f;
}

std::string& CPU::name() {
    return m_cpu_infos.name;
}

std::string& CPU::nproc() {
    return m_cpu_infos.nproc;
}

float& CPU::freq_cur() {
    return m_cpu_infos.freq_cur;
}

float& CPU::freq_max() {
    return m_cpu_infos.freq_max;
}

float& CPU::freq_min() {
    return m_cpu_infos.freq_min;
}

float& CPU::freq_bios_limit() {
    return m_cpu_infos.freq_max;
}

#endif

