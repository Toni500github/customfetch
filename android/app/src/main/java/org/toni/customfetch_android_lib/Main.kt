package org.toni.customfetch_android_lib

import android.content.Context
import android.text.SpannableStringBuilder
import gnu.getopt.Getopt
import gnu.getopt.LongOpt
import kotlinx.serialization.SerializationException
import net.peanuuutz.tomlkt.Toml
import org.toni.customfetch_android.BuildConfig
import org.toni.customfetch_android_lib.query.Battery
import org.toni.customfetch_android_lib.query.Cpu
import org.toni.customfetch_android_lib.query.Disk
import org.toni.customfetch_android_lib.query.DiskVolumeType
import org.toni.customfetch_android_lib.query.Gpu
import org.toni.customfetch_android_lib.query.Ram
import org.toni.customfetch_android_lib.query.Swap
import org.toni.customfetch_android_lib.query.System
import java.io.File
import java.nio.file.Paths
import kotlin.io.path.readText

private fun version(): String =
    "customfetch v${BuildConfig.VERSION_NAME} (${BuildConfig.GIT_COMMIT_HASH}) (${BuildConfig.BUILD_TYPE})"

fun strToBool(string: String): Boolean =
    (string == "true" || string == "1" || string == "enable")

private fun handleOptional(optarg: String?, fallback: String): String =
    optarg ?: fallback

private fun parseConfigArg(context: Context, args: Array<String>): String {
    val longOpts = arrayOf(
        LongOpt("config", LongOpt.REQUIRED_ARGUMENT, null, 'C'.code)
    )
    val g = Getopt(args[0], args, "-C:", longOpts)
    g.setOpterr(false)

    var c: Int
    while ((g.getopt().also { c = it }) != -1) {
        when (c) {
            'C'.code -> {
                if (!File(g.optarg).exists())
                    die(context, "No such config file ${g.optarg}")
                return g.optarg
            }
        }
    }

    return "$CONFIG_DIR/config.toml"
}

private fun listLogos(dataDir: String): String {
    val asciiDir = File(dataDir, "ascii")
    if (!asciiDir.isDirectory)
        return ""

    return asciiDir.listFiles()
        ?.filter { it.isFile }
        ?.map { it.nameWithoutExtension }
        ?.sorted()
        ?.joinToString("\n")
        ?: ""
}

private fun parseArgs(context: Context, args: Array<String>, config: Config): String {
    val longOpts = arrayOf(
        LongOpt("version", LongOpt.NO_ARGUMENT, null, 'V'.code),
        LongOpt("help", LongOpt.NO_ARGUMENT, null, 'h'.code),
        LongOpt("list-modules", LongOpt.NO_ARGUMENT, null, 'l'.code),
        LongOpt("how-it-works", LongOpt.NO_ARGUMENT, null, 'w'.code),
        LongOpt("logo-only", LongOpt.REQUIRED_ARGUMENT, null, 'L'.code),
        LongOpt("no-logo", LongOpt.REQUIRED_ARGUMENT, null, 'n'.code),
        LongOpt("no-color", LongOpt.REQUIRED_ARGUMENT, null, 'N'.code),
        LongOpt("ascii-logo-type", LongOpt.REQUIRED_ARGUMENT, null, 'a'.code),
        LongOpt("offset", LongOpt.REQUIRED_ARGUMENT, null, 'o'.code),
        LongOpt("override", LongOpt.REQUIRED_ARGUMENT, null, 'O'.code),
        LongOpt("font", LongOpt.REQUIRED_ARGUMENT, null, 'f'.code),
        LongOpt("config", LongOpt.REQUIRED_ARGUMENT, null, 'C'.code),
        LongOpt("layout-line", LongOpt.REQUIRED_ARGUMENT, null, 'm'.code),
        LongOpt("logo-position", LongOpt.REQUIRED_ARGUMENT, null, 'p'.code),
        LongOpt("data-dir", LongOpt.REQUIRED_ARGUMENT, null, 'D'.code),
        LongOpt("distro", LongOpt.REQUIRED_ARGUMENT, null, 'd'.code),
        LongOpt("source-path", LongOpt.REQUIRED_ARGUMENT, null, 's'.code),

        LongOpt("list-logos", LongOpt.NO_ARGUMENT, null, 1000),
        LongOpt("sep-reset-after", LongOpt.OPTIONAL_ARGUMENT, null, 1001),
        LongOpt("wrap-lines", LongOpt.OPTIONAL_ARGUMENT, null, 1002),
        LongOpt("gen-config", LongOpt.OPTIONAL_ARGUMENT, null, 1003),
        LongOpt("sep-reset", LongOpt.REQUIRED_ARGUMENT, null, 1004),
        LongOpt("title-sep", LongOpt.REQUIRED_ARGUMENT, null, 1005),
        LongOpt("logo-padding-top", LongOpt.REQUIRED_ARGUMENT, null, 1006),
        LongOpt("logo-padding-left", LongOpt.REQUIRED_ARGUMENT, null, 1007),
        LongOpt("layout-padding-top", LongOpt.REQUIRED_ARGUMENT, null, 1008),
        //LongOpt("loop-ms", LongOpt.REQUIRED_ARGUMENT, null, 1009),
        //LongOpt("bg-image", LongOpt.REQUIRED_ARGUMENT, null, 1010),
        LongOpt("color", LongOpt.REQUIRED_ARGUMENT, null, 1011),

        LongOpt(null, 0, null, 0)
    )

    val g = Getopt(args[0], args, "-VhlwLnNa:o:O:f:C:m:p:D:d:s:i:", longOpts)
    g.setOpterr(true)

    try {
        var c: Int
        while ((g.getopt().also { c = it }) != -1) {
            when (c) {
                '?'.code, 'h'.code -> return help()
                'V'.code -> return version()
                'l'.code -> return modulesList()
                'w'.code -> return howItWorks()
                'f'.code -> config.gui.font = g.optarg
                'o'.code -> config.t.offset = g.optarg.toInt()
                'O'.code -> overrideOption(context, g.optarg, config)
                'D'.code -> config.t.dataDir = g.optarg
                'd'.code -> config.args.customDistro = g.optarg.lowercase()
                'm'.code -> config.args.layout.add(g.optarg)
                'p'.code -> config.t.logoPosition = g.optarg
                's'.code -> config.t.sourcePath = g.optarg
                'N'.code -> config.args.disableColors = true
                'a'.code -> config.t.asciiLogoType = g.optarg
                'n'.code -> config.args.disableSource = true
                'L'.code -> config.args.printLogoOnly = true

                1000 -> return listLogos(config.t.dataDir)
                1001 -> config.t.sepResetAfter = strToBool(handleOptional(g.optarg, "true"))
                1002 -> config.t.wrapLines = strToBool(handleOptional(g.optarg, "true"))
                1003 -> generateConfig(File(g.optarg))
                1004 -> config.t.sepReset = g.optarg
                1005 -> config.t.titleSep = g.optarg
                1006 -> config.t.logoPaddingTop = g.optarg.toInt()
                1007 -> config.t.logoPaddingLeft = g.optarg.toInt()
                1008 -> config.t.layoutPaddingTop = g.optarg.toInt()
                1011 -> config.t.aliasColors.add(g.optarg)
            }
        }
    } catch (e: RuntimeException) {
        return e.message.toString()
    }

    return "success"
}

