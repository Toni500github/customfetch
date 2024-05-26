#include "query.hpp"
#include "util.hpp"

#include <fstream>
#include <sys/types.h>

using namespace Query;

std::string CPU::name() {
    std::string_view cpuinfo_path = "/proc/cpuinfo";
    std::ifstream file(cpuinfo_path.data());
    if (!file.is_open()) {
        error("Could not open {}", cpuinfo_path);
        return UNKNOWN;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("model name") != std::string::npos) {
            std::vector<std::string> _model = split(line, ':');
            strip(_model[1]);
            return _model[1];
        }
    }

    return UNKNOWN;
}
