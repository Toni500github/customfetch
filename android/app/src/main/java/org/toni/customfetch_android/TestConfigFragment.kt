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

package org.toni.customfetch_android

import android.graphics.Color
import android.os.Bundle
import android.text.TextPaint
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.Fragment
import org.toni.customfetch_android.databinding.TestConfigFragmentBinding
import org.toni.customfetch_android.widget.getParsedContent
import org.toni.customfetch_android_lib.mainRender
import org.toni.customfetch_android_lib.mainRenderStr

class TestConfigFragment : Fragment() {

    private var _binding: TestConfigFragmentBinding? = null
    // This property is only valid between onCreateView and
    // onDestroyView.
    private val binding get() = _binding!!

    var configFile = ""

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = TestConfigFragmentBinding.inflate(inflater, container, false)
        binding.toolbar.apply {
            setNavigationIcon(R.drawable.arrow_back)
            setNavigationOnClickListener { _ ->
                requireActivity().supportFragmentManager.popBackStack()
            }
        }

        val result = mainRender(requireContext(), 0, "-C $configFile -Nnm \$<os.name_id>")[0]
        if (result.contentEquals("android")) {
            binding.titleResult.setTextColor(Color.GREEN)
            binding.titleResult.text = "SUCCESS"
            binding.testConfigResult.text = "config file '$configFile' works!!"
        } else {
            binding.titleResult.setTextColor(Color.RED)
            binding.titleResult.text = "FAILURE"
            binding.testConfigResult.text = result
        }

        return binding.root
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
