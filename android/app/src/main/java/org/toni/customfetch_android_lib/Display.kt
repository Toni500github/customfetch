package org.toni.customfetch_android_lib

import android.content.Context
import android.text.SpannableStringBuilder
import android.util.Log
import org.toni.customfetch_android_lib.ParserFunctions.parse
import java.io.File
import java.nio.file.Files

fun joinSpannableBuilders(list: List<SpannableStringBuilder>): SpannableStringBuilder {
    return SpannableStringBuilder().apply {
        list.forEach { spannable ->
            append(spannable)
            append("\n")
        }
    }
}

fun render(context: Context, appWidgetId: Int, config: Config, asciiFile: File): List<SpannableStringBuilder> {
    val systemInfo: SystemInfo = mutableMapOf()
    val asciiArt: ArrayList<SpannableStringBuilder> = arrayListOf()
    val pureAsciiArtLens: ArrayList<UInt> = arrayListOf()
    val layout: MutableList<SpannableStringBuilder> = if (config.args.layout.isEmpty())
        config.t.layout.map { SpannableStringBuilder(it) }.toMutableList()
        else config.args.layout.map { SpannableStringBuilder(it) }.toMutableList()
    var maxLineLength = -1

    val mimeType = Files.probeContentType(asciiFile.toPath())
    if (!mimeType.startsWith("text/"))
        throw IllegalArgumentException("Customfetch android app does not support images as logos")

    for (i in 1..config.t.logoPaddingTop) {
        pureAsciiArtLens.add(0U)
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

            pureAsciiArtLens.add(pureOutputLen.toUInt())
        }
    }

    Log.d("testingLmao", "render: maxLineLength = $maxLineLength")
    val s = StringBuilder()
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

    layout.removeAll { str -> str.contains(MAGIC_LINE) }

    if (config.t.logoPosition == "top" || config.t.logoPosition == "bottom") {
        if (asciiArt.isNotEmpty()) {
            val insertPosition = if (config.t.logoPosition == "top") 0 else layout.size
            layout.addAll(insertPosition, asciiArt)
        }
        return layout
    }

    i = 0
    while (i < layout.size) {
        val currentLine = layout[i]
        var origin = config.t.logoPaddingLeft

        // The user-specified offset to be put before the logo
        currentLine.insert(0, " ".repeat(config.t.logoPaddingLeft))

        if (i < asciiArt.size) {
            currentLine.insert(origin, asciiArt[i])
            origin += asciiArt[i].length
        }

        val spaces = (maxLineLength + if (config.args.disableSource) 1 else config.t.offset) -
                     (if (i < asciiArt.size) pureAsciiArtLens[i].toInt() else 0)

        currentLine.insert(origin, " ".repeat(spaces))
        layout[i] = currentLine
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