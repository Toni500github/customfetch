package org.toni.customfetch_android_lib.query

// https://en.wikipedia.org/wiki/List_of_Qualcomm_Snapdragon_systems_on_chips
// by Toni500git
fun detectQualcomm(modelName: String): String {
    return when (modelName.uppercase()) {
        "SM8750-AB" -> "Snapdragon 8 Elite"
        "SM8635" -> "Snapdragon 8s Gen 3"
        "SM8650-AC" -> "Snapdragon 8 Gen 3 for Galaxy"
        "SM8650" -> "Snapdragon 8 Gen 3"
        "SM8550-AC" -> "Snapdragon 8 Gen 2 for Galaxy"
        "SM8550" -> "Snapdragon 8 Gen 2"
        "SM8475" -> "Snapdragon 8+ Gen 1"
        "SM8450" -> "Snapdragon 8 Gen 1"

        "SM7450-AB" -> "Snapdragon 7 Gen 1"
        "SM7475-AB" -> "Snapdragon 7+ Gen 2"
        "SM7435-AB" -> "Snapdragon 7s Gen 2"
        "SM7550-AB" -> "Snapdragon 7 Gen 3"
        "SM7675-AB" -> "Snapdragon 7+ Gen 3"
        "SM7635" -> "Snapdragon 7s Gen 3"

        "SM6375-AC" -> "Snapdragon 6s Gen 3"
        "SM6475-AB" -> "Snapdragon 6 Gen 3"
        "SM6115-AC" -> "Snapdragon 6s Gen 1"
        "SM6450" -> "Snapdragon 6 Gen 1"

        "SM4635" -> "Snapdragon 4s Gen 2"
        "SM4450" -> "Snapdragon 4 Gen 2"
        "SM4375" -> "Snapdragon 4 Gen 1"

        // adding fastfetch's many missing chips names
        "MSM8225Q",
        "MSM8625Q",
        "MSM8210",
        "MSM8610",
        "MSM8212",
        "MSM8612" -> "Snapdragon 200"

        "MSM8905" -> "205"  // Qualcomm 205
        "MSM8208" -> "Snapdragon 208"
        "MSM8909" -> "Snapdragon 210"
        "MSM8909AA" -> "Snapdragon 212"
        "QM215" -> "215"

        // holy crap
        "APQ8026",
        "MSM8226",
        "MSM8926",
        "APQ8028",
        "MSM8228",
        "MSM8628",
        "MSM8928",
        "MSM8230",
        "MSM8630",
        "MSM8930",
        "MSM8930AA",
        "APQ8030AB",
        "MSM8230AB",
        "MSM8630AB",
        "MSM8930AB" -> "Snapdragon 400"

        "APQ8016",
        "MSM8916" -> "Snapdragon 410"
        "MSM8929" -> "Snapdragon 415"
        "MSM8917" -> "Snapdragon 425"
        "MSM8920" -> "Snapdragon 427"
        "MSM8937" -> "Snapdragon 430"
        "MSM8940" -> "Snapdragon 435"
        "SDM429" -> "Snapdragon 429"
        "SDM439" -> "Snapdragon 439"
        "SDM450" -> "Snapdragon 450"
        "SM4250-AA" -> "Snapdragon 460"
        "SM4350" -> "Snapdragon 480"
        "SM4350-AC" -> "Snapdragon 480+"

        "APQ8064-1AA",
        "APQ8064M",
        "APQ8064T",
        "APQ8064AB" -> "Snapdragon 600"

        "MSM8936" -> "Snapdragon 610"
        "MSM8939" -> "Snapdragon 615"
        "MSM8952" -> "Snapdragon 617"
        "MSM8953" -> "Snapdragon 625"
        "SDM630" -> "Snapdragon 630"
        "SDM632" -> "Snapdragon 632"
        "SDM636" -> "Snapdragon 636"
        "MSM8956" -> "Snapdragon 650"
        "MSM8976" -> "Snapdragon 652"
        "SDA660",
        "SDM660" -> "Snapdragon 660"
        "SM6115" -> "Snapdragon 662"
        "SM6125" -> "Snapdragon 665"
        "SDM670" -> "Snapdragon 670"
        "SM6150" -> "Snapdragon 675"
        "SM6150-AC" -> "Snapdragon 678"
        "SM6225" -> "Snapdragon 680"
        "SM6225-AD" -> "Snapdragon 685"
        "SM6350" -> "Snapdragon 690"
        "SM6375" -> "Snapdragon 695"

        "SDM710" -> "Snapdragon 710"
        "SDM712" -> "Snapdragon 712"
        "SM7125" -> "Snapdragon 720G"
        "SM7150-AA" -> "Snapdragon 730"
        "SM7150-AB" -> "Snapdragon 730G"
        "SM7150-AC" -> "Snapdragon 732G"
        "SM7225" -> "Snapdragon 750G"
        "SM7250-AA" -> "Snapdragon 765"
        "SM7250-AB" -> "Snapdragon 765G"
        "SM7250-AC" -> "Snapdragon 768G"
        "SM7325" -> "Snapdragon 778G"
        "SM7325-AE" -> "Snapdragon 778G+"
        "SM7350-AB" -> "Snapdragon 780G"
        "SM7325-AF" -> "Snapdragon 782G"

        "APQ8074AA",
        "MSM8274AA",
        "MSM8674AA",
        "MSM8974AA",
        "MSM8274AB" -> "Snapdragon 800"

        "APQ8084" -> "Snapdragon 805"
        "MSM8992" -> "Snapdragon 808"
        "MSM8994" -> "Snapdragon 810"
        "MSM8996" -> "Snapdragon 820"
        "MSM8998" -> "Snapdragon 835"
        "SDM845" -> "Snapdragon 845"
        "SM8150" -> "Snapdragon 855"
        "SM8150P",
        "SM8150-AC" -> "Snapdragon 855+"  // both 855+ and 860
        "SM8250" -> "Snapdragon 865"
        "SM8250-AB" -> "Snapdragon 865+"
        "SM8250-AC" -> "Snapdragon 870"
        "SM8350" -> "Snapdragon 888"
        "SM8350-AC" -> "Snapdragon 888+"

        else -> modelName
    }
}

