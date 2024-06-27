#include "query.hpp"
#include "util.hpp"

#include <array>
#include <cerrno>
#include <algorithm>
#include <unistd.h>

using namespace Query;

enum {
    PRETTY_NAME = 0,
    NAME,
    ID_LIKE,
    ID,
    BUILD_ID,
    VERSION_ID,
    _VERSION, // conflicts with the macro VERSION so had to put _
    VERSION_CODENAME
};

static std::string get_var(std::string& line, u_short& iter_index) {
    std::string ret = line.substr(line.find('=')+1);
    ret.erase(std::remove(ret.begin(), ret.end(), '\"'), ret.end());
    ++iter_index;
    return ret;
}

static std::array<std::string, 4> get_os_release_vars() {
    std::array<std::string, 4> ret;
    for (size_t i = 0; i < ret.size(); i++)
            ret.at(i) = UNKNOWN;

    std::string_view os_release_path = "/etc/os-release";
    std::ifstream os_release_file(os_release_path.data());
    if (!os_release_file.is_open()) {
        error("Could not open {}", os_release_path);
        return ret;
    }
    
    u_short iter_index = 0;
    std::string line;
    while (std::getline(os_release_file, line) && iter_index < 3) {
        if(hasStart(line, "PRETTY_NAME="))
            ret.at(PRETTY_NAME) = get_var(line, iter_index);

        if(hasStart(line, "NAME="))
            ret.at(NAME) = get_var(line, iter_index);

        if(hasStart(line, "ID="))
            ret.at(ID) = get_var(line, iter_index);
    }

    return ret;
}

System::System() {

    if (uname(&m_uname_infos) != 0)
        die("uname() failed: {}\nCould not get system infos", errno);

    if (sysinfo(&m_sysInfos) != 0)
        die("uname() failed: {}\nCould not get system infos", errno);

    m_os_release_vars = get_os_release_vars();
}

std::string System::kernel_name() {
    return m_uname_infos.sysname;
}

std::string System::kernel_version() {
    return m_uname_infos.release;
}

std::string System::hostname() {
    return m_uname_infos.nodename;
}

std::string System::arch() {
    return m_uname_infos.machine;
}

long System::uptime() {
    return m_sysInfos.uptime;
}

std::string System::os_pretty_name() {
    return m_os_release_vars.at(PRETTY_NAME);
}

std::string System::os_name() {
    return m_os_release_vars.at(NAME);
}

std::string System::os_id() {
    return m_os_release_vars.at(ID);
}

std::string System::host_modelname() {
    return read_by_syspath("/sys/devices/virtual/dmi/id/board_name");
}

std::string System::host_vendor() {
    return read_by_syspath("/sys/devices/virtual/dmi/id/board_vendor");
}

std::string System::host_version() {
    return read_by_syspath("/sys/devices/virtual/dmi/id/board_version");
}
