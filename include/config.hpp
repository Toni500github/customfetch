#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#define TOML_HEADER_ONLY 0

#include <cstdint>

#include "toml++/toml.hpp"
#include "util.hpp"

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
    Config(const std::string_view configFile, const std::string_view configDir, colors_t& colors);

    // config file
    std::vector<std::string> layout;
    std::vector<std::string> percentage_colors;
    std::vector<std::string> colors_name, colors_value;
    std::string   source_path;
    std::string   font;
    std::string   data_dir;
    std::string   sep_reset;
    std::string   builtin_title_sep;
    std::string   gui_bg_image;
    std::string   ascii_logo_type;
    std::uint16_t offset             = 0;
    std::uint16_t logo_padding_left  = 0;
    std::uint16_t logo_padding_top   = 0;
    std::uint16_t layout_padding_top = 0;
    bool          gui                = false;
    bool          sep_reset_after    = false;
    bool          slow_query_warnings= false;

    // modules specific config
    std::string uptime_d_fmt;
    std::string uptime_h_fmt;
    std::string uptime_m_fmt;
    std::string uptime_s_fmt;

    std::vector<std::string> pkgs_managers;
    std::vector<std::string> pacman_dirs;
    std::vector<std::string> flatpak_dirs;
    std::vector<std::string> dpkg_files;
    std::vector<std::string> apk_files;

    // inner management / argument configs
    std::string m_custom_distro;
    std::string m_image_backend;
    bool        m_disable_source  = false;
    bool        m_display_distro  = true;
    bool        m_print_logo_only = false;

    void        loadConfigFile(const std::string_view filename, colors_t& colors);
    std::string getThemeValue(const std::string_view value, const std::string_view fallback) const;
    void        generateConfig(const std::string_view filename);
    void        addAliasColors(const std::string_view value);
    std::vector<std::string> getValueArrayStr(const std::string_view value, const std::vector<std::string>& fallback);

    template <typename T>
    T getValue(const std::string_view value, const T&& fallback) const
    {
        std::optional<T> ret = this->tbl.at_path(value).value<T>();
        if constexpr (toml::is_string<T>)  // if we want to get a value that's a string
            return ret ? expandVar(ret.value()) : expandVar(fallback);
        else
            return ret.value_or(fallback);
    }

private:
    toml::table tbl;
};

inline constexpr std::string_view AUTOCONFIG = R"#([config]
# customfetch is designed with customizability in mind
# here is how it works:
# the variable "layout" is used for showing the infos
# as like as the user want, no limitation.
# inside here there are 5 "tags": $<> $() ${} $[] $%%

# The Info tag $<> lets you print the value of a member of a module.
# e.g $<user.name> will print the username, $<os.kernel_version> will print the kernel version and so on.
# All the modules and their members are listed in the `--list-modules` argument

# The Bash command tag $() will execute bash commands and print the output.
# e.g $(echo \"hello world\") will indeed echo out hello world.
# you can even use pipes
# e.g $(echo \"hello world\" | cut -d' ' -f2) will only print world

# The Conditional tag $[] is used to display different outputs based on the comparison.
# syntax MUST be $[something,equalToSomethingElse,iftrue,ifalse]
# note: putting spaces between commas, could change the expected result
#
# Each part can have a tag or anything else.
# e.g $[$<user.name>,$(echo $USER),the name is correct,the name is NOT correct]
# This is useful when on some terminal or WM the detection can be different than others

# The Color tag ${} is used for printing the text of a certain color.
# e.g "${red}hello world" will indeed print "hello world" in red (or the color you set in the variable)
# The colors can be predefined such as: black, red, green, blue, cyan, yellow, magenta, white.
# They can be configured in the config file.
#
# They can have hexcodes colors (e.g "#5522dd").
# You can apply special effects to colors by using the following symbols before the '#' in hex codes:
#     Terminal and GUI                   GUI Only
# * b  for background color.     * o        for overline
# * u  to  underline the text    * a(value) for fg alpha (either a plain integer between 1 and 65536 or a percentage value like `50%`)
# * !  for bold text             * L(value) to  underline the text with a style (`none`, `single`, `double`, `low`, `error`)
# * i  for italic text           * U(value) for choosing the underline color (hexcode without #)
#                                * B(value) for bg color text (hexcode without #)
#     Terminal Only
# * l for blinking text
#
# Alternatively, ANSI escape codes can be used, e.g ${\e[1;32m} or ${\e[0;34m}.
# NOTE: 256-color ANSI escape codes (those that starts with \\[38 or \\[48) cannot be used in GUI mode.
#
# To reset colors, use ${0} for a normal reset or ${1} for a bold reset.
#
# To use the colors that the ascii art logo uses, use ${auto} for getting the 1st color, ${auto4} for the 4th one and so on.
# If you're using GUI mode and wants to display a custom source that's an image, all the auto colors will be the same colors as the distro ones

# The Percentage tag $%% is used for displaying the percentage between 2 numbers.\
# It **Must** contain a comma for separating the 2. They can be either be taken from a tag or it put yourself.\
# For example: $%50,100%
# For inverting colors of bad and great (red and green), before the first '%' put '!'
# without quotes ofc

# Little FAQ
# Q: Why when I use & or < in the config or ASCII art, it won't work on GUI mode?
# A: replace "<" with "\\<" in the config, or "\<" in the ascii art. Same goes for &
#    It won't affect the printing in terminal

