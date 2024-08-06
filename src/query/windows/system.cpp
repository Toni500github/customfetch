#include <string>
#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

/* https://stackoverflow.com/questions/36543301/detecting-windows-10-version */

// Fuck windows.

typedef LONG NTSTATUS, *PNTSTATUS;
#define STATUS_SUCCESS (0x00000000)

typedef NTSTATUS (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

RTL_OSVERSIONINFOW GetRealOSVersion() {
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr != nullptr) {
            RTL_OSVERSIONINFOW rovi = { 0 };
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if ( STATUS_SUCCESS == fxPtr(&rovi) ) {
                return rovi;
            }
        }
    }
    RTL_OSVERSIONINFOW rovi = { 0 };
    return rovi;
}

static System::System_t get_os_release_vars() {
    System::System_t ret;

    RTL_OSVERSIONINFOW osVersion = GetRealOSVersion();

    // Windows 11 detection, Microsoft is bad.
    if (osVersion.dwBuildNumber >= 22000) {
        osVersion.dwMajorVersion = 11;
    }

    ret.os_pretty_name = "Windows " + fmt::to_string(osVersion.dwMajorVersion);
    ret.os_name = "Windows";
    ret.os_id = "windows " + fmt::to_string(osVersion.dwMajorVersion);
    ret.os_version_id = fmt::to_string(osVersion.dwBuildNumber);

    return ret;
}

System::System() {
    debug("constructing {}", __func__);

    if (!m_bInit) {
        m_system_infos = get_os_release_vars();

        m_bInit = true;
    }

}

std::string& System::arch() {
    m_system_infos.arch = "x86_64";
    return m_system_infos.arch;
}

std::string& System::os_id() {
    return m_system_infos.os_id;
}

long& System::uptime() {
    m_system_infos.uptime = GetTickCount64() / 1000;
    return m_system_infos.uptime;
}

std::string& System::os_name() {
    return m_system_infos.os_name;
}

std::string& System::hostname() {
    char hostnameBuf[MAX_COMPUTERNAME_LENGTH + 1];
    u_long hostnameCharCount = MAX_COMPUTERNAME_LENGTH + 1;

    if (!GetComputerNameA(hostnameBuf, &hostnameCharCount))
        die("ERROR: GetComputerName failed!");

    m_system_infos.hostname = hostnameBuf;

    return m_system_infos.hostname;
}

std::string& System::host_vendor() {
    m_system_infos.host_vendor = "BIOS vendor";
    return m_system_infos.host_vendor;
}

std::string& System::kernel_name() {
    m_system_infos.kernel_name = "Windows NT";
    return m_system_infos.kernel_name;
}

std::string& System::host_version() {
    m_system_infos.host_version = "1.0";
    return m_system_infos.host_version;
}

std::string& System::host_modelname() {
    m_system_infos.host_modelname = UNKNOWN;
    return m_system_infos.host_modelname;
}

std::string& System::os_versionid() {
    return m_system_infos.os_version_id;
}

std::string& System::kernel_version() {
    return m_system_infos.os_version_id;
}

std::string& System::os_pretty_name() {
    return m_system_infos.os_pretty_name;
}

std::string& System::os_version_codename() {
    m_system_infos.os_version_codename = UNKNOWN;
    return m_system_infos.os_version_codename;
}

std::string& System::os_initsys_name() {
    m_system_infos.os_initsys_name = MAGIC_LINE;
    return m_system_infos.os_initsys_name;
}

std::string& System::pkgs_installed(const Config& config) {
    m_system_infos.pkgs_installed = MAGIC_LINE;
    return m_system_infos.pkgs_installed;
}

#endif
