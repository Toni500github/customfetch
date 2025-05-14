package org.toni.customfetch_android_lib

import android.content.Context
import android.util.Log
import android.widget.Toast
import java.io.File
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter
import java.util.Date
import kotlin.system.exitProcess


data class ByteUnits(val unit: String, val numBytes: Double)

const val CONFIG_DIR = "/storage/emulated/0/.config/customfetch"

private fun writeToFile(fmt: String) {
    val log = File("$CONFIG_DIR/log.txt")
    val diff: Long = Date().time - log.lastModified()
    if (diff > (10 * 24 * 60 * 60 * 1000L)) // 10 days in milliseconds
        log.delete()
    if (!log.exists())
        log.createNewFile()

    val currentDateTime = LocalDateTime.now()
    val formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss")
    val formattedDateTime = currentDateTime.format(formatter)
    log.appendText("[$formattedDateTime] $fmt\n")
}

internal fun info(fmt: String) {
    Log.i("customfetch-info", "info: $fmt")
    writeToFile("info: $fmt")
}

internal fun debug(fmt: String) {
    Log.d("customfetch-DEBUG", "[DEBUG]: $fmt")
    writeToFile("[DEBUG]: $fmt")
} 

internal fun warn(context: Context, fmt: String) {
    Log.e("customfetch-warn", "warning: $fmt")
    Toast.makeText(context, "warning: $fmt", Toast.LENGTH_SHORT).show()
    writeToFile("WARNING: $fmt")
}

internal fun error(context: Context, fmt: String) {
    Log.e("customfetch-error", "ERROR: $fmt")
    Toast.makeText(context, "ERROR: $fmt", Toast.LENGTH_SHORT).show()
    writeToFile("ERROR: $fmt")
}

internal fun die(context: Context, fmt: String) {
    Log.e("customfetch-FATAL", "FATAL: $fmt")
    Toast.makeText(context, "FATAL: $fmt", Toast.LENGTH_LONG).show()
    writeToFile("FATAL: $fmt")
    exitProcess(-1)
}

fun getSystemProperty(key: String, def: String?): String? {
    try {
        val systemPropertiesClass = Class.forName("android.os.SystemProperties")
        val getMethod = systemPropertiesClass.getMethod("get", String::class.java)
        return getMethod.invoke(systemPropertiesClass, key) as String
    } catch (e: Exception) {
        e.printStackTrace()
        return def
    }
}

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