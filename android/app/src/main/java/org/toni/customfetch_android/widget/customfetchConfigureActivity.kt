/*
 * Copyright 2024 Toni500git
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
import android.widget.Toast
import androidx.core.graphics.toColorInt
import androidx.core.text.HtmlCompat
import com.skydoves.colorpickerview.listeners.ColorEnvelopeListener
import org.toni.customfetch_android.R
import org.toni.customfetch_android.databinding.CustomfetchConfigureBinding
import org.toni.customfetch_android.getAppSettingsPrefBool
import org.toni.customfetch_android.getAppSettingsPrefString
import java.io.File
import java.nio.file.Files
import kotlin.io.path.Path


/**
 * The configuration screen for the [customfetch] AppWidget.
 */
class customfetchConfigureActivity : Activity() {
    private var appWidgetId = AppWidgetManager.INVALID_APPWIDGET_ID
    private lateinit var binding: CustomfetchConfigureBinding
    private var onAddWidget = View.OnClickListener {
        val context = this@customfetchConfigureActivity

        // When the button is clicked, store the string locally
        saveConfigPrefs(
            context,
            appWidgetId,
            binding.argumentsConfigure.text.toString(),
            binding.additionalTruncateWidth.text.toString(),
            binding.truncateText.isChecked,
            bgColor
        )

        // It is the responsibility of the configuration activity to update the app widget
        val appWidgetManager = AppWidgetManager.getInstance(context)
        updateAppWidget(context, appWidgetManager, appWidgetId)

        // Make sure we pass back the original appWidgetId
        val resultValue = Intent()
        resultValue.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, appWidgetId)
        setResult(RESULT_OK, resultValue)
        finish()
    }

    private var bgColor = 0

    public override fun onCreate(icicle: Bundle?) {
        super.onCreate(icicle)

        // Set the result to CANCELED.  This will cause the widget host to cancel
        // out of the widget placement if the user presses the back button.
        setResult(RESULT_CANCELED)

        binding = CustomfetchConfigureBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.addButton.setOnClickListener(onAddWidget)

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

        binding.argumentsConfigure.setText(getArgsPref(this@customfetchConfigureActivity, appWidgetId))
        binding.additionalTruncateWidth.setText(getAppSettingsPrefString(this, "additional_truncate"))
        binding.truncateText.isChecked = getAppSettingsPrefBool(this, "always_truncate")
        binding.argsHelp.text = customfetchRender.mainAndroid("customfetch --help", true)

        binding.showModulesList.setOnCheckedChangeListener { _, isChecked ->
            binding.argsHelp.text = customfetchRender.mainAndroid("customfetch ${if (isChecked) "-l" else "-h"}", true)
        }

        // set everything of the radio buttons at first configuration from the app.
        when (getAppSettingsPrefString(this, "default_bg_color")) {
            "system_bg_color" -> {
                binding.selectBgColor.check(R.id.radio_system_bg_color)
                setSystemBgColor()
            }
            "transparent_bg" -> {
                binding.selectBgColor.check(R.id.radio_transparent_bg)
                bgColor = 0x00FFFFFF
            }
            "custom_bg_color" -> {
                binding.selectBgColor.check(R.id.radio_custom_colors)
                setColorPickerView()
            }
        }

        binding.selectBgColor.setOnCheckedChangeListener { _, checkedId ->
            when (checkedId) {
                R.id.radio_system_bg_color -> {
                    binding.customColorSelect.visibility = View.GONE
                    setSystemBgColor()
                }

                R.id.radio_transparent_bg -> {
                    binding.customColorSelect.visibility = View.GONE
                    bgColor = 0x00FFFFFF
                }

                R.id.radio_custom_colors -> {
                    setColorPickerView()
                }
            }
        }
    }

    private fun setSystemBgColor() {
        val typedValue = TypedValue()
        this.theme.resolveAttribute(android.R.attr.colorBackground, typedValue, true)
        bgColor = typedValue.data
    }

    @SuppressLint("SetTextI18n", "ClickableViewAccessibility")
    private fun setColorPickerView() {
        binding.customColorSelect.visibility = View.VISIBLE
        // disable scroll when interacting with the color picker
        binding.colorPickerView.setOnTouchListener { view, _ ->
            view.parent.requestDisallowInterceptTouchEvent(true)
            false // allow colorPickerView to handle the touch event
        }

        val defaultColor = getAppSettingsPrefString(this, "default_custom_color")
        binding.colorPickerHex.setText(defaultColor)
        binding.colorPreview.setBackgroundColor(defaultColor.toColorInt())
        binding.colorPickerView.setInitialColor(defaultColor.toColorInt())

        binding.colorPickerHex.addTextChangedListener(object : TextWatcher {
            override fun afterTextChanged(s: Editable) {
                val col = s.toString()
                if (isValidHex(col))
                    binding.colorPickerView.setInitialColor(col.toColorInt())
            }
            override fun beforeTextChanged(p0: CharSequence?, p1: Int, p2: Int, p3: Int) {}
            override fun onTextChanged(p0: CharSequence?, p1: Int, p2: Int, p3: Int) {}
        } )

        binding.colorPickerView.setColorListener(ColorEnvelopeListener { envelope, fromUser ->
            if (!binding.colorPickerHex.text.contentEquals("#${envelope.hexCode}") && fromUser)
                binding.colorPickerHex.setText("#${envelope.hexCode}")

            binding.colorPreview.setBackgroundColor(envelope.color)
            bgColor = envelope.color
        })

        binding.colorPickerView.attachAlphaSlider(binding.alphaSlideBar)
        binding.colorPickerView.attachBrightnessSlider(binding.brightnessSlideBar)
    }
}

