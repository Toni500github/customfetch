package org.toni.customfetch_android_lib

data class ByteUnits(val unit: String, val numBytes: Double)

fun autoDivideBytes(num: Double, base: Int, maxPrefix: String = ""): ByteUnits {
    var size = num

    val prefixes = when (base) {
        1024 -> arrayOf("B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB")
        1000 -> arrayOf("B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
        else -> arrayOf("B")
    }

    var counter = 0
    if (maxPrefix.isEmpty()) {
        while (counter < prefixes.size && size >= base) {
            size /= base
            ++counter
        }
    } else {
        while (counter < prefixes.size && size >= base && prefixes[counter] != maxPrefix) {
            size /= base
            ++counter
        }
    }

    return ByteUnits(prefixes[counter], size)
}

fun divideBytes(num: Double, prefix: String): ByteUnits {
    if (prefix != "B") {
        // GiB
        // 012
        return if (prefix.length == 3 && prefix[1] == 'i')
            autoDivideBytes(num, 1024, prefix)
        else
            autoDivideBytes(num, 1000, prefix)
    }
    return autoDivideBytes(num, 0)
}