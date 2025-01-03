package org.toni.customfetch_android

import android.content.Intent
import android.content.res.AssetManager
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.util.Log
import android.view.View
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.FragmentTransaction
import org.toni.customfetch_android.databinding.ActivityMainBinding
import java.io.File
import java.io.FileOutputStream
import java.io.IOException
import java.nio.file.Files
import kotlin.io.path.Path

// kinda magic numbers
const val TEST_CONFIG_FILE_RC = 2
const val ABOUT_ME_RC = 4

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                val alert = AlertDialog.Builder(this)
                    .setTitle("Grant external storage management permission")
                    .setMessage("Customfetch needs permissions to manage external storage to able to access config files.\n"+
                            "By default we going to read/write the following directories:\n"+
                            "/storage/emulated/0/.config/\n"+
                            "/storage/emulated/0/.config/customfetch/")
                    .setPositiveButton("Grant permission"
                    ) { _, _ ->
                        val intent = Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
                        startActivity(intent)
                    }
                    .setIcon(R.drawable.icon_alert_yellow)

                val view: View = layoutInflater.inflate(R.layout.grant_perm, null, false)
                alert.setView(view)
                alert.show()
            }
        }

        if (!Files.exists(Path(filesDir.absolutePath + "/ascii")))
            copyToAssetFolder(assets, filesDir.absolutePath, "ascii")

        binding.testConfigFile.setOnClickListener { _ ->
            val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                type = "*/*"
            }
            startActivityForResult(intent, TEST_CONFIG_FILE_RC)
        }

        binding.aboutMe.setOnClickListener { _ ->
            val fragment = AboutMeFragment()
            val transaction: FragmentTransaction =
                supportFragmentManager.beginTransaction()
            transaction.replace(android.R.id.content, fragment)
            transaction.addToBackStack(null).commit()
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode == RESULT_OK && data != null && data.data != null) {
            data.data?.let { uri ->
                when(requestCode) {
                    TEST_CONFIG_FILE_RC -> {
                        val fragment = TestConfigFragment().apply {
                            configFile = PathUtil.getPath(this@MainActivity, uri)
                        }
                        val transaction: FragmentTransaction =
                            supportFragmentManager.beginTransaction()
                        transaction.replace(android.R.id.content, fragment)
                        transaction.addToBackStack(null).commit()
                    }
                    else -> {}
                }
            }
        }
    }
}

const val TAG: String = "AssetCopy"

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
    if (!destDir.exists() && !destDir.mkdirs())
        throw IOException("Failed to create directory: ${destDir.absolutePath}")

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
            while ((`in`.read(buffer).also { bytesRead = it }) != -1)
                out.write(buffer, 0, bytesRead)

            Log.d(TAG, "File copied: $destPath")
        }
    }
}