// https://en.wikipedia.org/wiki/Exynos#Current_Exynos_SoCs_(2020-present)
// by BurntRanch
fun detectExynos(modelName: String): String {
    return when (modelName.uppercase()) {
        "S5E3830" -> "Exynos 850"
        "S5E8805" -> "Exynos 880"

        "S5E9630" -> "Exynos 980"
        "S5E9830" -> "Exynos 990"

        "S5E9815" -> "Exynos 1080"
        "S5E8825" -> "Exynos 1280"
        "S5E8535" -> "Exynos 1330"
        "S5E8835" -> "Exynos 1380"
        "S5E8845" -> "Exynos 1480"
        "S5E8855" -> "Exynos 1580"
        /* TODO: alot of SoCs with no ID mentioned on Wikipedia.. */

        "S5E9840" -> "Exynos 2100"
        "S5E9925" -> "Exynos 2200"
        "S5E9945" -> "Exynos 2400[e]"

        "S5PC110" -> "Exynos 3 Single 3110"
        "S5E3470" -> "Exynos 3 Quad 3470"
        /* TODO: ASSUMPTION!! I could not find the full part number for this, I'm making an assumption here. */
        "S5E3475" -> "Exynos 3 Quad 3475"

        "S5E4210",
        "S5PC210" -> "Exynos 4 Dual 4210"

        "S5E4212" -> "Exynos 4 Dual 4212"

        "S55E4210",
        "S5PC220" -> "Exynos 4 Quad 4412"

        /* TODO: Exynos 4 Quad 4415 */

        "S5E5250",
        "S5PC520" -> "Exynos 5 Dual 5250"

        "S5E5260" -> "Exynos 5 Hexa 5260"
        "S5E5410" -> "Exynos 5 Octa 5410"
        "S5E5420" -> "Exynos 5 Octa 5420"

        /* 5800 for chromebooks */
        "S5E5422" -> "Exynos 5 Octa 5422/5800"

        "S5E5430" -> "Exynos 5 Octa 5430"
        "S5E5433" -> "Exynos 7 Octa 5433"
        "SC57270" -> "Exynos 7 Dual 7270"
        "S5E7420" -> "Exynos 7 Octa 7420"
        "S5E7570" -> "Exynos 7 Quad 7570"
        /* TODO: Exynos 7 Quad/Octa 7578/7580 */

        "S5E7870" -> "Exynos 7 Octa 7870"
        "S5E7872" -> "Exynos 5 7872"
        "S5E7880" -> "Exynos 7880"
        "S5E7884" -> "Exynos 7884/7885"
        "S5E7904" -> "Exynos 7904"
        "S5E8890" -> "Exynos 8 Octa 8890"
        "S5E8895" -> "Exynos 8895"
        "S5E9609" -> "Exynos 9609"
        "S5E9610" -> "Exynos 9610"
        "S5E9611" -> "Exynos 9611"
        "S5E9810" -> "Exynos 9810"
        "S5E9820" -> "Exynos 9820"
        "S5E9825" -> "Exynos 9825"
        /* TODO: Exynos 3 Dual 3250 */

        "SC59110XSC" -> "Exynos 9110"
        "SC55515XBD" -> "Exynos W920"
        "SC55515XBE" -> "Exynos W930"
        "SC55535AHA" -> "Exynos W1000"

        else -> modelName
    }
}

