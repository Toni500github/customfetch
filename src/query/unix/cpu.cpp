/*
 * Copyright 2024 Toni500git
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

#include <sys/types.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "fmt/format.h"
#include "platform.hpp"
#include "query.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

using namespace Query;

// https://en.wikipedia.org/wiki/List_of_Qualcomm_Snapdragon_systems_on_chips
#if CF_ANDROID
static std::string detect_qualcomm(const std::string& model_name)
{
    switch (fnv1a16::hash(model_name))
    {
        case "SM8750-AB"_fnv1a16: return "Snapdragon 8 Elite";
        case "SM8635"_fnv1a16:    return "Snapdragon 8s Gen 3";
        case "SM8650-AC"_fnv1a16: return "Snapdragon 8 Gen 3 for Galaxy";
        case "SM8650"_fnv1a16:    return "Snapdragon 8 Gen 3";
        case "SM8550-AC"_fnv1a16: return "Snapdragon 8 Gen 2 for Galaxy";
        case "SM8550"_fnv1a16:    return "Snapdragon 8 Gen 2";
        case "SM8475"_fnv1a16:    return "Snapdragon 8+ Gen 1";
        case "SM8450"_fnv1a16:    return "Snapdragon 8 Gen 1";

        case "SM7450-AB"_fnv1a16: return "Snapdragon 7 Gen 1";
        case "SM7475-AB"_fnv1a16: return "Snapdragon 7+ Gen 2";
        case "SM7435-AB"_fnv1a16: return "Snapdragon 7s Gen 2";
        case "SM7550-AB"_fnv1a16: return "Snapdragon 7 Gen 3";
        case "SM7675-AB"_fnv1a16: return "Snapdragon 7+ Gen 3";
        case "SM7635"_fnv1a16:    return "Snapdragon 7s Gen 3";

        case "SM6375-AC"_fnv1a16: return "Snapdragon 6s Gen 3";
        case "SM6475-AB"_fnv1a16: return "Snapdragon 6 Gen 3";
        case "SM6115-AC"_fnv1a16: return "Snapdragon 6s Gen 1";
        case "SM6450"_fnv1a16:    return "Snapdragon 6 Gen 1";

        case "SM4635"_fnv1a16: return "Snapdragon 4s Gen 2";
        case "SM4450"_fnv1a16: return "Snapdragon 4 Gen 2";
        case "SM4375"_fnv1a16: return "Snapdragon 4 Gen 1";

        // adding fastfetch's many missing chips names
        case "MSM8225Q"_fnv1a16:
        case "MSM8625Q"_fnv1a16:
        case "MSM8210"_fnv1a16:
        case "MSM8610"_fnv1a16:
        case "MSM8212"_fnv1a16:
        case "MSM8612"_fnv1a16:  return "Snapdragon 200";

        case "MSM8905"_fnv1a16:   return "205";  // Qualcomm 205
        case "MSM8208"_fnv1a16:   return "Snapdragon 208";
        case "MSM8909"_fnv1a16:   return "Snapdragon 210";
        case "MSM8909AA"_fnv1a16: return "Snapdragon 212";
        case "QM215"_fnv1a16:     return "215";

        // holy crap
        case "APQ8026"_fnv1a16:
        case "MSM8226"_fnv1a16:
        case "MSM8926"_fnv1a16:
        case "APQ8028"_fnv1a16:
        case "MSM8228"_fnv1a16:
        case "MSM8628"_fnv1a16:
        case "MSM8928"_fnv1a16:
        case "MSM8230"_fnv1a16:
        case "MSM8630"_fnv1a16:
        case "MSM8930"_fnv1a16:
        case "MSM8930AA"_fnv1a16:
        case "APQ8030AB"_fnv1a16:
        case "MSM8230AB"_fnv1a16:
        case "MSM8630AB"_fnv1a16:
        case "MSM8930AB"_fnv1a16: return "Snapdragon 400";

        case "APQ8016"_fnv1a16:
        case "MSM8916"_fnv1a16:   return "Snapdragon 410";
        case "MSM8929"_fnv1a16:   return "Snapdragon 415";
        case "MSM8917"_fnv1a16:   return "Snapdragon 425";
        case "MSM8920"_fnv1a16:   return "Snapdragon 427";
        case "MSM8937"_fnv1a16:   return "Snapdragon 430";
        case "MSM8940"_fnv1a16:   return "Snapdragon 435";
        case "SDM429"_fnv1a16:    return "Snapdragon 429";
        case "SDM439"_fnv1a16:    return "Snapdragon 439";
        case "SDM450"_fnv1a16:    return "Snapdragon 450";
        case "SM4250-AA"_fnv1a16: return "Snapdragon 460";
        case "SM4350"_fnv1a16:    return "Snapdragon 480";
        case "SM4350-AC"_fnv1a16: return "Snapdragon 480+";

        case "APQ8064-1AA"_fnv1a16:
        case "APQ8064M"_fnv1a16:
        case "APQ8064T"_fnv1a16:
        case "APQ8064AB"_fnv1a16:   return "Snapdragon 600";

        case "MSM8936"_fnv1a16:   return "Snapdragon 610";
        case "MSM8939"_fnv1a16:   return "Snapdragon 615";
        case "MSM8952"_fnv1a16:   return "Snapdragon 617";
        case "MSM8953"_fnv1a16:   return "Snapdragon 625";
        case "SDM630"_fnv1a16:    return "Snapdragon 630";
        case "SDM632"_fnv1a16:    return "Snapdragon 632";
        case "SDM636"_fnv1a16:    return "Snapdragon 636";
        case "MSM8956"_fnv1a16:   return "Snapdragon 650";
        case "MSM8976"_fnv1a16:   return "Snapdragon 652";
        case "SDA660"_fnv1a16:
        case "SDM660"_fnv1a16:    return "Snapdragon 660";
        case "SM6115"_fnv1a16:    return "Snapdragon 662";
        case "SM6125"_fnv1a16:    return "Snapdragon 665";
        case "SDM670"_fnv1a16:    return "Snapdragon 670";
        case "SM6150"_fnv1a16:    return "Snapdragon 675";
        case "SM6150-AC"_fnv1a16: return "Snapdragon 678";
        case "SM6225"_fnv1a16:    return "Snapdragon 680";
        case "SM6225-AD"_fnv1a16: return "Snapdragon 685";
        case "SM6350"_fnv1a16:    return "Snapdragon 690";
        case "SM6375"_fnv1a16:    return "Snapdragon 695";

        case "SDM710"_fnv1a16:    return "Snapdragon 710";
        case "SDM712"_fnv1a16:    return "Snapdragon 712";
        case "SM7125"_fnv1a16:    return "Snapdragon 720G";
        case "SM7150-AA"_fnv1a16: return "Snapdragon 730";
        case "SM7150-AB"_fnv1a16: return "Snapdragon 730G";
        case "SM7150-AC"_fnv1a16: return "Snapdragon 732G";
        case "SM7225"_fnv1a16:    return "Snapdragon 750G";
        case "SM7250-AA"_fnv1a16: return "Snapdragon 765";
        case "SM7250-AB"_fnv1a16: return "Snapdragon 765G";
        case "SM7250-AC"_fnv1a16: return "Snapdragon 768G";
        case "SM7325"_fnv1a16:    return "Snapdragon 778G";
        case "SM7325-AE"_fnv1a16: return "Snapdragon 778G+";
        case "SM7350-AB"_fnv1a16: return "Snapdragon 780G";
        case "SM7325-AF"_fnv1a16: return "Snapdragon 782G";

        case "APQ8074AA"_fnv1a16:
        case "MSM8274AA"_fnv1a16:
        case "MSM8674AA"_fnv1a16:
        case "MSM8974AA"_fnv1a16:
        case "MSM8274AB"_fnv1a16: return "Snapdragon 800";

        case "APQ8084"_fnv1a16:   return "Snapdragon 805";
        case "MSM8992"_fnv1a16:   return "Snapdragon 808";
        case "MSM8994"_fnv1a16:   return "Snapdragon 810";
        case "MSM8996"_fnv1a16:   return "Snapdragon 820";
        case "MSM8998"_fnv1a16:   return "Snapdragon 835";
        case "SDM845"_fnv1a16:    return "Snapdragon 845";
        case "SM8150"_fnv1a16:    return "Snapdragon 855";
        case "SM8150P"_fnv1a16:
        case "SM8150-AC"_fnv1a16: return "Snapdragon 855+";  // both 855+ and 860
        case "SM8250"_fnv1a16:    return "Snapdragon 865";
        case "SM8250-AB"_fnv1a16: return "Snapdragon 865+";
        case "SM8250-AC"_fnv1a16: return "Snapdragon 870";
        case "SM8350"_fnv1a16:    return "Snapdragon 888";
        case "SM8350-AC"_fnv1a16: return "Snapdragon 888+";
    }

    return UNKNOWN;
}

static std::string detect_exynos(const std::string& model_name)
{
    switch (fnv1a16::hash(str_toupper(model_name)))
    {
        case "S5E3830"_fnv1a16: return "Exynos 850";
        case "S5E8805"_fnv1a16: return "Exynos 880";

        case "S5E9630"_fnv1a16: return "Exynos 980";
        case "S5E9830"_fnv1a16: return "Exynos 990";

        case "S5E9815"_fnv1a16: return "Exynos 1080";
        case "S5E8825"_fnv1a16: return "Exynos 1280";
        case "S5E8535"_fnv1a16: return "Exynos 1330";
        case "S5E8835"_fnv1a16: return "Exynos 1380";
        case "S5E8845"_fnv1a16: return "Exynos 1480";
        case "S5E8855"_fnv1a16: return "Exynos 1580";
        /* TODO: alot of SoCs with no ID mentioned on Wikipedia.. */

        case "S5E9840"_fnv1a16: return "Exynos 2100";
        case "S5E9925"_fnv1a16: return "Exynos 2200";
        case "S5E9945"_fnv1a16: return "Exynos 2400[e]";

        case "S5PC110"_fnv1a16: return "Exynos 3 Single 3110";
        case "S5E3470"_fnv1a16: return "Exynos 3 Quad 3470";
        /* TODO: ASSUMPTION!! I could not find the full part number for this, I'm making an assumption here. */
        case "S5E3475"_fnv1a16: return "Exynos 3 Quad 3475";

        case "S5E4210"_fnv1a16:
        case "S5PC210"_fnv1a16: return "Exynos 4 Dual 4210";

        case "S5E4212"_fnv1a16: return "Exynos 4 Dual 4212";

        case "S55E4210"_fnv1a16:
        case "S5PC220"_fnv1a16: return "Exynos 4 Quad 4412";

        /* TODO: Exynos 4 Quad 4415 */

        case "S5E5250"_fnv1a16:
        case "S5PC520"_fnv1a16: return "Exynos 5 Dual 5250";

        case "S5E5260"_fnv1a16: return "Exynos 5 Hexa 5260";

        case "S5E5410"_fnv1a16: return "Exynos 5 Octa 5410";

        case "S5E5420"_fnv1a16: return "Exynos 5 Octa 5420";

        /* 5800 for chromebooks */
        case "S5E5422"_fnv1a16: return "Exynos 5 Octa 5422/5800";

        case "S5E5430"_fnv1a16: return "Exynos 5 Octa 5430";

        case "S5E5433"_fnv1a16: return "Exynos 7 Octa 5433";

        case "SC57270"_fnv1a16: return "Exynos 7 Dual 7270";

        case "S5E7420"_fnv1a16: return "Exynos 7 Octa 7420";

        case "S5E7570"_fnv1a16: return "Exynos 7 Quad 7570";
        /* TODO: Exynos 7 Quad/Octa 7578/7580 */

        case "S5E7870"_fnv1a16: return "Exynos 7 Octa 7870";

        case "S5E7872"_fnv1a16: return "Exynos 5 7872";

        case "S5E7880"_fnv1a16: return "Exynos 7880";

        case "S5E7884"_fnv1a16: return "Exynos 7884/7885";
        case "S5E7904"_fnv1a16: return "Exynos 7904";

        case "S5E8890"_fnv1a16: return "Exynos 8 Octa 8890";
        case "S5E8895"_fnv1a16: return "Exynos 8895";

        case "S5E9609"_fnv1a16: return "Exynos 9609";
        case "S5E9610"_fnv1a16: return "Exynos 9610";
        case "S5E9611"_fnv1a16: return "Exynos 9611";

        case "S5E9810"_fnv1a16: return "Exynos 9810";

        case "S5E9820"_fnv1a16:   return "Exynos 9820";
        case "S5E9825"_fnv1a16:   return "Exynos 9825";

        case "S5E4212"_fnv1a16: return "Exynos 4 Dual 4212";
        /* TODO: Exynos 3 Dual 3250 */

        case "SC57270"_fnv1a16: return "Exynos 7 Dual 7270";

        case "SC59110XSC"_fnv1a16:   return "Exynos 9110";
        case "SC55515XBD"_fnv1a16:   return "Exynos W920";
        case "SC55515XBE"_fnv1a16:   return "Exynos W930";
        case "SC55535AHA"_fnv1a16:   return "Exynos W1000";
    }

    return UNKNOWN;
}
#endif

