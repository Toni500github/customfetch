#include <string>
#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

System::System() {
    debug("constructing {}", __func__);
}

std::string System::arch() {
    return "x86_64";
}

std::string System::os_id() {
    return "23H04";
}

long System::uptime() {
    return 11243734;
}

std::string System::os_name() {
    return "Windows";
}

std::string System::hostname() {
    return "hostname";
}

std::string System::host_vendor() {
    return "BIOS vendor";
}

std::string System::kernel_name() {
    return "Windows NT";
}

std::string System::host_version() {
    return "1.0";
}

std::string System::host_modelname() {
    return UNKNOWN;
}

std::string System::os_versionid() {
    return UNKNOWN;
}

std::string System::kernel_version() {
    return UNKNOWN;
}

std::string System::os_pretty_name() {
    return "Windows 11 (bloatware)";
}

std::string System::os_version_codename() {
    return UNKNOWN;
}

#endif
