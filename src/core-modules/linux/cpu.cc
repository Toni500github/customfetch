#include "platform.hpp"
#if CF_ANDROID || CF_LINUX
#include <unistd.h>

#include <string>
#include <cstring>

#include "cufetch/common.hh"
#include "core-modules.hh"
#include "fmt/format.h"
#include "switch_fnv1a.hpp"
#include "util.hpp"

const std::string freq_dir = "/sys/devices/system/cpu/cpu0/cpufreq";

#if CF_ANDROID
// https://en.wikipedia.org/wiki/List_of_Qualcomm_Snapdragon_systems_on_chips
// by Toni500git
static std::string detect_qualcomm(const std::string& model_name)
{
    switch (fnv1a16::hash(str_toupper(model_name)))
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

    return model_name;
}

// https://en.wikipedia.org/wiki/Exynos#Current_Exynos_SoCs_(2020%E2%80%93present)
// by BurntRanch
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
        case "S5E8855"_fnv1a16:
            return "Exynos 1580";
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
        case "S5PC220"_fnv1a16:
            return "Exynos 4 Quad 4412";

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
        case "S5E7570"_fnv1a16:
            return "Exynos 7 Quad 7570";
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
        case "S5E9820"_fnv1a16: return "Exynos 9820";
        case "S5E9825"_fnv1a16:
            return "Exynos 9825";
            /* TODO: Exynos 3 Dual 3250 */

        case "SC59110XSC"_fnv1a16: return "Exynos 9110";
        case "SC55515XBD"_fnv1a16: return "Exynos W920";
        case "SC55515XBE"_fnv1a16: return "Exynos W930";
        case "SC55535AHA"_fnv1a16: return "Exynos W1000";
    }

    return model_name;
}

