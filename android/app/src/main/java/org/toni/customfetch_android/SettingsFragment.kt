package org.toni.customfetch_android

import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.widget.Toolbar
import androidx.preference.EditTextPreference
import androidx.preference.PreferenceFragmentCompat
import org.toni.customfetch_android.databinding.SettingsLayoutBinding
import org.toni.customfetch_android.widget.isValidHex

class SettingsFragment : PreferenceFragmentCompat() {

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        view.findViewById<Toolbar>(R.id.toolbar_settings)?.let {
            it.setNavigationIcon(R.drawable.arrow_back)
            it.setNavigationOnClickListener {
                requireActivity().supportFragmentManager.popBackStack()
            }
        }
    }

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.root_preferences, rootKey)
        findPreference<EditTextPreference>("default_custom_color")?.setOnPreferenceChangeListener { _, newValue ->
            if (!isValidHex(newValue.toString())) {
                Toast.makeText(
                    context,
                    "Default custom color '$newValue' is invalid",
                    Toast.LENGTH_SHORT
                ).show()
                Toast.makeText(
                    context,
                    "Value must be type #AARRGGBB",
                    Toast.LENGTH_SHORT
                ).show()
            }
            true
        }
    }
}