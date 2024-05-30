#include "query.hpp"
#include "util.hpp"

#include <cerrno>
#include <fstream>
#include <algorithm>
#include <pwd.h>
#include <unistd.h>

using namespace Query;

System::System() {
    uid_t uid = geteuid();

    if (uname(&this->uname_infos) != 0)
        die("uname() failed: {}\nCould not get system infos", errno);

    if (sysinfo(&this->sysInfos) != 0)
        die("uname() failed: {}\nCould not get system infos", errno);

    if (this->pwd = getpwuid(uid), !this->pwd)
        die("getpwent failed: {}\nCould not get user infos", errno);
}

std::string System::kernel_name() {
    return this->uname_infos.sysname;
}

std::string System::kernel_version() {
    return this->uname_infos.release;
}

std::string System::hostname() {
    return this->uname_infos.nodename;
}

std::string System::arch() {
    return this->uname_infos.machine;
}

std::string System::username() {
    return this->pwd->pw_name;
}

long System::uptime() {
    return this->sysInfos.uptime;
}

std::string System::os_name() {
    std::string sysName = this->kernel_name(); // Query::System::name()

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