// https://en.wikipedia.org/wiki/List_of_MediaTek_systems_on_chips
// by Toni500git
// i need some coffee.....
// wayland crashed and had to restart from Dimensity 1000 series.........
static std::string detect_mediatek(const std::string& model_name)
{
    switch (fnv1a16::hash(model_name))
    {
        // Helio series
        // Helio X series
        case "MT6795"_fnv1a16:
        case "MT6795M"_fnv1a16:
        case "MT6795T"_fnv1a16: return "Helio X10";

        case "MT6797"_fnv1a16:  return "Helio X20";
        case "MT6797D"_fnv1a16: return "Helio X23";
        case "MT6797X"_fnv1a16: return "Helio X27";
        case "MT6799"_fnv1a16:  return "Helio X30";

        // Helio A series
        case "MT6761V/WE"_fnv1a16:  return "Helio A20";
        case "MT6761V/WBB"_fnv1a16:
        case "MT6761V/WAB"_fnv1a16: return "Helio A22";
        case "MT6762V/WD"_fnv1a16:
        case "MT6762V/WB"_fnv1a16:  return "Helio A25";

        // Helio P series
        case "MT6755"_fnv1a16:
        case "MT6755M"_fnv1a16:    return "Helio P10";
        case "MT6755T"_fnv1a16:    return "Helio P15";
        case "MT6755S"_fnv1a16:    return "Helio P18";
        case "MT6757"_fnv1a16:     return "Helio P20";
        case "MT6762"_fnv1a16:     return "Helio P22";
        case "MT6763"_fnv1a16:     return "Helio P23";
        case "MT6763T"_fnv1a16:    return "Helio P23T";
        case "MT6757CD"_fnv1a16:   return "Helio P25";
        case "MT6757T"_fnv1a16:    return "Helio P25T";
        case "MT6758"_fnv1a16:     return "Helio P30";
        case "MT6765"_fnv1a16:     return "Helio P35";
        case "MT6771"_fnv1a16:     return "Helio P60";
        case "MT6768"_fnv1a16:     return "Helio P65";
        case "MT6771V/CT"_fnv1a16: return "Helio P70";
        case "MT6779V/CU"_fnv1a16: return "Helio P90";
        case "MT6779V/CV"_fnv1a16: return "Helio P95";

        // Helio G series
        case "MT6762G"_fnv1a16:     return "Helio G25";
        case "MT6765G"_fnv1a16:     return "Helio G35";
        case "MT6765V/XAA"_fnv1a16: return "Helio G36";
        case "MT6765V/XBA"_fnv1a16: return "Helio G36";
        case "MT6765H"_fnv1a16:     return "Helio G37";
        case "MT6765V"_fnv1a16:     return "Helio G50";
        case "MT6769V/CB"_fnv1a16:  return "Helio G70";

        case "MT6769T"_fnv1a16:
        case "MT6769V/CT"_fnv1a16:
        case "MT6769V/CU"_fnv1a16: return "Helio G80";

        case "MT6769J"_fnv1a16:    return "Helio G81";
        case "MT6769Z"_fnv1a16:    return "Helio G85";
        case "MT6769V/CZ"_fnv1a16: return "Helio G85";
        case "MT6769H"_fnv1a16:    return "Helio G88";
        case "MT6785V/CC"_fnv1a16: return "Helio G90T";
        case "MT6769G"_fnv1a16:    return "Helio G91";
        case "MT6769I"_fnv1a16:    return "Helio G92";
        case "MT6785V/CD"_fnv1a16: return "Helio G95";
        case "MT6781"_fnv1a16:     return "Helio G96";
        case "MT6781V/CD"_fnv1a16: return "Helio G96";
        case "MT6789H"_fnv1a16:    return "Helio G100";

        case "MT6789"_fnv1a16:
        case "MT6789G"_fnv1a16:
        case "MT6789U"_fnv1a16:
        case "MT6789V/CD"_fnv1a16:
        case "MT8781"_fnv1a16:
        case "MT8781V/CA"_fnv1a16:
        case "MT8781V/NB"_fnv1a16: return "Helio G99";

        // Dimensity Series
        // Dimensity 700 Series
        case "MT6833"_fnv1a16:
        case "MT6833G"_fnv1a16:
        case "MT6833V/ZA"_fnv1a16:
        case "MT6833V/NZA"_fnv1a16: return "Dimensity 700";

        case "MT6853V/ZA"_fnv1a16:
        case "MT6853V/NZA"_fnv1a16: return "Dimensity 720";

        // Dimensity 800 Series
        case "MT6873"_fnv1a16:       return "Dimensity 800";
        case "MT6853T"_fnv1a16:      return "Dimensity 800U";
        case "MT6853V/TNZA"_fnv1a16: return "Dimensity 800U";
        case "MT6875"_fnv1a16:       return "Dimensity 820";

        case "MT6833P"_fnv1a16:
        case "MT6833GP"_fnv1a16:
        case "MT6833V/PNZA"_fnv1a16: return "Dimensity 810";

        // Dimensity 900 Series
        case "MT6877V/ZA"_fnv1a16:  return "Dimensity 900";
        case "MT6855"_fnv1a16:
        case "MT6855V/AZA"_fnv1a16: return "Dimensity 930";

        // case "MT6877"_fnv1a16:
        case "MT6877T"_fnv1a16:
        case "MT6877V/TZA"_fnv1a16: return "Dimensity 920";

        // Dimensity 1000 Series
        case "MT6883Z/CZA"_fnv1a16: return "Dimensity 1000C";
        case "MT6885Z/CZA"_fnv1a16: return "Dimensity 1000L";
        case "MT6889"_fnv1a16:      return "Dimensity 1000";
        case "MT6889Z/CZA"_fnv1a16: return "Dimensity 1000+";

        case "MT6879"_fnv1a16:
        case "MT6879V/ZA"_fnv1a16:
        case "MT6879V_T/ZA"_fnv1a16: return "Dimensity 1050";

        case "MT6877"_fnv1a16:         return "Dimensity 1080 / Dimensity 920";
        case "MT6877V/TTZA"_fnv1a16:
        case "MT6877V_T/TTZA"_fnv1a16: return "Dimensity 1080";

        case "MT6891"_fnv1a16:
        case "MT6891Z/CZA"_fnv1a16:
        case "MT6891Z_Z/CZA"_fnv1a16:
        case "MT6891Z_T/CZA"_fnv1a16: return "Dimensity 1100";

        case "MT6893"_fnv1a16:
        case "MT6893Z/CZA"_fnv1a16:
        case "MT6893Z_A/CZA"_fnv1a16: return "Dimensity 1200";

        case "MT6893Z_Z/CZA"_fnv1a16:
        case "MT6893Z_T/CZA"_fnv1a16: return "Dimensity 1300";

        // Dimensity 6000 Series
        // note: Dimensity 6020 == Dimensity 700
        //       Dimensity 6080 == Dimensity 810
        case "MT6835"_fnv1a16:
        case "MT6835V/ZA"_fnv1a16:
        case "MT8755V/TZB"_fnv1a16: return "Dimensity 6100+";
        case "MT6835T"_fnv1a16:     return "Dimensity 6300";

        // Dimensity 7000 Series
        // note: Dimensity 7020 == Dimensity 930
        //       Dimensity 7030 == Dimensity 1050
        //       Dimensity 7050 == Dimensity 1080
        //       Dimensity 7300 == Dimensity 7300X
        case "MT6855V/ATZA"_fnv1a16: return "Dimensity 7025";
        case "MT6886V/TCZA"_fnv1a16: return "Dimensity 7350";

        case "MT6886"_fnv1a16:
        case "MT6886V_A/CZA"_fnv1a16:
        case "MT6886V_B/CZA"_fnv1a16: return "Dimensity 7200";

        case "MT6878"_fnv1a16:
        case "MT6878V/ZA"_fnv1a16:
        case "MT6878V_A/ZA"_fnv1a16: return "Dimensity 7300";

        // Dimensity 8000 Series
        // note: Dimensity 8020 == Dimensity 1100
        //       Dimensity 8050 == Dimensity 1300
        case "MT6895"_fnv1a16:      return "Dimensity 8000 / Dimensity 8100";
        case "MT6895Z/CZA"_fnv1a16: return "Dimensity 8000";

        // case "MT6895"_fnv1a16:
        case "MT6895Z/TCZA"_fnv1a16:
        case "MT6895Z_A/TCZA"_fnv1a16:
        case "MT6895Z_B/TCZA"_fnv1a16:
        case "MT6895ZB"_fnv1a16:
        case "MT8795"_fnv1a16:
        case "MT8795Z/TNZA"_fnv1a16:   return "Dimensity 8100";

        // case "MT6896"_fnv1a16:
        case "MT6896Z/CZA"_fnv1a16:
        case "MT6896Z_B/CZA"_fnv1a16: return "Dimensity 8200";

        case "MT6896"_fnv1a16:        return "Dimensity 8200 / Dimensity 8250";
        case "MT6896Z_C/CZA"_fnv1a16: return "Dimensity 8250";

        // case "MT6897"_fnv1a16:
        case "MT8792Z/NB"_fnv1a16: return "Dimensity 8300";

        case "MT6897"_fnv1a16:       return "Dimensity 8300 / Dimensity 8350";
        case "MT6897Z_A/ZA"_fnv1a16: return "Dimensity 8350";

        case "MT6899"_fnv1a16:
        case "MT6899Z_A/ZA"_fnv1a16: return "Dimensity 8400";

        // Dimensity 9000 Series
        case "MT6983"_fnv1a16:
        case "MT6983Z/CZA"_fnv1a16:
        case "MT8798"_fnv1a16:
        case "MT8798Z/CNZA"_fnv1a16: return "Dimensity 9000";

        // case "MT6983"_fnv1a16:
        // case "MT8798"_fnv1a16:
        case "MT6983W/CZA"_fnv1a16:
        case "MT8798Z/TNZA"_fnv1a16: return "Dimensity 9000+";

        case "MT6985"_fnv1a16:
        case "MT6985W/CZA"_fnv1a16: return "Dimensity 9200";

        // case "MT6985"_fnv1a16:
        case "MT6985W/TCZA"_fnv1a16: return "Dimensity 9200+";

        case "MT6989"_fnv1a16:
        case "MT6989W/CZA"_fnv1a16:
        case "MT8796"_fnv1a16:
        case "MT8796W/CNZA"_fnv1a16: return "Dimensity 9300";

        // case "MT6989"_fnv1a16:
        case "MT6989W/TCZA"_fnv1a16: return "Dimensity 9300+";

        case "MT6991"_fnv1a16:
        case "MT6991Z/CZA"_fnv1a16:
        case "MT6991W/CZA"_fnv1a16: return "Dimensity 9400";
    }

    return model_name;
}
#endif  // CF_ANDROID

