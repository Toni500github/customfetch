package org.toni.customfetch_android_lib

import org.toni.customfetch_android_lib.ParserFunctions.parse
import org.toni.customfetch_android_lib.query.CPU
import org.toni.customfetch_android_lib.query.Disk
import org.toni.customfetch_android_lib.query.System
import org.toni.customfetch_android_lib.query.User

// useless useful tmp string for parse() without using the original
// pureOutput
var s = StringBuilder()

const val UNKNOWN = "(unknown)"
// Usually in neofetch/fastfetch when some infos couldn't be queried,
// they remove it from the display. With customfetch is kinda difficult to know when to remove
// the info to display, since it's all modular with tags, so I have created
// magic line to be sure that I don't cut the wrong line.
//
// Every instance of this string in a layout line, the whole line will be erased.
const val MAGIC_LINE = "(cut this line NOW!! RAHHH)";

typealias SystemInfo = MutableMap<String, MutableMap<String, Variant>>
sealed class Variant {
    data class StringVal(val value: String) : Variant()
    data class SizeT(val value: ULong) : Variant()
    data class DoubleVal(val value: Double) : Variant()
}

data class ParseArgs(
    val systemInfo: SystemInfo,
    val pureOutput: StringBuilder,
    val layout: MutableList<String>,
    val tmpLayout: MutableList<String>,
    val config: Config,
    var parsingLayout: Boolean,
    var firstrunClr: Boolean = true,
    var noMoreReset: Boolean = false,
    var endspan: String = ""
)

class Parser(val src: String, val pureOutput: StringBuilder) {
    var dollarPos: Int = 0
    private var pos: Int = 0

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

object ParserFunctions {
    private fun parseConditionalTag(parser: Parser, parseArgs: ParseArgs, evaluate: Boolean = true): String? {
        if (!parser.tryRead('[')) return null

        val condA = parse(parser, parseArgs, evaluate, ',')
        val condB = parse(parser, parseArgs, evaluate, ',')

        val cond = (condA == condB)

        val condTrue = parse(parser, parseArgs, cond, ',')
        val condFalse = parse(parser, parseArgs, !cond, ']')

        return if (cond) condTrue else condFalse
    }

    private fun parseInfoTag(parser: Parser, parseArgs: ParseArgs, evaluate: Boolean = true): String? {
        if (!parser.tryRead('<')) return null

        val module = parse(parser, parseArgs, evaluate, '>')

        if (!evaluate) return null

        val dotPos = module.indexOf('.')
        if (dotPos == -1) {
            addValueFromModule(module, parseArgs)
            val info = getInfoFromName(parseArgs.systemInfo, module, "module-$module")

            if (parser.dollarPos != -1) {
                parseArgs.pureOutput.replace(
                    parser.dollarPos,
                    parser.dollarPos + module.length + "$<>".length,
                    info
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
                info
            )
        }
        return info
    }

    private fun parseTags(parser: Parser, parseArgs: ParseArgs, evaluate: Boolean = true): String? {
        if (!parser.tryRead('$')) return null

        if (parser.dollarPos != -1) {
            parser.dollarPos = parser.pureOutput.indexOf('$', parser.dollarPos)
        }

        parseConditionalTag(parser, parseArgs, evaluate)?.let {
            return it
        }
        parseInfoTag(parser, parseArgs, evaluate)?.let {
            return it
        }

        parser.rewind()
        return null
    }

    fun parse(parser: Parser, parseArgs: ParseArgs, evaluate: Boolean = true, until: Char = 0.toChar()): String {
        val result = StringBuilder()

        while (if (until == 0.toChar()) !parser.isEof() else !parser.tryRead(until)) {
            if (until != 0.toChar() && parser.isEof()) {
                error("PARSER: Missing tag close bracket $until in string '${parser.src}'")
                return result.toString()
            }

            if (parser.tryRead('\\')) {
                result.append(parser.readChar(until == 0.toChar()))
            } else {
                parseTags(parser, parseArgs, evaluate)?.let {
                    result.append(it)
                } ?: run {
                    result.append(parser.readChar(until == 0.toChar()))
                }
            }
        }

        return result.toString()
    }

    fun parse(input: String, parseArgs: ParseArgs): String =
        parse(input, parseArgs.systemInfo, parseArgs.pureOutput, parseArgs.layout, parseArgs.tmpLayout, parseArgs.config, parseArgs.parsingLayout, BooleanWrapper(parseArgs.noMoreReset))

    fun parse(input: String, s: StringBuilder, parseArgs: ParseArgs): String =
        parse(input, parseArgs.systemInfo, s, parseArgs.layout, parseArgs.tmpLayout, parseArgs.config, parseArgs.parsingLayout, BooleanWrapper(parseArgs.noMoreReset))

    fun parse(
        input: String,
        systemInfo: SystemInfo,
        pureOutput: StringBuilder,
        layout: MutableList<String>,
        tmpLayout: MutableList<String>,
        config: Config,
        parsingLayout: Boolean,
        noMoreReset: BooleanWrapper // Using wrapper for mutable boolean reference
    ): String {
        var modifiedInput = input

        if (config.t.sepReset.isNotEmpty() && parsingLayout && !noMoreReset.value) {
            modifiedInput = if (config.t.sepResetAfter) {
                modifiedInput.replace(config.t.sepReset, "${config.t.sepReset}\${0}")
            } else {
                modifiedInput.replace(config.t.sepReset, "\${0}${config.t.sepReset}")
            }
            noMoreReset.value = true
        }

        /*if (!parsingLayout) {
            modifiedInput = modifiedInput.replace(" ", "&nbsp;")
        }*/

        val parseArgs = ParseArgs(
            systemInfo,
            pureOutput,
            layout,
            tmpLayout,
            config,
            parsingLayout,
            true,
            noMoreReset.value,
            ""
        )

        val result = parse(Parser(modifiedInput, pureOutput), parseArgs)

        val pureOutputStr = pureOutput.toString()//.replace("&nbsp;", " ")
        pureOutput.clear()
        pureOutput.append(pureOutputStr)

        return if (parseArgs.firstrunClr) result else result + parseArgs.endspan
    }

    // Helper class for mutable boolean reference
    class BooleanWrapper(var value: Boolean)
}

fun getInfoFromName(
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
            }
        }
    }
    return "(unknown/invalid module)"
}

