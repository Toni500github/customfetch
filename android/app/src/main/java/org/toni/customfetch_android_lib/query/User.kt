package org.toni.customfetch_android_lib.query

import android.os.Process
import org.toni.customfetch_android_lib.UNKNOWN
import java.io.BufferedReader
import java.io.InputStreamReader


class User private constructor() {
    data class UserInfos(
        var name: String = UNKNOWN,
    )

    companion object {
        private var mUserInfos = UserInfos()
        private var mBInit = false

        // Singleton instance
        private val _instance by lazy { User() }
        fun getInstance(): User = _instance
    }

    val name: String get() = mUserInfos.name

    init {
        if (!mBInit) {
            mUserInfos.apply {
                name = getUsernameFromUid(Process.myUid()).toString()
            }
        }
        mBInit = true
    }
}

fun getUsernameFromUid(uid: Int): String? {
    try {
        val process: java.lang.Process? = Runtime.getRuntime().exec("id -nu $uid")
        val reader = BufferedReader(
            InputStreamReader(process?.inputStream)
        )
        return reader.readLine()
    } catch (e: Exception) {
        return null
    }
}