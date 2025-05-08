/*
 * Copyright 2025 Toni500git
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

package org.toni.customfetch_android.widget

import android.app.PendingIntent
import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.Context
import android.content.Intent
import android.content.res.Configuration.ORIENTATION_PORTRAIT
import android.os.Bundle
import android.text.TextPaint
import android.util.Log
import android.widget.RemoteViews
import org.toni.customfetch_android.R

const val WIDGET_CLICK_ACTION = "org.toni.customfetch_android.WIDGET_CLICK"

/**
 * Implementation of App Widget functionality.
 * App Widget Configuration implemented in [CustomfetchConfigureActivity]
 */
class Customfetch : AppWidgetProvider() {
    override fun onUpdate(
        context: Context,
        appWidgetManager: AppWidgetManager,
        appWidgetIds: IntArray
    ) {
        // There may be multiple widgets active, so update all of them
        for (appWidgetId in appWidgetIds) {
            updateAppWidget(context, appWidgetManager, appWidgetId)
        }
    }

    override fun onDeleted(context: Context, appWidgetIds: IntArray) {
        // When the user deletes the widget, delete the preference associated with it.
        for (appWidgetId in appWidgetIds) {
            deleteConfigPrefs(context, appWidgetId)
        }
    }

    override fun onReceive(context: Context?, intent: Intent?) {
        super.onReceive(context, intent)
        Log.d("onReceiveTest", "intent.action = ${intent?.action}")
        if (context != null && intent?.action == WIDGET_CLICK_ACTION) {
            val appWidgetId = intent.getIntExtra(
                AppWidgetManager.EXTRA_APPWIDGET_ID,
                AppWidgetManager.INVALID_APPWIDGET_ID
            )

            if (appWidgetId != AppWidgetManager.INVALID_APPWIDGET_ID) {
                Log.d("onReceiveTest", "Widget clicked!")
                updateAppWidget(context, AppWidgetManager.getInstance(context), appWidgetId)
            }
        }
    }

    override fun onAppWidgetOptionsChanged(
        context: Context,
        appWidgetManager: AppWidgetManager,
        appWidgetId: Int,
        newOptions: Bundle
    ) {
        updateAppWidget(context, appWidgetManager, appWidgetId)
    }
}

// https://stackoverflow.com/a/58501760
class WidgetSizeProvider(
    private val context: Context // Do not pass Application context
) {

    private val appWidgetManager = AppWidgetManager.getInstance(context)

    fun getWidgetsSize(widgetId: Int): Pair<Int, Int> {
        val isPortrait = (context.resources.configuration.orientation == ORIENTATION_PORTRAIT)
        val width = getWidgetWidth(isPortrait, widgetId)
        val height = getWidgetHeight(isPortrait, widgetId)
        val widthInPx = context.dip(width)
        val heightInPx = context.dip(height)
        return widthInPx to heightInPx
    }

    private fun getWidgetWidth(isPortrait: Boolean, widgetId: Int): Int =
        if (isPortrait) {
            getWidgetSizeInDp(widgetId, AppWidgetManager.OPTION_APPWIDGET_MIN_WIDTH)
        } else {
            getWidgetSizeInDp(widgetId, AppWidgetManager.OPTION_APPWIDGET_MAX_WIDTH)
        }

    private fun getWidgetHeight(isPortrait: Boolean, widgetId: Int): Int =
        if (isPortrait) {
            getWidgetSizeInDp(widgetId, AppWidgetManager.OPTION_APPWIDGET_MAX_HEIGHT)
        } else {
            getWidgetSizeInDp(widgetId, AppWidgetManager.OPTION_APPWIDGET_MIN_HEIGHT)
        }

    private fun getWidgetSizeInDp(widgetId: Int, key: String): Int =
        appWidgetManager.getAppWidgetOptions(widgetId).getInt(key, 0)

    private fun Context.dip(value: Int): Int = (value * resources.displayMetrics.density).toInt()

}

internal fun updateAppWidget(
    context: Context,
    appWidgetManager: AppWidgetManager,
    appWidgetId: Int
) {
    // if we are configuring the widget, then don't update the non existent widget
    if (!isWidgetConfigured(context, appWidgetId))
        return

    val truncateText = getTruncateText(context, appWidgetId)
    val bgColor = getBgColor(context, appWidgetId)
    val widgetTextColor = getWidgetTextColor(context, appWidgetId)

    // create a TextPaint to be used to measure text size
    val textPaint = TextPaint()
    val textSizeSp = 8f
    val textSizePx = textSizeSp * context.resources.displayMetrics.scaledDensity
    textPaint.textSize = textSizePx

    val additionalTruncateWidth = getTruncateWidthPref(context, appWidgetId).toFloat()
    var width = WidgetSizeProvider(context).getWidgetsSize(appWidgetId).first.toFloat()
    if (additionalTruncateWidth > 0.20)
        width *= additionalTruncateWidth

    Log.d("widthTesting", "textSizePx = $textSizePx")
    Log.d("widthTesting", "width = $width")

    val parsedContent =
        getParsedContent(
            context,
            appWidgetId,
            width,
            truncateText,
            textPaint
        )

    // both needed for when touching the widget
    val intent = Intent(context, Customfetch::class.java).apply {
        action = WIDGET_CLICK_ACTION
        putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, appWidgetId)
    }

    val pendingIntent = PendingIntent.getBroadcast(
        context,
        appWidgetId,
        intent,
        PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
    )

    // Construct the RemoteViews object
    val views = RemoteViews(context.packageName, R.layout.customfetch)
    views.setOnClickPendingIntent(R.id.widget_root, pendingIntent)
    views.setTextColor(R.id.customfetch_text, widgetTextColor)
    views.setTextViewText(R.id.customfetch_text, parsedContent)
    views.setInt(R.id.widget_root, "setBackgroundColor", bgColor)

    // Instruct the widget manager to update the widget
    appWidgetManager.updateAppWidget(appWidgetId, views)
}
