#include "query.hpp"
#include "util.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sys/types.h>

using namespace Query;

enum {
    USED = 0,
    FREE,
    TOTAL
};

std::string_view meminfo_path = "/proc/meminfo";

static std::array<size_t, 5> get_amount() {
    std::array<size_t, 5> memory_infos;
    std::ifstream file(meminfo_path.data());
    if (!file.is_open()) {
        error("Could not open {}", meminfo_path);
        return {0};
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("MemFree:") != std::string::npos) {
            std::vector<std::string> memfree = split(line, ':');
            strip(memfree[1]);
            
            int32_t ret = std::stoi(memfree[1]);
            memory_infos[FREE] = ret / 1024;
        }
        
        if (line.find("Active:") != std::string::npos) {
            std::vector<std::string> memused = split(line, ':');
            strip(memused[1]);
            
            int32_t ret = std::stoi(memused[1]);
            memory_infos[USED] = ret / 1024;
        }

        if (line.find("MemTotal:") != std::string::npos) {
            std::vector<std::string> memtot = split(line, ':');
            strip(memtot[1]);
            
            int32_t ret = std::stoi(memtot[1]);
            memory_infos[TOTAL] = ret / 1024;
        }

    }

    return memory_infos;
}

std::array<size_t, 5> memory_infos = get_amount();

size_t RAM::free_amount()  { return memory_infos[FREE]; }

size_t RAM::used_amount()  { return memory_infos[USED]; }

size_t RAM::total_amount() { return memory_infos[TOTAL]; }
