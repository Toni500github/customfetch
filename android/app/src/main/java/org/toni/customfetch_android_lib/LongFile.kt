package org.toni.customfetch_android_lib

fun help(): String = """
Usage: customfetch [OPTIONS]...
A command-line, GUI app, android widget system information tool (or neofetch like program), which its focus point is customizability and performance.

NOTE: Arguments that takes [<bool>] values, the values can be either: "true", 1, "enable" or leave it empty. Any other value will be treated as false.

    -n, --no-logo [<bool>]      Do not display the logo
    -N, --no-color [<bool>]     Do not output and parse colors. Useful for stdout or pipe operations
    -L, --logo-only [<bool>]    Print only the logo
    -s, --source-path <path>    Path to the ascii art or image file to display
    -C, --config <path>         Path to the config file to use
    -a, --ascii-logo-type [<type>]
                                The type of ASCII art to apply ("small" or "old").
                                Basically will add "_<type>" to the logo filename.
                                It will return the regular linux ascii art if it doesn't exist.
                                Leave it empty for regular.

    -D, --data-dir <path>       Path to the data dir where we'll taking the distros ascii arts (must contain subdirectory called "ascii")
    -d, --distro <name>         Print a custom logo from the given `data-dir` (must be the same name, uppercase or lowercase, e.g "windows 11" or "ArCh")
    -f, --font <name>           The font to be used in the GUI app (syntax must be "[FAMILY-LIST] [STYLE-OPTIONS] [SIZE]" without the double quotes and [])
                                An example: [Liberation Mono] [Normal] [12], which can be "Liberation Mono Normal 12"

    -m, --layout-line <string>  Will replace the config layout, with a layout you specify in the arguments
                                Example: `customfetch -m "${'$'}{auto}OS: ${'$'}{'$'}<os.name>" -m "${'$'}{auto}CPU: ${'$'}<cpu.cpu>"`
                                Will only print the logo (if not disabled), along side the parsed OS and CPU

    -O, --override <string>     Overrides a config value, but NOT arrays.
                                Syntax must be "name=value" E.g "auto.disk.fmt='Disk(%1): %6'".
                                For convinience purpose, names that doesn't have a dot for telling the table, will automatically be considered under the [config] table
                                E.g "sep-reset-after=true" works as "config.sep-reset-after=true"

    -p, --logo-position <value> Position of the logo ("top" or "left" or "bottom")
    -o, --offset <num>          Offset between the ascii art and the layout
    -l, --list-modules          Print the list of the info tag modules and its members
        --list-logos            Print the sorted list of the ascii logos you can you use by the given `data-dir`
    -h, --help                  Print this help menu
    -w, --how-it-works          Print how customfetch and the layout variable works
    -V, --version               Print the version along with the git branch it was built

    --loop-ms <num>             Execute customfetch in a loop (live mode) every <num> milliseconds.
                                It won't parse the config every time and will you only notice RAM, uptime etc. changes
                                Put 0 or a <num> minor than 50 to disable and just print once.
                                Not availabile in the android widget app.

    --bg-image <path>           Path to image to be used in the background in the GUI app (put "disable" for disabling in the config)
    --wrap-lines [<bool>]       Wrap lines when printing in terminal
    --logo-padding-top  <num>   Padding of the logo from the top
    --logo-padding-left <num>   Padding of the logo from the left
    --layout-padding-top <num>  Padding of the layout from the top
    --title-sep <string>        A character (or string) to use in ${'$'}<builtin.title_sep>
    --sep-reset <string>        A character (or string) that when encountered, will automatically reset color
    --sep-reset-after [<bool>]  Reset color either before or after 'sep-reset'
    --gen-config [<path>]       Generate default config file to config folder (if path, it will generate to the path)
                                Will ask for confirmation if file exists already

    --color <string>            Replace instances of a color with another value.
                                Syntax MUST be "name=value" with no space between "=", example: --color "foo=#444333".
                                Thus replaces any instance of foo with #444333. Can be done with multiple colors separately.

Read the manual "customfetch.1" or --how-it-works for more infos about customfetch and how it works
"""

