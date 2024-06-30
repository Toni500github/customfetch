#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

CPU::CPU() {
    debug("Constructing {}", __func__);
}

std::string CPU::name() {
    return UNKNOWN;
}

std::string CPU::nproc() {
    return "12";
}

float CPU::freq_cur() {
    return 3.41f;
}

float CPU::freq_max() {
    return 4.411f;
}

float CPU::freq_min() {
    return 1.01f;
}

float CPU::freq_bios_limit() {
    return freq_max();
}

#endif

