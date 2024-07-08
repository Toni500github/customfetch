#include "query.hpp"
#include "util.hpp"

#include <array>
#include <cerrno>
#include <algorithm>
#include <filesystem>
#include <unistd.h>

using namespace Query;

enum {
    PRETTY_NAME = 0,
    NAME,
    ID,
    VERSION_ID,
    VERSION_CODENAME,
    
    INIT, // init system
    //_VERSION, // conflicts with the macro VERSION so had to put _
};

struct host_paths {
    std::string name;
    std::string version;
    std::string vendor;
} host;

static std::string get_var(std::string& line, u_short& iter_index) {
    std::string ret = line.substr(line.find('=')+1);
    ret.erase(std::remove(ret.begin(), ret.end(), '\"'), ret.end());
    ++iter_index;
    return ret;
}

static void get_host_paths(struct host_paths& paths) {
    std::string syspath = "/sys/devices/virtual/dmi/id/";

    if (std::filesystem::exists(syspath + "/board_name")) {
        paths.name = read_by_syspath(syspath + "/board_name");
        paths.version = read_by_syspath(syspath + "/board_version");
        paths.vendor = read_by_syspath(syspath + "/board_vendor");
    }

    else if (std::filesystem::exists(syspath + "/product_name")) {
        paths.name = read_by_syspath(syspath + "/product_name");
        if (hasStart(paths.name, "Standard PC"))
            paths.vendor = "KVM/QEMU";

        paths.version = read_by_syspath(syspath + "/product_version");
    }
}

static std::array<std::string, 7> get_os_release_vars() {
    std::array<std::string, 7> ret;
    std::fill(ret.begin(), ret.end(), UNKNOWN);

    debug("calling {}", __PRETTY_FUNCTION__);
    std::string_view os_release_path = "/etc/os-release";
    std::ifstream os_release_file(os_release_path.data());
    if (!os_release_file.is_open()) {
        error("Could not open {}\nFailed to get OS infos", os_release_path);
        return ret;
    }
    
    static u_short iter_index = 0;
    std::string line;
    while (std::getline(os_release_file, line) && iter_index < 5) {
        if (hasStart(line, "PRETTY_NAME="))
            ret.at(PRETTY_NAME) = get_var(line, iter_index);

        if (hasStart(line, "NAME="))
            ret.at(NAME) = get_var(line, iter_index);

        if (hasStart(line, "ID="))
            ret.at(ID) = get_var(line, iter_index);
        
        if (hasStart(line, "VERSION_ID="))
            ret.at(VERSION_ID) = get_var(line, iter_index);

        if (hasStart(line, "VERSION_CODENAME="))
            ret.at(VERSION_CODENAME) = get_var(line, iter_index);
    }

    std::ifstream f_initsys("/proc/1/cmdline", std::ios::binary);
    std::string initsys;
    std::getline(f_initsys, initsys);
    size_t pos = 0;
    if ((pos = initsys.find('\0')) != std::string::npos)
        initsys.erase(pos);
        
    if ((pos = initsys.rfind('/')) != std::string::npos)
        initsys.erase(0, pos+1);

    
    ret.at(INIT) = initsys;

    return ret;
}

System::System() {
    debug("Constructing {}", __func__);
    
    if (!m_bInit) {
        if (uname(&m_uname_infos) != 0)
            die("uname() failed: {}\nCould not get system infos", errno);

        if (sysinfo(&m_sysInfos) != 0)
            die("uname() failed: {}\nCould not get system infos", errno);

        m_os_infos = get_os_release_vars();
        get_host_paths(host);
        m_bInit = true;
    }
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
    return m_os_infos.at(PRETTY_NAME);
}

std::string System::os_name() {
    return m_os_infos.at(NAME);
}

std::string System::os_id() {
    return m_os_infos.at(ID);
}

std::string System::os_versionid() {
    return m_os_infos.at(VERSION_ID);
}

std::string System::os_version_codename() {
    return m_os_infos.at(VERSION_CODENAME);
}

std::string System::os_initsys_name() {
    return m_os_infos.at(INIT);
}

std::string System::host_modelname() {
    return host.name;
}

std::string System::host_vendor() {
    return host.vendor;
}

std::string System::host_version() {
    return host.version;
}