// https://en.wikipedia.org/wiki/List_of_MediaTek_systems_on_chips
// by Toni500git
// i need some coffee.....
// wayland crashed and had to restart from Dimensity 1000 series.........
fun detectMediaTek(modelName: String): String {
    return when (modelName) {
        // Helio series
        // Helio X series
        "MT6795",
        "MT6795M",
        "MT6795T" -> "Helio X10"

        "MT6797" -> "Helio X20"
        "MT6797D" -> "Helio X23"
        "MT6797X" -> "Helio X27"
        "MT6799" -> "Helio X30"

        // Helio A series
        "MT6761V/WE" -> "Helio A20"
        "MT6761V/WBB",
        "MT6761V/WAB" -> "Helio A22"
        "MT6762V/WD",
        "MT6762V/WB" -> "Helio A25"

        // Helio P series
        "MT6755",
        "MT6755M" -> "Helio P10"
        "MT6755T" -> "Helio P15"
        "MT6755S" -> "Helio P18"
        "MT6757" -> "Helio P20"
        "MT6762" -> "Helio P22"
        "MT6763" -> "Helio P23"
        "MT6763T" -> "Helio P23T"
        "MT6757CD" -> "Helio P25"
        "MT6757T" -> "Helio P25T"
        "MT6758" -> "Helio P30"
        "MT6765" -> "Helio P35"
        "MT6771" -> "Helio P60"
        "MT6768" -> "Helio P65"
        "MT6771V/CT" -> "Helio P70"
        "MT6779V/CU" -> "Helio P90"
        "MT6779V/CV" -> "Helio P95"

        // Helio G series
        "MT6762G" -> "Helio G25"
        "MT6765G" -> "Helio G35"
        "MT6765V/XAA",
        "MT6765V/XBA" -> "Helio G36"
        "MT6765H" -> "Helio G37"
        "MT6765V" -> "Helio G50"
        "MT6769V/CB" -> "Helio G70"

        "MT6769T",
        "MT6769V/CT",
        "MT6769V/CU" -> "Helio G80"

        "MT6769J" -> "Helio G81"
        "MT6769Z",
        "MT6769V/CZ" -> "Helio G85"
        "MT6769H" -> "Helio G88"
        "MT6785V/CC" -> "Helio G90T"
        "MT6769G" -> "Helio G91"
        "MT6769I" -> "Helio G92"
        "MT6785V/CD" -> "Helio G95"
        "MT6781",
        "MT6781V/CD" -> "Helio G96"
        "MT6789H" -> "Helio G100"

        "MT6789",
        "MT6789G",
        "MT6789U",
        "MT6789V/CD",
        "MT8781",
        "MT8781V/CA",
        "MT8781V/NB" -> "Helio G99"

        // Dimensity Series
        // Dimensity 700 Series
        "MT6833",
        "MT6833G",
        "MT6833V/ZA",
        "MT6833V/NZA" -> "Dimensity 700"

        "MT6853V/ZA",
        "MT6853V/NZA" -> "Dimensity 720"

        // Dimensity 800 Series
        "MT6873" -> "Dimensity 800"
        "MT6853T",
        "MT6853V/TNZA" -> "Dimensity 800U"
        "MT6875" -> "Dimensity 820"

        "MT6833P",
        "MT6833GP",
        "MT6833V/PNZA" -> "Dimensity 810"

        // Dimensity 900 Series
        "MT6877V/ZA" -> "Dimensity 900"
        "MT6855",
        "MT6855V/AZA" -> "Dimensity 930"

        "MT6877T",
        "MT6877V/TZA" -> "Dimensity 920"

        // Dimensity 1000 Series
        "MT6883Z/CZA" -> "Dimensity 1000C"
        "MT6885Z/CZA" -> "Dimensity 1000L"
        "MT6889" -> "Dimensity 1000"
        "MT6889Z/CZA" -> "Dimensity 1000+"

        "MT6879",
        "MT6879V/ZA",
        "MT6879V_T/ZA" -> "Dimensity 1050"

        "MT6877" -> "Dimensity 1080 / Dimensity 920"
        "MT6877V/TTZA",
        "MT6877V_T/TTZA" -> "Dimensity 1080"

        "MT6891",
        "MT6891Z/CZA",
        "MT6891Z_Z/CZA",
        "MT6891Z_T/CZA" -> "Dimensity 1100"

        "MT6893",
        "MT6893Z/CZA",
        "MT6893Z_A/CZA" -> "Dimensity 1200"

        "MT6893Z_Z/CZA",
        "MT6893Z_T/CZA" -> "Dimensity 1300"

        // Dimensity 6000 Series
        "MT6835",
        "MT6835V/ZA",
        "MT8755V/TZB" -> "Dimensity 6100+"
        "MT6835T" -> "Dimensity 6300"

        // Dimensity 7000 Series
        "MT6855V/ATZA" -> "Dimensity 7025"
        "MT6886V/TCZA" -> "Dimensity 7350"

        "MT6886",
        "MT6886V_A/CZA",
        "MT6886V_B/CZA" -> "Dimensity 7200"

        "MT6878",
        "MT6878V/ZA",
        "MT6878V_A/ZA" -> "Dimensity 7300"

        // Dimensity 8000 Series
        "MT6895" -> "Dimensity 8000 / Dimensity 8100"
        "MT6895Z/CZA" -> "Dimensity 8000"

        "MT6895Z/TCZA",
        "MT6895Z_A/TCZA",
        "MT6895Z_B/TCZA",
        "MT6895ZB",
        "MT8795",
        "MT8795Z/TNZA" -> "Dimensity 8100"

        "MT6896Z/CZA",
        "MT6896Z_B/CZA" -> "Dimensity 8200"

        "MT6896" -> "Dimensity 8200 / Dimensity 8250"
        "MT6896Z_C/CZA" -> "Dimensity 8250"

        "MT8792Z/NB" -> "Dimensity 8300"

        "MT6897" -> "Dimensity 8300 / Dimensity 8350"
        "MT6897Z_A/ZA" -> "Dimensity 8350"

        "MT6899",
        "MT6899Z_A/ZA" -> "Dimensity 8400"

        // Dimensity 9000 Series
        "MT6983",
        "MT6983Z/CZA",
        "MT8798",
        "MT8798Z/CNZA" -> "Dimensity 9000"

        "MT6983W/CZA",
        "MT8798Z/TNZA" -> "Dimensity 9000+"

        "MT6985",
        "MT6985W/CZA" -> "Dimensity 9200"

        "MT6985W/TCZA" -> "Dimensity 9200+"

        "MT6989",
        "MT6989W/CZA",
        "MT8796",
        "MT8796W/CNZA" -> "Dimensity 9300"

        "MT6989W/TCZA" -> "Dimensity 9300+"

        "MT6991",
        "MT6991Z/CZA",
        "MT6991W/CZA" -> "Dimensity 9400"

        else -> modelName
    }
}