#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>

#define TOML_HEADER_ONLY 0
#include "toml++/toml.hpp"

#include "cufetch/common.hh"

enum types
{
    STR,
    BOOL,
    INT
};

struct override_configs_types
{
    types       value_type;
    std::string string_value = "";
    bool        bool_value   = false;
    int         int_value    = 0;
};

class EXPORT ConfigBase
{
public:

    /**
     * Get value of config variables
     * @param value The config variable "path" (e.g "config.source-path")
     * @param fallback Default value if couldn't retrive value
     */
    template <typename T>
    T getValue(const std::string_view value, const T&& fallback) const
    {
        const auto& overridePos = overrides.find(value.data());

        // user wants a bool (overridable), we found an override matching the name, and the override is a bool.
        if constexpr (std::is_convertible<T, bool>())
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == BOOL)
                return overrides.at(value.data()).bool_value;

        if constexpr (std::is_convertible<T, std::string>())
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == STR)
                return overrides.at(value.data()).string_value;

        if constexpr (std::is_convertible<T, int>())
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == INT)
                return overrides.at(value.data()).int_value;

        const std::optional<T>& ret = this->tbl.at_path(value).value<T>();
        return ret.value_or(fallback);
    }

    /**
     * getValue() but don't want to specify the template, so it's std::string,
     * and thus the name, only used when retriving the colors for terminal and GUI
     * @param value The config variable "path" (e.g "config.gui-red")
     * @param fallback Default value if couldn't retrive value
     */
    std::string getThemeValue(const std::string_view value, const std::string_view fallback) const
    {
        return this->tbl.at_path(value).value<std::string>().value_or(fallback.data());
    }

    /**
     * Get value of config array of string variables
     * @param value The config variable "path" (e.g "config.gui-red")
     * @param fallback Default value if couldn't retrive value
     */
    std::vector<std::string> getValueArrayStr(const std::string_view value, const std::vector<std::string>& fallback);

protected:
    std::unordered_map<std::string, override_configs_types> overrides;

    // Parsed config from loadConfigFile()
    toml::table tbl;
};

class EXPORT Config : public ConfigBase
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
    std::string              gui_css_file;
    std::string              gui_bg_image;
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
     * @param str The value to override.
     *            Must have a '=' for separating the name and value to override.
     *            NO spaces between
     */
    void overrideOption(const std::string& opt);

    /**
     * Override a config value from --override
     * @param str The value to override.
     *            Must have a '=' for separating the name and value to override.
     *            NO spaces between
     */
    void overrideOption(const std::string& opt, const override_configs_types& option);
};
