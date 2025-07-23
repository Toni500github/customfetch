#include "platform.hpp"
#if CF_MACOS

#include <sys/sysctl.h>
#include <unistd.h>

#include <cstdint>
#include <ratio>
#include <string>

#include "core-modules.hh"

static bool get_sysctl(int name[2], void* ret, size_t* oldlenp)
{
    return (sysctl(name, 2, ret, oldlenp, NULL, 0) == 0);
}

static bool get_sysctl(const char* name, void* ret, size_t* oldlenp)
{
    return (sysctlbyname(name, ret, oldlenp, NULL, 0) == 0);
}

float cpu_temp() { return 0; }

MODFUNC(cpu_name)
{
    char   buf[1024];
    size_t len = sizeof(buf);
    get_sysctl("machdep.cpu.brand_string", &buf, &len);
    return buf;
}

MODFUNC(cpu_nproc)
{
    char   buf[1024];
    size_t len = sizeof(buf);
    if (!get_sysctl("hw.logicalcpu_max", &buf, &len))
        get_sysctl("hw.ncpu", &buf, &len);
    return buf;
}

MODFUNC(cpu_freq_cur)
{
    std::uint64_t freq   = 0;
    size_t        length = sizeof(freq);
    if (!get_sysctl("hw.cpufrequency", &freq, &length))
        get_sysctl((int[2]){ CTL_HW, HW_CPU_FREQ }, &freq, &length);

    return fmt::to_string(static_cast<double>(freq) / std::giga().num);
}

MODFUNC(cpu_freq_min)
{
    std::uint64_t freq   = 0;
    size_t        length = sizeof(freq);
    get_sysctl("hw.cpufrequency_min", &freq, &length);

    return fmt::to_string(static_cast<double>(freq) / std::giga().num);
}

MODFUNC(cpu_freq_max)
{
    std::uint64_t freq   = 0;
    size_t        length = sizeof(freq);
    get_sysctl("hw.cpufrequency_max", &freq, &length);

    return fmt::to_string(static_cast<double>(freq) / std::giga().num);
}

MODFUNC(cpu_freq_bios) { return MAGIC_LINE; }

#endif
