package org.toni.customfetch_android_lib.query

import org.toni.customfetch_android_lib.MAGIC_LINE
import org.toni.customfetch_android_lib.ParseArgs
import org.toni.customfetch_android_lib.ParserFunctions.parse
import org.toni.customfetch_android_lib.SystemInfo
import org.toni.customfetch_android_lib.Variant
import org.toni.customfetch_android_lib.getInfoFromName
import java.io.File
import java.net.URI
import java.nio.file.FileStore
import java.nio.file.FileSystems

private fun isPhysicalDisk(store: FileStore): Boolean {
    return !(!store.name().startsWith("/dev/") || store.toString().startsWith("/apex/") ||
            (store.name().startsWith("/dev/loop") || store.name().startsWith("/dev/ram") || store.name().startsWith("/dev/fd")))
}

private fun formatAutoQueryString(str: String, store: FileStore, mntDir: String): String {
    str.replace("%1", mntDir)
    str.replace("%2", store.name())
    str.replace("%3", store.type())

    str.replace("%4", "\$<disk($mntDir).total>")
    str.replace("%5", "\$<disk($mntDir).free>")
    str.replace("%6", "\$<disk($mntDir).used>")
    str.replace("%7", "\$<disk($mntDir).used_perc>")
    str.replace("%8", "\$<disk($mntDir).free_perc>")
    return str
}

class Disk private constructor(
    path: String,
    queriedPaths: SystemInfo,
    parseArgs: ParseArgs,
    autoModule: Boolean = false
) {
    data class DiskInfo(
        var typefs: String = MAGIC_LINE,
        var device: String = MAGIC_LINE,
        var mountdir: String = MAGIC_LINE,
        var totalAmount: Double = 0.0,
        var freeAmount: Double = 0.0,
        var usedAmount: Double = 0.0,
        var typesDisk: Int = 0
    )

    companion object {
        private var mDiskInfos = DiskInfo()
        private val mDisksFormats = mutableListOf<String>()
        private val mQueriedDevices = mutableListOf<String>()

        fun getInstance(
            path: String,
            queriedPaths: SystemInfo,
            parseArgs: ParseArgs,
            autoModule: Boolean = false
        ): Disk {
            return Disk(path, queriedPaths, parseArgs, autoModule).apply {
                initialize(path, queriedPaths, parseArgs, autoModule)
            }
        }
    }

    // Properties (public getters)
    val totalAmount: Double get() = mDiskInfos.totalAmount
    val freeAmount: Double get() = mDiskInfos.freeAmount
    val usedAmount: Double get() = mDiskInfos.usedAmount
    val typesDisk: Int get() = mDiskInfos.typesDisk
    val typefs: String get() = mDiskInfos.typefs
    val device: String get() = mDiskInfos.device
    val mountdir: String get() = mDiskInfos.mountdir
    val disksFormats: List<String> get() = mDisksFormats

    private fun initialize(
        path: String,
        queriedPaths: SystemInfo,
        parseArgs: ParseArgs,
        autoModule: Boolean
    ) {
        // Check if already queried
        if (queriedPaths.containsKey(path)) {
            mDiskInfos.apply {
                device = getInfoFromName(queriedPaths, path, "device")
                mountdir = getInfoFromName(queriedPaths, path, "mountdir")
                typefs = getInfoFromName(queriedPaths, path, "typefs")
                totalAmount =
                    getInfoFromName(queriedPaths, path, "total_amount").toDoubleOrNull() ?: 0.0
                usedAmount =
                    getInfoFromName(queriedPaths, path, "used_amount").toDoubleOrNull() ?: 0.0
                freeAmount =
                    getInfoFromName(queriedPaths, path, "free_amount").toDoubleOrNull() ?: 0.0
            }
            return
        }

        if (!File(path).exists() && !autoModule) {
            mDiskInfos.apply {
                typefs = MAGIC_LINE
                device = MAGIC_LINE
                mountdir = MAGIC_LINE
            }
            return
        }

        if (autoModule) {
            for (store in FileSystems.getDefault().fileStores) {
                val mntDir = store.toString().substring(0, store.toString().indexOf(' ')) // /storage/emulated (/dev/fuse)
                if (!isPhysicalDisk(store))
                    continue
                if (!(parseArgs.config.autoDisk.displayTypes.contains("regular") && (mntDir == "/" || mntDir == "/storage/emulated")) ||
                     (parseArgs.config.autoDisk.displayTypes.contains("removable") && mntDir.startsWith("/mnt/media_rw/")) ||
                     (parseArgs.config.autoDisk.displayTypes.contains("hidden")))
                    continue

                if (!parseArgs.config.autoDisk.showDuplicated) {
                    if (mQueriedDevices.contains(store.type()))
                        continue
                    mQueriedDevices.add(store.type())
                }

                parseArgs.noMoreReset = false
                mDisksFormats.add(
                    parse(formatAutoQueryString(parseArgs.config.autoDisk.format, store, mntDir), parseArgs)
                )
            }
            return
        }

        if (!File(path).exists())
            return

        val store = FileSystems.getFileSystem(URI.create("file://$path")).fileStores.first()
        mDiskInfos.apply {
            typefs = store.type()
            device = store.name()
            mountdir = store.toString().substring(0, store.toString().indexOf(' ')) // /storage/emulated (/dev/fuse)
            totalAmount = store.totalSpace.toDouble()
            usedAmount = store.usableSpace.toDouble()
            freeAmount = totalAmount - usedAmount
        }

        // Cache results
        queriedPaths[path] = mutableMapOf(
            "total_amount" to Variant.DoubleVal(mDiskInfos.totalAmount),
            "used_amount" to Variant.DoubleVal(mDiskInfos.usedAmount),
            "free_amount" to Variant.DoubleVal(mDiskInfos.freeAmount),
            "typefs" to Variant.StringVal(mDiskInfos.typefs),
            "mountdir" to Variant.StringVal(mDiskInfos.mountdir),
            "device" to Variant.StringVal(mDiskInfos.device)
        )
    }
}