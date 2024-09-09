#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>

#include "query.hpp"
#include "util.hpp"

using namespace Query;

static std::string _get_name(const std::string_view m_vendor_id_s, const std::string_view m_device_id_s)
{
    std::string name = binarySearchPCIArray(m_vendor_id_s, m_device_id_s);
    debug("GPU binarySearchPCIArray name = {}", name);
    size_t first_bracket = name.find('[');
    size_t last_bracket  = name.rfind(']');

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

static std::string _get_vendor(const std::string_view m_vendor_id_s)
{ return binarySearchPCIArray(m_vendor_id_s); }

static GPU::GPU_t get_gpu_infos(const std::string_view m_vendor_id_s, const std::string_view m_device_id_s)
{
    debug("calling GPU {}", __func__);
    GPU::GPU_t ret;

    debug("GPU m_vendor_id_s = {} || m_device_id_s = {}", m_vendor_id_s, m_device_id_s);
    if (m_device_id_s == UNKNOWN || m_vendor_id_s == UNKNOWN)
        return ret;

    ret.name   = _get_name(m_vendor_id_s, m_device_id_s);
    ret.vendor = _get_vendor(m_vendor_id_s);

    return ret;
}

GPU::GPU(std::uint16_t& id, std::vector<std::uint16_t>& queried_gpus)
{
    if (std::find(queried_gpus.begin(), queried_gpus.end(), id) == queried_gpus.end())
        queried_gpus.push_back(id);
    else
        return;

    const u_short max_iter = 10;
    u_short       id_iter  = id;
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
