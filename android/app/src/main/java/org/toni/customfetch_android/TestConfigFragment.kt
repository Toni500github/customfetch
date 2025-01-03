package org.toni.customfetch_android

import android.content.res.ColorStateList
import android.graphics.Color
import android.os.Bundle
import android.text.SpannableString
import android.text.TextPaint
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.app.AppCompatActivity
import androidx.core.text.HtmlCompat
import androidx.fragment.app.Fragment
import org.toni.customfetch_android.databinding.TestConfigFragmentBinding
import org.toni.customfetch_android.widget.customfetchRender


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

        val result = customfetchRender.getParsedContent(
            AppCompatActivity(),
            0,
            0f,
            false,
            TextPaint(),
            "-C $configFile -Nnm \$<os.name_id>", // this is the important thing
            false
        )

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