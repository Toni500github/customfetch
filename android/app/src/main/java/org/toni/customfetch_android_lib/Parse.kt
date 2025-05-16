package org.toni.customfetch_android_lib

import android.content.Context
import android.graphics.Color
import android.graphics.Typeface
import android.text.SpannableStringBuilder
import android.text.Spanned
import android.text.style.BackgroundColorSpan
import android.text.style.ForegroundColorSpan
import android.text.style.StrikethroughSpan
import android.text.style.StyleSpan
import android.text.style.UnderlineSpan
import android.util.Log
import androidx.core.graphics.ColorUtils
import androidx.core.graphics.toColorInt
import org.toni.customfetch_android_lib.ParserFunctions.getAndColorPercentage
import org.toni.customfetch_android_lib.ParserFunctions.parse
import org.toni.customfetch_android_lib.query.Battery
import org.toni.customfetch_android_lib.query.Cpu
import org.toni.customfetch_android_lib.query.Disk
import org.toni.customfetch_android_lib.query.DiskVolumeType
import org.toni.customfetch_android_lib.query.Gpu
import org.toni.customfetch_android_lib.query.Ram
import org.toni.customfetch_android_lib.query.Swap
import org.toni.customfetch_android_lib.query.System
import org.toni.customfetch_android_lib.query.User
import java.util.concurrent.TimeUnit
import kotlin.math.roundToInt

// useless useful tmp string for parse() without using the original
// pureOutput
var s = StringBuilder()

val autoColors = arrayListOf<String>()

const val UNKNOWN = "(unknown)"
// Usually in neofetch/fastfetch when some infos couldn't be queried,
// they remove it from the display. With customfetch is kinda difficult to know when to remove
// the info to display, since it's all modular with tags, so I have created
// magic line to be sure that I don't cut the wrong line.
//
// Every instance of this string in a layout line, the whole line will be erased.
const val MAGIC_LINE = "(cut this line NOW!! RAHHH)"

typealias SystemInfo = MutableMap<String, MutableMap<String, Variant>>
sealed class Variant {
    data class StringVal(val value: String) : Variant()
    data class SSBVal(val value: SpannableStringBuilder) : Variant()
    data class SizeT(val value: ULong) : Variant()
    data class DoubleVal(val value: Double) : Variant()
}

data class ParseArgs(
    val context: Context,
    val appWidgetId: Int,
    val systemInfo: SystemInfo,
    val pureOutput: StringBuilder,
    val layout: MutableList<SpannableStringBuilder>,
    val tmpLayout: MutableList<SpannableStringBuilder>,
    val config: Config,
    var parsingLayout: Boolean,
    var firstrunClr: Boolean = true,
    var noMoreReset: Boolean = false,

    var spansDisabled: Boolean = false
)

class Parser(val src: String, val pureOutput: StringBuilder) {
    var dollarPos: Int = 0
    var pos: Int = 0

    fun tryRead(c: Char): Boolean {
        if (isEof()) return false

        if (src[pos] == c) {
            pos++
            return true
        }

        return false
    }

    fun readChar(addPureOutput: Boolean = false): Char {
        if (isEof()) return 0.toChar()

        if (addPureOutput) {
            pureOutput.append(src[pos])
        }

        pos++
        return src[pos - 1]
    }

    fun isEof(): Boolean = pos >= src.length

    fun rewind(count: Int = 1) {
        pos = maxOf(pos - count, 0)
    }
}

fun Double.roundTo(num: Int): Float {
    return "%.${num}f".format(this).replace(',', '.').toFloat()
}

object ParserFunctions {

    // converts pango numbers (1..65536) to color hex numbers (0x0..0FF)
    // also supports percentages
    private fun parsePangoValue(value: String): Int {
        if (value.endsWith('%'))
            return ((value.substringBefore('%').toFloat() / 100f) * 255f).roundToInt()+1
        return ((value.toInt() - 1) * 255) / 65535
    }

    private fun getAnsiColor(str: String, config: Config, currentSpans: ArrayList<Any>?) {
        var color = StringBuilder(str.removeSuffix("m"))
        if (color.startsWith("1;")) {
            currentSpans?.add(StyleSpan(Typeface.BOLD))
            color.delete(0, 2)
        } else if (color.startsWith("0;")) {
            color.delete(0, 2)
        }

        val n = color.toString().toInt()
        color = StringBuilder(when (color.last()) {
            '0' -> config.gui.black
            '1' -> config.gui.red
            '2' -> config.gui.green
            '3' -> config.gui.yellow
            '4' -> config.gui.blue
            '5' -> config.gui.magenta
            '6' -> config.gui.cyan
            '7' -> config.gui.white
            else -> color
        })
        if (color[0] != '#')
            color.delete(0, color.indexOf('#'))

        val colorInt = Color.parseColor(color.toString())
        val lighterColorInt = ColorUtils.blendARGB(colorInt, Color.WHITE, 0.5f)
        when (n) {
            in 100..107 -> currentSpans?.add(BackgroundColorSpan(lighterColorInt))
            in 90..97 -> currentSpans?.add(ForegroundColorSpan(lighterColorInt))
            in 40..47 -> currentSpans?.add(BackgroundColorSpan(colorInt))
            in 30..37 -> currentSpans?.add(ForegroundColorSpan(colorInt))
        }
    }