private fun createConfig() {
    File(CONFIG_DIR).mkdirs()
    if (!File("$CONFIG_DIR/config.toml").exists())
        generateConfig(File("$CONFIG_DIR/config.toml"))
}

private fun manageConfigStuff(context: Context, config: Config) {
    for (str in config.t.aliasColors)
        addAliasColor(context, str, config)

    config.autoDisk.displayTypes.let {
        if (it.contains("regular"))
            config.autoDisk.displayTypesInt = config.autoDisk.displayTypesInt or DiskVolumeType.REGULAR.value
        if (it.contains("removable"))
            config.autoDisk.displayTypesInt = config.autoDisk.displayTypesInt or DiskVolumeType.EXTERNAL.value
        if (it.contains("hidden"))
            config.autoDisk.displayTypesInt = config.autoDisk.displayTypesInt or DiskVolumeType.HIDDEN.value
        if (it.contains("read-only"))
            config.autoDisk.displayTypesInt = config.autoDisk.displayTypesInt or DiskVolumeType.READ_ONLY.value
    }
}

private fun detectDistroFile(config: Config): String {
    if (config.args.customDistro.isNotEmpty())
        return "${config.t.dataDir}/ascii/${config.args.customDistro}.txt"

    return "${config.t.dataDir}/ascii/android.txt"
}

fun mainRenderStr(context: Context, argsStr: String): String {
    createConfig()
    val args = argsStr.split(' ').toTypedArray()
    val toml = Toml {
        ignoreUnknownKeys = true
    }
    val tomlConfig = Paths.get(parseConfigArg(context, args)).readText()
        .replace("\\e[", "\\u001B[") // Escape ANSI codes
    val config: Config
    try {
        config = toml.decodeFromString(Config.serializer(), tomlConfig)
    } catch (e: SerializationException) {
        return e.message.toString()
    }
    return parseArgs(context, args, config)
}

fun mainRender(context: Context, appWidgetId: Int, argsStr: String): List<SpannableStringBuilder> {
    createConfig()
    val args = argsStr.split(' ').toTypedArray()
    val toml = Toml {
        ignoreUnknownKeys = true
    }
    val tomlConfig = Paths.get(parseConfigArg(context, args)).readText()
            .replace("\\e[", "\\u001B[") // Escape ANSI codes
    val config: Config
    try {
        config = toml.decodeFromString(Config.serializer(), tomlConfig)
    } catch (e: SerializationException) {
        return listOf(SpannableStringBuilder(e.toString()))
    }

    parseArgs(context, args, config).let {
        if (it != "success")
            return listOf(SpannableStringBuilder(it))
    }

    manageConfigStuff(context, config)

    var path = if (config.t.sourcePath == "os") detectDistroFile(config) else config.t.sourcePath
    if (config.t.asciiLogoType.isNotEmpty() && (config.t.sourcePath == "os")) {
        val logoTypePath = StringBuilder(path)
        val pos = logoTypePath.lastIndexOf('.')
        if (pos != -1)
            logoTypePath.insert(pos, "_${config.t.asciiLogoType}")
        else
            logoTypePath.append("_${config.t.asciiLogoType}")
        if (File(logoTypePath.toString()).exists())
            path = logoTypePath.toString()
    }

    var filePath = File(path)
    if (!filePath.exists()) {
        filePath = File.createTempFile("customfetch-ascii-logo_", ".txt")
        filePath.writeText("""
            ${'$'}{green}  ;,           ,;
            ${'$'}{green}   ';,.-----.,;'
            ${'$'}{green}  ,'           ',
            ${'$'}{green} /    O     O    \
            ${'$'}{green}|                 |
            ${'$'}{green}'-----------------'
        """.trimIndent())
        filePath.deleteOnExit()
    }

    Cpu.clearCache()
    Ram.clearCache()
    Swap.clearCache()
    Gpu.clearCache()
    System.clearCache()
    Battery.clearCache()
    Disk.clearCache()
    return render(context,  appWidgetId, config, filePath)
}