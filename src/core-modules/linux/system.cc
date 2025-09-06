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
#if CF_LINUX

#include <sys/utsname.h>

#include <filesystem>
#include <string>
#include <string_view>

#include "core-modules.hh"
#include "libcufetch/common.hh"
#include "util.hpp"

MODFUNC(host)
{
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    std::string board_name{ UNKNOWN }, board_version{ UNKNOWN }, board_vendor{ UNKNOWN };

    if (std::filesystem::exists(syspath + "/board_name"))
    {
        board_name    = read_by_syspath(syspath + "/board_name");
        board_version = read_by_syspath(syspath + "/board_version");
        board_vendor  = read_by_syspath(syspath + "/board_vendor");

        if (board_vendor == "Micro-Star International Co., Ltd.")
            board_vendor = "MSI";
    }
    else if (std::filesystem::exists(syspath + "/product_name"))
    {
        board_name = read_by_syspath(syspath + "/product_name");

        static constexpr std::string_view standard_pc_name = "Standard PC";
        if (board_name.substr(0, standard_pc_name.size()) == standard_pc_name)
        {
            // everyone does it like "KVM/QEMU Standard PC (...) (host_version)" so why not
            board_vendor  = "KVM/QEMU";
            board_version = std::string_view('(' + read_by_syspath(syspath + "/product_version") + ')').data();
        }
        else
            board_version = read_by_syspath(syspath + "/product_version");
    }

    return board_vendor + " " + board_name + " " + board_version;
}

MODFUNC(host_name)
{
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    if (std::filesystem::exists(syspath + "/board_name"))
        return read_by_syspath(syspath + "/board_name");
    else if (std::filesystem::exists(syspath + "/product_name"))
        return read_by_syspath(syspath + "/product_name");

    return UNKNOWN;
}

MODFUNC(host_version)
{
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    if (std::filesystem::exists(syspath + "/board_name"))
        return read_by_syspath(syspath + "/board_version");
    else if (std::filesystem::exists(syspath + "/product_name"))
        return read_by_syspath(syspath + "/product_version");

    return UNKNOWN;
}

MODFUNC(host_vendor)
{
    const std::string syspath = "/sys/devices/virtual/dmi/id";

    std::string board_vendor{ UNKNOWN };

    if (std::filesystem::exists(syspath + "/board_name"))
    {
        board_vendor = read_by_syspath(syspath + "/board_vendor");
    }
    else if (std::filesystem::exists(syspath + "/product_name"))
    {
        const std::string& board_name = read_by_syspath(syspath + "/product_name");
        if (hasStart(board_name, "Standard PC"))
            board_vendor = "KVM/QEMU";
    }

    return board_vendor;
}

MODFUNC(arch)
{
    return g_uname_infos.machine;
}

#endif
