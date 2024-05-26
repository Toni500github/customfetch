#include "query.hpp"
#include "util.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sys/types.h>

using namespace Query;

std::string_view meminfo_path = "/proc/meminfo";

static size_t read_from_amount(std::string_view str) {
    std::ifstream file(meminfo_path.data());
    if (!file.is_open()) {
        error("Could not open {}", meminfo_path);
        return -1;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(str.data()) != std::string::npos) {
            std::vector<std::string> memfree = split(line, ':');
            strip(memfree[1]);
            
            int32_t ret = std::stoi(memfree[1]);
            return ret / 1000;
        }
    }

    return -1;
}

size_t RAM::free_amount() { return read_from_amount("MemAvailable:"); }

size_t RAM::used_amount() { return read_from_amount("Active:"); }

size_t RAM::total_amount() { return read_from_amount("MemTotal:"); }
