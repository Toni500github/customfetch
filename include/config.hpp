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
#include <string_view>
#include "platform.hpp"
#include "cufetch/config.hh"

// default config
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
