package org.toni.customfetch_android_lib

import android.content.Context
import kotlinx.serialization.Contextual
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import java.io.File

@Serializable
data class ConfigTable(
    @SerialName("layout") var layout: ArrayList<String> = arrayListOf(),
    @SerialName("source-path") var sourcePath: String = "os",
    @SerialName("data-dir") var dataDir: String = "/data/user/0/org.toni.customfetch_android/files",
    @SerialName("ascii-logo-type") var asciiLogoType: String = "small",
    @SerialName("title-sep") var titleSep: String = "-",
    @SerialName("sep-reset") var sepReset: String = ":",
    @SerialName("sep-reset-after") var sepResetAfter: Boolean = false,
    @SerialName("logo-position") var logoPosition: String = "left",
    @SerialName("offset") var offset: Int = 5,
    @SerialName("logo-padding-left") var logoPaddingLeft: Int = 0,
    @SerialName("logo-padding-top") var logoPaddingTop: Int = 0,
    @SerialName("layout-padding-top") var layoutPaddingTop: Int = 0,
    @SerialName("wrap-lines") var wrapLines: Boolean = true,
    @SerialName("use-SI-byte-unit") var useSIByteUnit: Boolean = false,
    @SerialName("slow-query-warnings") var slowQueryWarnings: Boolean = false,
    @SerialName("alias-colors") var aliasColors: ArrayList<String> = arrayListOf("purple=magenta"),
    @SerialName("percentage-colors") var percentageColors: ArrayList<String> = arrayListOf("green", "yellow", "red"),

    @Contextual
    var colorsName: ArrayList<String> = arrayListOf(),
    @Contextual
    var colorsValue: ArrayList<String> = arrayListOf()
)

// Sub-tables (nested configurations)
@Serializable
data class AutoDiskConfig(
    @SerialName("fmt") var format: String = "\${auto}Disk (%1): $<disk(%1)>",
    @SerialName("display-types") var displayTypes: ArrayList<String> = arrayListOf("regular", "removable"),
    @SerialName("show-duplicated") var showDuplicated: Boolean = false,
    @Contextual var displayTypesInt: Int = 0
)

@Serializable
data class OsUptimeConfig(
    @SerialName("days") var daysFormat: String = " days",
    @SerialName("hours") var hoursFormat: String = " hours",
    @SerialName("mins") var minutesFormat: String = " mins",
    @SerialName("secs") var secondsFormat: String = " seconds"
)

@Serializable
data class OsPkgsConfig(
    @SerialName("pkg-managers") var pkgManagers: ArrayList<String> = arrayListOf("pacman", "dpkg", "flatpak"),
    @SerialName("pacman-dirs") var pacmanDirs: ArrayList<String> = arrayListOf("/var/lib/pacman/local/"),
    @SerialName("dpkg-files") var dpkgFiles: ArrayList<String> = arrayListOf("/var/lib/dpkg/status", "/data/data/com.termux/files/usr/var/lib/dpkg/status"),
    @SerialName("flatpak-dirs") var flatpakDirs: ArrayList<String> = arrayListOf("/var/lib/flatpak/app/", "~/.local/share/flatpak/app/"),
    @SerialName("apk-files") var apkFiles: ArrayList<String> = arrayListOf("/var/lib/apk/db/installed")
)

@Serializable
data class GuiConfig(
    @SerialName("font") var font: String = "Monospace 12",
    @SerialName("black") var black: String = "!#000000",
    @SerialName("red") var red: String = "!#FF0000",
    @SerialName("green") var green: String = "!#00FF00",
    @SerialName("blue") var blue: String = "!#0000FF",
    @SerialName("cyan") var cyan: String = "!#00FFFF",
    @SerialName("yellow") var yellow: String = "!#FFFF00",
    @SerialName("magenta") var magenta: String = "!#FF00FF",
    @SerialName("white") var white: String = "!#FFFFFF",
    @SerialName("bg-image") var bgImage: String = "none"
)

