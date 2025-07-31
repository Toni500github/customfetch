/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "platform.hpp"
#if CF_MACOS

#include <mach/mach.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include <cstdint>

#include "core-modules.hh"

xsw_usage xsw;
size_t    xsw_length{ sizeof(xsw) };

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
    return static_cast<double>(amount);
}

double ram_used()
{
    int                    name[2] = { CTL_HW, HW_PAGESIZE };
    uint64_t               amount = 0, page_size = 0;
    size_t                 length = sizeof(amount);
    mach_msg_type_number_t count  = HOST_VM_INFO64_COUNT;
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
    ) * page_size);
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
