#include "platform.hpp"
#if CF_MACOS

#include <mach/mach.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include <cstdint>

#include "core-modules.hh"

struct xsw_usage xsw;
const  uint64_t xsw_length{sizeof(xsw)};

static bool populate_xsw()
{
    int name[2] = { CTL_VM, VM_SWAPUSAGE };
    return (sysctl(name, 2, &xsw, &xsw_length, NULL, 0) != 0);
}

double ram_total()
{
    int      name[2] = { CTL_HW, HW_MEMSIZE };
    uint64_t amount  = 0;
    size_t   length  = sizeof(amount);
    if (sysctl(name, 2, &amount, &length, NULL, 0) != 0)
        return 0.0;
    return static_cast<double>(amount) / 1024;
}

double ram_used()
{
    int      name[2] = { CTL_HW, HW_PAGESIZE };
    uint64_t amount  = 0, page_size = 0;
    size_t   length  = sizeof(amount);
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vmstat;

    sysctl(name, 2, &page_size, &length, NULL, 0);    
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)(&vmstat), &count) != KERN_SUCCESS)
        return 0.0;

    return static_cast<double>(
    ((uint64_t)
        + vmstat.active_count
        + vmstat.inactive_count
        + vmstat.speculative_count
        + vmstat.wire_count
        + vmstat.compressor_page_count
        - vmstat.purgeable_count
        - vmstat.external_page_count
    ) * page_size) / 1024;
}

double ram_free()
{ return ram_total() - ram_used(); }

double swap_used()
{
    if (!populate_xsw())
        return 0.0;
    return xsw.xsu_used;
}

double swap_free()
{
    if (!populate_xsw())
        return 0.0;
    return xsw.xsu_avail;
}

double swap_total()
{
    if (!populate_xsw())
        return 0.0;
    return xsw.xsu_total;
}

#endif
