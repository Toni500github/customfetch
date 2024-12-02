package org.toni.customfetch_android

import android.content.Context
import android.content.Intent
import android.content.res.AssetManager
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import androidx.preference.PreferenceFragmentCompat
import java.io.File
import java.io.FileOutputStream
import java.io.IOException


class SettingsActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.settings_activity)
        if (savedInstanceState == null) {
            supportFragmentManager
                .beginTransaction()
                .replace(R.id.settings, SettingsFragment())
                .commit()
        }
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                val intent = Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                startActivity(intent)
            }
        }
    }

    class SettingsFragment : PreferenceFragmentCompat() {
        override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
            setPreferencesFromResource(R.xml.root_preferences, rootKey)
        }
    }
}

val TAG: String = "AssetCopy"

internal fun copyToAssetFolder(assets: AssetManager, absolutePath: String, assetSubFolder: String) {
    try {
        copyDirectory(assets, assetSubFolder, absolutePath)
        Log.d(TAG, "All files copied to: $absolutePath")
    } catch (e: IOException) {
        Log.e(TAG, "Failed to copy asset folder: $assetSubFolder", e)
    }
}

@Throws(IOException::class)
private fun copyDirectory(
    assetManager: AssetManager,
    sourceDir: String,
    destinationDir: String
) {
    val files = assetManager.list(sourceDir)
    if (files.isNullOrEmpty())
        return

    val destDir = File(destinationDir, sourceDir)
    if (!destDir.exists() && !destDir.mkdirs()) {
        throw IOException("Failed to create directory: " + destDir.absolutePath)
    }

    for (fileName in files) {
        val assetPath = "$sourceDir/$fileName"
        val destPath = destDir.path + "/" + fileName
        if (isDirectory(assetManager, assetPath)) {
            copyDirectory(assetManager, assetPath, destinationDir)
        } else {
            copyFile(assetManager, assetPath, destPath)
        }
    }
}

@Throws(IOException::class)
private fun isDirectory(assetManager: AssetManager, path: String): Boolean {
    val files = assetManager.list(path)
    return !files.isNullOrEmpty()
}

@Throws(IOException::class)
private fun copyFile(assetManager: AssetManager, assetPath: String, destPath: String) {
    assetManager.open(assetPath).use { `in` ->
        FileOutputStream(destPath).use { out ->
            val buffer = ByteArray(8192)
            var bytesRead: Int
            while ((`in`.read(buffer).also { bytesRead = it }) != -1) {
                out.write(buffer, 0, bytesRead)
            }
            Log.d(TAG, "File copied: $destPath")
        }
    }
}