/*
 * Copyright 2025 Toni500git
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 * following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#undef TOML_HEADER_ONLY
#define TOML_HEADER_ONLY 0

#include <filesystem>

#include "libcufetch/config.hh"

class Config : public ConfigBase
{
public:
    // Create .config directories and files and load the config file (args or default)
    Config(const std::filesystem::path& configFile, const std::filesystem::path& configDir);

    // config colors
    // those without gui_ prefix are for the terminal
    struct colors_t
    {
        std::string black;
        std::string red;
        std::string green;
        std::string blue;
        std::string cyan;
        std::string yellow;
        std::string magenta;
        std::string white;

        std::string gui_black;
        std::string gui_red;
        std::string gui_green;
        std::string gui_blue;
        std::string gui_cyan;
        std::string gui_yellow;
        std::string gui_magenta;
        std::string gui_white;
    } colors;

    // Variables of config file in [config] table
    std::vector<std::string> layout;
    std::vector<std::string> percentage_colors;
    std::vector<std::string> colors_name, colors_value;
    std::string              source_path;
    std::string              data_dir;
    std::string              sep_reset;
    std::string              title_sep;
    std::string              gui_bg_image;
    std::string              gui_css_file;
    std::string              ascii_logo_type;
    std::string              logo_position;
    std::string              offset;
    std::uint16_t            logo_padding_left   = 0;
    std::uint16_t            logo_padding_top    = 0;
    std::uint16_t            layout_padding_top  = 0;
    std::uint32_t            loop_ms             = 0;
    bool                     sep_reset_after     = false;
    bool                     slow_query_warnings = false;
    bool                     use_SI_unit         = false;
    bool                     wrap_lines          = false;

    // Variables of config file for
    // modules specific configs
    // [auto.disk]
    std::string auto_disks_fmt;
    int         auto_disks_types     = 0;
    bool        auto_disks_show_dupl = false;

    // [os.uptime]
    std::string uptime_d_fmt;
    std::string uptime_h_fmt;
    std::string uptime_m_fmt;
    std::string uptime_s_fmt;

    // [os.pkgs]
    std::vector<std::string> pkgs_managers;
    std::vector<std::string> pacman_dirs;
    std::vector<std::string> flatpak_dirs;
    std::vector<std::string> dpkg_files;
    std::vector<std::string> apk_files;

    // inner management / argument configs
    std::vector<std::string> args_layout;
    std::string              args_custom_distro;
    std::string              args_image_backend;
    std::uint16_t            m_offset_calc          = 0;
    bool                     m_display_distro       = true;
    bool                     args_disable_source    = false;
    bool                     args_disable_colors    = false;
    bool                     args_disallow_commands = false;
    bool                     args_print_logo_only   = false;

    /**
     * Load config file and parse every config variables
     * @param filename The config file path
     * @param colors The colors struct where we'll put the default config colors.
     *               It doesn't include the colors in config.alias-colors
     */
    void loadConfigFile(const std::filesystem::path& filename);

    /**
     * Generate the default config file at path
     * @param filename The config file path
     */
    void generateConfig(const std::filesystem::path& filename);

    /**
     * Add alias values to colors_name and colors_value.
     * @param str The alias color to add.
     *            Must have a '=' for separating color name and value,
     *            E.g "pink=!#FFC0CB"
     */
    void addAliasColors(const std::string& str);

    /**
     * Override a config value from --override
     * @param opt The value to override.
     *            Must have a '=' for separating the name and value to override.
     *            NO spaces between
     */
    void overrideOption(const std::string& opt);

    /**
     * Override a config value from --override
     * @param key The value name to override.
     *            Must have a '=' for separating the name and value to override.
     *            NO spaces between
     * @param value The value that will overwrite
     */
    template <typename T>
    void overrideOption(const std::string& key, const T& value)
    {
        override_configs_types o;
        if constexpr (std::is_same_v<T, bool>)
        {
            o.value_type = BOOL;
            o.bool_value = value;
        }
        else if constexpr (std::is_convertible_v<T, std::string>)
        {
            o.value_type   = STR;
            o.string_value = value;
        }
        else if constexpr (std::is_convertible_v<T, int>)
        {
            o.value_type = INT;
            o.int_value  = value;
        }
        overrides[key] = std::move(o);
    }
};

#endif  // _CONFIG_HPP
