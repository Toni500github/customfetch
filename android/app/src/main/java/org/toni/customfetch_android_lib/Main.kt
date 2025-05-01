package org.toni.customfetch_android_lib

import net.peanuuutz.tomlkt.Toml
import java.io.File
import kotlinx.serialization.decodeFromString
import java.nio.file.Paths
import kotlin.io.path.readText

fun main(args: Array<String>) {
    val toml = Toml {
        ignoreUnknownKeys = true
    }
    val tomlConfig = Paths.get("/tmp/taur/config.toml").readText()
            .replace("\\e", "\\u001B") // Escape ANSI codes
    val config = toml.decodeFromString(Config.serializer(), tomlConfig)

    println("Config source-path: ${config.t.sourcePath}")
    println("Config offset: ${config.t.offset}")
    println("Disk display types: ${config.autoDisk.displayTypes}")
    println("Uptime days format: ${config.osUptime.daysFormat}")
    println("Package managers: ${config.osPkgs.pkgManagers}")
    println("GUI font: ${config.gui.font}")

    File("/tmp/taur/te.txt").writeText(render(
        config, File("/usr/share/customfetch/ascii/arch.txt")).joinToString(separator = "\n"))
}