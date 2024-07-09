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

    ret.os_pretty_name = "Windows " + fmt::to_string(osVersion.dwMajorVersion);
    ret.os_name = "Windows";
    ret.os_id = "windows " + fmt::to_string(osVersion.dwMajorVersion);
    ret.os_version_id = fmt::to_string(osVersion.dwBuildNumber);

    return ret;
}

System::System(const Config& config) {
    debug("constructing {}", __func__);

    if (!m_bInit) {
        m_system_infos = get_os_release_vars();

        m_bInit = true;
    }

}

std::string System::arch() {
    return "x86_64";
}

std::string System::os_id() {
    return m_system_infos.os_id;
}

long System::uptime() {
    return GetTickCount64() / 1000;
}

std::string System::os_name() {
    return m_system_infos.os_name;
}

std::string System::hostname() {
    char hostnameBuf[MAX_COMPUTERNAME_LENGTH + 1];
    u_long hostnameCharCount = MAX_COMPUTERNAME_LENGTH + 1;

    if (!GetComputerNameA(hostnameBuf, &hostnameCharCount))
        die("ERROR: GetComputerName failed!");

    return hostnameBuf;
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
    return m_system_infos.os_version_id;
}

std::string System::kernel_version() {
    return m_system_infos.os_version_id;
}

std::string System::os_pretty_name() {
    return m_system_infos.os_pretty_name;
}

std::string System::os_version_codename() {
    return UNKNOWN;
}

#endif
