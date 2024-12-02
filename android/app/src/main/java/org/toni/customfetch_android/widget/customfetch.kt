package org.toni.customfetch_android.widget

import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.Context
import android.text.Spanned
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
    appWidgetId: Int
) {
    val arguments = loadTitlePref(context, appWidgetId)
    val htmlContent = customfetchConfigureActivity().mainAndroid("customfetch $arguments")

    // Construct the RemoteViews object
    val views = RemoteViews(context.packageName, R.layout.customfetch)
    views.setTextViewText(R.id.customfetch_text, htmlContent?.let { HtmlCompat.fromHtml(it, HtmlCompat.FROM_HTML_MODE_LEGACY) })

    // Instruct the widget manager to update the widget
    appWidgetManager.updateAppWidget(appWidgetId, views)
}