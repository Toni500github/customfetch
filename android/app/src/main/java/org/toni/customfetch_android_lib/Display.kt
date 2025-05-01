package org.toni.customfetch_android_lib

import org.toni.customfetch_android_lib.ParserFunctions.parse
import java.io.File
import java.nio.file.Files

fun render(config: Config, file: File): List<String> {
    val systemInfo: SystemInfo = mutableMapOf()
    val asciiArt: ArrayList<String> = arrayListOf("")
    val pureAsciiArtLens: ArrayList<UInt> = arrayListOf(0U)
    val layout = config.t.layout as MutableList<String>
    var maxLineLength = -1

    val mimeType = Files.probeContentType(file.toPath())
    if (mimeType.startsWith("image/"))
        throw IllegalArgumentException("Customfetch android app does not support images as logos")

    for (i in 0..config.t.logoPaddingTop) {
        pureAsciiArtLens.add(0U)
        asciiArt.add("")
    }

    for (i in 0..config.t.layoutPaddingTop) {
        layout.add(0, "")
    }

    Files.readAllLines(file.toPath()).forEach { line ->
        val pureOutput = StringBuilder()
        val tmpLayout = arrayListOf("")
        val parseArgs = ParseArgs(systemInfo, pureOutput, layout, tmpLayout, config, false)

        val asciiArtStr = parse(line, parseArgs)
        parseArgs.noMoreReset = false
        asciiArt.add(asciiArtStr)

        val pureOutputLen = asciiArtStr.codePoints().count()
        if (pureOutputLen > maxLineLength)
            maxLineLength = pureOutputLen.toInt()

        pureAsciiArtLens.add(pureOutputLen.toUInt())
    }

    val s = StringBuilder()
    val tmpLayout = arrayListOf("")
    val parseArgs = ParseArgs(systemInfo, s, layout, tmpLayout, config, false)
    var i = 0
    while (i < layout.size) {
        layout[i] = parse(layout[i], parseArgs)
        parseArgs.noMoreReset = false

        if (tmpLayout.isNotEmpty()) {
            layout.removeAt(i)
            layout.addAll(i, tmpLayout)
            i += tmpLayout.size - 1
            tmpLayout.clear()
        }
        i++
    }

    layout.removeAll { str -> str.contains("(cut this line NOW!! RAHH)") }

    if (config.t.logoPosition == "top" || config.t.logoPosition == "bottom") {
        if (asciiArt.isNotEmpty()) {
            val insertPosition = if (config.t.logoPosition == "top") 0 else layout.size
            layout.addAll(insertPosition, asciiArt)
        }
        return layout
    }

    i = 0
    while (i < layout.size) {
        var currentLine = layout[i]
        var origin = config.t.logoPaddingLeft

        // The user-specified offset to be put before the logo
        currentLine = " ".repeat(config.t.logoPaddingLeft) + currentLine

        if (i < asciiArt.size) {
            currentLine = currentLine.substring(0, origin) + asciiArt[i] + currentLine.substring(origin)
            origin += asciiArt[i].length
        }

        val spaces = (maxLineLength + config.t.offset) -
                     (if (i < asciiArt.size) pureAsciiArtLens[i].toInt() else 0)

        currentLine = currentLine.substring(0, origin) + " ".repeat(spaces) + currentLine.substring(origin)
        layout[i] = currentLine
        i++
    }
    while (i < asciiArt.size) {
        val line = " ".repeat(config.t.logoPaddingLeft) + asciiArt[i]
        layout.add(line)
        i++
    }

    return layout
}