package org.toni.customfetch_android_lib.query

import java.io.File
import java.nio.file.Files

class Swap {
    data class SwapInfo(
        var totalAmount: Double = 0.0,
        var freeAmount: Double = 0.0,
        var usedAmount: Double = 0.0,
    )

    companion object {
        private var mSwapInfos = SwapInfo()
        private var mBInit = false

        fun clearCache() { mBInit = false }
    }

    val totalAmount: Double get() = mSwapInfos.totalAmount
    val freeAmount: Double get() = mSwapInfos.freeAmount
    val usedAmount: Double get() = mSwapInfos.usedAmount

    init {
        if (!mBInit)
            mSwapInfos = getSwapInfos()
        mBInit = true
    }

    private fun getSwapInfos(): SwapInfo {
        val ret = SwapInfo()
        Files.readAllLines(File("/proc/meminfo").toPath()).forEach { line ->
            if (line.startsWith("SwapFree:"))
                ret.freeAmount = line.substring("SwapFree: ".length, line.lastIndexOf(' ')).trim().toDouble()
            else if (line.startsWith("SwapTotal:"))
                ret.totalAmount = line.substring("SwapTotal: ".length, line.lastIndexOf(' ')).trim().toDouble()
        }
        ret.usedAmount = ret.totalAmount - ret.freeAmount
        return ret
    }
}