package org.toni.customfetch_android_lib.query

import android.os.Build
import android.os.SystemClock
import java.lang.System.getProperty
import org.toni.customfetch_android_lib.UNKNOWN

class System {
    // DO NOT GET CONFUSED WITH "typealias SystemInfo"
    data class SystemInfo(
        var kernelName: String = UNKNOWN,
        var kernelVersion: String = UNKNOWN,
        var hostname: String = UNKNOWN,
        var arch: String = UNKNOWN,
        var uptime: Long = 0L,

        var osPrettyName: String = UNKNOWN,
        var osName: String = UNKNOWN,
        var osId: String = UNKNOWN,
        var osVersionId: String = UNKNOWN,
        var osVersionCodename: String = UNKNOWN,
        var osInitsysName: String = UNKNOWN,
        var osInitsysVersion: String = UNKNOWN,
        var hostModelname: String = UNKNOWN,
        var hostVersion: String = UNKNOWN,
        var hostVendor: String = UNKNOWN,
        var pkgsInstalled: String = UNKNOWN
    )

    companion object {
        private var mSystemInfos = SystemInfo()
        private var mBInit = false

        fun clearCache() { mBInit = false }
    }

    // System information
    val kernelName: String get() = mSystemInfos.kernelName
    val kernelVersion: String get() = mSystemInfos.kernelVersion
    val hostname: String get() = mSystemInfos.hostname
    val arch: String get() = mSystemInfos.arch

    // OS
    val osPrettyName: String get() = mSystemInfos.osPrettyName
    val osName: String get() = mSystemInfos.osName
    val osId: String get() = mSystemInfos.osId
    val osInitsysName: String get() = mSystemInfos.osInitsysName
    val osInitsysVersion: String get() = mSystemInfos.osInitsysVersion
    val osVersionId: String get() = mSystemInfos.osVersionId
    val osVersionCodename: String get() = mSystemInfos.osVersionCodename
    val uptime: Long get() = mSystemInfos.uptime

    // Host/motherboard
    val hostModelname: String get() = mSystemInfos.hostModelname
    val hostVendor: String get() = mSystemInfos.hostVendor
    val hostVersion: String get() = mSystemInfos.hostVersion

    init {
        if (!mBInit)
            mSystemInfos = getSystemInfos()
        mBInit = true
    }

    private fun getSystemInfos(): SystemInfo =
        SystemInfo(
            osName = "Android",
            osId = "android",
            kernelName = getProperty("os.name", null) ?: UNKNOWN,
            kernelVersion = getProperty("os.version", null) ?: UNKNOWN,
            hostname = "localhost",
            arch = Build.SUPPORTED_ABIS[0],
            uptime = SystemClock.uptimeMillis() / 1000,
            osVersionId = Build.VERSION.RELEASE,
            osVersionCodename = Build.VERSION.CODENAME,
            osPrettyName = "Android " + Build.VERSION.CODENAME + " " + Build.VERSION.RELEASE,
            hostModelname = Build.BOARD,
            hostVendor = Build.MANUFACTURER,
            hostVersion = Build.MODEL,
            osInitsysName = "init",
            osInitsysVersion = Build.BOOTLOADER,
        )
}