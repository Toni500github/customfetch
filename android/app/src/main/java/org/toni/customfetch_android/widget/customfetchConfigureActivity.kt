package org.toni.customfetch_android.widget

import android.annotation.SuppressLint
import android.app.Activity
import android.appwidget.AppWidgetManager
import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.Editable
import android.text.SpannableStringBuilder
import android.text.Spanned
import android.text.TextPaint
import android.text.TextUtils.TruncateAt
import android.text.TextUtils.ellipsize
import android.text.TextWatcher
import android.util.TypedValue
import android.view.View
import android.widget.CheckBox
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.RadioGroup
import android.widget.TextView
import android.widget.Toast
import androidx.core.graphics.toColorInt
import androidx.core.text.HtmlCompat
import com.skydoves.colorpickerview.ColorPickerView
import com.skydoves.colorpickerview.listeners.ColorEnvelopeListener
import com.skydoves.colorpickerview.sliders.AlphaSlideBar
import com.skydoves.colorpickerview.sliders.BrightnessSlideBar
import org.toni.customfetch_android.R
import org.toni.customfetch_android.databinding.CustomfetchConfigureBinding
import org.toni.customfetch_android.errorFile
import org.toni.customfetch_android.errorLock
import java.io.File
import java.nio.file.Files
import kotlin.io.path.Path


/**
 * The configuration screen for the [customfetch] AppWidget.
 */
class customfetchConfigureActivity : Activity() {
    private var appWidgetId = AppWidgetManager.INVALID_APPWIDGET_ID
    private lateinit var argumentsConfig: EditText
    private lateinit var additionalTruncateWidth: EditText
    private lateinit var argsHelp: TextView
    private lateinit var showModulesList: CheckBox
    private lateinit var disableWrapLinesCheck: CheckBox
    private lateinit var selectBgColor: RadioGroup
    private lateinit var colorPickerView: ColorPickerView
    private lateinit var colorPickerHex: EditText
    private lateinit var colorPreview: View
    private lateinit var brightnessSlideBar: BrightnessSlideBar
    private lateinit var alphaSlideBar: AlphaSlideBar
    private lateinit var customColorSelect: LinearLayout
    private var onClickListener = View.OnClickListener {
        val context = this@customfetchConfigureActivity

        // When the button is clicked, store the string locally
        saveConfigPrefs(
            context,
            appWidgetId,
            argumentsConfig.text.toString(),
            additionalTruncateWidth.text.toString(),
            disableLineWrap,
            bgColor)

        // It is the responsibility of the configuration activity to update the app widget
        val appWidgetManager = AppWidgetManager.getInstance(context)
        updateAppWidget(context, appWidgetManager, appWidgetId)

        // Make sure we pass back the original appWidgetId
        val resultValue = Intent()
        resultValue.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, appWidgetId)
        setResult(RESULT_OK, resultValue)
        finish()
    }

    // truncate text
    private var disableLineWrap = false
    private var bgColor = 0
    private lateinit var binding: CustomfetchConfigureBinding

    @SuppressLint("SetTextI18n", "ClickableViewAccessibility")
    public override fun onCreate(icicle: Bundle?) {
        super.onCreate(icicle)

        // Set the result to CANCELED.  This will cause the widget host to cancel
        // out of the widget placement if the user presses the back button.
        setResult(RESULT_CANCELED)

        binding = CustomfetchConfigureBinding.inflate(layoutInflater)
        setContentView(binding.root)

        argumentsConfig = binding.argumentsConfigure
        additionalTruncateWidth = binding.additionalTruncateN
        argsHelp = binding.argsHelp
        showModulesList = binding.showModulesList
        disableWrapLinesCheck = binding.disableWrapLines
        selectBgColor = binding.selectBgColor
        colorPickerView = binding.bgColor
        colorPreview = binding.colorPreview
        colorPickerHex = binding.bgColorHex
        brightnessSlideBar = binding.brightnessSlide
        alphaSlideBar = binding.alphaSlideBar
        customColorSelect = binding.customColorsSelect
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

        argumentsConfig.setText(getArgsPref(this@customfetchConfigureActivity, appWidgetId))
        additionalTruncateWidth.setText("0.6")
        argsHelp.text = customfetchRender.mainAndroid("customfetch --help")

        showModulesList.setOnCheckedChangeListener { _, isChecked ->
            argsHelp.text = customfetchRender.mainAndroid("customfetch ${if (isChecked) "-l" else "-h"}")
        }

        disableWrapLinesCheck.setOnCheckedChangeListener { _, isChecked ->
            disableLineWrap = isChecked
        }

        selectBgColor.setOnCheckedChangeListener { _, checkedId ->
            when (checkedId) {
                R.id.radio_system_bg_color -> {
                    customColorSelect.visibility = View.GONE
                    val typedValue = TypedValue()
                    this.theme.resolveAttribute(android.R.attr.colorBackground, typedValue, true)
                    bgColor = typedValue.data
                }

                R.id.radio_transparent_bg -> {
                    customColorSelect.visibility = View.GONE
                    bgColor = 0x00FFFFFF
                }

                R.id.radio_custom_colors -> {
                    customColorSelect.visibility = View.VISIBLE
                    // disable scroll when interacting with the color picker
                    colorPickerView.setOnTouchListener { view, _ ->
                        view.parent.requestDisallowInterceptTouchEvent(true)
                        false // allow colorPickerView to handle the touch event
                    }

                    // if modified edittext and it's valid, apply to the preview
                    // else if modified in the color picker, apply to the edittext
                    var hexColor = ""
                    colorPickerHex.addTextChangedListener (object : TextWatcher {
                        override fun afterTextChanged(s: Editable) {
                            val col = s.toString()
                            if (isValidHex(col)) {
                                colorPreview.setBackgroundColor(Color.parseColor(col))
                                colorPickerView.setInitialColor(col.toColorInt())
                                hexColor = col
                                bgColor = col.toColorInt()
                            }
                        }
                        override fun beforeTextChanged(p0: CharSequence?, p1: Int, p2: Int, p3: Int) {}
                        override fun onTextChanged(p0: CharSequence?, p1: Int, p2: Int, p3: Int) {}
                    })
                    var firstRun = true
                    colorPickerView.setColorListener(ColorEnvelopeListener { envelope, _ ->
                        if (firstRun)
                            hexColor = "#${envelope.hexCode}"

                        if (hexColor != "#${envelope.hexCode}") {
                            colorPickerHex.setText("#${envelope.hexCode}")
                            hexColor = "#${envelope.hexCode}"
                        }
                        colorPreview.setBackgroundColor(envelope.color)
                        bgColor = envelope.color
                        firstRun = false
                    })
                    colorPickerView.attachAlphaSlider(alphaSlideBar)
                    colorPickerView.attachBrightnessSlider(brightnessSlideBar)
                }
            }
        }
    }

    private fun isValidHex(color: String): Boolean =
        color.matches("^#[0-9A-Fa-f]{8}$".toRegex())
}

