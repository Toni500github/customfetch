package org.toni.customfetch_android

import android.os.Bundle
import androidx.core.text.HtmlCompat
import android.text.method.LinkMovementMethod
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import org.toni.customfetch_android.databinding.AboutMeFragmentBinding

class AboutMeFragment : Fragment() {

    private var _binding: AboutMeFragmentBinding? = null
    // This property is only valid between onCreateView and
    // onDestroyView.
    private val binding get() = _binding!!

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = AboutMeFragmentBinding.inflate(inflater, container, false)
        binding.toolbar.apply {
            setNavigationIcon(R.drawable.arrow_back)
            setNavigationOnClickListener { _ ->
                requireActivity().supportFragmentManager.popBackStack()
            }
        }

        binding.burntGithubLink.movementMethod = LinkMovementMethod.getInstance()
        binding.toniGithubLink.movementMethod = LinkMovementMethod.getInstance()
        binding.bcppDiscordLink.movementMethod = LinkMovementMethod.getInstance()
        binding.fmtlibGithubLink.movementMethod = LinkMovementMethod.getInstance()
        binding.tomlLibGithubLink.movementMethod = LinkMovementMethod.getInstance()

        return binding.root
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}