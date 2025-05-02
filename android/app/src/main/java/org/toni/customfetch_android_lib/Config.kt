package org.toni.customfetch_android_lib

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@Serializable
data class ConfigTable(
    @SerialName("layout") val layout: List<String> = emptyList(),
    @SerialName("source-path") val sourcePath: String = "os",
    @SerialName("data-dir") val dataDir: String = "/data/user/0/org.toni.customfetch_android_app/files",
    @SerialName("ascii-logo-type") val asciiLogoType: String = "small",
    @SerialName("title-sep") val titleSep: String = "-",
    @SerialName("sep-reset") val sepReset: String = ":",
    @SerialName("sep-reset-after") val sepResetAfter: Boolean = false,
    @SerialName("logo-position") val logoPosition: String = "left",
    @SerialName("offset") val offset: Int = 5,
    @SerialName("logo-padding-left") val logoPaddingLeft: Int = 0,
    @SerialName("logo-padding-top") val logoPaddingTop: Int = 0,
    @SerialName("layout-padding-top") val layoutPaddingTop: Int = 0,
    @SerialName("wrap-lines") val wrapLines: Boolean = true,
    @SerialName("use-SI-byte-unit") val useSIByteUnit: Boolean = false,
    @SerialName("slow-query-warnings") val slowQueryWarnings: Boolean = false,

    // Colors (mapped directly)
    @SerialName("black") val black: String = "\u001B[1;30m",
    @SerialName("red") val red: String = "\u001B[1;31m",
    @SerialName("green") val green: String = "\u001B[1;32m",
    @SerialName("yellow") val yellow: String = "\u001B[1;33m",
    @SerialName("blue") val blue: String = "\u001B[1;34m",
    @SerialName("magenta") val magenta: String = "\u001B[1;35m",
    @SerialName("cyan") val cyan: String = "\u001B[1;36m",
    @SerialName("white") val white: String = "\u001B[1;37m",

    @SerialName("alias-colors") val aliasColors: List<String> = arrayListOf("purple=magenta"),
    @SerialName("percentage-colors") val percentageColors: List<String> = arrayListOf("green", "yellow", "red"),
)

// Sub-tables (nested configurations)
@Serializable
data class AutoDiskConfig(
    @SerialName("fmt") val format: String = "\${auto}Disk (%1): $<disk(%1)>",
    @SerialName("display-types") val displayTypes: List<String> = listOf("regular", "removable"),
    @SerialName("show-duplicated") val showDuplicated: Boolean = false
)

@Serializable
data class OsUptimeConfig(
    @SerialName("days") val daysFormat: String = " days",
    @SerialName("hours") val hoursFormat: String = " hours",
    @SerialName("mins") val minutesFormat: String = " mins",
    @SerialName("secs") val secondsFormat: String = " seconds"
)

@Serializable
data class OsPkgsConfig(
    @SerialName("pkg-managers") val pkgManagers: List<String> = listOf("pacman", "dpkg", "flatpak"),
    @SerialName("pacman-dirs") val pacmanDirs: List<String> = listOf("/var/lib/pacman/local/"),
    @SerialName("dpkg-files") val dpkgFiles: List<String> = listOf("/var/lib/dpkg/status", "/data/data/com.termux/files/usr/var/lib/dpkg/status"),
    @SerialName("flatpak-dirs") val flatpakDirs: List<String> = listOf("/var/lib/flatpak/app/", "~/.local/share/flatpak/app/"),
    @SerialName("apk-files") val apkFiles: List<String> = listOf("/var/lib/apk/db/installed")
)

@Serializable
data class GuiConfig(
    @SerialName("font") val font: String = "Monospace 12",
    @SerialName("black") val black: String = "!#000000",
    @SerialName("red") val red: String = "!#FF0000",
    @SerialName("green") val green: String = "!#00FF00",
    @SerialName("blue") val blue: String = "!#0000FF",
    @SerialName("cyan") val cyan: String = "!#00FFFF",
    @SerialName("yellow") val yellow: String = "!#FFFF00",
    @SerialName("magenta") val magenta: String = "!#FF00FF",
    @SerialName("white") val white: String = "!#FFFFFF",
    @SerialName("bg-image") val bgImage: String = "none"
)

@Serializable
data class Config(
    // Nested tables
    @SerialName("config") val t: ConfigTable = ConfigTable(),
    @SerialName("auto.disk") val autoDisk: AutoDiskConfig = AutoDiskConfig(),
    @SerialName("os.uptime") val osUptime: OsUptimeConfig = OsUptimeConfig(),
    @SerialName("os.pkgs") val osPkgs: OsPkgsConfig = OsPkgsConfig(),
    @SerialName("gui") val gui: GuiConfig = GuiConfig(),
)