    private fun convertAnsiEscapeRgb(parseArgs: ParseArgs, noescStr: String): String {
        // Validate input format
        if (noescStr.count { it == ';' } < 4) {
            die(parseArgs.context, 
                "ANSI escape code color '\\e[$noescStr' should have an rgb type value\n" +
                        "e.g \\e[38;2;255;255;255m"
            )
        }

        if (!noescStr.endsWith('m')) {
            die(parseArgs.context, 
                "Parser: failed to parse layout/ascii art: missing 'm' while using " +
                        "ANSI color escape code in '\\e[$noescStr'"
            )
        }

        val rgbComponents = noescStr
            .substring(5) // Skip "38;2;"
            .removeSuffix("m")
            .split(';')
            .take(3)

        val r = rgbComponents[0].toUInt()
        val g = rgbComponents[1].toUInt()
        val b = rgbComponents[2].toUInt()

        val result = (r shl 16) or (g shl 8) or b

        // Convert to 6-digit hex string (pad with leading zeros if needed)
        return result.toString(16).padStart(6, '0')
    }

    fun getAndColorPercentage(n1: Double, n2: Double, parseArgs: ParseArgs, invert: Boolean = false): SpannableStringBuilder {
        val config = parseArgs.config
        val result = n1 / n2 * 100.0

        val color = if (!invert) {
            when {
                result <= 45 -> "\${${config.t.percentageColors[0]}}"
                result <= 80 -> "\${${config.t.percentageColors[1]}}"
                else -> "\${${config.t.percentageColors[2]}}"
            }
        } else {
            when {
                result <= 45 -> "\${${config.t.percentageColors[2]}}"
                result <= 80 -> "\${${config.t.percentageColors[1]}}"
                else -> "\${${config.t.percentageColors[0]}}"
            }
        }

        return parse("$color${result.roundTo(2)}%\${0}", s, parseArgs)
    }

    private fun parseConditionalTag(parser: Parser, parseArgs: ParseArgs, evaluate: Boolean = true): SpannableStringBuilder? {
        if (!parser.tryRead('[')) return null

        val condA = parse(parser, parseArgs, evaluate, ',')
        val condB = parse(parser, parseArgs, evaluate, ',')

        val cond = (condA == condB)

        val condTrue = parse(parser, parseArgs, cond, ',')
        val condFalse = parse(parser, parseArgs, !cond, ']')

        return if (cond) condTrue else condFalse
    }

    private fun parsePercTag(parser: Parser, parseArgs: ParseArgs, evaluate: Boolean = true): SpannableStringBuilder? {
        if (!parser.tryRead('%')) return null

        val command = parse(parser, parseArgs, evaluate, '%')

        if (!evaluate) return null

        val commaPos = command.indexOf(',')
        if (commaPos == -1)
            die(parseArgs.context, "percentage tag '$command' doesn't have a comma for separating the 2 numbers")

        val invert = command[0] == '!'
        val n1 = parse(command.substring(if (invert) 1 else 0, commaPos), s, parseArgs).toString().toDouble()
        val n2 = parse(command.substring(commaPos + 1), s, parseArgs).toString().toDouble()
        return getAndColorPercentage(n1, n2, parseArgs, invert)
    }

    private fun parseInfoTag(parser: Parser, parseArgs: ParseArgs, evaluate: Boolean = true): SpannableStringBuilder? {
        if (!parser.tryRead('<')) return null

        val module = parse(parser, parseArgs, evaluate, '>').toString()

        if (!evaluate) return null

        val dotPos = module.indexOf('.')
        if (dotPos == -1) {
            addValueFromModule(module, parseArgs)
            val info = getInfoFromName(parseArgs.systemInfo, module, "module-$module")

            if (parser.dollarPos != -1) {
                parseArgs.pureOutput.replace(
                    parser.dollarPos,
                    parser.dollarPos + module.length + "$<>".length,
                    info.toString()
                )
            }

            return info
        }

        val moduleName = module.substring(0, dotPos)
        val moduleMemberName = module.substring(dotPos + 1)
        addValueFromModuleMember(moduleName, moduleMemberName, parseArgs)

        val info = getInfoFromName(parseArgs.systemInfo, moduleName, moduleMemberName)
        if (parser.dollarPos != -1) {
            parseArgs.pureOutput.replace(
                parser.dollarPos,
                parser.dollarPos + module.length + "$<>".length,
                info.toString()
            )
        }

        return info
    }

