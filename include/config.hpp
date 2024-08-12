#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#define TOML_HEADER_ONLY 0

#include "toml++/toml.hpp"
#include "util.hpp"
#include <cstdint>

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
    std::string              source_path;
    std::string              font;
    std::string              data_dir;
    std::string              sep_reset;
    std::uint16_t            offset = 0;
    std::uint16_t            logo_padding_top = 0;
    bool                     gui    = false;
    std::vector<std::string> layouts;
    std::vector<std::string> pkgs_managers;

    // modules specific config
    std::string uptime_d_fmt;
    std::string uptime_h_fmt;
    std::string uptime_m_fmt;
    std::string uptime_s_fmt;

    // inner management
    std::string m_custom_distro;
    bool        m_disable_source = false;
    bool        m_display_distro = true;
    bool        m_print_logo_only = false;

    void        loadConfigFile(const std::string_view filename, colors_t& colors);
    std::string getThemeValue(const std::string& value, const std::string& fallback) const;

    template <typename T>
    T getConfigValue(const std::string& value, const T&& fallback) const
    {
        std::optional<T> ret = this->tbl.at_path(value).value<T>();
        if constexpr ( toml::is_string<T> )  // if we want to get a value that's a string
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
# the variable "layout" is used for showing the infos and/or something else
# as like as the user want, no limitation.
# inside here there are 3 "modules": $<> $() ${}

# $<> lets you access a member of a module
# e.g $<user.name> will print the username, $<os.kernel_version> will print the kernel version and so on.
# run "cufetch -l" for a list of builti-in modules

# $() let's you execute bash commands
# e.g $(echo \"hello world\") will indeed echo out Hello world.
# you can even use pipes
# e.g $(echo \"hello world\" | cut -d' ' -f2) will only print world

# ${} for which color to use for colorizing the text
# e.g "${red}hello world" will indeed print "hello world" in red (or the color you set in the variable)
# you can even put a custom hex color e.g: ${#ff6622} (for bold text put ! before # e.g ${!#ff6622} )
# OR bash escape code colors e.g ${\e[1;32m} or ${\e[0;34m}

# Little FAQ
# Q: "but then if I want to make only some words/chars in a color and the rest normal?"
# A: there is ${0}. e.g "${red}hello ${0}world, yet again" will only print "hello" in red, and then "world, yet again" normal

# Q: "a website for box drawing symbols?"
# A: "https://symbl.cc/en/unicode/blocks/box-drawing/" is really good

layout = [
    "${red}$<user.name>${0}@${cyan}$<os.hostname>",
    "───────────────────────────",
    "${red}OS: $<os.name> $<system.arch>",
    "${yellow}Host: $<system.host>",
    "${green}Kernel: $<os.kernel>",
    "${cyan}Uptime: $<os.uptime>",
    "${!#fff220}Terminal: $<user.term>",
    "${cyan}Shell: $<user.shell>",
    "${!#343488}Packages: $<os.pkgs>",
    "${!#34affd}Theme: $<theme-gtk-all.name>",
    "${!#34aaff}Icons: $<theme-gtk-all.icons>",
    "${!#fa1bba}WM: $<user.wm_name>",
    "${!#f11f2a}DE: $<user.de_name>",
    "${!#117f23}Disk(/): $<disk(/).disk>",
    "${magenta}CPU: $<cpu.cpu>",
    "${blue}GPU: $<gpu.name>",
    "${!#03ff93}RAM: $<ram.ram>",
    "",
    "${\e[40m}   ${\e[41m}   ${\e[42m}   ${\e[43m}   ${\e[44m}   ${\e[45m}   ${\e[46m}   ${\e[47m}   ", # normal colors
    "${\e[100m}   ${\e[101m}   ${\e[102m}   ${\e[103m}   ${\e[104m}   ${\e[105m}   ${\e[106m}   ${\e[107m}   " # light colors
]

# Ordered list of which packages installed count should be displayed in $<os.pkgs>
# remember to not enter the same name twice, else the world will finish
# Choices: pacman
# Pro-tip: if your package manager isnt listed here, yet,
# use the bash command module in the layout
# e.g "Packages: $(pacman -Q | wc -l)"
pkg-managers = ["pacman", "flatpak"]

# display ascii-art or image/gif (GUI only) near layout
# put "os" for displaying the OS ascii-art
# or the "/path/to/file" for displaying custom files
# or "off" for disabling ascii-art or image displaying
source-path = "os"

# Path to where we'll take all the distros/OSs ascii arts
# note: it MUST contain an "ascii" subdirectory
data-dir = "/usr/share/customfetch"

# A separetor (string) that when ecountered, will automatically
# reset color, aka. automatically add ${0} (only in layout)
# Make it empty for disabling
sep-reset = ":"

# Offset between the ascii art and the system infos
offset = 5

# Padding of the logo from the top
logo-padding-top = 0

# Colors can be with: hexcodes (#55ff88) and for bold put '!' (!#55ff88)
# OR ANSI escape code colors like "\e[1;34m"
# remember to add ${0} where you want to reset color
black = "\e[1;30m"
red = "\e[1;31m"
green = "\e[1;32m"
yellow = "\e[1;33m"
blue = "\e[1;34m"
magenta = "\e[1;35m"
cyan = "\e[1;36m"
white = "\e[1;37m"

# $<os.uptime> format config
[os.uptime]
# how to display the name of the uptime
# e.g: hours = "h" -> "Uptime: 3h"
days = " days"
hours = " hours"
mins = " mins"
secs = " seconds"

# GUI options
# note: customfetch needs to be compiled with GUI_MODE=1 (check with "cufetch --version")
[gui]
enable = false

# Font to be used
# syntax must be [FAMILY-LIST] [STYLE-OPTIONS] [SIZE]
# e.g "Liberation Mono Normal 12"
# check https://lazka.github.io/pgi-docs/Pango-1.0/classes/FontDescription.html#Pango.FontDescription for more infos
font = "Liberation Mono Normal 12"

# These are the colors palette you can use in the GUI mode.
# They can overwritte with ANSI escape code colors
# but they don't work with those, only hexcodes
black = "!#000005"
red = "!#ff2000"
green = "!#00ff00"
blue = "!#00aaff"
cyan = "!#00ffff"
yellow = "!#ffff00"
magenta = "!#f881ff"
white = "!#ffffff"

)#";

#endif
