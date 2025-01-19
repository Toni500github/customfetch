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

package org.toni.customfetch_android

import android.animation.ArgbEvaluator
import android.animation.ValueAnimator
import android.annotation.SuppressLint
import android.content.Context
import android.content.Intent
import android.content.res.AssetManager
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.graphics.drawable.LayerDrawable
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.text.method.LinkMovementMethod
import android.util.Log
import android.view.MotionEvent
import android.view.View
import android.view.animation.AnimationUtils
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import androidx.preference.PreferenceManager
import org.toni.customfetch_android.databinding.ActivityMainBinding
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.nio.file.Files
import kotlin.io.path.Path


// kinda magic numbers
const val TEST_CONFIG_FILE_RC = 2

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding

    @SuppressLint("ClickableViewAccessibility")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                val alert = AlertDialog.Builder(this)
                    .setTitle("Grant external storage management permission")
                    .setMessage("Customfetch needs permissions to manage external storage to able to access config files.\n"+
                            "By default we going to read/write in the following directories:\n"+
                            "/storage/emulated/0/.config/\n"+
                            "/storage/emulated/0/.config/customfetch/")
                    .setPositiveButton("Grant permission"
                    ) { _, _ ->
                        val intent = Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                        startActivity(intent)
                    }
                    .setIcon(R.drawable.icon_alert_yellow)

                val view = layoutInflater.inflate(R.layout.grant_perm, null, false)
                alert.setView(view)
                alert.show()
            }
        }

        if (!Files.exists(Path(filesDir.absolutePath + "/ascii")))
            copyToAssetFolder(assets, filesDir.absolutePath, "ascii")

        binding.discordLink.movementMethod = LinkMovementMethod.getInstance()
        binding.redditLink.movementMethod = LinkMovementMethod.getInstance()

        binding.testConfigFile.setOnClickListener { _ ->
            val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                type = "*/*"
            }
            startActivityForResult(intent, TEST_CONFIG_FILE_RC)
        }
        binding.testConfigFile.setOnTouchListener { _, event ->
            startAnimation(binding.testConfigFile, event)
        }

        binding.aboutMe.setOnClickListener { _ ->
            setFragment(AboutMeFragment())
        }
        binding.aboutMe.setOnTouchListener { _, event ->
            startAnimation(binding.aboutMe, event)
        }

        binding.widgetSettings.setOnClickListener { _ ->
            setFragment(SettingsFragment())
        }
        binding.widgetSettings.setOnTouchListener { _, event ->
            startAnimation(binding.widgetSettings, event)
        }

    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode == RESULT_OK && data != null && data.data != null) {
            data.data?.let { uri ->
                when (requestCode) {
                    TEST_CONFIG_FILE_RC -> {
                        val fragment = TestConfigFragment().apply {
                            configFile = PathUtil.getPath(this@MainActivity, uri)
                        }
                        setFragment(fragment, android.R.anim.slide_in_left)
                    }
                    else -> {}
                }
            }
        }
    }

    private fun setFragment(fragment: Fragment, slideInAnim: Int = R.anim.slide_in) {
        supportFragmentManager.beginTransaction()
            .setCustomAnimations(
                slideInAnim,  // enter
                android.R.animator.fade_out,  // exit
                android.R.animator.fade_in,   // popEnter
                R.anim.slide_out  // popExit
            )
            .replace(android.R.id.content, fragment)
            .addToBackStack(null).commit()
    }

    private fun startAnimation(view: View, event: MotionEvent): Boolean {
        val drawable = view.background as GradientDrawable
        var colorAnimator = ValueAnimator()
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                colorAnimator =
                    ValueAnimator.ofObject(ArgbEvaluator(), 0xFF3D3D3D.toInt(), 0xFF5A5A5A.toInt())
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                colorAnimator =
                    ValueAnimator.ofObject(ArgbEvaluator(), 0xFF5A5A5A.toInt(), 0xFF3D3D3D.toInt())
            }
        }
        colorAnimator.duration = 300
        colorAnimator.addUpdateListener { animator ->
            drawable.setColor(animator.animatedValue as Int)
        }
        colorAnimator.start()
        return false
    }
}

internal fun copyToAssetFolder(assets: AssetManager, absolutePath: String, assetSubFolder: String) {
    try {
        copyDirectory(assets, assetSubFolder, absolutePath)
        Log.d("AssetCopy", "All files copied to: $absolutePath")
    } catch (e: IOException) {
        Log.e("AssetCopy", "Failed to copy asset folder: $assetSubFolder", e)
    }
}

internal fun getAppSettingsPrefString(context: Context, name: String): String {
    val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
    val str = sharedPref.getString(name, null) ?: ""
    return str
}

internal fun getAppSettingsPrefBool(context: Context, name: String): Boolean {
    val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
    return sharedPref.getBoolean(name, false)
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
    if (!destDir.exists() && !destDir.mkdirs())
        throw IOException("Failed to create directory: " + destDir.absolutePath)

    for (fileName in files) {
        val assetPath = "$sourceDir/$fileName"
        val destPath = destDir.path + "/" + fileName
        if (isDirectory(assetManager, assetPath))
            copyDirectory(assetManager, assetPath, destinationDir)
        else
            copyFile(assetManager, assetPath, destPath)
    }
}

@Throws(IOException::class)
private fun isDirectory(assetManager: AssetManager, path: String): Boolean =
    !assetManager.list(path).isNullOrEmpty()

@Throws(IOException::class)
private fun copyFile(assetManager: AssetManager, assetPath: String, destPath: String) {
    assetManager.open(assetPath).use { `in` ->
        FileOutputStream(destPath).use { out ->
            val buffer = ByteArray(8192)
            var bytesRead: Int
            while ((`in`.read(buffer).also { bytesRead = it }) != -1)
                out.write(buffer, 0, bytesRead)

            Log.d("AssetCopy", "File copied: $destPath")
        }
    }
}