fun modulesList(): String = """
--------------------------------------------------------[ MODULE ONLY ]------------------------------------------------------------------------
Should be used as like as ${'$'}<module>
NOTE: module "title_sep" as an extended name version called "title_separator"

Syntax:
# maybe comments of the module
module:
  description [example of what it prints]

ram:
  used and total amount of RAM (auto) with used percentage [2.81 GiB / 15.88 GiB (5.34%)]

swap:
  used and total amount of the swapfile (auto) with used percentage [477.68 MiB / 512.00 MiB (88.45%)]

# note: the module can have either a device path
#       or a filesystem path
#       e.g disk(/) or disk(/dev/sda5)
disk(/path/to/fs):
  used and total amount of disk space (auto) with type of filesystem and used percentage [379.83 GiB / 438.08 GiB (86.70%) - ext4]

# usually people have 1 GPU in their PC,
# but if you got more than 1 and want to query it,
# you should call gpu module with a number, e.g gpu1 (default gpu0).
# Infos are gotten from `/sys/class/drm/` and on each cardN directory
gpu:
  GPU shorter vendor name and model name [NVIDIA GeForce GTX 1650]

cpu:
  CPU model name with number of virtual processors and max freq [AMD Ryzen 5 5500 (12) @ 4.90 GHz]

battery:
  battery current percentage and status [50.00% [Discharging]]

title:
  user and hostname colored with ${'$'}{auto2} [toni@arch2]

title_sep:
  separator between the title and the system infos (with the title length) [--------]

colors:
  color palette with background spaces

colors_light:
  light color palette with background spaces

# with `symbol` I mean a symbol to be used for the
# view of the color palette
colors_symbol(symbol):
  color palette with specific symbol

# with `symbol` I mean a symbol to be used for the
# view of the color palette
colors_light_symbol(symbol):
  light color palette with specific symbol

--------------------------------------------------------[ MODULE MEMBERS ]------------------------------------------------------------------------

Should be used as like as ${'$'}<module.member>
NOTE: module members such as "os.pkgs" or "disk.used_perc" have an extended name version
      "os.pkgs" == "os.packages"
      any module member that has "perc" can be replaced with "percentage"

Syntax:
# maybe comments of the module
module
  member: description [example of what it prints; maybe another]

os
  name:             OS name (pretty name) [Ubuntu 22.04.4 LTS; Arch Linux]
  name_id:          OS name id [ubuntu, arch]
  kernel:           kernel name and version [Linux 6.9.3-zen1-1-zen]
  kernel_name:      kernel name [Linux]
  kernel_version:   kernel version [6.9.3-zen1-1-zen]
  version_id:       OS version id [22.04.4, 20240101.0.204074]
  version_codename: OS version codename [jammy]
  pkgs:             count of the installed packages by a package manager [1869 (pacman), 4 (flatpak)]
  uptime:           (auto) uptime of the system [36 mins, 3 hours, 23 days]
  uptime_secs:      uptime of the system in seconds (should be used along with others uptime_ members) [45]
  uptime_mins:      uptime of the system in minutes (should be used along with others uptime_ members) [12]
  uptime_hours:     uptime of the system in hours   (should be used along with others uptime_ members) [34]
  uptime_days:      uptime of the system in days    (should be used along with others uptime_ members) [2]
  hostname:         hostname of the OS [myMainPC]
  initsys_name:     Init system name [systemd]
  initsys_version:  Init system version [256.5-1-arch]

user
  name:             name you are currently logged in (not real name) [toni69]
  shell:            login shell name and version [zsh 5.9]
  shell_name:       login shell [zsh]
  shell_path:       login shell (with path) [/bin/zsh]
  shell_version:    login shell version (may be not correct) [5.9]
  de_name:          Desktop Environment current session name [Plasma]
  wm_name:          Window Manager current session name [dwm; xfwm4]
  wm_version:       Window Manager version (may not work correctly) [6.2; 4.18.0]
  terminal:         terminal name and version [alacritty 0.13.2]
  terminal_name:    terminal name [alacritty]
  terminal_version: terminal version [0.13.2]

# this module is just for generic theme stuff
# such as indeed cursor
# because it is not GTK-Qt specific
theme
  cursor:      cursor name with its size (auto add the size if queried) [Bibata-Modern-Ice (16px)]
  cursor_name: cursor name [Bibata-Modern-Ice]
  cursor_size: cursor size [16]

# If USE_DCONF flag is set, then we're going to use
# dconf, else backing up to gsettings
theme-gsettings
  name:        gsettings theme name [Decay-Green]
  icons:       gsettings icons theme name [Papirus-Dark]
  font:        gsettings font theme name [Cantarell 10]
  cursor:      gsettings cursor name with its size (auto add the size if queried) [Bibata-Modern-Ice (16px)]
  cursor_name: gsettings cursor name [Bibata-Modern-Ice]
  cursor_size: gsettings cursor size [16]

# the N stands for the gtk version number to query
# so for example if you want to query the gtk3 theme name
# write it like "theme-gtk3.name"
# note: may be slow because of calling "gsettings" if couldn't read from configs.
#       Read theme-gsettings module comments
theme-gtkN
  name:  gtk theme name [Arc-Dark]
  icons: gtk icons theme name [Qogir-Dark]
  font:  gtk font theme name [Noto Sans 10]

# basically as like as the "theme-gtkN" module above
# but with gtk{2,3,4} and auto format gkt version
# note: may be slow because of calling "gsettings" if couldn't read from configs.
# 	Read theme-gsettings module comments
theme-gtk-all
  name:  gtk theme name [Arc-Dark [GTK2/3/4]]
  icons: gtk icons theme name [Papirus-Dark [GTK2/3], Qogir [GTK4]]
  font:  gtk font theme name [Hack Nerd Font 13 [GTK2], Noto Sans 10 [GTK3/4]]

# note: these members are auto displayed in from B to YB (depending if using SI byte unit or not(IEC)).
# they all (except those that has the same name as the module or that ends with "_perc")
# have variants from -B to -YB and -B to -YiB
# example: if you want to show your 512MiB of used RAM in GiB
# use the `used-GiB` variant (they don't print the unit tho)
ram
  used:      used amount of RAM (auto) [2.81 GiB]
  free:      available amount of RAM (auto) [10.46 GiB]
  total:     total amount of RAM (auto) [15.88 GiB]
  used_perc: percentage of used amount of RAM in total [17.69%]
  free_perc: percentage of available amount of RAM in total [82.31%]

# same comments as RAM (above)
swap
  used:      used amount of the swapfile (auto) [477.68 MiB]
  free:      available amount of the swapfile (auto) [34.32 MiB]
  total:     total amount of the swapfile (auto) [512.00 MiB]
  used_perc: percentage of used amount of the swapfile in total [93.29%]
  free_perc: percentage of available amount of the swapfile in total [6.71%]

# same comments as RAM (above)
# note: the module can have either a device path
#	or a filesystem path
#	e.g disk(/) or disk(/dev/sda5)
disk(/path/to/fs)
  used:      used amount of disk space (auto) [360.02 GiB]
  free:      available amount of disk space (auto) [438.08 GiB]
  total:     total amount of disk space (auto) [100.08 GiB]
  used_perc: percentage of used amount of the disk in total [82.18%]
  free_perc: percentage of available amount of the disk in total [17.82%]
  fs:        type of filesystem [ext4]
  device:    path to device [/dev/sda5]
  types:     an array of type options (pretty format) [Regular, External]
  mountdir:  path to the device mount point [/]

# usually people have 1 GPU in their PC,
# but if you got more than 1 and want to query it,
# you should call gpu module with a number, e.g gpu1 (default gpu0).
# Infos are gotten from `/sys/class/drm/` and on each cardN directory
gpu
  name:        GPU model name [GeForce GTX 1650]
  vendor:      GPU short vendor name [NVIDIA]
  vendor_long: GPU vendor name [NVIDIA Corporation]

# cpu module has a member called "temp" and it has 3 variant units:
# "temp_C" (Celsius) "temp_F" (Fahrenheit) "temp_K" (Kelvin)
cpu
  name:     CPU model name [AMD Ryzen 5 5500]
  temp:     CPU temperature (by the chosen unit) [40.62]
  nproc:    CPU number of virtual processors [12]
  freq_cur: CPU current frequency (in GHz) [3.42]
  freq_min: CPU minimum frequency (in GHz) [2.45]
  freq_max: CPU maximum frequency (in GHz) [4.90]
  freq_bios_limit: CPU frequency limited by bios (in GHz) [4.32]

# battery module has a member called "temp" and it has 3 variant units:
# "temp_C" (Celsius) "temp_F" (Fahrenheit) "temp_K" (Kelvin)
battery
  name:           battery model name
  temp:           battery temperature (by the chosen unit)
  perc:           battery current percentage
  vendor:         battery manufacturer name
  status:         battery current status [Discharging, AC Connected]
  technology:     battery technology [Li-lion]
  capacity_level: battery capacity level [Normal, Critical]

system
  host:         Host (aka. Motherboard) model name with vendor and version [Micro-Star International Co., Ltd. PRO B550M-P GEN3 (MS-7D95) 1.0]
  host_name:    Host (aka. Motherboard) model name [PRO B550M-P GEN3 (MS-7D95)]
  host_version: Host (aka. Motherboard) model version [1.0]
  host_vendor:  Host (aka. Motherboard) model vendor [Micro-Star International Co., Ltd.]
  arch:         the architecture of the machine [x86_64, aarch64]
"""

