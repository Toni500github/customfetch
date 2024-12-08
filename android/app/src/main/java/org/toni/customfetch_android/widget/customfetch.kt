package org.toni.customfetch_android.widget

import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.Context
import android.os.Bundle
import android.text.SpannableStringBuilder
import android.text.Spanned
import android.text.TextPaint
import android.text.TextUtils.TruncateAt
import android.text.TextUtils.ellipsize
import android.util.TypedValue
import android.widget.RemoteViews
import androidx.core.text.HtmlCompat
import org.toni.customfetch_android.R


/**
 * Implementation of App Widget functionality.
 * App Widget Configuration implemented in [customfetchConfigureActivity]
 */
class customfetch : AppWidgetProvider() {
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
            deleteTitlePref(context, appWidgetId)
        }
    }

    override fun onAppWidgetOptionsChanged(
        context: Context,
        appWidgetManager: AppWidgetManager,
        appWidgetId: Int,
        newOptions: Bundle
    ) {
        // Get the new widget size
        val minWidthDp = newOptions.getInt(AppWidgetManager.OPTION_APPWIDGET_MIN_WIDTH)
        val maxWidthDp = newOptions.getInt(AppWidgetManager.OPTION_APPWIDGET_MAX_WIDTH)
        val minWidthPx = convertDpToPx(context, minWidthDp)
        val maxWidthPx = convertDpToPx(context, maxWidthDp)

        val parsedContent = SpannableStringBuilder()
        val arguments = loadTitlePref(context, appWidgetId)
        val htmlContent = customfetchConfigureActivity().mainAndroid("customfetch $arguments")

        val eachLine = htmlContent!!.split("<br>").map { it.trim() }
        val paint = TextPaint()
        for (line in eachLine) {
            var parsedLine = HtmlCompat.fromHtml(line, HtmlCompat.FROM_HTML_MODE_LEGACY)
            parsedLine = ellipsize(parsedLine, paint, minWidthDp * 0.8f, TruncateAt.END) as Spanned
            parsedContent.appendLine(parsedLine)
        }

        val views = RemoteViews(context.packageName, R.layout.customfetch)
        views.setTextViewText(R.id.customfetch_text, parsedContent)
        appWidgetManager.updateAppWidget(appWidgetId, views)
    }

    private fun convertDpToPx(context: Context, dp: Int): Int {
        return TypedValue.applyDimension(
            TypedValue.COMPLEX_UNIT_DIP,
            dp.toFloat(),
            context.resources.displayMetrics
        ).toInt()
    }

    override fun onEnabled(context: Context) {
        // Enter relevant functionality for when the first widget is created
    }

    override fun onDisabled(context: Context) {
        // Enter relevant functionality for when the last widget is disabled
    }
}

internal fun updateAppWidget(
    context: Context,
    appWidgetManager: AppWidgetManager,
    appWidgetId: Int,
    initConfigureActivity: Boolean = false
) {
    // Construct the RemoteViews object
    val views = RemoteViews(context.packageName, R.layout.customfetch)
    views.setTextViewText(R.id.customfetch_text, "Loading...")

    // Instruct the widget manager to update the widget
    appWidgetManager.updateAppWidget(appWidgetId, views)
}