data class ArgsConfig(
    var layout: ArrayList<String> = arrayListOf(),
    var customDistro: String = "",
    var disableSource: Boolean = false,
    var disableColors: Boolean = false,
    var printLogoOnly: Boolean = false
)

@Serializable
data class Config(
    @Contextual
    val args: ArgsConfig = ArgsConfig(),
    // Nested tables
    @SerialName("config") val t: ConfigTable = ConfigTable(),
    @SerialName("auto.disk") val autoDisk: AutoDiskConfig = AutoDiskConfig(),
    @SerialName("os.uptime") val osUptime: OsUptimeConfig = OsUptimeConfig(),
    @SerialName("os.pkgs") val osPkgs: OsPkgsConfig = OsPkgsConfig(),
    @SerialName("gui") val gui: GuiConfig = GuiConfig(),
)

fun generateConfig(file: File) {
    //if (file.exists())
    file.writeText(AUTOCONFIG)
}

fun addAliasColor(context: Context, str: String, config: Config) {
    val pos = str.indexOf('=')
    if (pos == -1)
        die(context, "alias color '{}' does NOT have an equal sign '=' for separating config name and value\n" +
                "For more check with --help")
    val name = str.substring(0, pos)
    val value = str.substring(pos + 1)

    config.t.colorsName.add(name)
    config.t.colorsValue.add(value)
}

fun overrideOption(context: Context, opt: String, config: Config) {
    val pos = opt.indexOf('=')
    if (pos == -1)
        die(context, "override option '{}' does NOT have an equal sign '=' for separating config name and value\n" +
                "For more check with --help")
    var name = opt.substring(0, pos)
    val value = opt.substring(pos + 1)
    // usually the user finds inconvenient to write "config.foo"
    // for general config options
    if (name.indexOf('.') == -1)
        name = "config.$name"

    when (name) {
        "config.source-path" -> config.t.sourcePath = value
        "config.data-dir" -> config.t.dataDir = value
        "config.ascii-logo-type" -> config.t.asciiLogoType = value
        "config.title-sep" -> config.t.titleSep = value
        "config.sep-reset" -> config.t.sepReset = value
        "config.sep-reset-after" -> config.t.sepResetAfter = strToBool(value)
        "config.logo-position" -> config.t.logoPosition = value
        "config.offset" -> config.t.offset = value.toInt()
        "config.logo-padding-left" -> config.t.logoPaddingLeft = value.toInt()
        "config.logo-padding-top" -> config.t.logoPaddingTop = value.toInt()
        "config.layout-padding-top" -> config.t.layoutPaddingTop = value.toInt()
        "config.wrap-lines" -> config.t.wrapLines = strToBool(value)
        "config.use-SI-byte-unit" -> config.t.useSIByteUnit = strToBool(value)
        "config.slow-query-warnings" -> config.t.slowQueryWarnings = strToBool(value)

        "auto.disk.fmt" -> config.autoDisk.format = value
        "auto.disk.show-duplicated" -> config.autoDisk.showDuplicated = strToBool(value)

        "os.uptime.days" -> config.osUptime.daysFormat = value
        "os.uptime.hours" -> config.osUptime.hoursFormat = value
        "os.uptime.mins" -> config.osUptime.minutesFormat = value
        "os.uptime.secs" -> config.osUptime.secondsFormat = value

        "gui.font" -> config.gui.font = value
        "gui.black" -> config.gui.black = value
        "gui.red" -> config.gui.red = value
        "gui.green" -> config.gui.green = value
        "gui.blue" -> config.gui.blue = value
        "gui.cyan" -> config.gui.cyan = value
        "gui.yellow" -> config.gui.yellow = value
        "gui.magenta" -> config.gui.magenta = value
        "gui.white" -> config.gui.white = value
        "gui.bg-image" -> config.gui.bgImage = value

        else -> die(context, "Unknown config property: $name")
    }
}