# Q: I want to use `cbonsai` as ASCII art, how do I use it?
# A: First off, create a text file and there put only `$(!cbonsai -p)`
#    Save the file and use `-s "/path/to/text/file"`.
#    Use `--offset` (`-o`) for aligning and put it under the bonsai.
#
#    Read the manual cufetch.1 for more infos with $() tag

layout = [
    "$<builtin.title>",
    "$<builtin.title_sep>",
    "${auto}OS: $<os.name> $<system.arch>",
    "${auto}Host: $<system.host>",
    "${auto}Kernel: $<os.kernel>",
    "${auto}Uptime: $<os.uptime>",
    "${auto}Terminal: $<user.term>",
    "${auto}Shell: $<user.shell>",
    "${auto}Packages: $<os.pkgs>",
    "${auto}Theme: $<theme-gtk-all.name>",
    "${auto}Icons: $<theme-gtk-all.icons>",
    "${auto}Font: $<theme-gtk-all.font>",
    "${auto}Cursor: $<theme.cursor> ($<theme.cursor_size>px)",
    "${auto}WM: $<user.wm_name>",
    "${auto}DE: $<user.de_name>",
    "${auto}Disk (/): $<disk(/).disk>",
    "${auto}Swap: $<swap.swap>",
    "${auto}CPU: $<cpu.cpu>",
    "${auto}GPU: $<gpu.vendor> $<gpu.name>",
    "${auto}RAM: $<ram.ram>",
    "",
    "$<builtin.colors>", # normal colors
    "$<builtin.colors_light>" # light colors
]

# display ascii-art or image/gif (GUI only) near layout
# put "os" for displaying the OS ascii-art
# or the "/path/to/file" for displaying custom files
# or "off" for disabling ascii-art or image displaying
source-path = "os"

# Path to where we'll take all the distros/OSs ascii arts
# note: it MUST contain an "ascii" subdirectory
data-dir = "/usr/share/customfetch"

# The type of ASCII art to apply ("small", "old").
# Basically will add "_<type>" to the logo filename.
# It will return the regular linux ascii art if it doesn't exist.
# Leave empty it for regular.
ascii-logo-type = ""

# A char (or string) to use in $<builtin.title_sep>
title-sep = "-"

# A separator (or string) that when ecountered, will automatically
# reset color, aka. automatically add ${0} (only in layout)
# Make it empty for disabling
sep-reset = ":"

# Should we reset color after or before the separator?
# true  = after  ("test ->${0} ")
# false = before ("test ${0}-> ")
sep-reset-after = false

# Warn against tradeoffs between slower queries for availability
# e.g. falling back to gsettings when we can't find the config file for GTK
slow-query-warnings = false

# Offset between the ascii art and the layout
offset = 5

# Padding between the start and the ascii art
logo-padding-left = 0

# Padding of the ascii art from the top
logo-padding-top = 0

# Padding of the layout from the top
layout-padding-top = 0

# Colors
black   = "\e[1;30m"
red     = "\e[1;31m"
green   = "\e[1;32m"
yellow  = "\e[1;33m"
blue    = "\e[1;34m"
magenta = "\e[1;35m"
cyan    = "\e[1;36m"
white   = "\e[1;37m"

# Alias colors.
# They can be used as like as the color tag.
# This is as like as using the --color argument
# Syntax must be "name=value", e.g "purple=magenta" or "orange=!#F08000"
alias-colors = ["purple=magenta"]

# Colors to be used in percentage tag and modules members.
# They are used as if you're using the color tag.
# It's an array just for "convinience"
# 1st color for good
# 2nd color for normal
# 3rd color for bad
percentage-colors = ["green", "yellow", "red"]

# $<os.uptime> config
[os.uptime]
# how to display the name of the uptime
# e.g: hours = "hrs" -> "Uptime: 3hrs"
days  = " days"
hours = " hours"
mins  = " mins"
secs  = " seconds"

# $<os.pkgs> config
[os.pkgs]
# Ordered list of which packages installed count should be displayed in $<os.pkgs>
# remember to not enter the same name twice, else the world will finish
# Choices: pacman, flatpak, dpkg, apk
#
# Pro-tip: if your package manager isnt listed here, yet,
# use the bash command tag in the layout
# e.g "Packages: $(pacman -Q | wc -l) (pacman)"
pkg-managers = ["pacman", "dpkg", "flatpak"]

# Distros and package manager specific
# package manager paths for getting the packages count from path.
# They are arrayies so you can add multiple paths.
#
# If you don't know what these ares, leave them by default settings
pacman-dirs  = ["/var/lib/pacman/local/"]
dpkg-files   = ["/var/lib/dpkg/status"]
flatpak-dirs = ["/var/lib/flatpak/app/", "~/.local/share/flatpak/app/"]
apk-files    = ["/var/lib/apk/db/installed"]

# GUI options
# note: customfetch needs to be compiled with GUI_MODE=1 (check with "cufetch --version")
[gui]
enable = false

# Font to be used
# syntax must be [FAMILY-LIST] [STYLE-OPTIONS] [SIZE]
# e.g "Liberation Mono Normal 12"
# check https://lazka.github.io/pgi-docs/Pango-1.0/classes/FontDescription.html#Pango.FontDescription for more infos
font = "Liberation Mono Normal 12"

# These are the colors you can use in the GUI mode.
# They overwrite the normal colors from above,
# but they can only have hexcodes colors
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

#endif
