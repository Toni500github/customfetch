package org.toni.customfetch_android.widget

import android.app.Activity
import android.appwidget.AppWidgetManager
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.text.SpannableStringBuilder
import android.text.Spanned
import android.text.TextPaint
import android.text.TextUtils.TruncateAt
import android.text.TextUtils.ellipsize
import android.view.View
import android.widget.CheckBox
import android.widget.EditText
import android.widget.TextView
import androidx.core.text.HtmlCompat
import org.toni.customfetch_android.copyToAssetFolder
import org.toni.customfetch_android.databinding.CustomfetchConfigureBinding
import java.nio.file.Files
import kotlin.io.path.Path

var disableLineWrap = false

/**
 * The configuration screen for the [customfetch] AppWidget.
 */
class customfetchConfigureActivity : Activity() {
    private var appWidgetId = AppWidgetManager.INVALID_APPWIDGET_ID
    private lateinit var appWidgetText: EditText
    private lateinit var argsHelp: TextView
    private lateinit var showModulesList: CheckBox
    private lateinit var disableWrapLines: CheckBox
    private var onClickListener = View.OnClickListener {
        val context = this@customfetchConfigureActivity

        // When the button is clicked, store the string locally
        val widgetText = appWidgetText.text.toString()
        saveTitlePref(context, appWidgetId, widgetText)

        // It is the responsibility of the configuration activity to update the app widget
        val appWidgetManager = AppWidgetManager.getInstance(context)
        updateAppWidget(context, appWidgetManager, appWidgetId)

        // Make sure we pass back the original appWidgetId
        val resultValue = Intent()
        resultValue.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, appWidgetId)
        setResult(RESULT_OK, resultValue)
        finish()
    }

    private lateinit var binding: CustomfetchConfigureBinding
    public override fun onCreate(icicle: Bundle?) {
        super.onCreate(icicle)

        // Set the result to CANCELED.  This will cause the widget host to cancel
        // out of the widget placement if the user presses the back button.
        setResult(RESULT_CANCELED)

        binding = CustomfetchConfigureBinding.inflate(layoutInflater)
        setContentView(binding.root)

        if (!Files.exists(Path(filesDir.absolutePath + "ascii")))
            copyToAssetFolder(assets, filesDir.absolutePath, "ascii")

        appWidgetText = binding.appwidgetText
        argsHelp = binding.argsHelp
        showModulesList = binding.showModulesList
        disableWrapLines = binding.disableWrapLines
        binding.addButton.setOnClickListener(onClickListener)

        // Find the widget id from the intent.
        val extras = intent.extras
        if (extras != null) {
            appWidgetId = extras.getInt(
                AppWidgetManager.EXTRA_APPWIDGET_ID, AppWidgetManager.INVALID_APPWIDGET_ID
            )
        }

        // If this activity was started with an intent without an app widget ID, finish with an error.
        if (appWidgetId == AppWidgetManager.INVALID_APPWIDGET_ID) {
            finish()
            return
        }

        appWidgetText.setText(loadTitlePref(this@customfetchConfigureActivity, appWidgetId))
        argsHelp.text = customfetchRender.mainAndroid("customfetch --help")

        showModulesList.setOnCheckedChangeListener { _, isChecked ->
            if (isChecked)
                argsHelp.text = customfetchRender.mainAndroid("customfetch -l")
            else
                argsHelp.text = customfetchRender.mainAndroid("customfetch --help")
        }
        disableWrapLines.setOnCheckedChangeListener { _, isChecked ->
            disableLineWrap = isChecked
        }
    }
}

class CustomfetchMainRender {
    fun getParsedContent(context: Context, appWidgetId: Int, width: Float, disableLineWrap: Boolean, otherArguments: String = ""): SpannableStringBuilder {
        val parsedContent = SpannableStringBuilder()
        val arguments = otherArguments.ifEmpty {
            loadTitlePref(context, appWidgetId)
        }

        val htmlContent = mainAndroid("customfetch $arguments")

        if (disableLineWrap) {
            val eachLine = htmlContent!!.split("<br>").map { it.trim() }
            val paint = TextPaint()//.apply { textSize = 7f }
            for (line in eachLine) {
                var parsedLine = HtmlCompat.fromHtml(line, HtmlCompat.FROM_HTML_MODE_COMPACT)
                parsedLine =
                    ellipsize(parsedLine, paint, width, TruncateAt.END) as Spanned
                parsedContent.appendLine(parsedLine)
            }
        } else {
            parsedContent.append(htmlContent?.let {
                HtmlCompat.fromHtml(
                    it,
                    HtmlCompat.FROM_HTML_MODE_COMPACT
                )
            })
        }

        return parsedContent
    }

    external fun mainAndroid(argv: String): String?
    companion object {
        init {
            System.loadLibrary("customfetch")
        }
    }
}
val customfetchRender = CustomfetchMainRender()

private const val PREFS_NAME = "org.toni.customfetch_android.customfetch"
private const val PREF_PREFIX_KEY = "appwidget_"

// Write the prefix to the SharedPreferences object for this widget
internal fun saveTitlePref(context: Context, appWidgetId: Int, text: String) {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0).edit()
    prefs.putString(PREF_PREFIX_KEY + appWidgetId, text)
    prefs.apply()
}

// Read the prefix from the SharedPreferences object for this widget.
// If there is no preference saved, get the default from a resource
internal fun loadTitlePref(context: Context, appWidgetId: Int): String {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    val titleValue = prefs.getString(PREF_PREFIX_KEY + appWidgetId, null)
    return titleValue ?: "-D ${context.filesDir.absolutePath} -a small"
}

internal fun deleteTitlePref(context: Context, appWidgetId: Int) {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0).edit()
    prefs.remove(PREF_PREFIX_KEY + appWidgetId)
    prefs.apply()
}
