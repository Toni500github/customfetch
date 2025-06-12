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

#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#define TOML_HEADER_ONLY 0

#include <cstdint>
#include <type_traits>
#include <unordered_map>

#include "toml++/toml.hpp"
#include "util.hpp"

enum types
{
    STR,
    BOOL,
    INT
};

struct override_configs_types
{
    types value_type;
    std::string string_value = "";
    bool bool_value = false;
    int int_value = 0;
};

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
};

class Config
{
public:
    // Create .config directories and files and load the config file (args or default)
    Config(const std::string_view configFile, const std::string_view configDir);

    // Variables of config file in [config] table
    std::vector<std::string> layout;
    std::vector<std::string> percentage_colors;
    std::vector<std::string> colors_name, colors_value;
    std::string              source_path;
    std::string              font;
    std::string              data_dir;
    std::string              sep_reset;
    std::string              title_sep;
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
    int         auto_disks_types = 0;
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
    std::uint16_t            m_offset_calc        = 0;
    bool                     args_disable_source  = false;
    bool                     args_disable_colors  = false;
    bool                     m_display_distro     = true;
    bool                     args_print_logo_only = false;

    std::unordered_map<std::string, override_configs_types> overrides;

    /**
     * Load config file and parse every config variables
     * @param filename The config file path
     * @param colors The colors struct where we'll put the default config colors.
     *               It doesn't include the colors in config.alias-colors
     */
    void loadConfigFile(const std::string_view filename, colors_t& colors);

    /**
     * Generate the default config file at path
     * @param filename The config file path
     */
    void generateConfig(const std::string_view filename);

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

private:
    // Parsed config from loadConfigFile()
    toml::table tbl;

    /**
     * Get value of config variables
     * @param value The config variable "path" (e.g "config.source-path")
     * @param fallback Default value if couldn't retrive value
     */
    template <typename T>
    T getValue(const std::string_view value, const T&& fallback, bool dont_expand_var = false) const
    {
        const auto& overridePos = overrides.find(value.data());

        // user wants a bool (overridable), we found an override matching the name, and the override is a bool.
        if constexpr (std::is_same<T, bool>())
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == BOOL)
                return overrides.at(value.data()).bool_value;

        // user wants a str (overridable), we found an override matching the name, and the override is a str.
        if constexpr (std::is_same<T, std::string>())
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == STR)
                return overrides.at(value.data()).string_value;

        if constexpr (std::is_same<T, std::uint16_t>())
            if (overridePos != overrides.end() && overrides.at(value.data()).value_type == INT)
                return overrides.at(value.data()).int_value;

        const std::optional<T>& ret = this->tbl.at_path(value).value<T>();
        if constexpr (toml::is_string<T>)  // if we want to get a value that's a string
            return ret ? expandVar(ret.value(), dont_expand_var) : expandVar(fallback, dont_expand_var);
        else
            return ret.value_or(fallback);
    }

    /**
     * getValue() but don't want to specify the template, so it's std::string,
     * and because of the name, only used when retriving the colors for terminal and GUI
     * @param value The config variable "path" (e.g "config.gui-red")
     * @param fallback Default value if couldn't retrive value
     */
    std::string getThemeValue(const std::string_view value, const std::string_view fallback) const;

    /**
     * Get value of config array of string variables
     * @param value The config variable "path" (e.g "config.gui-red")
     * @param fallback Default value if couldn't retrive value
     */
    std::vector<std::string> getValueArrayStr(const std::string_view value, const std::vector<std::string>& fallback);
};

// default config
inline constexpr std::string_view AUTOCONFIG = R"#([config]

# For more information on how customfetch works and the layout,
# Read either:
# * -w or --how-it-works
# * the manual customfetch.1
# * if on the android app, click the button "how it works" during widget configuration
layout = [
    "$<title>",
    "$<title_sep>",
    "${auto}OS: $<os.name> $<system.arch>",
    "${auto}Host: $<system.host>",
    "${auto}Kernel: $<os.kernel>",
    "${auto}Uptime: $<os.uptime>",)#"
#if !CF_ANDROID
    R"#(
    "${auto}Theme: $<theme-gtk-all.name>",
    "${auto}Icons: $<theme-gtk-all.icons>",
    "${auto}Font: $<theme-gtk-all.font>",
    "${auto}Cursor: $<theme.cursor>",
    "${auto}WM: $<user.wm_name>",
    "${auto}DE: $<user.de_name>",)#"
#endif
    R"#(
    "$<auto.disk>",
    "${auto}Swap: $<swap>",
    "${auto}CPU: $<cpu>",
    "${auto}GPU: $<gpu>",
    "${auto}RAM: $<ram>",
    "",
    "$<colors>", # normal colors
    "$<colors_light>" # light colors
]

# display ascii-art or image/gif (GUI only) near layout
# put "os" for displaying the OS ascii-art
# or the "/path/to/file" for displaying custom files
# or "off" for disabling ascii-art or image displaying
source-path = "os"

# Path to where we'll take all the distros/OSs ascii arts.
# note: it MUST contain an "ascii" subdirectory
data-dir = "/usr/share/customfetch"

# The type of ASCII art to apply ("small", "old").
# Basically will add "_<type>" to the logo filename.
# It will return the regular linux ascii art if it doesn't exist.
# Leave empty it for regular.
ascii-logo-type = ""