static void trim(char* str)
{
    if (!str)
        return;

    // Trim leading space
    char* p = str;
    while (isspace((unsigned char)*p))
        ++p;
    memmove(str, p, strlen(p) + 1);

    // Trim trailing space
    p = str + strlen(str) - 1;
    while (p >= str && isspace((unsigned char)*p))
        --p;
    p[1] = '\0';
}

static bool read_value(const char* name, size_t n, bool do_rewind, char* buf, size_t buf_size)
{
    if (!cpuinfo || !buf || !buf_size)
        return false;
    if (do_rewind)
        rewind(cpuinfo);

    char*  line  = NULL;
    size_t len   = 0;
    bool   found = false;
    while (getline(&line, &len, cpuinfo) != -1)
    {
        if (strncmp(line, name, n))
            continue;

        char* colon = strchr(line, ':');
        if (!colon)
            continue;

        // Extract and trim value
        char* val = colon + 1;
        while (isspace((unsigned char)*val))
            ++val;
        trim(val);

        // Safe copy to buffer
        strncpy(buf, val, buf_size - 1);
        buf[buf_size - 1] = '\0';

        found = true;
        break;
    }

    free(line);
    return found;
}

float cpu_temp()
{
    for (const auto& dir : std::filesystem::directory_iterator{ "/sys/class/hwmon/" })
    {
        const std::string& name = read_by_syspath((dir.path() / "name").string());
        debug("name = {}", name);
        if (name != "cpu" && name != "k10temp" && name != "coretemp")
            continue;

        const std::string& temp_file = (access((dir.path() / "temp1_input").string().c_str(), F_OK) != 0)
                                           ? dir.path() / "device/temp1_input"
                                           : dir.path() / "temp1_input";
        if (access(temp_file.c_str(), F_OK) != 0)
            continue;

        const float ret = std::stof(read_by_syspath(temp_file));
        debug("cpu temp ret = {}", ret);

        return ret / 1000.0f;
    }
    return 0.0f;
}