fun howItWorks(): String = """
customfetch is designed with customizability in mind
here is how it works:
the variable "layout" is used for showing the infos as like as the user want, no limitation.
inside here there are 5 "tags": ${'$'}<> ${'$'}() ${'$'}{} ${'$'}[] ${'$'}%%

The Info tag ${'$'}<> lets you print the value of a member of a module.
e.g ${'$'}<user.name> will print the username, ${'$'}<os.kernel_version> will print the kernel version and so on.

There are variants who you only need the module name,
such as ${'$'}<ram> or ${'$'}<title>
All the modules and their members are listed in the `--list-modules` argument

The Bash command tag ${'$'}() will execute bash commands and print the output.
e.g ${'$'}(echo \"hello world\") will indeed echo out hello world.
you can even use pipes
e.g ${'$'}(echo \"hello world\" | cut -d' ' -f2) will only print world

The Conditional tag ${'$'}[] is used to display different outputs based on the comparison.
syntax MUST be ${'$'}[something,equalToSomethingElse,iftrue,ifalse]
note: putting spaces between commas, could change the expected result

Each part can have a tag or anything else.
e.g ${'$'}[${'$'}<user.name>,${'$'}(echo ${'$'}USER),the name is correct,the name is NOT correct]

This is useful when on some terminal or WM the detection can be different than others
Or maybe even on holidays for printing special texts
e.g ${'$'}[${'$'}(date +%d-%m),25-12,${'$'}{red}Happy ${'$'}{white}Christmas!,]

The Color tag ${'$'}{} is used for printing the text of a certain color.
e.g "${'$'}{red}hello world" will indeed print "hello world" in red (or the color you set in the variable)
The colors can be predefined such as: black, red, green, blue, cyan, yellow, magenta, white.
They can be configured in the config file.

They can have hex codes colors (e.g "#5522dd").
You can apply special effects to colors by using the following symbols before the '#' in hex codes:

    Terminal and GUI app                  GUI app only
* b - for background color.     * o        - for overline
* u - to  underline the text    * a(value) - for fg alpha (either a plain integer between 1 and 65536 or a percentage value like `50%`)
* ! - for bold text             * L(value) - to  underline the text with a style (`none`, `single`, `double`, `low`, `error`)
* i - for italic text           * U(value) - for choosing the underline color (hexcode color)
* s - for strikethrough text    * B(value) - for bg color text (hexcode color)
                                * S(value) - for strikethrough color (hexcode color)
    Terminal Only               * O(value) - for overline color (hexcode color)
* l - for blinking text         * A(value) - for bg alpha (either a plain integer between 1 and 65536 or a percentage value like `50%`)
                                * w(value) - for specify font weight (`ultralight`, `light`, `normal`, `bold`, `ultrabold`, `heavy`, or a numeric weight)

Alternatively, ANSI escape codes can be used, e.g ${'$'}{\e[1;32 m} or ${'$'}{\e[38;2;34;255;11m}.

To reset colors, use ${'$'}{0} for a normal reset or ${'$'}{1} for a bold reset.

To use the colors that the ascii art logo uses, use ${'$'}{auto} for getting the 1st color, ${'$'}{auto4} for the 4th one and so on.
If you're using the GUI app and wants to display a custom source that's an image, all the auto colors will be the same colors as the distro ones

The Percentage tag ${'$'}%% is used for displaying the percentage between 2 numbers.\
It **Must** contain a comma for separating the 2. They can be either be taken from a tag or it put yourself.\
For example: ${'$'}%50,100%
For inverting colors of bad and great (red and green), before the first '%' put '!'
without quotes ofc

################################################################
# Little FAQ
# Q: Why when I use & or < in the config or ASCII art, it won't work on the GUI app?
# A: replace "<" with "\\<" in the config, or "\<" in the ascii art. Same goes for &
#    It won't affect the printing in terminal
#
# Q: I want to use `cbonsai` as ASCII art, how do I use it?
# A: First off, create a text file and there put only `${'$'}(!cbonsai -p)`
#    Save the file and use `-s "/path/to/text/file"`.
#    Use `--offset` (`-o`) for aligning and put it under the bonsai.
#
#    Read the manual customfetch.1 for more infos with ${'$'}() tag
#
# Q: Can I use recursive tags?
# A: If "${'$'}<disk(${'$'}<disk(${'$'}[1,1,${'$'}(echo -n ${'$'}<disk(/).mountdir>),23]).mountdir>)>" works,
#    Then I guess yeah
################################################################
"""