# A char (or string) to use in $<title_sep>
title-sep = "-"

# A separator (or string) that when encountered, will automatically
# reset color, aka. automatically add ${0} (only in layout)
# Make it empty for disabling
sep-reset = ":"

# Should we reset color after or before the separator?
# true  = after  ("test ->${0} ")
# false = before ("test ${0}-> ")
sep-reset-after = false

# Where the logo should be displayed.
# Values: "top" or "left" or "bottom"
logo-position = "left"

# Offset between the ascii art and the layout
# Can also be rapresented as a %, but super unstable sometimes.
offset = "5"

# Padding between the start and the ascii art
logo-padding-left = 0

# Padding of the ascii art from the top
logo-padding-top = 0

# Padding of the layout from the top
layout-padding-top = 0

# Usually in neofetch/fastfetch, when your terminal size is too small,
# to render some text in 1 line, they don't wrap those lines, instead they truncate them.
# Enable/Disable if you want this
wrap-lines = false

# Used in disk, ram and swap modules.
# If true, we're going to use the SI standard byte unit (1kB == 1000 bytes)
# Else if false, we using the IEC byte unit (1KiB == 1024 bibytes)
# Really nerdy stuff
use-SI-byte-unit = false

# Warn against tradeoffs between slower queries for availability
# e.g. falling back to gsettings when we can't find the config file for GTK
slow-query-warnings = false

# Colors in the terminal (for Desktop/Android app, use the ones under [gui])
black   = "\e[1;30m"
red     = "\e[1;31m"
green   = "\e[1;32m"
yellow  = "\e[1;33m"
blue    = "\e[1;34m"
magenta = "\e[1;35m"
cyan    = "\e[1;36m"
white   = "\e[1;37m"

# Alias colors. Basically more color variables.
# They can be used as like as the color tag.
# This is as like as using the --add-color argument
# Syntax must be "name=value", e.g "purple=magenta" or "orange=!#F08000"
alias-colors = ["purple=magenta"]

# Colors to be used in percentage tag and modules members.
# They are used as if you're using the color tag.
# It's an array just for "convenience"
# 1st color for good
# 2nd color for normal
# 3rd color for bad
percentage-colors = ["green", "yellow", "red"]

# $<auto.disk> config
[auto.disk]
# Format for displaying the auto detected disks infos
# %1 = mount directory
# %2 = device path
# %3 = type of filesystem
# %4 = total amount of storage
# %5 = free amount of storage
# %6 = used amount of storage
# %7 = percentage of used storage
# %8 = percentage of free storage
fmt = "${auto}Disk (%1): $<disk(%1)>"

# Only print disks that matches the description
# of the following types:
# regular   = Regular disks (internel M.2 SSD, ...) (won't be specified)
# external  = External disks (USB, SATA, ...)
# read-only = Disks with read-only filesystems
# hidden    = Disks that are not really mounted by the user
display-types = ["regular", "external", "read-only"]

# In some OSes such as NixOS or Android, there might be some directories that are bind mounted.
# Bind mounted directories create an additional view of an existing directory,
# and `statfs()` on the mount point will return the filesystem statistics of the original directory.
show-duplicated = false

# $<os.uptime> config
[os.uptime]
# how to display the name of the uptime
# e.g: hours = "hrs" -> "Uptime: 3hrs"
days  = " days"
hours = " hours"
mins  = " mins"
secs  = " secs"

# $<os.pkgs> config
[os.pkgs]
# Ordered list of which packages installed count should be displayed in $<os.pkgs>
# remember to not enter the same name twice, else the world will finish
# Choices: pacman, flatpak, dpkg, apk
#
# Pro-tip: if your package manager isn't listed here, yet,
# use the bash command tag in the layout
# e.g "Packages: $(pacman -Q | wc -l) (pacman)"
pkg-managers = ["pacman", "dpkg", "flatpak"]

# Distros and package manager specific
# package manager paths for getting the packages count from path.
# They are arrays so you can add multiple paths.
#
# If you don't know what these ares, leave them by default settings
pacman-dirs  = ["/var/lib/pacman/local/"]
dpkg-files   = ["/var/lib/dpkg/status", "/data/data/com.termux/files/usr/var/lib/dpkg/status"]
flatpak-dirs = ["/var/lib/flatpak/app/", "~/.local/share/flatpak/app/"]
apk-files    = ["/var/lib/apk/db/installed"]

# Desktop/Android app options
[gui]

# Font to be used
# syntax must be [FAMILY-LIST] [STYLE-OPTIONS] [SIZE]
# e.g "Liberation Mono Normal 12"
# check https://lazka.github.io/pgi-docs/Pango-1.0/classes/FontDescription.html#Pango.FontDescription for more infos
font = "Liberation Mono Normal 12"

# These are the colors you can use in the GUI mode.
# They overwrite the terminal colors from above.
# They can only have hexcodes colors
black   = "!#000005"
red     = "!#ff2000"
green   = "!#00ff00"
blue    = "!#00aaff"
cyan    = "!#00ffff"
yellow  = "!#ffff00"
magenta = "!#f881ff"
white   = "!#ffffff"

# Path to image as a background.
# put "disable" for disabling and use the theme color as background.
bg-image = "disable"

)#";

#endif  // _CONFIG_HPP
