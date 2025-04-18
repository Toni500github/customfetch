/*
 * Copyright 2025 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "platform.hpp"
#if CF_MACOS

#include <cstdint>
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include "query.hpp"
#include "util.hpp"

using namespace Query;

// https://github.com/fastfetch-cli/fastfetch/blob/dev/src/detection/memory/memory_apple.c
static RAM::RAM_t get_amount()
{
    RAM::RAM_t ret;
    uint64_t total = 0, used = 0, page_size = 0;
    int name[2] = { CTL_HW, HW_MEMSIZE };
    size_t length = sizeof(total);
    if (sysctl(name, 2, &total, &length, NULL, 0))
        die(_("Failed to get RAM infos"));

    name[0] = CTL_HW;
    name[1] = HW_PAGESIZE;
    sysctl(name, 2, &page_size, &length, NULL, 0);
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vmstat;
    if(host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t) (&vmstat), &count) != KERN_SUCCESS)
        die(_("Failed to read host_statistics64"));

    // https://github.com/exelban/stats/blob/master/Modules/RAM/readers.swift#L56
    used = ((uint64_t)
        + vmstat.active_count
        + vmstat.inactive_count
        + vmstat.speculative_count
        + vmstat.wire_count
        + vmstat.compressor_page_count
        - vmstat.purgeable_count
        - vmstat.external_page_count
    ) * page_size;

    ret.total_amount = static_cast<double>(total) / 1024;
    ret.used_amount = static_cast<double>(used) / 1024;
    // I have just now saw fastfetch doesn't have a way to know free memory
    // why??
    ret.free_amount = ret.total_amount - ret.used_amount;

    struct xsw_usage xsw;
    length = sizeof(xsw);
    name[0] = CTL_VM;
    name[1] = VM_SWAPUSAGE;
    if(sysctl(name, 2, &xsw, &length, NULL, 0) != 0)
        return ret;

    ret.swap_total_amount = static_cast<double>(xsw.xsu_total);
    ret.swap_used_amount = static_cast<double>(xsw.xsu_used);
    ret.swap_free_amount = static_cast<double>(xsw.xsu_avail);

    return ret;
}

RAM::RAM() noexcept
{
    CHECK_INIT(!m_bInit)
    {
        m_memory_infos = get_amount();
        m_bInit        = true;
    }
}

// clang-format off
double& RAM::free_amount() noexcept
{ return m_memory_infos.free_amount; }

double& RAM::total_amount() noexcept
{ return m_memory_infos.total_amount; }

double& RAM::used_amount() noexcept
{ return m_memory_infos.used_amount; }

double& RAM::swap_total_amount() noexcept
{ return m_memory_infos.swap_total_amount; }

double& RAM::swap_used_amount() noexcept
{ return m_memory_infos.swap_used_amount; }

double& RAM::swap_free_amount() noexcept
{ return m_memory_infos.swap_free_amount; }

#endif // CF_MACOS
