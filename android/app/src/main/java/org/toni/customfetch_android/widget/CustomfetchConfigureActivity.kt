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

import android.annotation.SuppressLint
import android.app.Activity
import android.appwidget.AppWidgetManager
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.Editable
import android.text.SpannableStringBuilder
import android.text.TextPaint
import android.text.TextUtils.TruncateAt
import android.text.TextUtils.ellipsize
import android.text.TextWatcher
import android.util.TypedValue
import android.view.View
import android.widget.LinearLayout
import android.widget.Toast
import androidx.core.graphics.toColorInt
import androidx.core.text.toSpanned
import com.skydoves.colorpickerview.listeners.ColorEnvelopeListener
import org.toni.customfetch_android.R
import org.toni.customfetch_android.databinding.ColorpickerviewLayoutBinding
import org.toni.customfetch_android.databinding.CustomfetchConfigureBinding
import org.toni.customfetch_android.getAppSettingsPrefBool
import org.toni.customfetch_android.getAppSettingsPrefInt
import org.toni.customfetch_android.getAppSettingsPrefString
import org.toni.customfetch_android_lib.mainRender
import org.toni.customfetch_android_lib.mainRenderStr
import java.io.File
import java.nio.file.Files
import kotlin.io.path.Path

/**
 * The configuration screen for the [Customfetch] AppWidget.
 */
class CustomfetchConfigureActivity : Activity() {
    private var appWidgetId = AppWidgetManager.INVALID_APPWIDGET_ID
    private lateinit var binding: CustomfetchConfigureBinding
    private var onAddWidget = View.OnClickListener {
        val context = this@CustomfetchConfigureActivity

        // When the button is clicked, store the string locally
        saveConfigPrefs(
            context,
            appWidgetId,
            binding.argumentsConfigure.text.toString(),
            binding.additionalTruncateWidth.text.toString(),
            binding.truncateText.isChecked,
            bgColor,
            widgetTextColor
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
    private var widgetTextColor = 0xFF8F9099.toInt()

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

        binding.argumentsConfigure.setText(getArgsPref(this@CustomfetchConfigureActivity, appWidgetId))
        binding.additionalTruncateWidth.setText(getAppSettingsPrefString(this, "additional_truncate"))
        binding.truncateText.isChecked = getAppSettingsPrefBool(this, "always_truncate")
        binding.docsHelp.text = mainRenderStr(this, "customfetch --help")

        binding.btnArgsHelp.setOnClickListener {
            binding.docsHelp.text = mainRenderStr(this, "customfetch --help")
        }
        binding.btnConfigHelp.setOnClickListener {
            binding.docsHelp.text = mainRenderStr(this, "customfetch --how-it-works")
        }
        binding.btnModulesHelp.setOnClickListener {
            binding.docsHelp.text = mainRenderStr(this, "customfetch --list-modules")
        }
        binding.btnLogosList.setOnClickListener {
            binding.docsHelp.text =
                mainRenderStr(this, "customfetch ${binding.argumentsConfigure.text} --list-logos")
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
                setColorPickerView(binding.customBgColorSelect, true)
            }
        }

        binding.selectTextColor.setOnCheckedChangeListener { _, checkedId ->
            when (checkedId) {
                R.id.radio_default_text_color -> {
                    binding.customTextColorSelect.visibility = View.GONE
                    widgetTextColor = 0xFF8F9099.toInt()
                }

                R.id.radio_custom_text_color -> {
                    setColorPickerView(binding.customTextColorSelect, false)
                }
            }
        }

        binding.selectBgColor.setOnCheckedChangeListener { _, checkedId ->
            when (checkedId) {
                R.id.radio_system_bg_color -> {
                    binding.customBgColorSelect.visibility = View.GONE
                    setSystemBgColor()
                }

                R.id.radio_transparent_bg -> {
                    binding.customBgColorSelect.visibility = View.GONE
                    bgColor = 0x00FFFFFF
                }

                R.id.radio_custom_colors -> {
                    setColorPickerView(binding.customBgColorSelect, true)
                }
            }
        }

        markWidgetConfigured(this, appWidgetId)
    }

    private fun setSystemBgColor() {
        val typedValue = TypedValue()
        this.theme.resolveAttribute(android.R.attr.colorBackground, typedValue, true)
        this.bgColor = typedValue.data
    }