    private var currentSpans: ArrayList<Any>? = arrayListOf()
    private var lastColorIndex = -1
    private fun parseColorTag(parser: Parser, parseArgs: ParseArgs, evaluate: Boolean = true): SpannableStringBuilder? {
        if (!parser.tryRead('{')) return null

        var color = parse(parser, parseArgs, evaluate, '}').toString()

        if (!evaluate) return null

        val output = SpannableStringBuilder()
        val config = parseArgs.config
        val tagLen = color.length + "\${}".length

        if (config.args.disableColors) {
            if (parser.dollarPos != -1)
                parseArgs.pureOutput.delete(parser.dollarPos, tagLen)
            return output
        }

        lastColorIndex = parser.pos - tagLen
        Log.d("lastColorIndex", "parseColorTag: lastColorIndex = $lastColorIndex")

        val index = config.t.colorsName.indexOf(color)
        if (index != -1)
            color = config.t.colorsValue[index]

        if (color.startsWith("auto")) {
            var ver = if (color.length > 4)
                color.substring(4).toInt().minus(1) else 0

            if (autoColors.isEmpty())
                autoColors.add("bold")

            if (ver < 0 || ver >= autoColors.size)
                ver = 0

            color = autoColors[ver]
        }

        if (color == "0") {
            parseArgs.spansDisabled = true
            currentSpans = null
        }
        else if (color == "1") {
            parseArgs.spansDisabled = true
            currentSpans = null
        }
        else {
            parseArgs.spansDisabled = false
            if (currentSpans.isNullOrEmpty())
                currentSpans = arrayListOf()

            val strColor = when (color) {
                "black" -> config.gui.black
                "red" -> config.gui.red
                "blue" -> config.gui.blue
                "green" -> config.gui.green
                "yellow" -> config.gui.yellow
                "cyan" -> config.gui.cyan
                "magenta" -> config.gui.magenta
                "white" -> config.gui.white
                else -> color
            }

            val hashPos = strColor.lastIndexOf('#')
            var argModePos = 0
            if (hashPos != -1) {
                val optColor = strColor.substring(0, hashPos)
                fun appendArgMode(mode: String): Pair<String, Int> {
                    if (optColor[argModePos + 1] == '(') {
                        val closeBracket = optColor.indexOf(')', argModePos)
                        if (closeBracket != -1)
                            die(parseArgs.context, "mode '$mode' in color '$color' doesn't have a closet bracket")

                        val value = optColor.substring(argModePos+2, closeBracket - argModePos - 2)
                        return Pair(value, closeBracket)
                    }
                    return Pair("", 0)
                }

                var bgColor = false
                var i = 0
                while (i < optColor.length) {
                    when (optColor[i]) {
                        'b' -> {
                            bgColor = true
                            currentSpans?.add(BackgroundColorSpan(strColor.substring(hashPos).toColorInt()))
                        }
                        '!' -> currentSpans?.add(StyleSpan(Typeface.BOLD))
                        'u' -> currentSpans?.add(UnderlineSpan())
                        'i' -> currentSpans?.add(StyleSpan(Typeface.ITALIC))
                        's' -> currentSpans?.add(StrikethroughSpan())

                        /*'a' -> {
                            argModePos = i
                            val pairs = appendArgMode("fgalpha")
                            if (pairs.second != 0) {
                                tempStyle.addSpan(
                                    TextAppearanceSpan(null, 0,0,
                                        ColorStateList.valueOf(parsePangoValue(pairs.first)), null
                                    )
                                )
                                i += pairs.second
                            }
                        }*/
                    }
                    i++
                }
                if (!bgColor)
                    currentSpans?.add(ForegroundColorSpan(strColor.substring(hashPos).toColorInt()))
            }
            else if (strColor.startsWith("\\e") || strColor.startsWith("\u001B")) {
                val noEscapeStr = if (strColor.startsWith("\\e")) strColor.substring(3) else strColor.substring(4)
                if (noEscapeStr.startsWith("38;2;")){
                    val hexColor = convertAnsiEscapeRgb(parseArgs, noEscapeStr)
                    currentSpans?.add(ForegroundColorSpan(Color.parseColor(hexColor)))
                }
                else if (noEscapeStr.startsWith("48;2;")) {
                    val hexColor = convertAnsiEscapeRgb(parseArgs, noEscapeStr)
                    currentSpans?.add(BackgroundColorSpan(Color.parseColor(hexColor)))
                }
                else if (noEscapeStr.startsWith("38;5;") || noEscapeStr.startsWith("48;5;"))
                    die(parseArgs.context, "256 true color '$noEscapeStr' works only in terminal")
                else
                    getAnsiColor(noEscapeStr, config, currentSpans)
            }
            else {
                error(parseArgs.context, "PARSER: failed to parse line with color '$color'")
                if (!parseArgs.parsingLayout && parser.dollarPos != -1)
                    parseArgs.pureOutput.delete(parser.dollarPos, tagLen)
                return output
            }
            if (!parseArgs.parsingLayout && autoColors.indexOf(color) == -1)
                autoColors.add(color)
        }

        if (!parseArgs.parsingLayout && parser.dollarPos != -1)
            parseArgs.pureOutput.delete(parser.dollarPos, tagLen)

        parseArgs.firstrunClr = false
        return output
    }

    private fun parseTags(parser: Parser, parseArgs: ParseArgs, evaluate: Boolean = true): SpannableStringBuilder? {
        if (!parser.tryRead('$')) return null

        if (parser.dollarPos != -1)
            parser.dollarPos = parser.pureOutput.indexOf('$', parser.dollarPos)

        parseConditionalTag(parser, parseArgs, evaluate)?.let {
            return it
        }
        parseInfoTag(parser, parseArgs, evaluate)?.let {
            return it
        }
        parseColorTag(parser, parseArgs, evaluate)?.let {
            return it
        }
        parsePercTag(parser, parseArgs, evaluate)?.let {
            return it
        }

        parser.rewind()
        return null
    }

