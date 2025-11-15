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

#ifndef _TEXTS_HPP_
#define _TEXTS_HPP_

#include <string_view>

#include "platform.hpp"

// cufetchpm
inline constexpr std::string_view cufetchpm_help = (R"(Usage: cufetchpm <COMMAND> [OPTIONS]...
Manage plugins for customfetch.

Terms:
    REPO:
        - With install: a Git repository, either URL or local path, both containing plugins and a 'cufetchpm.toml' manifest.
        - With enable OR disable: the name of a repository already installed (as listed with 'cufetchpm list').

Examples:
    Install a plugin repository from GitHub:
        cufetchpm install https://github.com/Toni500github/customfetch-plugins-github
    Disable a plugin from an installed repository:
        cufetchpm disable customfetch-plugins-github/github-user-fetch
    Uninstall an entire plugin repository:
        cufetchpm uninstall customfetch-plugins-github

Commands:
    help <COMMAND>                     Show help for a specific command.
    install [OPTIONS] <REPO(s)>...     Install one or more plugin repository from a Git repo or local path.
    uninstall <REPO(s)>...             Uninstall one or more installed plugin repository.
    enable <REPO/PLUGIN>...            Enable one or more plugins from an installed repository.
    disable <REPO/PLUGIN>...           Disable one or more plugins from an installed repository.
    list                               Show all plugins installed via state.toml.
    update                             Update and upgrade all repositories
    gen-manifest                       Generate a template 'cufetchpm.toml' file.

Global options:
    -h, --help          Show this help message.
    -V, --version       Show version and build information.

)");

inline constexpr std::string_view cufetchpm_help_install = (R"(Usage: cufetchpm install [OPTIONS] <REPO>...

Install one or more plugin repositories. If a given argument exists on disk,
it is treated as a local directory. Otherwise, it is treated as a Git
repository URL and will be cloned.

All plugins found within the repository will be installed.

Options:
    -f, --force        Force installation, even if already installed.
    -h, --help         Show help for this command.
)");

inline constexpr std::string_view cufetchpm_help_list = (R"(Usage: cufetchpm list [options]
List all installed plugins.

Options:
    -v, --verbose      Show detailed plugin information.
    -h, --help         Show help for this command.
)");

// customfetch

inline constexpr std::string_view customfetch_help = (R"(Usage: customfetch [OPTIONS]...
A command-line, GUI app, and Android widget system information tool (like neofetch) focused on customizability and performance.

NOTE: Boolean flags [<BOOL>] accept: "true", 1, "enable", or empty. Any other value is treated as false.

GENERAL OPTIONS:
    -h, --help                  Print this help menu.
    -V, --version               Print version and other infos about the build.
    -C, --config <PATH>         Path to the config file (default: ~/.config/customfetch/config.toml).

    --gen-config [<PATH>]       Generate default config file. If PATH is omitted, saves to default location.
                                Prompts before overwriting.

LOGO OPTIONS:
    -n, --no-logo               Disable logo display.
    -L, --logo-only             Print only the logo (skip layout completely).
    -s, --source-path <PATH>    Path to custom ASCII art/image file.

    -a, --ascii-logo-type <TYPE>
                                Type of ASCII art (typically "small", "old", or empty for default).
                                Example: "-d arch -a older" looks for "arch_older.txt".

    -D, --data-dir <PATH>       Path to data directory containing "ascii/" subfolder with distro logos.
    -d, --distro <NAME>         Use a custom distro logo (case-insensitive, e.g., "windows 11" or "ArCh").
    -p, --logo-position <POS>   Logo position: "top" (default), "left", or "bottom".
    -o, --offset <NUM>          Space between logo and layout (default: 5).
    --logo-padding-top <NUM>    Logo padding from top (default: 0).
    --logo-padding-left <NUM>   Logo padding from left (default: 0).

LAYOUT & FORMATTING:
    -m, --layout-line <STRING>  Override config layout with custom line(s).
                                Example: `-m "${auto}OS: $<os.name>" -m "${auto}CPU: $<cpu>"`.

    -N, --no-color              Disable all colors (useful for pipes/scripts).
    --layout-padding-top <NUM>	Layout padding from top (default: 0).
    --wrap-lines=[<BOOL>]       Enable terminal line wrapping (default: false).
    --title-sep <STRING>        String to use for $<title.sep> (default: "-").
    --sep-reset <STRING>        String that resets color (default: ":").
    --sep-reset-after=[<BOOL>]  Reset color after (default) or before 'sep-reset'.

GUI/TERMINAL OPTIONS:
    -f, --font <STRING>         GUI font (format: "FAMILY STYLE SIZE", e.g., "Liberation Mono Normal 12").
    -i, --image-backend <NAME>  Terminal image backend ("kitty" or "viu").
    --bg-image <PATH>           GUI background image path ("disable" to turn off).

CONFIG:
    -O, --override <STRING>     Override a config value (non-array). Syntax: "name=value" (no spaces around "=").
                                Example: "auto.disk.fmt='Disk(%1): %6'".
                                Note: Names without dots (e.g., "sep-reset-after") gets auto-appended to "config.".

    --color <STRING>            Replace a color globally. Syntax: "name=value" (no spaces around "=").
                                Example: "--color magenta=#FF00FF".

    --disallow-command-tag      Do not allow command tags $() to be executed.
                                This is a safety measure for preventing malicious code to be executed because you didn't want to check the config first.

INFORMATIONAL:
    -l, --list-modules          List all available info tag modules (e.g., $<cpu> or $<os.name>).
    -w, --how-it-works          Explain tags and general customization.
    --list-logos                List available ASCII logos in --data-dir.

LIVE MODE:
    --loop-ms <NUM>             Run in live mode, updating every <NUM> milliseconds (min: 50).
                                Use inferior <NUM> than 200 to disable. Press 'q' to exit.

EXAMPLES:
    1. Minimal output with default logo:
       customfetch --no-color

    2. Custom distro logo with live updates:
       customfetch --distro "arch" --loop-ms 1000

    3. Override layout and colors:
       customfetch -m "${magenta}OS: $<os.name>" --color "magenta=#FF00FF"

For details, see `man customfetch` or run `--how-it-works`.
)");