const val AUTOCONFIG = """[config]

# For more information on how customfetch works and the layout,
# Read either:
# * -w or --how-it-works
# * the manual customfetch.1
# * if on the android app, click the button "how it works" during widget configuration
layout = [
    "${'$'}<title>",
    "${'$'}<title_sep>",
    "${'$'}{auto}OS: ${'$'}<os.name> ${'$'}<system.arch>",
    "${'$'}{auto}Host: ${'$'}<system.host>",
    "${'$'}{auto}Kernel: ${'$'}<os.kernel>",
    "${'$'}{auto}Uptime: ${'$'}<os.uptime>",
    "${'$'}<auto.disk>",
    "${'$'}{auto}Swap: ${'$'}<swap>",
    "${'$'}{auto}CPU: ${'$'}<cpu>",
    "${'$'}{auto}GPU: ${'$'}<gpu>",
    "${'$'}{auto}RAM: ${'$'}<ram>",
    "",
    "${'$'}<colors>", # normal colors
    "${'$'}<colors_light>" # light colors
]

# display ascii-art or image/gif (GUI only) near layout
# put "os" for displaying the OS ascii-art
# or the "/path/to/file" for displaying custom files
# or "off" for disabling ascii-art or image displaying
source-path = "os"

# Path to where we'll take all the distros/OSs ascii arts.
# note: it MUST contain an "ascii" subdirectory
data-dir = "/data/user/0/org.toni.customfetch_android/files"

# The type of ASCII art to apply ("small", "old").
# Basically will add "_<type>" to the logo filename.
# It will return the regular linux ascii art if it doesn't exist.
# Leave empty it for regular.
ascii-logo-type = "small"

# A char (or string) to use in ${'$'}<title_sep>
title-sep = "-"

# A separator (or string) that when encountered, will automatically
# reset color, aka. automatically add ${'$'}{0} (only in layout)
# Make it empty for disabling
sep-reset = ":"

# Should we reset color after or before the separator?
# true  = after  ("test ->${'$'}{0} ")
# false = before ("test ${'$'}{0}-> ")
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
wrap-lines = true

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

# ${'$'}<auto.disk> config
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
fmt = "${'$'}{auto}Disk (%1): ${'$'}<disk(%1)>"

# Only print disks that matches the description
# of the following types:
# regular   = Regular disks (internel M.2 SSD, ...) (won't be specified)
# removable = External disks (USB, SATA, ...)
# read-only = Disks with read-only filesystems
# hidden    = Disks that are not really mounted by the user
display-types = ["regular", "removable", "read-only"]

# In some OSes such as NixOS or Android, there might be some directories that are bind mounted.
# Bind mounted directories create an additional view of an existing directory,
# and `statfs()` on the mount point will return the filesystem statistics of the original directory.
show-duplicated = false

# ${'$'}<os.uptime> config
[os.uptime]
# how to display the name of the uptime
# e.g: hours = "hrs" -> "Uptime: 3hrs"
days  = " days"
hours = " hours"
mins  = " mins"
secs  = " seconds"

# ${'$'}<os.pkgs> config
[os.pkgs]
# Ordered list of which packages installed count should be displayed in ${'$'}<os.pkgs>
# remember to not enter the same name twice, else the world will finish
# Choices: pacman, flatpak, dpkg, apk
#
# Pro-tip: if your package manager isn't listed here, yet,
# use the bash command tag in the layout
# e.g "Packages: ${'$'}(pacman -Q | wc -l) (pacman)"
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

"""