package org.toni.customfetch_android_lib.query

import android.text.SpannableStringBuilder
import org.toni.customfetch_android_lib.MAGIC_LINE
import org.toni.customfetch_android_lib.ParseArgs
import org.toni.customfetch_android_lib.ParserFunctions.parse
import org.toni.customfetch_android_lib.SystemInfo
import org.toni.customfetch_android_lib.Variant
import org.toni.customfetch_android_lib.getInfoFromNameStr
import org.toni.customfetch_android_lib.debug
import java.io.File
import java.nio.file.FileStore
import java.nio.file.FileSystems

// shl == <<
enum class DiskVolumeType(val value: Int) {
    HIDDEN(1 shl 2),
    REGULAR(1 shl 3),
    EXTERNAL(1 shl 4),
    READ_ONLY(1 shl 5),
    EXTERNAL_STORAGE(1 shl 6)
}

private fun isPhysicalDisk(store: FileStore): Boolean =
    !(store.toString().startsWith("/apex/") || !store.name().startsWith("/dev/")   ||
     (store.name().startsWith("/dev/loop")  || store.name().startsWith("/dev/ram") || store.name().startsWith("/dev/fd")))

private fun formatAutoQueryString(str: String, store: FileStore, mntDir: String): String {
    return str.replace("%1", mntDir)
        .replace("%2", store.name())
        .replace("%3", store.type())
        .replace("%4", "\$<disk($mntDir).total>")
        .replace("%5", "\$<disk($mntDir).free>")
        .replace("%6", "\$<disk($mntDir).used>")
        .replace("%7", "\$<disk($mntDir).used_perc>")
        .replace("%8", "\$<disk($mntDir).free_perc>")
}

private fun getTypesDisk(mntDir: String): Int {
    if (mntDir == "/" || mntDir == "/storage/emulated")
        return DiskVolumeType.REGULAR.value
    if (mntDir.startsWith("/mnt/media_rw/"))
        return DiskVolumeType.EXTERNAL.value
    // UUIDs on Android External drives are often 4–8 uppercase hex digits, a hyphen, then 4–8 more hex digits
    // e.g A1B2-C3D4 or 1234ABCD-5678EFGH
    if (mntDir.matches(Regex("^/storage/[A-F0-9]{4,8}-[A-F0-9]{4,8}$", RegexOption.IGNORE_CASE)))
        return DiskVolumeType.EXTERNAL_STORAGE.value

    return DiskVolumeType.HIDDEN.value
}

private fun getFileStore(path: String): FileStore {
    if (path != "/") {
        FileSystems.getDefault().fileStores.forEach {
            if (it.name() == path || it.toString().substring(0, it.toString().indexOf(' ')) == path)
                return it
        }
    }
    return FileSystems.getDefault().fileStores.first()
}

class Disk(
    path: String,
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
        private var mQueriedPaths: SystemInfo = mutableMapOf()
        private val mDisksFormats = mutableListOf<SpannableStringBuilder>()
        private val mQueriedDevices = mutableListOf<String>()

        fun clearCache() {
            mDiskInfos = DiskInfo()
            mDisksFormats.clear()
            mQueriedDevices.clear()
            mQueriedPaths.clear()
        }
    }

    init {
        initialize(path, parseArgs, autoModule)
    }

    // Properties (public getters)
    val totalAmount: Double get() = mDiskInfos.totalAmount
    val freeAmount: Double get() = mDiskInfos.freeAmount
    val usedAmount: Double get() = mDiskInfos.usedAmount
    val typesDisk: Int get() = mDiskInfos.typesDisk
    val typefs: String get() = mDiskInfos.typefs
    val device: String get() = mDiskInfos.device
    val mountdir: String get() = mDiskInfos.mountdir
    val disksFormats: List<SpannableStringBuilder> get() = mDisksFormats

    private fun initialize(
        path: String,
        parseArgs: ParseArgs,
        autoModule: Boolean
    ) {
        // Check if already queried
        if (mQueriedPaths.containsKey(path)) {
            mDiskInfos.apply {
                device = getInfoFromNameStr(mQueriedPaths, path, "device")
                mountdir = getInfoFromNameStr(mQueriedPaths, path, "mountdir")
                typefs = getInfoFromNameStr(mQueriedPaths, path, "typefs")
                totalAmount =
                    getInfoFromNameStr(mQueriedPaths, path, "total_amount").toDoubleOrNull() ?: 0.0
                usedAmount =
                    getInfoFromNameStr(mQueriedPaths, path, "used_amount").toDoubleOrNull() ?: 0.0
                freeAmount =
                    getInfoFromNameStr(mQueriedPaths, path, "free_amount").toDoubleOrNull() ?: 0.0
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
                if (!isPhysicalDisk(store))
                    continue

                var mntDir = store.toString().substring(0, store.toString().indexOf(' ')) // /storage/emulated (/dev/fuse)
                mDiskInfos.typesDisk = getTypesDisk(mntDir)
                if (mDiskInfos.typesDisk and DiskVolumeType.EXTERNAL_STORAGE.value != 0) {
                    mntDir.replaceFirst("/storage/", "/mnt/media_rw/").let {
                        if (File(it).exists()) {
                            mntDir = it
                        }
                        mDiskInfos.typesDisk = mDiskInfos.typesDisk or DiskVolumeType.EXTERNAL.value
                    }
                }
                debug("AUTO: Trying to query at path '$mntDir'")
                if ((parseArgs.config.autoDisk.displayTypesInt and mDiskInfos.typesDisk) == 0)
                    continue

                if (!parseArgs.config.autoDisk.showDuplicated) {
                    if (mQueriedDevices.contains(mntDir))
                        continue
                    mQueriedDevices.add(mntDir)
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

        debug("disk path = $path")
        val store = getFileStore(path)
        mDiskInfos.apply {
            typefs = store.type()
            device = store.name()
            mountdir = store.toString().substring(0, store.toString().indexOf(' ')) // /storage/emulated (/dev/fuse)
            totalAmount = store.totalSpace.toDouble()
            freeAmount = store.usableSpace.toDouble()
            usedAmount = totalAmount - freeAmount
        }

        // Cache results
        mQueriedPaths[path] = mutableMapOf(
            "total_amount" to Variant.DoubleVal(mDiskInfos.totalAmount),
            "used_amount" to Variant.DoubleVal(mDiskInfos.usedAmount),
            "free_amount" to Variant.DoubleVal(mDiskInfos.freeAmount),
            "typefs" to Variant.StringVal(mDiskInfos.typefs),
            "mountdir" to Variant.StringVal(mDiskInfos.mountdir),
            "device" to Variant.StringVal(mDiskInfos.device)
        )
    }
}
