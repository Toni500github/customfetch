#include <fstream>
#include "query.hpp"
#include <algorithm>
#include <unistd.h>

using namespace Query;

System::System() {
    uname(&this->sysInfos);
}

std::string_view System::kernel_name() {
    return this->sysInfos.sysname;
}

std::string_view System::kernel_version() {
    return this->sysInfos.release;
}

std::string_view System::hostname() {
    return this->sysInfos.nodename;
}

std::string_view System::arch() {
    return this->sysInfos.machine;
}

std::string System::OS_pretty_name() {
    std::string_view sysName = this->kernel_name(); // Query::System::name()

    if (sysName == "Linux") {
        std::string os_pretty_name;
        std::string_view os_release_path = "/etc/os-release";
        std::ifstream os_release_file(os_release_path.data());
        if (!os_release_file.is_open()) {
            error("Could not open {}", os_release_path.data());
            return UNKNOWN;
        }

        std::string line;
        while (std::getline(os_release_file, line)) {
            if(line.find("PRETTY_NAME=") != std::string::npos) {
                os_pretty_name = line.substr(12);
                os_pretty_name.erase(std::remove(os_pretty_name.begin(), os_pretty_name.end(), '\"' ), os_pretty_name.end());
                
                return os_pretty_name;
            }
        }

    }

    return UNKNOWN;
}