static std::string get_from_text(std::string& line)
{
    std::string amount = line.substr(line.find(':') + 1);
    strip(amount);
    return amount;
}

static CPU::CPU_t get_cpu_infos()
{
    CPU::CPU_t ret;
    debug("calling in CPU {}", __PRETTY_FUNCTION__);
    constexpr std::string_view cpuinfo_path = "/proc/cpuinfo";
    std::ifstream              file(cpuinfo_path.data());
    if (!file.is_open())
    {
        error(_("Could not open {}"), cpuinfo_path);
        return ret;
    }

    std::string line;
    float       cpu_mhz = -1;
    while (std::getline(file, line))
    {
        if (hasStart(line, "model name"))
            ret.name = get_from_text(line);

        else if (hasStart(line, "processor"))
            ret.nproc = get_from_text(line);

        else if (hasStart(line, "cpu MHz"))
        {
            double tmp = std::stof(get_from_text(line));
            if (tmp > cpu_mhz)
                cpu_mhz = tmp;
        }
    }

#if CF_ANDROID
    if (ret.name == UNKNOWN)
    {
        ret.modelname = get_android_property("ro.soc.model");
        if (ret.modelname.empty())
        {
            ret.vendor    = "MTK";
            ret.modelname = get_android_property("ro.mediatek.platform");
        }
        if (ret.vendor.empty())
        {
            ret.vendor = get_android_property("ro.soc.manufacturer");
            if (ret.vendor.empty())
                ret.vendor = get_android_property("ro.product.product.manufacturer");
        }

        if ((ret.vendor == "QTI" || ret.vendor == "QUALCOMM") &&
            (hasStart(ret.modelname, "SM") || hasStart(ret.modelname, "APQ") || hasStart(ret.modelname, "MSM") ||
             hasStart(ret.modelname, "SDM") || hasStart(ret.modelname, "QM")))
            ret.name = fmt::format("Qualcomm {} [{}]", detect_qualcomm(ret.modelname), ret.modelname);
        else if (ret.vendor == "Samsung") {
            ret.name = fmt::format("Samsung {} [{}]", detect_exynos(ret.modelname), ret.modelname);
        }
        else
            ret.name = ret.vendor + " " + ret.modelname;
    }
#endif

    // sometimes /proc/cpuinfo at model name
    // the name will contain the min freq
    // happens on intel cpus especially
    const size_t pos = ret.name.rfind('@');
    if (pos != std::string::npos)
        ret.name.erase(pos - 1);

    cpu_mhz /= 1000;
    ret.freq_max_cpuinfo = cpu_mhz;

    // add 1 to the nproc
    ret.nproc = fmt::to_string(std::stoi(ret.nproc) + 1);

    const std::string freq_dir = "/sys/devices/system/cpu/cpu0/cpufreq";
    if (std::filesystem::exists(freq_dir))
    {
        std::ifstream cpu_bios_limit_f(freq_dir + "/bios_limit");
        std::ifstream cpu_scaling_cur_f(freq_dir + "/scaling_cur_freq");
        std::ifstream cpu_scaling_max_f(freq_dir + "/scaling_max_freq");
        std::ifstream cpu_scaling_min_f(freq_dir + "/scaling_min_freq");

        std::string freq_bios_limit, freq_cpu_scaling_cur, freq_cpu_scaling_max, freq_cpu_scaling_min;

        std::getline(cpu_bios_limit_f, freq_bios_limit);
        std::getline(cpu_scaling_cur_f, freq_cpu_scaling_cur);
        std::getline(cpu_scaling_max_f, freq_cpu_scaling_max);
        std::getline(cpu_scaling_min_f, freq_cpu_scaling_min);

        ret.freq_bios_limit = freq_bios_limit.empty() ? 0 : (std::stof(freq_bios_limit) / 1000000);
        ret.freq_cur        = freq_cpu_scaling_cur.empty() ? 0 : (std::stof(freq_cpu_scaling_cur) / 1000000);
        ret.freq_max        = freq_cpu_scaling_max.empty() ? 0 : (std::stof(freq_cpu_scaling_max) / 1000000);
        ret.freq_min        = freq_cpu_scaling_min.empty() ? 0 : (std::stof(freq_cpu_scaling_min) / 1000000);
    }

    return ret;
}

