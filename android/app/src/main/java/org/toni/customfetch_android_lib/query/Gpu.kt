package org.toni.customfetch_android_lib.query

import android.graphics.SurfaceTexture
import android.opengl.GLES20
import org.toni.customfetch_android_lib.MAGIC_LINE
import com.otaliastudios.opengl.core.EglCore
import com.otaliastudios.opengl.surface.EglWindowSurface
import com.otaliastudios.opengl.texture.GlTexture

class Gpu {
    data class GpuInfo(
        var vendor: String = MAGIC_LINE,
        var name: String = MAGIC_LINE,
    )

    companion object {
        private var mGpuInfos = GpuInfo()
        private var mBInit = false
        fun clearCache() { mBInit = false }
    }

    val vendor: String get() = mGpuInfos.vendor
    val name: String get() = mGpuInfos.name

    init {
        if (!mBInit)
            mGpuInfos = getGpuInfos()
        mBInit = true
    }

    private fun getGpuInfos(): GpuInfo {
        val eglCore = EglCore()
        val surface = EglWindowSurface(eglCore, SurfaceTexture(GlTexture().id))
        surface.makeCurrent()
        val ret = GpuInfo(
            vendor = GLES20.glGetString(GLES20.GL_VENDOR) ?: MAGIC_LINE,
            name = GLES20.glGetString(GLES20.GL_RENDERER) ?: MAGIC_LINE
        )
        surface.release()
        eglCore.release()
        return ret
    }
}