#if CF_ANDROID
MODFUNC(android_cpu_model_name)
{ return get_android_property("ro.soc.model"); }

MODFUNC(android_cpu_vendor)
{
    if (android_cpu_model_name(nullptr).empty())
        return "MTK";

    std::string vendor = get_android_property("ro.soc.manufacturer");
    if (vendor.empty())
        vendor = get_android_property("ro.product.product.manufacturer");

    return vendor;
}
#endif

MODFUNC(cpu_name)
{
    char name[4096];
#if CF_LINUX
    if (!read_value("model name", "model name"_len, true, name, sizeof(name)))
        return UNKNOWN;
#elif CF_ANDROID
    if (!read_value("model name", "model name"_len, true, name, sizeof(name)))
    {
        const std::string& vendor = android_cpu_vendor(nullptr);
        const std::string& model_name = android_cpu_model_name(nullptr);
        if (vendor == "QTI" || vendor == "QUALCOMM")
            strcpy(name, fmt::format("Qualcomm {} [{}]", detect_qualcomm(model_name), model_name).c_str());
        else if (vendor == "Samsung")
            strcpy(name, fmt::format("Samsung {} [{}]", detect_exynos(model_name), model_name).c_str());
        else if (vendor == "MTK")
            strcpy(name, fmt::format("Mediatek {} [{}]", detect_mediatek(model_name), model_name).c_str());
        else
            strcpy(name, (vendor + " " + model_name).c_str());
    }
#endif
    // sometimes /proc/cpuinfo at model name
    // the name will contain the min freq
    // happens on intel cpus especially
    char* at = strrchr(name, '@');
    if (!at)
        return name;
    if (at > name && *(at - 1) == ' ')
        *(at - 1) = '\0';
    else
        *at = '\0';

    trim(name);
    return name;
}

MODFUNC(cpu_nproc)
{
    uint nproc = 0;
    rewind(cpuinfo);

    char*  line = NULL;
    size_t len  = 0;
    while (getline(&line, &len, cpuinfo) != -1)
    {
        if (strncmp(line, "processor", "processor"_len) == 0)
            nproc++;
    }
    free(line);
    return fmt::to_string(nproc);
}

MODFUNC(cpu_freq_cur)
{
    if (access((freq_dir + "/scaling_cur_freq").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/scaling_cur_freq")) / 1000000);
}

MODFUNC(cpu_freq_max)
{
    if (access((freq_dir + "/scaling_max_freq").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/scaling_max_freq")) / 1000000);
}

MODFUNC(cpu_freq_min)
{
    if (access((freq_dir + "/scaling_min_freq").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/scaling_min_freq")) / 1000000);
}

MODFUNC(cpu_freq_bios)
{
    if (access((freq_dir + "/bios_limit").c_str(), F_OK) != 0)
        return "0";
    return fmt::format("{:.2f}", std::stof(read_by_syspath(freq_dir + "/bios_limit")) / 1000000);
}

#endif
