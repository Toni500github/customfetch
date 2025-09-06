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

#include <cstdint>
#include <filesystem>
#include <string>

#include "core-modules.hh"
#include "fmt/format.h"
#include "util.hpp"

static std::string get_name(const std::string_view m_vendor_id_s, const std::string_view m_device_id_s)
{
    const std::string& name = binarySearchPCIArray(m_vendor_id_s, m_device_id_s);
    debug("GPU binarySearchPCIArray name = {}", name);
    const size_t first_bracket = name.find('[');
    const size_t last_bracket  = name.rfind(']');

    // remove the chips name "TU106 [GeForce GTX 1650]"
    // This should work for AMD and Intel too.
    if (first_bracket != std::string::npos && last_bracket != std::string::npos)
        return name.substr(first_bracket + 1, last_bracket - first_bracket - 1);

    return name;
}

static std::string get_vendor(const std::string_view m_vendor_id_s)
{ return binarySearchPCIArray(m_vendor_id_s); }

static std::string get_gpu_syspath(const std::string& id)
{
    const std::uint16_t max_iter = 10;
    std::uint16_t       id_iter  = std::stoi(id);
    std::string         sys_path;
    int                 i = 0;
    for (; i <= max_iter; i++)
    {
        sys_path = "/sys/class/drm/card" + fmt::to_string(id_iter);
        if (std::filesystem::exists(sys_path + "/device/device") &&
            std::filesystem::exists(sys_path + "/device/vendor"))
            return sys_path;
        else
            id_iter++;
    }

    error(_("Failed to parse GPU infos on the path /sys/class/drm/"));
    return UNKNOWN;
}

MODFUNC(gpu_name)
{
    const std::string& id = (callbackInfo && callbackInfo->moduleArgs->name.length() > 3)
        ? callbackInfo->moduleArgs->name.substr(3)
        : "0";
    const std::string& sys_path = get_gpu_syspath(id);
    return get_name(read_by_syspath(sys_path + "/device/vendor"), read_by_syspath(sys_path + "/device/device"));
}

MODFUNC(gpu_vendor)
{
    const std::string& id = (callbackInfo && callbackInfo->moduleArgs->name.length() > 3)
        ? callbackInfo->moduleArgs->name.substr(3)
        : "0";
    const std::string& sys_path = get_gpu_syspath(id);
    return get_vendor(read_by_syspath(sys_path + "/device/vendor"));
}

#endif