static double get_cpu_temp()
{
#if CF_ANDROID
    // https://github.com/kamgurgul/cpu-info/blob/master/shared/src/androidMain/kotlin/com/kgurgul/cpuinfo/data/provider/TemperatureProvider.android.kt#L119
    constexpr std::array<std::string_view, 20> temp_paths = {
        "/sys/devices/system/cpu/cpu0/cpufreq/cpu_temp",
        "/sys/devices/system/cpu/cpu0/cpufreq/FakeShmoo_cpu_temp",
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/i2c-adapter/i2c-4/4-004c/temperature",
        "/sys/devices/platform/tegra-i2c.3/i2c-4/4-004c/temperature",
        "/sys/devices/platform/omap/omap_temp_sensor.0/temperature",
        "/sys/devices/platform/tegra_tmon/temp1_input",
        "/sys/kernel/debug/tegra_thermal/temp_tj",
        "/sys/devices/platform/s5p-tmu/temperature",
        "/sys/class/thermal/thermal_zone1/temp",
        "/sys/class/hwmon/hwmon0/device/temp1_input",
        "/sys/devices/virtual/thermal/thermal_zone1/temp",
        "/sys/devices/virtual/thermal/thermal_zone0/temp",
        "/sys/class/thermal/thermal_zone3/temp",
        "/sys/class/thermal/thermal_zone4/temp",
        "/sys/class/hwmon/hwmonX/temp1_input",
        "/sys/devices/platform/s5p-tmu/curr_temp",
        "/sys/htc/cpu_temp",
        "/sys/devices/platform/tegra-i2c.3/i2c-4/4-004c/ext_temperature",
        "/sys/devices/platform/tegra-tsensor/tsensor_temperature",
    };
    for (const std::string_view path : temp_paths)
    {
	debug("checking {}", path);
        if (!std::filesystem::exists(path))
            continue;

        const double ret = std::stod(read_by_syspath(path)) / 1000.0;
        debug("cpu temp ret = {}", ret);

        if (ret >= -1.0 && ret <= 250.0)
            return ret;
    }
#else
    for (const auto& dir : std::filesystem::directory_iterator{ "/sys/class/hwmon/" })
    {
        const std::string& name = read_by_syspath((dir.path() / "name").string());
        debug("name = {}", name);
        if (name != "cpu" && name != "k10temp" && name != "coretemp")
            continue;

        const std::string& temp_file = std::filesystem::exists(dir.path() / "temp1_input")
                                           ? dir.path() / "temp1_input"
                                           : dir.path() / "device/temp1_input";
        if (!std::filesystem::exists(temp_file))
            continue;

        const double ret = std::stod(read_by_syspath(temp_file));
        debug("cpu temp ret = {}", ret);

        return ret / 1000.0;
    }
#endif

    return 0.0;
}

CPU::CPU() noexcept
{
    if (!m_bInit)
    {
        m_cpu_infos = get_cpu_infos();
        m_bInit     = true;
    }
}

// clang-format off
std::string& CPU::name() noexcept
{ return m_cpu_infos.name; }

std::string& CPU::nproc() noexcept
{ return m_cpu_infos.nproc; }

std::string& CPU::vendor() noexcept
{ return m_cpu_infos.vendor; }

std::string& CPU::modelname() noexcept
{ return m_cpu_infos.modelname; }

double& CPU::freq_bios_limit() noexcept
{ return m_cpu_infos.freq_bios_limit; }

double& CPU::freq_cur() noexcept
{ return m_cpu_infos.freq_cur; }

double& CPU::freq_max() noexcept
{ return (m_cpu_infos.freq_max <= 0) ? m_cpu_infos.freq_max_cpuinfo : m_cpu_infos.freq_max; }

double& CPU::freq_min() noexcept
{ return m_cpu_infos.freq_min; }

double& CPU::temp() noexcept
{
    static bool done = false;
    if (!done)
        m_cpu_infos.temp = get_cpu_temp();
    done = true;

    return m_cpu_infos.temp;
}
