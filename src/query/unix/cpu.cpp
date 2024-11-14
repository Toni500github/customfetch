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

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

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
    std::string cpuname = "Qualcomm ";
    switch (fnv1a16::hash(model_name))
    {
        case "SM8750-AB"_fnv1a16: cpuname += "Snapdragon 8 Elite"; break;
        case "SM8635"_fnv1a16:    cpuname += "Snapdragon 8s Gen 3"; break;
        case "SM8650-AC"_fnv1a16: cpuname += "Snapdragon 8 Gen 3 for Galaxy"; break;
        case "SM8650"_fnv1a16:    cpuname += "Snapdragon 8 Gen 3"; break;
        case "SM8550-AC"_fnv1a16: cpuname += "Snapdragon 8 Gen 2 for Galaxy"; break;
        case "SM8550"_fnv1a16:    cpuname += "Snapdragon 8 Gen 2"; break;
        case "SM8475"_fnv1a16:    cpuname += "Snapdragon 8+ Gen 1"; break;
        case "SM8450"_fnv1a16:    cpuname += "Snapdragon 8 Gen 1"; break;

        case "SM7450-AB"_fnv1a16: cpuname += "Snapdragon 7 Gen 1"; break;
        case "SM7475-AB"_fnv1a16: cpuname += "Snapdragon 7+ Gen 2"; break;
        case "SM7435-AB"_fnv1a16: cpuname += "Snapdragon 7s Gen 2"; break;
        case "SM7550-AB"_fnv1a16: cpuname += "Snapdragon 7 Gen 3"; break;
        case "SM7675-AB"_fnv1a16: cpuname += "Snapdragon 7+ Gen 3"; break;
        case "SM7635"_fnv1a16:    cpuname += "Snapdragon 7s Gen 3"; break;

        case "SM6375-AC"_fnv1a16: cpuname += "Snapdragon 6s Gen 3"; break;
        case "SM6475-AB"_fnv1a16: cpuname += "Snapdragon 6 Gen 3"; break;
        case "SM6115-AC"_fnv1a16: cpuname += "Snapdragon 6s Gen 1"; break;
        case "SM6450"_fnv1a16:    cpuname += "Snapdragon 6 Gen 1"; break;

        case "SM4635"_fnv1a16: cpuname += "Snapdragon 4s Gen 2"; break;
        case "SM4450"_fnv1a16: cpuname += "Snapdragon 4 Gen 2"; break;
        case "SM4375"_fnv1a16: cpuname += "Snapdragon 4 Gen 1"; break;

        // adding fastfetch's many missing chips names
        case "MSM8225Q"_fnv1a16:
        case "MSM8625Q"_fnv1a16:
        case "MSM8210"_fnv1a16:
        case "MSM8610"_fnv1a16:
        case "MSM8212"_fnv1a16:
        case "MSM8612"_fnv1a16:  cpuname += "Snapdragon 200"; break;

        case "MSM8905"_fnv1a16:   cpuname += "205"; break;  // Qualcomm 205 break;
        case "MSM8208"_fnv1a16:   cpuname += "Snapdragon 208"; break;
        case "MSM8909"_fnv1a16:   cpuname += "Snapdragon 210"; break;
        case "MSM8909AA"_fnv1a16: cpuname += "Snapdragon 212"; break;
        case "QM215"_fnv1a16:     cpuname += "215"; break;

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
        case "MSM8930AB"_fnv1a16: cpuname += "Snapdragon 400"; break;

        case "APQ8016"_fnv1a16:
        case "MSM8916"_fnv1a16:   cpuname += "Snapdragon 410"; break;
        case "MSM8929"_fnv1a16:   cpuname += "Snapdragon 415"; break;
        case "MSM8917"_fnv1a16:   cpuname += "Snapdragon 425"; break;
        case "MSM8920"_fnv1a16:   cpuname += "Snapdragon 427"; break;
        case "MSM8937"_fnv1a16:   cpuname += "Snapdragon 430"; break;
        case "MSM8940"_fnv1a16:   cpuname += "Snapdragon 435"; break;
        case "SDM429"_fnv1a16:    cpuname += "Snapdragon 429"; break;
        case "SDM439"_fnv1a16:    cpuname += "Snapdragon 439"; break;
        case "SDM450"_fnv1a16:    cpuname += "Snapdragon 450"; break;
        case "SM4250-AA"_fnv1a16: cpuname += "Snapdragon 460"; break;
        case "SM4350"_fnv1a16:    cpuname += "Snapdragon 480"; break;
        case "SM4350-AC"_fnv1a16: cpuname += "Snapdragon 480+"; break;

        case "APQ8064-1AA"_fnv1a16:
        case "APQ8064M"_fnv1a16:
        case "APQ8064T"_fnv1a16:
        case "APQ8064AB"_fnv1a16:   cpuname += "Snapdragon 600"; break;

        case "MSM8936"_fnv1a16:   cpuname += "Snapdragon 610"; break;
        case "MSM8939"_fnv1a16:   cpuname += "Snapdragon 615"; break;
        case "MSM8952"_fnv1a16:   cpuname += "Snapdragon 617"; break;
        case "MSM8953"_fnv1a16:   cpuname += "Snapdragon 625"; break;
        case "SDM630"_fnv1a16:    cpuname += "Snapdragon 630"; break;
        case "SDM632"_fnv1a16:    cpuname += "Snapdragon 632"; break;
        case "SDM636"_fnv1a16:    cpuname += "Snapdragon 636"; break;
        case "MSM8956"_fnv1a16:   cpuname += "Snapdragon 650"; break;
        case "MSM8976"_fnv1a16:   cpuname += "Snapdragon 652"; break;
        case "SDA660"_fnv1a16:
        case "SDM660"_fnv1a16:    cpuname += "Snapdragon 660"; break;
        case "SM6115"_fnv1a16:    cpuname += "Snapdragon 662"; break;
        case "SM6125"_fnv1a16:    cpuname += "Snapdragon 665"; break;
        case "SDM670"_fnv1a16:    cpuname += "Snapdragon 670"; break;
        case "SM6150"_fnv1a16:    cpuname += "Snapdragon 675"; break;
        case "SM6150-AC"_fnv1a16: cpuname += "Snapdragon 678"; break;
        case "SM6225"_fnv1a16:    cpuname += "Snapdragon 680"; break;
        case "SM6225-AD"_fnv1a16: cpuname += "Snapdragon 685"; break;
        case "SM6350"_fnv1a16:    cpuname += "Snapdragon 690"; break;
        case "SM6375"_fnv1a16:    cpuname += "Snapdragon 695"; break;

        case "SDM710"_fnv1a16:    cpuname += "Snapdragon 710"; break;
        case "SDM712"_fnv1a16:    cpuname += "Snapdragon 712"; break;
        case "SM7125"_fnv1a16:    cpuname += "Snapdragon 720G"; break;
        case "SM7150-AA"_fnv1a16: cpuname += "Snapdragon 730"; break;
        case "SM7150-AB"_fnv1a16: cpuname += "Snapdragon 730G"; break;
        case "SM7150-AC"_fnv1a16: cpuname += "Snapdragon 732G"; break;
        case "SM7225"_fnv1a16:    cpuname += "Snapdragon 750G"; break;
        case "SM7250-AA"_fnv1a16: cpuname += "Snapdragon 765"; break;
        case "SM7250-AB"_fnv1a16: cpuname += "Snapdragon 765G"; break;
        case "SM7250-AC"_fnv1a16: cpuname += "Snapdragon 768G"; break;
        case "SM7325"_fnv1a16:    cpuname += "Snapdragon 778G"; break;
        case "SM7325-AE"_fnv1a16: cpuname += "Snapdragon 778G+"; break;
        case "SM7350-AB"_fnv1a16: cpuname += "Snapdragon 780G"; break;
        case "SM7325-AF"_fnv1a16: cpuname += "Snapdragon 782G"; break;

        case "APQ8074AA"_fnv1a16:
        case "MSM8274AA"_fnv1a16:
        case "MSM8674AA"_fnv1a16:
        case "MSM8974AA"_fnv1a16:
        case "MSM8274AB"_fnv1a16: cpuname += "Snapdragon 800"; break;

        case "APQ8084"_fnv1a16:   cpuname += "Snapdragon 805"; break;
        case "MSM8992"_fnv1a16:   cpuname += "Snapdragon 808"; break;
        case "MSM8994"_fnv1a16:   cpuname += "Snapdragon 810"; break;
        case "MSM8996"_fnv1a16:   cpuname += "Snapdragon 820"; break;
        case "MSM8998"_fnv1a16:   cpuname += "Snapdragon 835"; break;
        case "SDM845"_fnv1a16:    cpuname += "Snapdragon 845"; break;
        case "SM8150"_fnv1a16:    cpuname += "Snapdragon 855"; break;
        case "SM8150P"_fnv1a16:
        case "SM8150-AC"_fnv1a16: cpuname += "Snapdragon 855+"; break;  // both 855+ and 860 break;
        case "SM8250"_fnv1a16:    cpuname += "Snapdragon 865"; break;
        case "SM8250-AB"_fnv1a16: cpuname += "Snapdragon 865+"; break;
        case "SM8250-AC"_fnv1a16: cpuname += "Snapdragon 870"; break;
        case "SM8350"_fnv1a16:    cpuname += "Snapdragon 888"; break;
        case "SM8350-AC"_fnv1a16: cpuname += "Snapdragon 888+"; break;

        default: return UNKNOWN;
    }

    return cpuname + " [" + model_name + "]";
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
        error("Could not open {}", cpuinfo_path);
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
        std::string vendor;
        ret.name = get_android_property("ro.soc.model");
        if (ret.name.empty())
        {
            vendor   = "MTK";
            ret.name = get_android_property("ro.mediatek.platform");
        }
        if (vendor.empty())
        {
            vendor = get_android_property("ro.soc.manufacturer");
            if (vendor.empty())
                vendor = get_android_property("ro.soc.manufacturer");
        }

        if ((vendor == "QTI" || vendor == "QUALCOMM") &&
            (hasStart(ret.name, "SM") || hasStart(ret.name, "APQ") || hasStart(ret.name, "MSM") ||
             hasStart(ret.name, "SDM") || hasStart(ret.name, "QM")))
            ret.name = detect_qualcomm(ret.name);
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
#if !CF_ANDROID
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
