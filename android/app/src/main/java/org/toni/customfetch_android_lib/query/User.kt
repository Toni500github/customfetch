package org.toni.customfetch_android_lib.query

import org.toni.customfetch_android_lib.UNKNOWN
import java.lang.reflect.Field

// https://stackoverflow.com/a/28057167
fun getNameForUid(id: Int): Any? {
    try {
        val clazz = Class.forName("libcore.io.Libcore")
        val field: Field = clazz.getDeclaredField("os").apply{ isAccessible = true }
        val os: Any? = field.get(null)
        val getpwuid = os!!.javaClass.getMethod("getpwuid", Int::class.javaPrimitiveType).apply{ isAccessible = true }
        val passwd = getpwuid.invoke(os, id)
        if (passwd != null) {
            val pw_name: Field = passwd.javaClass.getDeclaredField("pw_name").apply{ isAccessible = true }
            return pw_name.get(passwd)
        }
    } catch (ignored: Exception) {
    }
    return null
}

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
                name = getNameForUid(1000).toString()
            }
        }
        mBInit = true
    }
}