    private fun parse(parser: Parser, parseArgs: ParseArgs, evaluate: Boolean = true, until: Char = 0.toChar()): SpannableStringBuilder {
        val result = SpannableStringBuilder()
        val spanStartPositions = mutableMapOf<Any, Int>() // Tracks start position for each span type

        while (if (until == 0.toChar()) !parser.isEof() else !parser.tryRead(until)) {
            if (until != 0.toChar() && parser.isEof()) {
                error(parseArgs.context, "Missing tag close bracket '$until' in string '${parser.src}'")
                return result
            }

            val start = result.length

            if (parser.tryRead('\\')) {
                result.append(parser.readChar(until == 0.toChar()))
            } else {
                parseTags(parser, parseArgs, evaluate)?.let {
                    result.append(it)
                } ?: run {
                    result.append(parser.readChar(until == 0.toChar()))
                }
            }

            if (!parseArgs.spansDisabled) {
                // update start positions for new spans
                currentSpans?.forEach { span ->
                    if (!spanStartPositions.containsKey(span)) {
                        spanStartPositions[span] = start
                    }
                }

                // apply all active spans to the new content
                currentSpans?.forEach { span ->
                    spanStartPositions[span]?.let { spanStart ->
                        if (spanStart < result.length) {
                            result.setSpan(
                                span,
                                spanStart,
                                result.length,
                                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE
                            )
                        }
                    }
                }
            }
        }
        currentSpans = null

        return result
    }

    fun parse(input: String, parseArgs: ParseArgs): SpannableStringBuilder =
        parse(input, parseArgs.context, parseArgs.appWidgetId, parseArgs.systemInfo, parseArgs.pureOutput, parseArgs.layout, parseArgs.tmpLayout, parseArgs.config, parseArgs.parsingLayout, BooleanWrapper(parseArgs.noMoreReset))

    fun parse(input: String, s: StringBuilder, parseArgs: ParseArgs): SpannableStringBuilder =
        parse(input, parseArgs.context, parseArgs.appWidgetId, parseArgs.systemInfo, s, parseArgs.layout, parseArgs.tmpLayout, parseArgs.config, parseArgs.parsingLayout, BooleanWrapper(parseArgs.noMoreReset))

    fun parse(
        input: String,
        context: Context,
        appWidgetId: Int,
        systemInfo: SystemInfo,
        pureOutput: StringBuilder,
        layout: MutableList<SpannableStringBuilder>,
        tmpLayout: MutableList<SpannableStringBuilder>,
        config: Config,
        parsingLayout: Boolean,
        noMoreReset: BooleanWrapper // Using wrapper for mutable boolean reference
    ): SpannableStringBuilder {
        var modifiedInput = input

        if (config.t.sepReset.isNotEmpty() && parsingLayout && !noMoreReset.value) {
            modifiedInput = if (config.t.sepResetAfter) {
                modifiedInput.replace(config.t.sepReset, "${config.t.sepReset}\${0}")
            } else {
                modifiedInput.replace(config.t.sepReset, "\${0}${config.t.sepReset}")
            }
            noMoreReset.value = true
        }

        val parseArgs = ParseArgs(
            context,
            appWidgetId,
            systemInfo,
            pureOutput,
            layout,
            tmpLayout,
            config,
            parsingLayout,
            true,
            noMoreReset.value,
        )

        return parse(Parser(modifiedInput, pureOutput), parseArgs)
    }

    // Helper class for mutable boolean reference
    class BooleanWrapper(var value: Boolean)
}

fun getInfoFromName(
    systemInfo: SystemInfo,
    moduleName: String,
    moduleMemberName: String
): SpannableStringBuilder {
    systemInfo[moduleName]?.let { moduleInfo ->
        moduleInfo[moduleMemberName]?.let { result ->
            return when (result) {
                is Variant.StringVal -> SpannableStringBuilder(result.value)
                is Variant.DoubleVal -> SpannableStringBuilder("%.2f".format(result.value))
                is Variant.SizeT -> SpannableStringBuilder(result.value.toString())
                is Variant.SSBVal -> result.value
            }
        }
    }
    return SpannableStringBuilder("(unknown/invalid module)")
}

fun getInfoFromNameStr(
    systemInfo: SystemInfo,
    moduleName: String,
    moduleMemberName: String
): String {
    systemInfo[moduleName]?.let { moduleInfo ->
        moduleInfo[moduleMemberName]?.let { result ->
            return when (result) {
                is Variant.StringVal -> result.value
                is Variant.DoubleVal -> "%.2f".format(result.value)
                is Variant.SizeT -> result.value.toString()
                is Variant.SSBVal -> result.value.toString()
            }
        }
    }
    return "(unknown/invalid module)"
}