class CustomfetchMainRender {
    fun getParsedContent(
        context: Context,
        appWidgetId: Int,
        width: Float,
        truncateText: Boolean,
        paint: TextPaint,
        otherArguments: String = "",
        postToast: Boolean = true,
        doNotLoadConfig: Boolean = false
    ): SpannableStringBuilder {
        val parsedContent = SpannableStringBuilder()
        val arguments = otherArguments.ifEmpty {
            getArgsPref(context, appWidgetId)
        }
        val htmlContent = mainAndroid("customfetch $arguments", doNotLoadConfig)

        val errorFile = "/storage/emulated/0/.config/customfetch/error_log.txt"
        val errorLock = "/storage/emulated/0/.config/customfetch/error.lock"
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

        if (truncateText) {
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

    external fun mainAndroid(argv: String, doNotLoadConfig: Boolean): String?
    companion object {
        init {
            System.loadLibrary("customfetch")
        }
    }
}
val customfetchRender = CustomfetchMainRender()

private const val PREFS_NAME = "org.toni.customfetch_android.customfetch"
private const val PREF_PREFIX_KEY = "appwidget_"

// Save the preferences to the SharedPreferences object for this widget
internal fun saveConfigPrefs(
    context: Context,
    appWidgetId: Int,
    args: String,
    truncateWidth: String,
    truncateText: Boolean,
    bgColor: Int
) {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0).edit()
    prefs.putString(PREF_PREFIX_KEY + appWidgetId + "_args", args)
    prefs.putString(PREF_PREFIX_KEY + appWidgetId + "_truncateWidth", truncateWidth)
    prefs.putBoolean(PREF_PREFIX_KEY + appWidgetId + "_truncateText", truncateText)
    prefs.putInt(PREF_PREFIX_KEY + appWidgetId + "_bgColor", bgColor)
    prefs.apply()
}

internal fun getArgsPref(context: Context, appWidgetId: Int): String {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    val args = prefs.getString(PREF_PREFIX_KEY + appWidgetId + "_args", null)
    return args ?: getAppSettingsPrefString(context, "default_args")
}

internal fun getTruncateText(context: Context, appWidgetId: Int): Boolean {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    val value = prefs.getBoolean(PREF_PREFIX_KEY + appWidgetId + "_truncateText", false)
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
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_truncateText")
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_truncateWidth")
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_bgColor")
    prefs.apply()
}

internal fun isValidHex(color: String): Boolean =
    color.matches("^#[0-9A-Fa-f]{8}$".toRegex())
