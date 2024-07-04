#include <string>
#include "query.hpp"
#include "util.hpp"

#ifdef CF_WINDOWS

using namespace Query;

enum {
    PRETTY_NAME = 0,
    NAME,
    ID,
    VERSION_ID,
    VERSION_CODENAME,
    ID_LIKE,
    BUILD_ID,
    _VERSION, // conflicts with the macro VERSION so had to put _
};

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

static std::array<std::string, 5> get_os_release_vars() {
    std::array<std::string, 5> ret;
    std::fill(ret.begin(), ret.end(), UNKNOWN);

    RTL_OSVERSIONINFOW osVersion = GetRealOSVersion();

    ret.at(PRETTY_NAME) = "Windows " + std::to_string(osVersion.dwMajorVersion);
    ret.at(NAME) = "Windows";
    ret.at(ID) = "windows " + std::to_string(osVersion.dwMajorVersion);
    ret.at(VERSION_ID) = std::to_string(osVersion.dwBuildNumber);

    return ret;
}

System::System() {
    debug("constructing {}", __func__);

    if (!m_bInit) {
        m_os_release_vars = get_os_release_vars();

        m_bInit = true;
    }
}

std::string System::arch() {
    return "x86_64";
}

std::string System::os_id() {
    return m_os_release_vars.at(ID);
}

long System::uptime() {
    return GetTickCount64() / 1000;
}

std::string System::os_name() {
    return m_os_release_vars.at(NAME);
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
    return m_os_release_vars.at(VERSION_ID);
}

std::string System::kernel_version() {
    return m_os_release_vars.at(VERSION_ID);
}

std::string System::os_pretty_name() {
    return m_os_release_vars.at(PRETTY_NAME);
}

std::string System::os_version_codename() {
    return UNKNOWN;
}

#endif