private fun getAutoUptime(uptimeSecs: Long, config: Config): String {
    val remainingSecs = (TimeUnit.SECONDS.toSeconds(uptimeSecs) % 60)
    val remainingMins = (TimeUnit.SECONDS.toMinutes(uptimeSecs) % 60)
    val remainingHours = (TimeUnit.SECONDS.toHours(uptimeSecs) % 24)
    val remainingDays = (TimeUnit.SECONDS.toDays(uptimeSecs))

    if (remainingDays == 0L && remainingHours == 0L && remainingMins == 0L)
        return "$remainingSecs${config.osUptime.secondsFormat}"

    val parts = mutableListOf<String>()
    if (remainingDays > 0L) parts.add("$remainingDays${config.osUptime.daysFormat}")
    if (remainingHours > 0L) parts.add("$remainingHours${config.osUptime.hoursFormat}")
    if (remainingMins > 0L) parts.add("$remainingMins${config.osUptime.minutesFormat}")

    return parts.joinToString(", ")
}

// C enums-like because I ain't writing MemoryMetrics.FREE.index
const val USED = 0
const val FREE = 1
const val TOTAL = 2

infix fun Int.has(flag: DiskVolumeType): Boolean = this and flag.value != 0

fun addValueFromModuleMember(moduleName: String, moduleMemberName: String, parseArgs: ParseArgs) {
    // Aliases for convention
    val config = parseArgs.config
    val sysInfo = parseArgs.systemInfo

    fun sysInfoInsert(value: Any) {
        sysInfo.getOrPut(moduleName) { mutableMapOf() }[moduleMemberName] = when (value) {
            is String -> Variant.StringVal(value)
            is Double -> Variant.DoubleVal(value)
            is ULong -> Variant.SizeT(value)
            is SpannableStringBuilder -> Variant.SSBVal(value)
            else -> throw IllegalArgumentException("Unsupported variant type")
        }
    }

    val byteUnit = if (config.t.useSIByteUnit) 1000 else 1024
    val sortedValidPrefixes = arrayOf("B", "EB", "EiB", "GB", "GiB", "kB",
        "KiB", "MB", "MiB", "PB", "PiB", "TB",
        "TiB", "YB", "YiB", "ZB", "ZiB")

    val returnDividedBytes = { amount: Double ->
        val prefix = moduleMemberName.substringAfter('-')
        if (sortedValidPrefixes.binarySearch(prefix) >= 0)
            divideBytes(amount, prefix).numBytes
        else
            0.0
    }

    if (moduleName == "os") {
        val querySystem = System()

        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            when (moduleMemberName) {
                "name" -> sysInfoInsert(querySystem.osPrettyName)
                "name_id" -> sysInfoInsert(querySystem.osId)

                "uptime" -> sysInfoInsert(getAutoUptime(querySystem.uptime, config))
                "uptime_secs" -> sysInfoInsert((querySystem.uptime % 60).toULong())
                "uptime_mins" -> sysInfoInsert((querySystem.uptime / 60 % 60).toULong())
                "uptime_hours" -> sysInfoInsert((querySystem.uptime / 3600 % 24).toULong())
                "uptime_days" -> sysInfoInsert((querySystem.uptime / (3600 * 24)).toULong())

                "kernel" -> sysInfoInsert(querySystem.kernelName + " " + querySystem.kernelVersion)
                "kernel_name" -> sysInfoInsert(querySystem.kernelName)
                "kernel_version" -> sysInfoInsert(querySystem.kernelVersion)
                "packages", "pkgs" -> sysInfoInsert(MAGIC_LINE)
                "initsys_name" -> sysInfoInsert(querySystem.osInitsysName)
                "initsys_version" -> sysInfoInsert(querySystem.osInitsysVersion)
                "hostname" -> sysInfoInsert(querySystem.hostname)
                "version_codename" -> sysInfoInsert(querySystem.osVersionCodename)
                "version_id" -> sysInfoInsert(querySystem.osVersionId)
            }
        }
    }
    else if (moduleName == "system") {
        val querySystem = System()

        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            when (moduleMemberName) {
                "host" -> sysInfoInsert(
                    "${querySystem.hostVendor} ${querySystem.hostModelname} ${querySystem.hostVersion}"
                )
                "host_name" -> sysInfoInsert(querySystem.hostModelname)
                "host_vendor" -> sysInfoInsert(querySystem.hostVendor)
                "host_version" -> sysInfoInsert(querySystem.hostVersion)
                "arch" -> sysInfoInsert(querySystem.arch)
            }
        }
    }
    else if (moduleName == "user") {
        val queryUser = User.getInstance()

        if (!sysInfo.containsKey(moduleName))
            sysInfo[moduleName] = mutableMapOf()

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            when (moduleMemberName) {
                "name" -> sysInfoInsert(queryUser.name)

                "shell", "shell_name", "shell_path", "shell_version",
                "de_name", "de_version", "wm_name", "wm_version",
                "terminal", "terminal_name", "terminal_version" -> sysInfoInsert(MAGIC_LINE)
            }
        }
    }
    else if (moduleName == "cpu") {
        val queryCPU = Cpu()

        if (!sysInfo.containsKey(moduleName))
            sysInfo[moduleName] = mutableMapOf()

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            when (moduleMemberName) {
                "name" -> sysInfoInsert(queryCPU.name)
                "nproc" -> sysInfoInsert(queryCPU.nproc)
                "freq_cur" -> sysInfoInsert(queryCPU.freqCurrent)
                "freq_max" -> sysInfoInsert(queryCPU.freqMax)
                "freq_min" -> sysInfoInsert(queryCPU.freqMin)
                "freq_bios_limit" -> sysInfoInsert(MAGIC_LINE)

                "temp_C" -> sysInfoInsert(queryCPU.temp)
                "temp_F" -> sysInfoInsert((queryCPU.temp * 1.8 + 34))
                "temp_K" -> sysInfoInsert((queryCPU.temp + 273.15))
            }
        }
    }
    else if (moduleName.startsWith("disk")) {
        if (moduleName.length < "disk()".length)
            die(parseArgs.context, "invalid disk module name '$moduleName', must be disk(/path/to/fs) e.g: disk(/)")

        val path = moduleName.substring(6, moduleName.length-1)
        val queryDisk = Disk(path, parseArgs)

        if (!sysInfo.containsKey(moduleName))
            sysInfo[moduleName] = mutableMapOf()

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val byteUnits = arrayOf(
                autoDivideBytes(queryDisk.usedAmount, byteUnit),
                autoDivideBytes(queryDisk.freeAmount, byteUnit),
                autoDivideBytes(queryDisk.totalAmount, byteUnit)
            )
            when (moduleMemberName) {
                "fs" -> sysInfoInsert(queryDisk.typesDisk)
                "device" -> sysInfoInsert(queryDisk.device)
                "mountdir" -> sysInfoInsert(queryDisk.mountdir)

                "types" -> {
                    var typesDisksStr = "["
                    if (queryDisk.typesDisk has DiskVolumeType.EXTERNAL)
                        typesDisksStr += "External, "
                    if (queryDisk.typesDisk has DiskVolumeType.HIDDEN)
                        typesDisksStr += "Hidden, "
                    if (queryDisk.typesDisk has DiskVolumeType.READ_ONLY)
                        typesDisksStr += "Read-only, "

                    if (typesDisksStr.isNotEmpty())
                        typesDisksStr = typesDisksStr.removeRange(typesDisksStr.length - 2, typesDisksStr.length)
                    sysInfoInsert(typesDisksStr)
                }

                "used" -> sysInfoInsert("%.2f %s".format(byteUnits[USED].numBytes, byteUnits[USED].unit))
                "free" -> sysInfoInsert("%.2f %s".format(byteUnits[FREE].numBytes, byteUnits[FREE].unit))
                "total" -> sysInfoInsert("%.2f %s".format(byteUnits[TOTAL].numBytes, byteUnits[TOTAL].unit))

                "free_perc", "free_percentage" -> sysInfoInsert(getAndColorPercentage(queryDisk.freeAmount, queryDisk.totalAmount, parseArgs))
                "used_perc", "used_percentage" -> sysInfoInsert(getAndColorPercentage(queryDisk.usedAmount, queryDisk.totalAmount, parseArgs))

                else -> {
                    if (moduleMemberName.startsWith("free-"))
                        sysInfoInsert(returnDividedBytes(queryDisk.freeAmount))
                    else if (moduleMemberName.startsWith("used-"))
                        sysInfoInsert(returnDividedBytes(queryDisk.usedAmount))
                    else if (moduleMemberName.startsWith("total-"))
                        sysInfoInsert(returnDividedBytes(queryDisk.totalAmount))
                }
            }
        }
    }
    else if (moduleName == "ram") {
        if (!sysInfo.containsKey(moduleName))
            sysInfo[moduleName] = mutableMapOf()

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val queryRam = Ram()
            val byteUnits = listOf(
                autoDivideBytes(queryRam.usedAmount * byteUnit, byteUnit),
                autoDivideBytes(queryRam.freeAmount * byteUnit, byteUnit),
                autoDivideBytes(queryRam.totalAmount * byteUnit, byteUnit)
            )

            when (moduleMemberName) {
                "used" -> sysInfoInsert("%.2f %s".format(byteUnits[USED].numBytes, byteUnits[USED].unit))
                "total" -> sysInfoInsert("%.2f %s".format(byteUnits[TOTAL].numBytes, byteUnits[TOTAL].unit))
                "free" -> sysInfoInsert("%.2f %s".format(byteUnits[FREE].numBytes, byteUnits[FREE].unit))

                "free_perc", "free_percentage" -> sysInfoInsert(getAndColorPercentage(queryRam.freeAmount, queryRam.totalAmount, parseArgs))
                "used_perc", "used_percentage" -> sysInfoInsert(getAndColorPercentage(queryRam.usedAmount, queryRam.totalAmount, parseArgs))

                else -> {
                    if (moduleMemberName.startsWith("free-"))
                        sysInfoInsert(returnDividedBytes(queryRam.freeAmount))
                    else if (moduleMemberName.startsWith("used-"))
                        sysInfoInsert(returnDividedBytes(queryRam.usedAmount))
                    else if (moduleMemberName.startsWith("total-"))
                        sysInfoInsert(returnDividedBytes(queryRam.totalAmount))
                }
            }
        }
    }
    else if (moduleName == "swap") {
        if (!sysInfo.containsKey(moduleName))
            sysInfo[moduleName] = mutableMapOf()

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val querySwap = Swap()
            val byteUnits = listOf(
                autoDivideBytes(querySwap.usedAmount * byteUnit, byteUnit),
                autoDivideBytes(querySwap.freeAmount * byteUnit, byteUnit),
                autoDivideBytes(querySwap.totalAmount * byteUnit, byteUnit)
            )

            when (moduleMemberName) {
                "used" -> sysInfoInsert("%.2f %s".format(byteUnits[USED].numBytes, byteUnits[USED].unit))
                "total" -> sysInfoInsert("%.2f %s".format(byteUnits[TOTAL].numBytes, byteUnits[TOTAL].unit))
                "free" -> sysInfoInsert("%.2f %s".format(byteUnits[FREE].numBytes, byteUnits[FREE].unit))

                "free_perc", "free_percentage" -> sysInfoInsert(getAndColorPercentage(querySwap.freeAmount, querySwap.totalAmount, parseArgs))
                "used_perc", "used_percentage" -> sysInfoInsert(getAndColorPercentage(querySwap.usedAmount, querySwap.totalAmount, parseArgs))

                else -> {
                    if (moduleMemberName.startsWith("free-"))
                        sysInfoInsert(returnDividedBytes(querySwap.freeAmount))
                    else if (moduleMemberName.startsWith("used-"))
                        sysInfoInsert(returnDividedBytes(querySwap.usedAmount))
                    else if (moduleMemberName.startsWith("total-"))
                        sysInfoInsert(returnDividedBytes(querySwap.totalAmount))
                }
            }
        }
    }
    else if (moduleName == "battery") {
        if (!sysInfo.containsKey(moduleName))
            sysInfo[moduleName] = mutableMapOf()

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val queryBattery = Battery(parseArgs.context)
            when (moduleMemberName) {
                "vendor", "manufacturer", "name" -> sysInfoInsert(MAGIC_LINE)
                "percentage", "perc" -> sysInfoInsert(getAndColorPercentage(queryBattery.perc, 100.0, parseArgs, true))
                "technology" -> sysInfoInsert(queryBattery.technology)
                "status" -> sysInfoInsert(queryBattery.status)

                "temp_C" -> sysInfoInsert(queryBattery.temp)
                "temp_F" -> sysInfoInsert(queryBattery.temp * 1.8 + 32)
                "temp_K" -> sysInfoInsert(queryBattery.temp + 273.15)
            }
        }
    }
    else if (moduleName.startsWith("gpu")) {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val queryGpu = Gpu()
            when (moduleMemberName) {
                "name" -> sysInfoInsert(queryGpu.name)
                "vendor" -> sysInfoInsert(queryGpu.vendor)
            }
        }
    }
    else if (moduleName == "auto") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            when (moduleMemberName) {
                "disk" -> {
                    val queryDisk = Disk("", parseArgs, true)
                    for (str in queryDisk.disksFormats) {
                        parseArgs.tmpLayout.add(str)
                        sysInfoInsert(str.toString())
                    }
                }
            }
        }
    }
    else if (moduleName.startsWith("theme")) {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }
        sysInfoInsert(MAGIC_LINE)
    }
}

