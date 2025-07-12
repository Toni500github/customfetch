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

#undef  TOML_HEADER_ONLY
#define TOML_HEADER_ONLY 0

#include <filesystem>
#include <string_view>
#include <string>
#include <vector>
#include <map>

#include "platform.hpp"
#include "cufetch/config.hh"

struct box_chars_t {
    std::string horizontal;
    std::string vertical;
};

class Config : public ConfigBase
{
public:
    int box_extra_padding = 0; // additional spaces added to every column
    // Create .config directories and files and load the config file (args or default)
    Config(const std::filesystem::path& configFile, const std::filesystem::path& configDir);

    // config colors
    struct colors_t
    {
        std::string black, red, green, blue, cyan, yellow, magenta, white;
        std::string gui_black, gui_red, gui_green, gui_blue, gui_cyan, gui_yellow, gui_magenta, gui_white;
    } colors;

    // Variables of config file in [config] table
    std::vector<std::string> layout;
    std::vector<std::string> percentage_colors;
    std::vector<std::string> colors_name, colors_value;
    std::string               source_path, data_dir, sep_reset, title_sep, gui_css_file, gui_bg_image;
    std::string               ascii_logo_type, logo_position, offset;
    std::uint16_t             logo_padding_left = 0, logo_padding_top = 0, layout_padding_top = 0;
    std::uint32_t             loop_ms = 0;
    bool                      sep_reset_after = false, slow_query_warnings = false, use_SI_unit = false;
    bool                      wrap_lines = false, box_drawing_enabled = false;
    box_chars_t               box_chars;

    // [auto.disk]
    std::string auto_disks_fmt;
    int         auto_disks_types = 0;
    bool        auto_disks_show_dupl = false;

    // [os.uptime]
    std::string uptime_d_fmt, uptime_h_fmt, uptime_m_fmt, uptime_s_fmt;

    // [os.pkgs]
    std::vector<std::string> pkgs_managers, pacman_dirs, flatpak_dirs, dpkg_files, apk_files;

    // inner management / argument configs
    std::vector<std::string> args_layout;
    std::string               args_custom_distro, args_image_backend;
    std::uint16_t             m_offset_calc = 0;
    bool                      m_display_distro = true, args_disable_source = false, args_disable_colors = false;
    bool                      args_disallow_commands = false, args_print_logo_only = false;

    void loadConfigFile(const std::filesystem::path& filename);
    void generateConfig(const std::filesystem::path& filename);
    void addAliasColors(const std::string& str);
    void overrideOption(const std::string& opt);

    template <typename T>
    void overrideOption(const std::string& key, const T& value)
    {
        override_configs_types o;
        if constexpr (std::is_same_v<T, bool>) {
            o.value_type = BOOL;
            o.bool_value = value;
        } else if constexpr (std::is_convertible_v<T, std::string>) {
            o.value_type = STR;
            o.string_value = value;
        } else if constexpr (std::is_convertible_v<T, int>) {
            o.value_type = INT;
            o.int_value = value;
        }
        overrides[key] = std::move(o);
    }
};

// MODIFICATION: The default config now lives entirely in the header file.
// It also uses the new $<room> and $<pin> syntax for a robust default layout.
inline constexpr std::string_view AUTOCONFIG = R"#([config]
layout = [
    "$<title>",
    "$<title.sep>",
    "$<room>",
    "  ╭$<fill>╮",
    "  │ ${red} ${0}user   $<pin>│",
    "  │ ${yellow} ${0}hname  $<pin>│",
    "  │ ${green} ${0}distro $<pin>│",
    "  ╰$<fill>╯",
    "$<endroom>",
    "$<room>",
    "  ╭$<fill>╮",
    "  │ ${cyan} ${0}kernel $<pin>│",
    "  │ ${blue} ${0}uptime $<pin>│",
    "  │ ${red} ${0}shell  $<pin>│",
    "  ╰$<fill>╯",
    "$<endroom>",
    "",
    "$<colors>",
    "$<colors.light>"
]

box-drawing-enabled = true
box-extra-padding = 1

[config.box-chars]
horizontal = "─"
vertical   = "│"

source-path = "os"
data-dir = "/usr/share/customfetch"
ascii-logo-type = ""
title-sep = "-"
sep-reset = ":"
sep-reset-after = false
logo-position = "left"
offset = "5"
logo-padding-left = 0
logo-padding-top = 0
layout-padding-top = 0
wrap-lines = false
use-SI-byte-unit = false
slow-query-warnings = false

black   = "\e[1;30m"
red     = "\e[1;31m"
green   = "\e[1;32m"
yellow  = "\e[1;33m"
blue    = "\e[1;34m"
magenta = "\e[1;35m"
cyan    = "\e[1;36m"
white   = "\e[1;37m"

alias-colors = ["purple=magenta"]
percentage-colors = ["green", "yellow", "red"]

[auto.disk]
fmt = "${auto}Disk (%1): $<disk(%1)>"
display-types = ["regular", "external", "read-only"]
show-duplicated = false

[os.uptime]
days  = " days"
hours = " hours"
mins  = " mins"
secs  = " secs"

[os.pkgs]
pkg-managers = ["pacman", "dpkg", "flatpak"]
pacman-dirs  = ["/var/lib/pacman/local/"]
dpkg-files   = ["/var/lib/dpkg/status", "/data/data/com.termux/files/usr/var/lib/dpkg/status"]
flatpak-dirs = ["/var/lib/flatpak/app/", "~/.local/share/flatpak/app/"]
apk-files    = ["/var/lib/apk/db/installed"]

[gui]
black   = "!#000005"
red     = "!#ff2000"
green   = "!#00ff00"
blue    = "!#00aaff"
cyan    = "!#00ffff"
yellow  = "!#ffff00"
magenta = "!#f881ff"
white   = "!#ffffff"
bg-image = "disable"
gtk-css = "disable"
)#";

#endif  // _CONFIG_HPP
