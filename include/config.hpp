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

    // inner management
    std::string m_custom_distro;
    bool        m_disable_source  = false;
    bool        m_display_distro  = true;
    bool        m_print_logo_only = false;
    std::vector<std::string> m_arg_colors_name, m_arg_colors_value;

    void        loadConfigFile(const std::string_view filename, colors_t& colors);
    std::string getThemeValue(const std::string& value, const std::string& fallback) const;
    void        generateConfig(const std::string_view filename);

    std::vector<std::string> getValueArrayStr(const std::string_view value, const std::vector<std::string>& fallback);

    template <typename T>
    T getValue(const std::string& value, const T&& fallback) const
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
# inside here there are 4 "tags": $<> $() ${} $[]

# The Info tag $<> lets you print the value of a member of a module.
# e.g $<user.name> will print the username, $<os.kernel_version> will print the kernel version and so on.
# All the modules and their members are listed in the `--list-modules` argument

# The Bash command tag $() let's you execute bash commands.
# e.g $(echo \"hello world\") will indeed echo out Hello world.
# you can even use pipes
# e.g $(echo \"hello world\" | cut -d' ' -f2) will only print world

# The Conditional tag $[] is used to display different outputs based on the comparison.
# syntax MUST be $[something,equalToSomethingElse,iftrue,ifalse] with no spaces between commas ','
# Each part can have a tag or anything else.
# e.g $[$<user.name>,$(echo $USER),the name is correct,the name is NOT correct]
# This is useful when on some terminal or WM the detection can be different than others

# The Color tag ${} is used for printing the text of a certain color.
# e.g "${red}hello world" will indeed print "hello world" in red (or the color you set in the variable)
# you can even put a custom hex color e.g: ${#ff6622}
# It's possible to enable multiple options, put these symbols before '#':
# * b, for making the color in the background
# * u, for underline the text
# * !, for making the text bold
# * i, for making the text italic
#
# Alternatively, ANSI escape codes can be used, e.g ${\e[1;32m} or ${\e[0;34m}.
# NOTE: 256-color ANSI escape codes (those that starts with \\[38 or \\[48) cannot be used in GUI mode.
#
# To reset colors, use ${0} for a full reset or ${1} for a bold reset.
#
# For auto coloring, depending on the ascii logo colors, use ${auto}.
# They can be used for different colors too. So for getting the 2nd color of the ascii logo,
# use ${auto2}, for the 4th one use ${auto4} and so on.
#
# If you're using GUI mode, all the \fB${auto}\fR colors are going to be ${white}

# The Percentage tag $%% is used for displaying the percentage between 2 numbers.\
# It **Must** contain a comma for separating the 2. They can be either be taken from a tag or it put yourself.\
# For example: $%10,5%

# Little FAQ
# Q: "Why when I use something like "$<os.kernel> <- Kernel" it won't work on GUI mode?"
# A: replace "<-" with "\\<-". It won't affect the printing in terminal

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
    "${auto}Disk(/): $<disk(/).disk>",
    "${auto}Swap: $<ram.swap>",
    "${auto}CPU: $<cpu.cpu>",
    "${auto}GPU: $<gpu.name>",
    "${auto}RAM: $<ram.ram>",
    "",
    "$<builtin.colors_bg>", # normal colors
    "$<builtin.colors_light_bg>" # light colors
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

# Offset between the ascii art and the layout
offset = 5

# Padding between the start and the ascii art
logo-padding-left = 0

# Padding of the ascii art from the top
logo-padding-top = 0

# Padding of the layout from the top
layout-padding-top = 0

# Colors can be with: hexcodes (#55ff88) and for bold put '!' (!#55ff88)
# OR ANSI escape code colors like "\e[1;34m"
# remember to add ${0} where you want to reset color
black   = "\e[1;30m"
red     = "\e[1;31m"
green   = "\e[1;32m"
yellow  = "\e[1;33m"
blue    = "\e[1;34m"
magenta = "\e[1;35m"
cyan    = "\e[1;36m"
white   = "\e[1;37m"

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

# These are the colors palette you can use in the GUI mode.
# They can overwritte with ANSI escape code colors
# but they don't work with those, only hexcodes
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
