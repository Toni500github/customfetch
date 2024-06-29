#include "fmt/format.h"
#include "query.hpp"
#include "util.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sys/types.h>

using namespace Query;

enum {
    NAME = 0,
    NPROC,
    FREQ_MAX_CPUINFO,

    FREQ_BIOS_LIMIT = 0,
    FREQ_CUR,
    FREQ_MAX,
    FREQ_MIN
};

static std::string get_from_text(std::string& line) {
    std::string amount = line.substr(line.find(':')+1);
    strip(amount);
    return amount;
}

static std::array<std::string, 3> get_cpu_infos_str() {
    std::array<std::string, 3> ret;
    for (size_t i = 0; i < ret.size(); i++)
            ret.at(i) = UNKNOWN;

    debug("calling in CPU {}", __PRETTY_FUNCTION__);
    constexpr std::string_view cpuinfo_path = "/proc/cpuinfo";
    std::ifstream file(cpuinfo_path.data());
    if (!file.is_open()) {
        error("Could not open {}", cpuinfo_path);
        return ret;
    }

    std::string line;
    float cpu_mhz = -1;
    while (std::getline(file, line)) {
        if (line.find("model name") != std::string::npos)
            ret.at(NAME) = get_from_text(line);
        
        if (line.find("siblings") != std::string::npos)
            ret.at(NPROC) = get_from_text(line);

        if (line.find("cpu MHz") != std::string::npos) {
            float tmp = std::stof(get_from_text(line));
            if (tmp > cpu_mhz)
                cpu_mhz = tmp;
        }
    }
    
    // sometimes /proc/cpuinfo at model name
    // the name will contain the min freq
    // happens on intel cpus especially
    size_t pos = 0;
    if ((pos = ret[NAME].find('@')) != std::string::npos)
        ret[NAME].erase(pos-1);

    cpu_mhz /= 1000;
    ret.at(FREQ_MAX_CPUINFO) = std::to_string(cpu_mhz);

    return ret;
}

static std::array<float, 4> get_cpu_infos_t() {
    debug("calling in CPU {}", __PRETTY_FUNCTION__);
    std::array<float, 4> ret;
    for (size_t i = 0; i < ret.size(); i++)
            ret.at(i) = -1;

    constexpr std::string_view freq_dir = "/sys/devices/system/cpu/cpu0/cpufreq";
    
    if (std::filesystem::exists(freq_dir)) {
        std::ifstream cpu_bios_limit_f(fmt::format("{}/bios_limit", freq_dir));
        std::ifstream cpu_scaling_cur_f(fmt::format("{}/scaling_cur_freq", freq_dir));
        std::ifstream cpu_scaling_max_f(fmt::format("{}/scaling_max_freq", freq_dir));
        std::ifstream cpu_scaling_min_f(fmt::format("{}/scaling_min_freq", freq_dir));

        std::string freq_bios_limit, freq_cpu_scaling_cur, freq_cpu_scaling_max, freq_cpu_scaling_min;
        
        std::getline(cpu_bios_limit_f, freq_bios_limit);
        std::getline(cpu_scaling_cur_f, freq_cpu_scaling_cur);
        std::getline(cpu_scaling_max_f, freq_cpu_scaling_max);
        std::getline(cpu_scaling_min_f, freq_cpu_scaling_min);

        ret.at(FREQ_BIOS_LIMIT) = freq_bios_limit.empty() ? 0 : (std::stof(freq_bios_limit)      / 1000000);
        ret.at(FREQ_CUR) = freq_cpu_scaling_cur.empty() ? 0   : (std::stof(freq_cpu_scaling_cur) / 1000000);
        ret.at(FREQ_MAX) = freq_cpu_scaling_max.empty() ? 0   : (std::stof(freq_cpu_scaling_max) / 1000000);
        ret.at(FREQ_MIN) = freq_cpu_scaling_min.empty() ? 0   : (std::stof(freq_cpu_scaling_min) / 1000000);
    }

    return ret;
}

CPU::CPU() {
    debug("Constructing {}", __func__);
    if (!m_bInit) {
        m_cpu_infos_str = get_cpu_infos_str();
        m_cpu_infos_t = get_cpu_infos_t();
    }
}

std::string CPU::name() {
    return m_cpu_infos_str.at(NAME);
}

std::string CPU::nproc() {
    return m_cpu_infos_str.at(NPROC);
}

float CPU::freq_bios_limit() {
    return m_cpu_infos_t.at(FREQ_BIOS_LIMIT);
}

float CPU::freq_cur() {
    return m_cpu_infos_t.at(FREQ_CUR);
}

float CPU::freq_max() {
    return (m_cpu_infos_t.at(FREQ_MAX) < 0) ? std::stof(m_cpu_infos_str.at(FREQ_MAX_CPUINFO)) : m_cpu_infos_t.at(FREQ_MAX);
}

float CPU::freq_min() {
    return m_cpu_infos_t.at(FREQ_MIN);
}
