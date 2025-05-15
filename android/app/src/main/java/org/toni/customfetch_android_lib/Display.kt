package org.toni.customfetch_android_lib

import android.content.Context
import android.text.SpannableStringBuilder
import org.toni.customfetch_android_lib.ParserFunctions.parse
import java.io.File
import java.nio.file.Files

fun render(context: Context, appWidgetId: Int, config: Config, asciiFile: File): List<SpannableStringBuilder> {
    val systemInfo: SystemInfo = mutableMapOf()
    val asciiArt: ArrayList<SpannableStringBuilder> = arrayListOf()
    val pureAsciiArtLens: ArrayList<Int> = arrayListOf()
    val layout: MutableList<SpannableStringBuilder> = if (config.args.layout.isEmpty())
        config.t.layout.map { SpannableStringBuilder(it) }.toMutableList()
        else config.args.layout.map { SpannableStringBuilder(it) }.toMutableList()
    var maxLineLength = -1

    val mimeType = Files.probeContentType(asciiFile.toPath())
    if (!mimeType.startsWith("text/"))
        die(context, "Customfetch widget app does not support images as logos")

    for (i in 1..config.t.logoPaddingTop) {
        pureAsciiArtLens.add(0)
        asciiArt.add(SpannableStringBuilder(""))
    }

    for (i in 1..config.t.layoutPaddingTop) {
        layout.add(0, SpannableStringBuilder(""))
    }

    if (!config.args.disableSource) {
        Files.readAllLines(asciiFile.toPath()).forEach { line ->
            val pureOutput = StringBuilder()
            val tmpLayout = arrayListOf<SpannableStringBuilder>()
            val parseArgs = ParseArgs(context, appWidgetId, systemInfo, pureOutput, layout, tmpLayout, config, false)

            val asciiArtStr = parse(line, parseArgs)
            parseArgs.noMoreReset = false
            asciiArt.add(asciiArtStr)

            val pureOutputLen = asciiArtStr.length
            if (pureOutputLen > maxLineLength)
                maxLineLength = pureOutputLen

            pureAsciiArtLens.add(pureOutputLen)
        }
    }

    if (config.args.printLogoOnly)
        return asciiArt

    val tmpLayout = arrayListOf<SpannableStringBuilder>()
    val parseArgs = ParseArgs(context, appWidgetId, systemInfo, s, layout, tmpLayout, config, true)
    var i = 0
    while (i < layout.size) {
        layout[i] = parse(layout[i].toString(), parseArgs)
        parseArgs.noMoreReset = false

        if (tmpLayout.isNotEmpty()) {
            layout.removeAt(i)
            layout.addAll(i, tmpLayout)
            i += tmpLayout.size - 1
            tmpLayout.clear()
        }
        i++
    }

    layout.removeAll { it.contains(MAGIC_LINE) }

    if (config.t.logoPosition == "top" || config.t.logoPosition == "bottom") {
        if (asciiArt.isNotEmpty()) {
            val insertPosition = if (config.t.logoPosition == "top") 0 else layout.size
            layout.addAll(insertPosition, asciiArt)
        }
        return layout
    }

    i = 0
    while (i < layout.size) {
        var origin = config.t.logoPaddingLeft

        // The user-specified offset to be put before the logo
        layout[i].insert(0, " ".repeat(config.t.logoPaddingLeft))

        if (i < asciiArt.size) {
            layout[i].insert(origin, asciiArt[i])
            origin += asciiArt[i].length
        }

        val spaces = (maxLineLength + (if (config.args.disableSource) 1 else config.t.offset)) -
                     (if (i < asciiArt.size) pureAsciiArtLens[i].toInt() else 0)

        layout[i].insert(origin, " ".repeat(spaces))
        i++
    }
    while (i < asciiArt.size) {
        val line = SpannableStringBuilder(" ".repeat(config.t.logoPaddingLeft))
        layout.add(line)
        layout.add(asciiArt[i])
        i++
    }

    return layout
}