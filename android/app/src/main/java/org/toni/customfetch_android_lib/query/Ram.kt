package org.toni.customfetch_android_lib.query

import java.io.File
import java.nio.file.Files

class Ram {
    data class RamInfo(
        var totalAmount: Double = 0.0,
        var freeAmount: Double = 0.0,
        var usedAmount: Double = 0.0,
    )

    companion object {
        private var mRamInfos = RamInfo()
        private var mBInit = false

        fun clearCache() { mBInit = false }
    }

    val totalAmount: Double get() = mRamInfos.totalAmount
    val freeAmount: Double get() = mRamInfos.freeAmount
    val usedAmount: Double get() = mRamInfos.usedAmount

    init {
        if (!mBInit)
            mRamInfos = getRamInfos()
        mBInit = true
    }

    private fun getRamInfos(): RamInfo {
        val ret = RamInfo()
        Files.readAllLines(File("/proc/meminfo").toPath()).forEach { line ->
            if (line.startsWith("MemAvailable:"))
                ret.freeAmount = line.substring("MemAvailable: ".length, line.lastIndexOf(' ')).trim().toDouble()
            else if (line.startsWith("MemTotal:"))
                ret.totalAmount = line.substring("MemTotal: ".length, line.lastIndexOf(' ')).trim().toDouble()
        }
        ret.usedAmount = ret.totalAmount - ret.freeAmount
        return ret
    }
}