fun addValueFromModule(moduleName: String, parseArgs: ParseArgs) {
    val moduleMemberName = "module-$moduleName"

    // Aliases for convention
    val config = parseArgs.config
    val sysInfo = parseArgs.systemInfo
    val byteUnit = if (config.t.useSIByteUnit) 1000 else 1024

    fun sysInfoInsert(value: Any) {
        sysInfo.getOrPut(moduleName) { mutableMapOf() }[moduleMemberName] = when (value) {
            is String -> Variant.StringVal(value)
            is Double -> Variant.DoubleVal(value)
            is ULong -> Variant.SizeT(value)
            is SpannableStringBuilder -> Variant.SSBVal(value)
            else -> throw IllegalArgumentException("Unsupported variant type")
        }
    }

    if (moduleName == "title") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            sysInfoInsert(parse("\${auto2}$<user.name>\${0}@\${auto2}$<os.hostname>", s, parseArgs).toString())
        }
    }

    else if (moduleName == "title_sep" || moduleName == "title_separator") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val queryUser = User.getInstance()
            val querySystem = System()
            val titleLen = (queryUser.name + "@" + querySystem.hostname).length

            val str = config.t.titleSep.repeat(titleLen)
            sysInfoInsert(str)
        }
    }

    else if (moduleName == "colors") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            sysInfoInsert(parse("\${\\\\e[40m}   \${\\\\e[41m}   \${\\\\e[42m}   \${\\\\e[43m}   \${\\\\e[44m}   \${\\\\e[45m}   \${\\\\e[46m}   \${\\\\e[47m}   \${0}", s, parseArgs))
        }
    }

    else if (moduleName == "colors_light") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            sysInfoInsert(parse("\${\\\\e[100m}   \${\\\\e[101m}   \${\\\\e[102m}   \${\\\\e[103m}   \${\\\\e[104m}   \${\\\\e[105m}   \${\\\\e[106m}   \${\\\\e[107m}   \${0}", s, parseArgs))
        }
    }

    else if (moduleName == "colors_symbol") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            if (moduleName.length <= "colors_symbol()".length)
                die(parseArgs.context, "color palette module member '$moduleName' in invalid.\n"+
                        "Must be used like 'colors_symbol(`symbol for printing the color palette`)'.\n"+
                        "e.g 'colors_symbol(@)' or 'colors_symbol(string)'")

            val sym = moduleName.substring("colors_symbol(".length, moduleName.length-1)
            sysInfoInsert(parse("\${\\\\e[30m} $sym \${\\\\e[31m} $sym \${{\\\\e[32m}} $sym \${{\\\\e[33m}} $sym \${{\\\\e[34m}} $sym \${{\\\\e[35m}} $sym \${{\\\\e[36m}} $sym \${{\\\\e[37m}} $sym \${0}", s, parseArgs))
        }
    }

    else if (moduleName == "colors_symbol_light") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            if (moduleName.length <= "colors_symbol_light()".length)
                die(parseArgs.context, "light color palette module member '$moduleName' in invalid.\n"+
                        "Must be used like 'colors_symbol_light(`symbol for printing the color palette`)'.\n"+
                        "e.g 'colors_symbol_light(@)' or 'colors_symbol_light(string)'")

            val sym = moduleName.substring("colors_symbol_light(".length, moduleName.length-1)
            sysInfoInsert(parse("\${\\\\e[90m} $sym \${\\\\e[91m} $sym \${{\\\\e[92m}} $sym \${{\\\\e[93m}} $sym \${{\\\\e[94m}} $sym \${{\\\\e[95m}} $sym \${{\\\\e[96m}} $sym \${{\\\\e[97m}} $sym \${0}", s, parseArgs))
        }
    }

    else if (moduleName == "cpu") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val queryCPU = Cpu()
            sysInfoInsert("%s (%s) @ %.2f GHz".format(queryCPU.name, queryCPU.nproc, queryCPU.freqMax))
        }
    }

    else if (moduleName == "gpu") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val queryGpu = Gpu()
            sysInfoInsert(queryGpu.vendor + " " + queryGpu.name)
        }
    }

    else if (moduleName == "ram") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val queryRam = Ram()
            val byteUnits = listOf(
                autoDivideBytes(queryRam.usedAmount * byteUnit, byteUnit),
                autoDivideBytes(queryRam.totalAmount * byteUnit, byteUnit)
            )
            val perc = getAndColorPercentage(queryRam.usedAmount, queryRam.totalAmount, parseArgs)
            val str = parse("%.2f %s / %.2f %s \${0}()".format(
                byteUnits[0].numBytes, byteUnits[0].unit,
                byteUnits[1].numBytes, byteUnits[1].unit
            ), s, parseArgs)
            str.insert(str.length-1, perc)
            sysInfoInsert(str)
        }
    }

    else if (moduleName == "swap") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val querySwap = Swap()
            val byteUnits = listOf(
                autoDivideBytes(querySwap.usedAmount * byteUnit, byteUnit),
                autoDivideBytes(querySwap.totalAmount * byteUnit, byteUnit)
            )
            val perc = getAndColorPercentage(querySwap.usedAmount, querySwap.totalAmount, parseArgs)
            val str = parse("%.2f %s / %.2f %s \${0}()".format(
                byteUnits[0].numBytes, byteUnits[0].unit,
                byteUnits[1].numBytes, byteUnits[1].unit
            ), s, parseArgs)
            str.insert(str.length-1, perc)
            sysInfoInsert(str)
        }
    }

    else if (moduleName.startsWith("disk")) {
        if (moduleName.length < "disk()".length)
            die(parseArgs.context, "invalid disk module name '$moduleName', must be disk(/path/to/fs) e.g: disk(/)")

        val path = moduleName.substring(5, moduleName.length-1)
        val queryDisk = Disk(path, parseArgs)

        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val byteUnits = listOf(
                autoDivideBytes(queryDisk.usedAmount, byteUnit),
                autoDivideBytes(queryDisk.totalAmount, byteUnit)
            )

            val perc = getAndColorPercentage(queryDisk.usedAmount, queryDisk.totalAmount, parseArgs)
            val result = parse("%.2f %s / %.2f %s \${0}()".format(
                byteUnits[0].numBytes, byteUnits[0].unit,
                byteUnits[1].numBytes, byteUnits[1].unit
            ), s, parseArgs)
            result.insert(result.length-1, perc)

            if (queryDisk.typefs != MAGIC_LINE)
                result.append(" - ${queryDisk.typefs}")

            var typesDisksStr = "["
            if (queryDisk.typesDisk has DiskVolumeType.EXTERNAL)
                typesDisksStr += "External, "
            if (queryDisk.typesDisk has DiskVolumeType.HIDDEN)
                typesDisksStr += "Hidden, "
            if (queryDisk.typesDisk has DiskVolumeType.READ_ONLY)
                typesDisksStr += "Read-only, "

            if (typesDisksStr.length > 3) {
                typesDisksStr = typesDisksStr.removeRange(typesDisksStr.length - 2, typesDisksStr.length)
                result.append(" $typesDisksStr]")
            }

            sysInfoInsert(result)
        }
    }

    else if (moduleName == "battery") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            val queryBattery = Battery(parseArgs.context)
            val perc = getAndColorPercentage(queryBattery.perc, 100.0, parseArgs, true)
            perc.append(" [${queryBattery.status}]")
            sysInfoInsert(perc)
        }
    }

    else
        die(parseArgs.context, "Invalid module name: $moduleName")
}