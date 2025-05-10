package org.toni.customfetch_android_lib.query

import android.app.ActivityManager
import android.content.Context
import android.content.Context.ACTIVITY_SERVICE

class Ram private constructor(context: Context) {
    data class RamInfo(
        var totalAmount: Double = 0.0,
        var freeAmount: Double = 0.0,
        var usedAmount: Double = 0.0,
    )

    companion object {
        private var mRamInfos = RamInfo()
        private var mBInit = false

        // Singleton instance
        fun getInstance(context: Context): Ram {
            return Ram(context).apply {
                getRamInfos(context)
            }
        }
    }

    val totalAmount: Double get() = mRamInfos.totalAmount
    val freeAmount: Double get() = mRamInfos.freeAmount
    val usedAmount: Double get() = mRamInfos.usedAmount

    init {
        if (!mBInit)
            mRamInfos = getRamInfos(context)
        mBInit = true
    }

    private fun getRamInfos(context: Context): RamInfo {
        val memoryInfo = ActivityManager.MemoryInfo()
        val activityManager =  context.getSystemService(ACTIVITY_SERVICE) as ActivityManager?
        activityManager!!.getMemoryInfo(memoryInfo)
        return RamInfo(
            totalAmount = memoryInfo.totalMem.toDouble(),
            freeAmount = memoryInfo.availMem.toDouble(),
            usedAmount = totalAmount - freeAmount
        )
    }
}