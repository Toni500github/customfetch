package org.toni.customfetch_android_lib.query

import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.BatteryManager
import org.toni.customfetch_android_lib.MAGIC_LINE

class Battery(context: Context) {
    data class BatteryInfo(
        var status: String = MAGIC_LINE,
        var technology: String = MAGIC_LINE,
        var temp: Double = 0.0,
        var perc: Double = 0.0,
    )

    companion object {
        private var mBatteryInfos = BatteryInfo()
        private var mBInit = false

        fun clearCache() { mBInit = false }
    }

    val status: String get() = mBatteryInfos.status
    val technology: String get() = mBatteryInfos.technology
    val temp: Double get() = mBatteryInfos.temp
    val perc: Double get() = mBatteryInfos.perc

    init {
        if (!mBInit)
            mBatteryInfos = getBatteryInfos(context)
        mBInit = true
    }

    private fun getBatteryInfos(context: Context): BatteryInfo {
        val ret = BatteryInfo()
        val batteryStatus = context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
        if (batteryStatus != null) {
            val level = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1)
            val scale = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1)
            if (level != -1 && scale != -1)
                ret.perc = level / scale.toDouble() * 100.0

            val temperature = getBatteryTemperature(context)
            if (temperature != null)
                ret.temp = temperature

            val technology = batteryStatus.getStringExtra(BatteryManager.EXTRA_TECHNOLOGY)
            if (!technology.isNullOrEmpty())
                ret.technology = technology

            val status = batteryStatus.getIntExtra(BatteryManager.EXTRA_STATUS, -1)
            if (status == BatteryManager.BATTERY_STATUS_CHARGING ||
                status == BatteryManager.BATTERY_STATUS_FULL) {
                val chargePlug = batteryStatus.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1)
                when (chargePlug) {
                    BatteryManager.BATTERY_PLUGGED_USB -> ret.status = "USB Connected"
                    BatteryManager.BATTERY_PLUGGED_AC -> ret.status = "AC Connected"
                    BatteryManager.BATTERY_PLUGGED_WIRELESS -> ret.status = "Wireless Connected"
                    BatteryManager.BATTERY_PLUGGED_DOCK -> ret.status = "Dock Connected"
                    else -> ret.status = "Discharging"
                }
            } else {
                ret.status = "Discharging"
            }
        }
        return ret
    }

    private fun getBatteryTemperature(context: Context): Double? {
        val batteryStatus = context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
        val temp = batteryStatus?.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, Int.MIN_VALUE)
        return if (temp != null && temp != Int.MIN_VALUE)
            temp / 10.0
        else
            null
    }
}