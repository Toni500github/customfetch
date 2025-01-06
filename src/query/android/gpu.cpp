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

#include "platform.hpp"
#if CF_ANDROID

#include <string>

#include "query.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

using namespace Query;

// https://en.wikipedia.org/wiki/List_of_Qualcomm_Snapdragon_systems_on_chips
static std::string detect_adreno(const std::string& cpu_model_name)
{
    switch (fnv1a16::hash(cpu_model_name))
    {
        case "MSM8225Q"_fnv1a16:
        case "MSM8625Q"_fnv1a16: return "Adreno (TM) 203";

        case "MSM8210"_fnv1a16:
        case "MSM8610"_fnv1a16:
        case "MSM8212"_fnv1a16:
        case "MSM8612"_fnv1a16: return "Adreno (TM) 302";

        case "MSM8905"_fnv1a16:
        case "MSM8208"_fnv1a16:
        case "MSM8909"_fnv1a16:
        case "MSM8909AA"_fnv1a16: return "Adreno (TM) 304";

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
        case "MSM8930AB"_fnv1a16: return "Adreno (TM) 305";

        case "APQ8016"_fnv1a16:
        case "MSM8916"_fnv1a16: return "Adreno (TM) 306";

        case "QM215"_fnv1a16:
        case "MSM8917"_fnv1a16:
        case "MSM8920"_fnv1a16: return "Adreno (TM) 308";

        case "APQ8064M"_fnv1a16:
        case "APQ8064T"_fnv1a16:
        case "APQ8064AB"_fnv1a16: return "Adreno (TM) 320";

        case "APQ8074AA"_fnv1a16:
        case "MSM8274AA"_fnv1a16:
        case "MSM8674AA"_fnv1a16:
        case "MSM8974AA"_fnv1a16:
        case "MSM8274AB"_fnv1a16: return "Adreno (TM) 330";

        case "MSM8936"_fnv1a16:
        case "MSM8939"_fnv1a16:
        case "MSM8952"_fnv1a16:
        case "MSM8929"_fnv1a16: return "Adreno (TM) 405";

        case "MSM8992"_fnv1a16: return "Adreno (TM) 418";
        case "APQ8084"_fnv1a16: return "Adreno (TM) 420";
        case "MSM8994"_fnv1a16: return "Adreno (TM) 430";

        case "SDM429"_fnv1a16: return "Adreno (TM) 504";

        case "SDM439"_fnv1a16:
        case "SDM450"_fnv1a16:
        case "MSM8937"_fnv1a16:
        case "MSM8940"_fnv1a16: return "Adreno (TM) 505";

        case "MSM8953"_fnv1a16:
        case "SDM632"_fnv1a16:  return "Adreno (TM) 506";
        case "SDM630"_fnv1a16:  return "Adreno (TM) 508";
        case "SDM636"_fnv1a16:  return "Adreno (TM) 509";

        case "MSM8956"_fnv1a16:
        case "MSM8976"_fnv1a16: return "Adreno (TM) 510";

        case "SDM660"_fnv1a16:  return "Adreno (TM) 512";
        case "MSM8996"_fnv1a16: return "Adreno (TM) 530";
        case "MSM8998"_fnv1a16: return "Adreno (TM) 540";

        case "SM6225"_fnv1a16:
        case "SM6115"_fnv1a16:
        case "SM6125"_fnv1a16:
        case "SM6115-AC"_fnv1a16:
        case "SM6225-AD"_fnv1a16:
        case "SM4250-AA"_fnv1a16: return "Adreno (TM) 610";

        case "SM4635"_fnv1a16:    return "Adreno (TM) 611";
        case "SM6150"_fnv1a16:
        case "SM6150-AC"_fnv1a16: return "Adreno (TM) 612";
        case "SM4450"_fnv1a16:    return "Adreno (TM) 613";
        case "SDM670"_fnv1a16:    return "Adreno (TM) 615";

        case "SDM710"_fnv1a16:
        case "SDM712"_fnv1a16: return "Adreno (TM) 616";

        case "SM7125"_fnv1a16:
        case "SM7150-AA"_fnv1a16:
        case "SM7150-AB"_fnv1a16:
        case "SM7150-AC"_fnv1a16: return "Adreno (TM) 618";

        case "SM4375"_fnv1a16:
        case "SM4350"_fnv1a16:
        case "SM6375"_fnv1a16:
        case "SM7225"_fnv1a16:
        case "SM6375-AC"_fnv1a16:
        case "SM4350-AC"_fnv1a16: return "Adreno (TM) 619";

        case "SM6350"_fnv1a16: return "Adreno (TM) 619L";

        case "SM7250-AA"_fnv1a16:
        case "SM7250-AB"_fnv1a16:
        case "SM7250-AC"_fnv1a16: return "Adreno (TM) 620";

        case "SDM845"_fnv1a16: return "Adreno (TM) 630";

        case "SM8150"_fnv1a16:
        case "SM8150-AC"_fnv1a16:
        case "SM8150P"_fnv1a16:   return "Adreno (TM) 640";

        case "SM7350-AB"_fnv1a16: return "Adreno (TM) 642";

        case "SM7325"_fnv1a16:
        case "SM7325-AF"_fnv1a16:
        case "SM7325-AE"_fnv1a16: return "Adreno (TM) 642L";
        case "SM7450-AB"_fnv1a16: return "Adreno (TM) 644";

        case "SM8250"_fnv1a16:
        case "SM8250-AB"_fnv1a16:
        case "SM8250-AC"_fnv1a16: return "Adreno (TM) 650";

        case "SM8350"_fnv1a16:
        case "SM8350-AC"_fnv1a16: return "Adreno (TM) 660";

        case "SM7435-AB"_fnv1a16:
        case "SM6475-AB"_fnv1a16:
        case "SM6450"_fnv1a16:    return "Adreno (TM) 710";

        case "SM7550-AB"_fnv1a16: return "Adreno (TM) 720";
        case "SM7475-AB"_fnv1a16: return "Adreno (TM) 725";

        case "SM8450"_fnv1a16:
        case "SM8475"_fnv1a16:
        case "SM8425-100-AC"_fnv1a16: return "Adreno (TM) 730";

        case "SM7675-AB"_fnv1a16: return "Adreno (TM) 732";
        case "SM8635"_fnv1a16:    return "Adreno (TM) 735";

        case "SM8550"_fnv1a16:
        case "SM8550-AB"_fnv1a16:
        case "SM8550-AC"_fnv1a16: return "Adreno (TM) 740";

        case "SM8650-AB"_fnv1a16:
        case "SM8650-AC"_fnv1a16: return "Adreno (TM) 750";

        case "SM7635"_fnv1a16:    return "Adreno (TM) 810";
        case "SM8750-AB"_fnv1a16: return "Adreno (TM) 830";
    }

    return MAGIC_LINE;
}

GPU::GPU(const std::string& id, systemInfo_t& queried_gpus)
{
    CPU query_cpu;
    if (query_cpu.vendor() == "QUALCOMM" || query_cpu.vendor() == "QTI")
    {
        m_gpu_infos.vendor = "Qualcomm";
        m_gpu_infos.name   = detect_adreno(query_cpu.modelname());
    }
    else
    {
        m_gpu_infos.name = m_gpu_infos.vendor = MAGIC_LINE;
    }
}

// clang-format off
std::string& GPU::name() noexcept
{ return m_gpu_infos.name; }

std::string& GPU::vendor() noexcept
{ return m_gpu_infos.vendor; }

#endif
