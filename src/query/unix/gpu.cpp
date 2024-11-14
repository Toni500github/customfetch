/*
 * Copyright 2024 Toni500git
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
#if CF_UNIX

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>

#include "query.hpp"
#include "util.hpp"

using namespace Query;

static std::string get_name(const std::string_view m_vendor_id_s, const std::string_view m_device_id_s)
{
    std::string name = binarySearchPCIArray(m_vendor_id_s, m_device_id_s);
    debug("GPU binarySearchPCIArray name = {}", name);
    const size_t first_bracket = name.find('[');
    const size_t last_bracket  = name.rfind(']');

    // remove the chips name "TU106 [GeForce GTX 1650]"
    // This should work for AMD and Intel too.
    if (first_bracket != std::string::npos && last_bracket != std::string::npos)
        name = name.substr(first_bracket + 1, last_bracket - first_bracket - 1);

    // name = this->vendor() + ' ' + name;

    // replace_str(name, "NVIDIA Corporation", "NVIDIA");
    // replace_str(name, "Advanced Micro Devices Inc.", "AMD");
    // replace_str(name, "Intel Corporation", "Intel");

    return name;
}

static std::string get_vendor(const std::string_view m_vendor_id_s)
{ return binarySearchPCIArray(m_vendor_id_s); }

static GPU::GPU_t get_gpu_infos(const std::string_view m_vendor_id_s, const std::string_view m_device_id_s)
{
    debug("calling GPU {}", __func__);
    GPU::GPU_t ret;

    debug("GPU m_vendor_id_s = {} || m_device_id_s = {}", m_vendor_id_s, m_device_id_s);
    if (m_device_id_s == UNKNOWN || m_vendor_id_s == UNKNOWN)
        return ret;

    ret.name   = get_name(m_vendor_id_s, m_device_id_s);
    ret.vendor = get_vendor(m_vendor_id_s);

    return ret;
}

GPU::GPU(const std::uint16_t id, std::vector<std::uint16_t>& queried_gpus)
{
    if (std::find(queried_gpus.begin(), queried_gpus.end(), id) == queried_gpus.end())
        queried_gpus.push_back(id);
    else
        return;

    const std::uint16_t max_iter = 10;
    std::uint16_t       id_iter  = id;
    std::string   sys_path;
    int           i = 0;
    for (; i <= max_iter; i++)
    {
        sys_path = "/sys/class/drm/card" + fmt::to_string(id_iter);
        if (std::filesystem::exists(sys_path + "/device/device") &&
            std::filesystem::exists(sys_path + "/device/vendor"))
            break;
        else
            id_iter++;
    }

    if (i >= max_iter)
    {
        error("Failed to parse GPU infos on the path /sys/class/drm/");
        return;
    }

    m_vendor_id_s = read_by_syspath(sys_path + "/device/vendor");
    m_device_id_s = read_by_syspath(sys_path + "/device/device");

    m_gpu_infos = get_gpu_infos(m_vendor_id_s, m_device_id_s);
}

// clang-format off
std::string& GPU::name() noexcept
{ return m_gpu_infos.name; }

std::string& GPU::vendor() noexcept
{ return m_gpu_infos.vendor; }

#endif