    @SuppressLint("SetTextI18n", "ClickableViewAccessibility")
    // why tf didn't kotlin add an option for making function parameters mutable???
    private fun setColorPickerView(cpvLayout: LinearLayout, isBgColor: Boolean) {
        val binding = ColorpickerviewLayoutBinding.inflate(layoutInflater, cpvLayout, true)
        cpvLayout.visibility = View.VISIBLE

        // disable scrolling when interacting with the color picker
        binding.colorPickerView.setOnTouchListener { view, _ ->
            view.parent.requestDisallowInterceptTouchEvent(true); false
        }
        binding.alphaSlideBar.setOnTouchListener { view, _ ->
            view.parent.requestDisallowInterceptTouchEvent(true); false
        }
        binding.brightnessSlideBar.setOnTouchListener { view, _ ->
            view.parent.requestDisallowInterceptTouchEvent(true); false
        }

        val defaultColor =
            if (isBgColor)
                getAppSettingsPrefInt(this, "default_bg_custom_color")
            else
                getAppSettingsPrefInt(this, "default_widget_text_color")

        binding.colorPickerHex.setText("#"+ Integer.toHexString(defaultColor).uppercase())
        binding.colorPreview.setBackgroundColor(defaultColor)
        binding.colorPickerView.setInitialColor(defaultColor)

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
            if (!binding.colorPickerHex.text.contentEquals("#"+envelope.hexCode) && fromUser)
                binding.colorPickerHex.setText("#"+envelope.hexCode)

            binding.colorPreview.setBackgroundColor(envelope.color)
            if (isBgColor) bgColor = envelope.color
            else widgetTextColor = envelope.color
        })

        binding.colorPickerView.attachAlphaSlider(binding.alphaSlideBar)
        binding.colorPickerView.attachBrightnessSlider(binding.brightnessSlideBar)
    }
}

fun getParsedContent(
        context: Context,
        appWidgetId: Int,
        width: Float,
        truncateText: Boolean,
        paint: TextPaint,
        otherArguments: String = "",
): SpannableStringBuilder {
    val parsedContent = SpannableStringBuilder()
    val arguments = otherArguments.ifEmpty {
        getArgsPref(context, appWidgetId)
    }
    val content = mainRender(context, appWidgetId, "customfetch $arguments")

    for (line in content) {
        if (truncateText)
            parsedContent.append(ellipsize(line, paint, width, TruncateAt.END).toSpanned())
        else
            parsedContent.append(line)
        parsedContent.append("\n")
    }

    return parsedContent
}

private const val PREFS_NAME = "org.toni.customfetch_android.customfetch"
private const val PREF_PREFIX_KEY = "appwidget_"

// Save the preferences to the SharedPreferences object for this widget
internal fun saveConfigPrefs(
    context: Context,
    appWidgetId: Int,
    args: String,
    truncateWidth: String,
    truncateText: Boolean,
    bgColor: Int,
    widgetTextColor: Int
) {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0).edit()
    prefs.putString(PREF_PREFIX_KEY + appWidgetId + "_args", args)
    prefs.putString(PREF_PREFIX_KEY + appWidgetId + "_truncateWidth", truncateWidth)
    prefs.putBoolean(PREF_PREFIX_KEY + appWidgetId + "_truncateText", truncateText)
    prefs.putInt(PREF_PREFIX_KEY + appWidgetId + "_widgetTextColor", widgetTextColor)
    prefs.putInt(PREF_PREFIX_KEY + appWidgetId + "_bgColor", bgColor)
    prefs.apply()
}

internal fun getArgsPref(context: Context, appWidgetId: Int): String {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    val args = prefs.getString(PREF_PREFIX_KEY + appWidgetId + "_args", null)
    return args ?: getAppSettingsPrefString(context, "default_args")
}

internal fun getTruncateWidthPref(context: Context, appWidgetId: Int): String {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    val value = prefs.getString(PREF_PREFIX_KEY + appWidgetId + "_truncateWidth", null)
    return value ?: "0"
}

internal fun getTruncateText(context: Context, appWidgetId: Int): Boolean {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    return prefs.getBoolean(PREF_PREFIX_KEY + appWidgetId + "_truncateText", false)
}

internal fun getBgColor(context: Context, appWidgetId: Int): Int {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    return prefs.getInt(PREF_PREFIX_KEY + appWidgetId + "_bgColor", 0)
}

internal fun getWidgetTextColor(context: Context, appWidgetId: Int): Int {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0)
    return prefs.getInt(PREF_PREFIX_KEY + appWidgetId + "_widgetTextColor", 0)
}

internal fun deleteConfigPrefs(context: Context, appWidgetId: Int) {
    val prefs = context.getSharedPreferences(PREFS_NAME, 0).edit()
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_args")
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_truncateText")
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_truncateWidth")
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_bgColor")
    prefs.remove(PREF_PREFIX_KEY + appWidgetId + "_widgetTextColor")
    prefs.apply()
}

fun markWidgetConfigured(context: Context, appWidgetId: Int) {
    val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    prefs.edit().putBoolean(PREF_PREFIX_KEY + appWidgetId + "_configuring", true).apply()
}

fun isWidgetConfigured(context: Context, appWidgetId: Int): Boolean {
    val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
    return prefs.getBoolean(PREF_PREFIX_KEY + appWidgetId + "_configuring", false)
}

internal fun isValidHex(color: String): Boolean =
    color.matches("^#[0-9A-Fa-f]{8}$".toRegex())