class CustomfetchMainRender {
    fun getParsedContent(
        context: Context,
        appWidgetId: Int,
        width: Float,
        disableLineWrap: Boolean,
        paint: TextPaint,
        otherArguments: String = "",
        postToast: Boolean = true
    ): SpannableStringBuilder {
        val parsedContent = SpannableStringBuilder()
        val arguments = otherArguments.ifEmpty {
            getArgsPref(context, appWidgetId)
        }

        val htmlContent = mainAndroid("customfetch $arguments")
        if (Files.exists(Path(errorLock))) {
            val file = File(errorLock)
            val error = file.bufferedReader().use { it.readText() }
            if (postToast) {
                val handler = Handler(Looper.getMainLooper())
                handler.post {
                    Toast.makeText(context, error, Toast.LENGTH_LONG).show()
                }
                handler.post {
                    Toast.makeText(context, "read error logs at $errorFile", Toast.LENGTH_LONG)
                        .show()
                }
            }
            file.delete()
            parsedContent.append("read error logs at $errorFile\n\n$error")
            return parsedContent
        }

        if (disableLineWrap) {
            val eachLine = htmlContent!!.split("<br>").map { it.trim() }
            for (line in eachLine) {
                var parsedLine = HtmlCompat.fromHtml(line, HtmlCompat.FROM_HTML_MODE_COMPACT)
                parsedLine =
                    ellipsize(parsedLine, paint, width, TruncateAt.END) as Spanned
                parsedContent.appendLine(parsedLine)
            }
        } else {
            parsedContent.append(htmlContent?.let {
                HtmlCompat.fromHtml(it, HtmlCompat.FROM_HTML_MODE_COMPACT)
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
internal fun saveConfigPrefs(
    context: Context,
    appWidgetId: Int,
    args: String,
    truncateWidth: String,
    disableLineWrap: Boolean,
    bgColor: Int
) {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0).edit()
    prefs.putString(PREF_PREFIX_KEY + appWidgetId + "_args", args)
    prefs.putString(PREF_PREFIX_KEY + appWidgetId + "_truncateWidth", truncateWidth)
    prefs.putBoolean(PREF_PREFIX_KEY + appWidgetId + "_disableLineWrap", disableLineWrap)
    prefs.putInt(PREF_PREFIX_KEY + appWidgetId + "_bgColor", bgColor)
    prefs.apply()
}

// Read the prefix from the SharedPreferences object for this widget.
// If there is no preference saved, get the default from a resource
internal fun getArgsPref(context: Context, appWidgetId: Int): String {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    val args = prefs.getString(PREF_PREFIX_KEY + appWidgetId + "_args", null)
    return args ?: "-D ${context.filesDir.absolutePath} -a small"
}

internal fun getDisableLineWrap(context: Context, appWidgetId: Int): Boolean {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    val value = prefs.getBoolean(PREF_PREFIX_KEY + appWidgetId + "_disableLineWrap", false)
    return value
}

internal fun getBgColor(context: Context, appWidgetId: Int): Int {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    val value = prefs.getInt(PREF_PREFIX_KEY + appWidgetId + "_bgColor", 0)
    return value
}

internal fun getTruncateWidthPref(context: Context, appWidgetId: Int): String {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    val value = prefs.getString(PREF_PREFIX_KEY + appWidgetId + "_truncateWidth", null)
    return value ?: "0"
}

internal fun deleteConfigPrefs(context: Context, appWidgetId: Int) {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0).edit()
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_args")
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_disableLineWrap")
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_truncateWidth")
    prefs.apply()
}
