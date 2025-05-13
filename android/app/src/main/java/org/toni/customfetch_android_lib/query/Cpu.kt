package org.toni.customfetch_android_lib.query

import android.os.Build
import org.toni.customfetch_android_lib.UNKNOWN
import org.toni.customfetch_android_lib.getSystemProperty
import java.io.File
import java.nio.file.Files

class Cpu {
    data class CPUInfos(
        var name: String = UNKNOWN,
        var nproc: String = UNKNOWN,
        var freqCurrent: Double = 0.0,
        var freqMax: Double = 0.0,
        var freqMin: Double = 0.0,
        var temp: Double = 0.0,

        // private
        var modelName: String = "",
        var vendor: String = ""
    )

    val name: String get() = mCpuInfos.name
    val nproc: String get() = mCpuInfos.nproc
    val freqCurrent: Double get() = mCpuInfos.freqCurrent
    val freqMax: Double get() = mCpuInfos.freqMax
    val freqMin: Double get() = mCpuInfos.freqMin
    val temp: Double get() = mCpuInfos.temp

    init {
        if (!mBInit)
            mCpuInfos = getCpuInfos()
        mBInit = true
    }

    private fun getCpuInfos(): CPUInfos {
        val ret = CPUInfos()
        // temperature
        for (path in CPU_TEMP_FILE_PATHS) {
            val tempfile = File(path)
            if (!tempfile.exists())
                continue

            try {
                ret.temp = tempfile.bufferedReader().use { it.readLine().toDoubleOrNull() }?.let {
                    if (temp in -50.0..250.0)
                        it
                    else
                        it / 1000
                } ?: 0.toDouble()
            } catch (ignore: Exception) {
                continue
            }
        }

        // frequency
        try {
            File("$CPU_INFO_DIR/cpu0/cpufreq/scaling_cur_freq").bufferedReader().use {
                ret.freqCurrent = it.readLine().toDouble() / 1000000
            }
            File("$CPU_INFO_DIR/cpu0/cpufreq/scaling_max_freq").bufferedReader().use {
                ret.freqMax = it.readLine().toDouble() / 1000000
            }
            File("$CPU_INFO_DIR/cpu0/cpufreq/scaling_min_freq").bufferedReader().use {
                ret.freqMin = it.readLine().toDouble() / 1000000
            }
        } catch (_: Exception){}

        // n. of processors (also the name of CPU but we then need to use getSystemProperty)
        Files.readAllLines(File("/proc/cpuinfo").toPath()).forEach { line ->
            if (line.startsWith("model name"))
                ret.name = line.substring(line.indexOf(':')+1).trim()
            else if (line.startsWith("processor"))
                ret.nproc = line.substring(line.indexOf(':')+1).trim()
        }
        ret.nproc = (ret.nproc.toInt()+1).toString()

        // the name of CPU by the modelName
        if (ret.name == UNKNOWN) {
            ret.modelName = getSystemProperty("ro.soc.model", null) ?: "no"
            if (ret.modelName == "no") {
                ret.vendor = "MTK"
                ret.modelName = getSystemProperty("ro.mediatek.platform", null) ?: "no"
            }
            if (ret.vendor.isEmpty()) {
                ret.vendor = getSystemProperty("ro.soc.manufacturer", null) ?: "no"
                if (ret.vendor == "no")
                    ret.vendor = getSystemProperty("ro.product.product.manufacturer", null) ?: "no"
            }
            ret.name = when (ret.vendor) {
                "QTI", "QUALCOMM" -> "Qualcomm ${detectQualcomm(ret.modelName)} [${ret.modelName}]"
                "Samsung" -> "Samsung ${detectExynos(ret.modelName)} [${ret.modelName}]"
                "MTK" -> "Mediatek ${detectMediaTek(ret.modelName)} [${ret.modelName}]"
                else -> Build.MANUFACTURER + " " + ret.modelName
            }
        }
        return ret
    }

    companion object {
        private var mCpuInfos = CPUInfos()
        private var mBInit = false

        fun clearCache() { mBInit = false }

        // https://github.com/kamgurgul/cpu-info/blob/master/shared/src/androidMain/kotlin/com/kgurgul/cpuinfo/data/provider/TemperatureProvider.android.kt#L119
        private val CPU_TEMP_FILE_PATHS = listOf(
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
        )
        private const val CPU_INFO_DIR = "/sys/devices/system/cpu"
    }
}