constexpr std::string_view explain_customfetch = (R"(
customfetch is designed for super customizability, allowing users to display fetched information the way they want.
The layout and logo is controlled through special tags that can output fetched infos, execute commands, apply conditional logic, add colors, and calculate percentages with some colors.

Tag References:
1. Information Tag ($<>)
    Retrieves fetched information from modules.
    Information can be from installed plugins or builtin system-information fetching.

    Syntax: $<module.submodule.recurisve..> or $<module>

    Examples:
    - $<user.name>       # Shows login username
    - $<os.kernel.name>  # Shows kernel name only
    - $<ram>             # Shows formatted RAM usage

    Use `--list-modules` to see all available modules and recursive submodules.

2. Bash Command Tag ($())
    Executes shell commands and outputs the result.
    Supports full shell syntax including pipes and redirection.

    Syntax: $(command)

    Examples:
    - $(echo "hello")             # Outputs: hello
    - $(uname -r | cut -d'-' -f1) # Shows kernel version number only

3. Conditional Tag ($[])
    Displays different outputs based on conditions.

    Syntax: $[something,something_else,if_equal,if_not_equal]

    Examples:
    - $[$<user.name>,toni,Welcome back!,Access denied]
    - $[$(date +%m-%d),12-25,Merry Christmas!,]
    - $[$<os.name.id>,arch,${green}I use arch btw,${red}Non-arch user]

4. Color Tag (${})
    Applies colors and text formatting for following text.

    Basic syntax: ${color} or ${modifiers#RRGGBB}

    Color options:
    - Named colors from config (cyan, from alias-colors, ...)
    - Hex colors: ${#ff00cc}
    - Special colors: ${auto} (uses logo colors)
    - Reset styles: ${0} (normal reset), ${1} (bold reset)

    Formatting modifiers (prefix before hexcolor):
    - ! = Bold
    - u = Underline
    - i = Italic
    - s = Strikethrough
    - l = Blink (terminal only)
    - b = Background color

    Advanced GUI-only modifiers:
    - o        = Overline
    - a(value) = Foreground alpha (1-65536 or 0%-100%)
    - L(value) = Underline style (none/single/double/low/error)
    - U(color) = Underline color (hex)
    - B(color) = Background color (hex)
    - S(color) = Strikethrough color (hex)
    - O(color) = Overline color (hex)
    - A(value) = Background alpha (1-65536 or 0%-100%)
    - w(value) = Font weight (light/normal/bold/ultrabold or 100-1000)

    Examples:
    GUI App only:
        ${oU(#ff0000)L(double)}Error            # Double red underline
        ${a(50%)#00ff00}Semi-transparent green
    GUI and Terminal:
        ${\e[1;33m}Bold yellow
        ${b#222222}${white}White on gray
        ${auto3}The 3rd logo color

    Notes:
    - customfetch will ignore GUI-specific modifiers on terminal.
    - if you're using the GUI app and want to display a custom logo that's an image,
      all the ${auto} colors will be the same colors as the auto-detected distro ASCII art.

5. Percentage Tag ($%%)
    Calculates and displays colored percentages.

    Syntax: $%value,total% or $%!value,total% (inverted colors)

    Examples:
    - $%$<ram.used>,$<ram.total>%
    - $%!50,100% (shows red if low)
    - $%$(cat /sys/class/power_supply/BAT0/capacity),100%

Pro Tip:
- Combine tags for powerful formatting:
  ${u#5522dd}$[$(date +%H),12,Good ${yellow}morning,Good ${#ff8800}afternoon]

FAQ:
Q: Why do special characters (&, <) break the GUI display?
A: Escape these characters with \\ (e.g replace "<" with "\\<"):
   This doesn't affect terminal output.

Q: How can I use cbonsai as ASCII art?
A: 1. Create a text file containing: $(!cbonsai -p)
   2. Use: customfetch -s "/path/to/file.txt"
   3. Adjust offset for proper alignment (not dynamic)

Q: Does customfetch support nested tags?
A: Yes! Complex nesting is supported, for example:
   $<disk($<disk($[1,1,$(echo -n $<disk(/).mountdir>),23]).mountdir>)>
)");

// default customfetch config
inline constexpr std::string_view AUTOCONFIG = R"#([config]

# For more information on how customfetch works and the layout,
# Read either:
# * -w or --how-it-works
# * the manual customfetch.1
# * if on the android app, click the button "how it works" during widget configuration
layout = [
    "$<title>",
    "$<title.sep>",
    "${auto}OS: $<os.name> $<system.arch>",
    "${auto}Host: $<system.host>",
    "${auto}Kernel: $<os.kernel>",
    "${auto}Uptime: $<os.uptime>",
    "${auto}Terminal: $<user.terminal>",
    "${auto}Shell: $<user.shell>",)#"
#if !CF_ANDROID
                                               R"#(
    "${auto}Theme: $<theme.gtk.all.name>",
    "${auto}Icons: $<theme.gtk.all.icon>",
    "${auto}Font: $<theme.gtk.all.font>",
    "${auto}Cursor: $<theme.cursor>",
    "${auto}WM: $<user.wm.name>",
    "${auto}DE: $<user.de.name>",)#"
#endif
                                               R"#(
    "$<auto.disk>",
    "${auto}Swap: $<swap>",
    "${auto}CPU: $<cpu>",
    "${auto}GPU: $<gpu>",
    "${auto}RAM: $<ram>",
    "",
    "$<colors>", # normal colors
    "$<colors.light>" # light colors
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

# These are the colors you can use in the GUI mode.
# They overwrite the terminal colors from above.
# They can only have hexcodes colors and its modifiers
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

# Path to gtk css file to be used.
# put "disable" for disabling.
gtk-css = "disable"

)#";

inline constexpr std::string_view AUTO_MANIFEST = R"([repository]
# The repositry name.
# It must contain only alpha-numeric characters and symbols such as '-' or '_'
name = "repo_name"

# The repository git clone / homepage url.
url = "https://github.com/user/repo"

# Platform-dependent packages required to build all plugins in this repository.
# NOTE: This will ONLY tell the user which packages are missing;
#       it will not actually install them.
#
# Use the "all" key for dependencies common to every platform.
# Current platforms: all, linux, macos, android
[dependencies]
all     = ["pkg-config", "cmake"]
linux   = ["wayland-protocols", "xorg-dev"]
android = ["ndk-build"]
macos   = ["gtk+3"]

# From now on, each table that is neiter "repository" nor "dependencies" will be treated as a plugin entry.
# The tables' names still must conform alpha-numeric characters and symbols such as '-' or '_'
[test-plugin]

# The plugin description.
description = "Test plugin"

# The plugin authors.
authors = ["user1", "friend_user1"]

# The plugin SPDX License Identifiers (not validated)
licenses = ["MIT", "GPL-2.0"]

# A list of registered root modules the plugin can provide.
prefixes = ["github", "git"]

# What platforms are supported by the plugin (case-sensitive).
# Use just ["all"] for cross-platform plugins.
# Current platforms: all, linux, macos, android
platforms = ["all"]

# The directory where the final plugin output shall be seen.
# It will be evaluated/checked after building the plugin library.
# The path is relative to the repository root unless absolute.
output-dir = "build/plugin-dir/"

# A list of commands to be executed for building the plugin.
# All commands are executed in a single shared shell session,
# so environment variables, `cd`, and other shell state persist across steps.
# Commands are executed in order and stop at the first failure.
build-steps = [
    "make -C ./test-plugin-entry/",
    "mkdir -p ./build/plugin-dir/",
    "mv ./test-plugin-entry/library.so ./build/plugin-dir/library.so"
])";

#endif  // !_TEXTS_HPP_