private fun getAutoUptime(uptimeSecs: Long, config: Config): String {
    val uptimeMins = uptimeSecs / 60
    val uptimeHours = uptimeSecs / 3600
    val uptimeDays = uptimeSecs / (3600 * 24)

    val remainingSecs = (uptimeSecs % 60)
    val remainingMins = (uptimeMins % 60)
    val remainingHours = (uptimeHours % 24)

    if (uptimeDays == 0L && remainingHours == 0L && remainingMins == 0L)
        return "$remainingSecs${config.osUptime.secondsFormat}"

    val parts = mutableListOf<String>()
    if (uptimeDays > 0L) parts.add("$uptimeDays${config.osUptime.daysFormat}")
    if (remainingHours > 0L) parts.add("$remainingHours${config.osUptime.hoursFormat}")
    if (remainingMins > 0L) parts.add("$remainingMins${config.osUptime.minutesFormat}")

    return parts.joinToString(", ")
}

// C enums-like because I ain't writing MemoryMetrics.FREE.index
const val USED = 0
const val FREE = 1
const val TOTAL = 2

val queriedPaths: SystemInfo = mutableMapOf()
fun addValueFromModuleMember(moduleName: String, moduleMemberName: String, parseArgs: ParseArgs) {
    // Aliases for convention
    val config = parseArgs.config
    val sysInfo = parseArgs.systemInfo

    fun sysInfoInsert(value: Any) {
        sysInfo.getOrPut(moduleName) { mutableMapOf() }[moduleMemberName] = when (value) {
            is String -> Variant.StringVal(value)
            is Double -> Variant.DoubleVal(value)
            is ULong -> Variant.SizeT(value)
            else -> throw IllegalArgumentException("Unsupported variant type")
        }
    }

    val byteUnit = if (config.t.useSIByteUnit) 1000 else 1024
    val sortedValidPrefixes = arrayOf("B", "EB", "EiB", "GB", "GiB", "kB",
        "KiB", "MB", "MiB", "PB", "PiB", "TB",
        "TiB", "YB", "YiB", "ZB", "ZiB")

    val returnDividedBytes = { amount: Double ->
        val prefix = moduleMemberName.substringAfter('-')
        if (sortedValidPrefixes.binarySearch(prefix) >= 0) {
            divideBytes(amount, prefix).numBytes
        } else {
            0.0
        }
    }

    if (moduleName == "os") {
        val querySystem = System.getInstance()

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
                "packages", "pkgs" -> sysInfoInsert(querySystem.pkgsInstalled(config))
                "initsys_name" -> sysInfoInsert(querySystem.osInitsysName)
                "initsys_version" -> sysInfoInsert(querySystem.osInitsysVersion)
                "hostname" -> sysInfoInsert(querySystem.hostname)
                "version_codename" -> sysInfoInsert(querySystem.osVersionCodename)
                "version_id" -> sysInfoInsert(querySystem.osVersionId)
            }
        }
    }
    else if (moduleName == "system") {
        val querySystem = System.getInstance()

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
        val queryCPU = CPU.getInstance()

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
            throw IllegalArgumentException("invalid disk module name '$moduleName', must be disk(/path/to/fs) e.g: disk(/)")

        val path = moduleName.substring(6, moduleName.length-1)
        val queryDisk = Disk.getInstance(path, queriedPaths, parseArgs)

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

                "used" -> sysInfoInsert("%.2f %s".format(byteUnits[USED].numBytes, byteUnits[USED].unit))
                "free" -> sysInfoInsert("%.2f %s".format(byteUnits[FREE].numBytes, byteUnits[FREE].unit))
                "total" -> sysInfoInsert("%.2f %s".format(byteUnits[TOTAL].numBytes, byteUnits[TOTAL].unit))

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

    fun sysInfoInsert(value: Any) {
        sysInfo.getOrPut(moduleName) { mutableMapOf() }[moduleMemberName] = when (value) {
            is String -> Variant.StringVal(value)
            is Double -> Variant.DoubleVal(value)
            is ULong -> Variant.SizeT(value)
            else -> throw IllegalArgumentException("Unsupported variant type")
        }
    }

    if (moduleName == "title") {
        if (!sysInfo.containsKey(moduleName)) {
            sysInfo[moduleName] = mutableMapOf()
        }

        if (!sysInfo[moduleName]!!.containsKey(moduleMemberName)) {
            sysInfoInsert(parse("\${auto2}$<user.name>${0}@\${auto2}$<os.hostname>", s, parseArgs))
        }
    }
}