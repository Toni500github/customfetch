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

#include <dlfcn.h>
#include <getopt.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "config.hpp"
#include "display.hpp"
#include "fmt/ranges.h"
#include "gui.hpp"
#include "platform.hpp"
#include "query.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

#include "core-modules.hh"

#if (!__has_include("version.h"))
# error "version.h not found, please generate it with ./scripts/generateVersion.sh"
#else
# include "version.h"
#endif

// clang-format off
// https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
// because "--opt-arg arg" won't work
// but "--opt-arg=arg" will
#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))
// clang-format on

using namespace std::string_view_literals;

// Print the version and some other infos, then exit successfully
static void version()
{
    std::string version{ fmt::format(
        "customfetch {} built from branch {} at {} commit {} ({}).\n"
        "Date: {}\n"
        "Tag: {}",
        VERSION, GIT_BRANCH, GIT_DIRTY, GIT_COMMIT_HASH, GIT_COMMIT_MESSAGE, GIT_COMMIT_DATE, GIT_TAG) };

#if !(USE_DCONF)
    version += "\n\nNO flags were set\n";
#else
    version += "\n\nset flags:\n";
#if USE_DCONF
    version += "USE_DCONF\n";
#endif
#endif

    fmt::print("{}\n", version);

    // if only everyone would not return error when querying the program version :(
    std::exit(EXIT_SUCCESS);
}

// Print the args help menu, then exit with code depending if it's from invalid or -h arg
static void help(bool invalid_opt = false)
{
    constexpr std::string_view help(
R"(Usage: customfetch [OPTIONS]...  
A command-line, GUI, and Android widget system information tool (like neofetch) focused on customizability and performance.

NOTE: Boolean flags [<BOOL>] accept: "true", 1, "enable", or empty. Any other value is treated as false.

GENERAL OPTIONS:
    -h, --help                  Print this help menu.
    -V, --version               Print version and Git branch info.
    -C, --config <PATH>         Path to the config file (default: ~/.config/customfetch.conf).

    --gen-config [<PATH>]       Generate default config file. If PATH is omitted, saves to default location.
                                Prompts before overwriting.

LOGO OPTIONS:
    -n, --no-logo               Disable logo display.
    -L, --logo-only             Print only the logo (skip info layout).
    -s, --source-path <PATH>    Path to custom ASCII art/image file.
    
    -a, --ascii-logo-type <TYPE>
                                Type of ASCII art (typically "small", "old", or empty for default).
                                Example: "--ascii-logo-type small" looks for "logo_small.txt".

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
    --wrap-lines=[<BOOL>]       Enable line wrapping (default: false).
    --title-sep <STRING>        String to use for $<title_sep> (default: "-").
    --sep-reset <STRING>        String that resets color (default: ":").
    --sep-reset-after=[<BOOL>]  Reset color after (default) or before 'sep-reset'.

GUI/TERMINAL OPTIONS:
    -f, --font <STRING>         GUI font (format: "FAMILY STYLE SIZE", e.g., "Liberation Mono Normal 12").
    -i, --image-backend <NAME>  (Experimental) Terminal image backend ("kitty" or "viu").
    --bg-image <PATH>           GUI background image path ("disable" to turn off).

ADVANCED/CONFIG:
    -O, --override <STRING>     Override a config value (non-array). Syntax: "name=value" (no spaces around "=").
                                Example: "auto.disk.fmt='Disk(%1): %6'".
                                Note: Names without dots (e.g., "sep-reset-after") gets mapped to "config.<name>".

    --color <STRING>            Replace a color globally. Syntax: "name=hex" (no spaces around "=").
                                Example: "--color magenta=#FF00FF".

    --disallow-command-tag      Do not allow command tags $() to be executed in the config or -m args.
                                This is a safety measure for preventing malicious code to be executed because you didn't want to check the config first.

INFORMATIONAL:
    -l, --list-modules          List all available info tag modules (e.g., $<cpu> or $<os.name>).
    -w, --how-it-works          Explain layout variables and customization.
    --list-logos                List available ASCII logos in --data-dir.

LIVE MODE:
    --loop-ms <NUM>             Run in live mode, updating every <NUM> milliseconds (min: 50).
                                Use 0 to disable (default).

EXAMPLES:
    1. Minimal output with default logo:
       customfetch --no-color

    2. Custom distro logo with live updates:
       customfetch --distro "arch" --loop-ms 1000

    3. Override layout and colors:
       customfetch -m "${auto}OS: $<os.name>" --color "magenta=#FF00FF"

For details, see `man customfetch` or run `--how-it-works`.
)");

    fmt::print("{}\n", help);
    std::exit(invalid_opt);
}

// Print all info modules you can put in $<>, then exit successfully
static void modules_list()
{
    constexpr std::string_view list(R"(
--------------------------------------------------------[ MODULE ONLY ]------------------------------------------------------------------------
Should be used as like as $<module>
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
  user and hostname colored with ${auto2} [toni@arch2]

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

Should be used as like as $<module.member>
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

)");

    fmt::print("{}", list);
    fmt::print("\n");
    std::exit(EXIT_SUCCESS);
}

// Print how customfetch works, then exit successfully
static void explain_how_this_works()
{
    constexpr std::string_view str(
R"(
customfetch is designed for maximum customizability, allowing users to display system information exactly how they want it.
The layout and logo is controlled through special tags that can output system info, execute commands, apply conditional logic, add colors, and calculate percentages with some colors.

Tag References:
1. Information Tag ($<>)
    Retrieves system information from modules.

    Syntax: $<module.member> or $<module>

    Examples:
    - $<user.name>  # Displays login username
    - $<os.kernel>  # Shows kernel version
    - $<ram>        # Shows formatted RAM usage

    Use `--list-modules` to see all available modules and members.

2. Bash Command Tag ($())
    Executes shell commands and outputs the result.

    Syntax: $(command)

    Examples:
    - $(echo "hello")             # Outputs: hello
    - $(date +%F)                 # Shows current date
    - $(uname -r | cut -d'-' -f1) # Shows kernel version number only

    Supports full shell syntax including pipes and redirection.

3. Conditional Tag ($[])
    Displays different outputs based on conditions.
    
    Syntax: $[condition,comparison,true_output,false_output]
    
    Examples:
    - $[$<user.name>,toni,Welcome back!,Access denied]
    - $[$(date +%m-%d),12-25,Merry Christmas!,]
    - $[$<os.name_id>,arch,${green}I use arch btw,${red}Non-arch user]

4. Color Tag (${})
    Applies colors and text formatting.
    
    Basic syntax: ${color} or ${modifiers#RRGGBB}

    Color options:
    - Named colors from config
    - Hex colors: ${#ff00cc}
    - Special colors: ${auto} (uses logo colors)
    - Reset styles: ${0} (normal), ${1} (bold reset)

    Formatting modifiers (prefix before color):
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
    Cross-platform:
        ${\e[1;33m}Bold yellow
        ${b#222222}${white}White on gray
        ${auto3}The 3rd logo color

    Notes:
    - customfetch will try to convert ANSI escape codes to GUI app equivalent
    - customfetch will ignore GUI-specific modifiers on terminal.
    - if you're using the GUI app and want to display a custom logo that's an image, all the auto colors will be the same colors as the distro ones.

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
A: Escape these characters with \\ (e.g replace "<" with "\\<" from both config and ASCII art):
   This doesn't affect terminal output.

Q: How can I use cbonsai as ASCII art?
A: 1. Create a text file containing: $(!cbonsai -p)
   2. Use: customfetch -s "/path/to/file.txt"
   3. Adjust offset for proper alignment

Q: Does customfetch support nested tags?
A: Yes! Complex nesting is supported, for example:
   $<disk($<disk($[1,1,$(echo -n $<disk(/).mountdir>),23]).mountdir>)>
)");

    fmt::print("{}", str);
    fmt::print("\n");
    std::exit(EXIT_SUCCESS);
}

// Print a sorted list of ascii logos you can use at a "data-dir"
// @param data_dir The data directory
static void list_logos(const std::string& data_dir)
{
    debug("data = {}", data_dir);
    if (access(data_dir.c_str(), F_OK) != 0)
        die("failed to access data directory '{}'", data_dir);

    std::vector<std::string> list;
    for (const auto& logo : std::filesystem::directory_iterator{ data_dir })
    {
        if (logo.is_regular_file())
            list.push_back(logo.path().stem());
    }

    std::sort(list.begin(), list.end());

    fmt::print("{}\n", fmt::join(list, "\n"));
    std::exit(EXIT_SUCCESS);
}

// Return true if optarg says something true
static bool str_to_bool(const std::string_view str)
{
    return (str == "true" || str == "1" || str == "enable");
}

// clang-format off
// parseargs() but only for parsing the user config path trough args
// and so we can directly construct Config
static std::filesystem::path parse_config_path(int argc, char* argv[], const std::filesystem::path &configDir)
{
    int opt = 0;
    int option_index = 0;
    opterr = 0;
    const char *optstring = "-C:";
    static const struct option opts[] = {
        {"config", required_argument, 0, 'C'},
        {0,0,0,0}
    };

    while ((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1)
    {
        switch (opt)
        {
            // skip errors or anything else
            case 0:
            case '?':
                break;

            case 'C':
                if (!std::filesystem::exists(optarg))
                    die(_("config file '{}' doesn't exist"), optarg);
                return optarg;
        }
    }

    return configDir / "config.toml";
}

static bool parseargs(int argc, char* argv[], Config& config, const std::filesystem::path &configFile)
{
    int opt = 0;
    int option_index = 0;
    opterr = 1; // re-enable since before we disabled for "invalid option" error
    const char *optstring = "-VhwnLlNa:f:o:C:O:i:d:D:p:s:m:";
    static const struct option opts[] = {
        {"version",          no_argument,       0, 'V'},
        {"help",             no_argument,       0, 'h'},
        {"list-modules",     no_argument,       0, 'l'},
        {"how-it-works",     no_argument,       0, 'w'},
        {"logo-only",        no_argument,       0, 'L'},
        {"no-logo",          no_argument,       0, 'n'},
        {"no-color",         no_argument,       0, 'N'},
        {"ascii-logo-type",  required_argument, 0, 'a'},
        {"offset",           required_argument, 0, 'o'},
        {"override",         required_argument, 0, 'O'},
        {"font",             required_argument, 0, 'f'},
        {"config",           required_argument, 0, 'C'},
        {"layout-line",      required_argument, 0, 'm'},
        {"logo-position",    required_argument, 0, 'p'},
        {"data-dir",         required_argument, 0, 'D'},
        {"distro",           required_argument, 0, 'd'},
        {"source-path",      required_argument, 0, 's'},
        {"image-backend",    required_argument, 0, 'i'},

        {"list-logos",           no_argument,       0, "list-logos"_fnv1a16},
        {"disallow-command-tag", no_argument,       0, "disallow-command-tag"_fnv1a16},
        {"sep-reset-after",      optional_argument, 0, "sep-reset-after"_fnv1a16},
        {"debug",                optional_argument, 0, "debug"_fnv1a16},
        {"wrap-lines",           optional_argument, 0, "wrap-lines"_fnv1a16},
        {"gen-config",           optional_argument, 0, "gen-config"_fnv1a16},
        {"sep-reset",            required_argument, 0, "sep-reset"_fnv1a16},
        {"title-sep",            required_argument, 0, "title-sep"_fnv1a16},
        {"logo-padding-top",     required_argument, 0, "logo-padding-top"_fnv1a16},
        {"logo-padding-left",    required_argument, 0, "logo-padding-left"_fnv1a16},
        {"layout-padding-top",   required_argument, 0, "layout-padding-top"_fnv1a16},
        {"loop-ms",              required_argument, 0, "loop-ms"_fnv1a16},
        {"bg-image",             required_argument, 0, "bg-image"_fnv1a16},
        {"color",                required_argument, 0, "color"_fnv1a16},

        {0,0,0,0}
    };

    /* parse operation */
    optind = 0;
    while ((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1)
    {
        switch (opt)
        {
            case 0:
                break;
            case '?':
                help(EXIT_FAILURE); break;

            case 'V':
                version(); break;
            case 'h':
                help(); break;
            case 'l':
                modules_list(); break;
            case 'w':
                explain_how_this_works(); break;
            case "list-logos"_fnv1a16:
                list_logos(config.data_dir+"/ascii"); break;
            case 'f':
                config.overrides["gui.font"] = {.value_type = STR, .string_value = optarg}; break;
            case 'o':
                config.overrides["config.offset"] = {.value_type = STR, .string_value = optarg}; break;
            case 'C': // we have already did it in parse_config_path()
                break;
            case 'D':
                config.overrides["config.data-dir"] = {.value_type = STR, .string_value = optarg}; break;
            case 'd':
                config.args_custom_distro = str_tolower(optarg); break;
            case 'm':
                config.args_layout.push_back(optarg); break;
            case 'p':
                config.overrides["config.logo-position"] = {.value_type = STR, .string_value = optarg}; break;
            case 's':
                config.overrides["config.source-path"] = {.value_type = STR, .string_value = optarg}; break;
            case 'i':
                config.args_image_backend = optarg; break;
            case 'O':
                config.overrideOption(optarg); break;
            case 'N':
                config.args_disable_colors = true; break;
            case 'a':
                config.overrides["config.ascii-logo-type"] = {.value_type = STR, .string_value = optarg}; break;
            case 'n':
                config.args_disable_source = true; break;
            case 'L':
                config.args_print_logo_only = true; break;

            case "disallow-command-tag"_fnv1a16:
                config.args_disallow_commands = true; break;

            case "logo-padding-top"_fnv1a16:
                config.overrides["config.logo-padding-top"] = {.value_type = INT, .int_value = std::stoi(optarg)}; break;

            case "logo-padding-left"_fnv1a16:
                config.overrides["config.logo-padding-left"] = {.value_type = INT, .int_value = std::stoi(optarg)}; break;

            case "layout-padding-top"_fnv1a16:
                config.overrides["config.layout-padding-top"] = {.value_type = INT, .int_value = std::stoi(optarg)}; break;

            case "loop-ms"_fnv1a16:
                config.loop_ms = std::stoul(optarg); break;

            case "debug"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    debug_print = str_to_bool(optarg);
                else
                    debug_print = true;
                break;

            case "bg-image"_fnv1a16:
                config.overrides["gui.bg-image"] = {.value_type = STR, .string_value = optarg}; break;

            case "wrap-lines"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.overrides["config.wrap-lines"] = {.value_type = BOOL, .bool_value = str_to_bool(optarg)};
                else
                    config.overrides["config.wrap-lines"] = {.value_type = BOOL, .bool_value = true};
                break;

            case "color"_fnv1a16:
                config.addAliasColors(optarg); break;

            case "gen-config"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.generateConfig(optarg);
                else
                    config.generateConfig(configFile);
                exit(EXIT_SUCCESS);

            case "sep-reset"_fnv1a16:
                config.overrides["config.sep-reset"] = {.value_type = STR, .string_value = optarg}; break;

            case "title-sep"_fnv1a16:
                config.overrides["config.title-sep"] = {.value_type = STR, .string_value = optarg}; break;

            case "sep-reset-after"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.overrides["config.sep-reset-after"] = {.value_type = BOOL, .bool_value = str_to_bool(optarg)};
                else
                    config.overrides["config.sep-reset-after"] = {.value_type = BOOL, .bool_value = true};
                break;

            default:
                return false;
        }
    }

    return true;
}

static void enable_cursor()
{
    fmt::print("\x1B[?25h\x1B[?7h");
}

/** Sets up gettext localization. Safe to call multiple times.
 */
/* Inspired by the monotone function localize_monotone. */
// taken from pacman
static void localize(void)
{
#if ENABLE_NLS && !CF_MACOS
    static bool init = false;
    if (!init)
    {
        setlocale(LC_ALL, "");
        bindtextdomain("customfetch", LOCALEDIR);
        textdomain("customfetch");
        init = true;
    }
#endif
}

void core_plugins_start(void *handle);
int main(int argc, char *argv[])
{

#ifdef VENDOR_TEST
    // test
    fmt::println("=== VENDOR TEST! ===");

    fmt::println("Intel: {}", binarySearchPCIArray("8086"));
    fmt::println("AMD: {}", binarySearchPCIArray("1002"));
    fmt::println("NVIDIA: {}", binarySearchPCIArray("10de"));
#endif

#ifdef DEVICE_TEST
    // test
    fmt::println("=== DEVICE TEST! ===");

    fmt::println("an Intel iGPU: {}", binarySearchPCIArray("8086", "0f31"));
    fmt::println("RX 7700 XT: {}", binarySearchPCIArray("1002", "747e"));
    fmt::println("GTX 1650: {}", binarySearchPCIArray("10de", "1f0a"));
    fmt::println("?: {}", binarySearchPCIArray("1414", "0006"));
#endif

    // clang-format on
    colors_t colors;

    const std::filesystem::path configDir  = getConfigDir();
    const std::filesystem::path configFile = parse_config_path(argc, argv, configDir);

    localize();

    Config config(configFile, configDir);
    if (!parseargs(argc, argv, config, configFile))
        return 1;
    config.loadConfigFile(configFile, colors);

    void* cufetch_handle = LOAD_LIBRARY("libcufetch.so")
    if (!cufetch_handle)
        die("Failed to load {}", dlerror());

    /* TODO(burntranch): track each library and unload them. */
    core_plugins_start(cufetch_handle);
    const std::filesystem::path modDir = configDir / "mods";
    std::filesystem::create_directories(modDir);
    for (const auto& entry : std::filesystem::directory_iterator{modDir})
    {
        debug("loading mod at {}!", entry.path().string());

        void *handle = LOAD_LIBRARY(std::filesystem::absolute(entry.path()).c_str());
        if (!handle)
        {
            // dlerror() is pretty formatted
            warn("Failed to load mod {}", dlerror());
            dlerror();
            continue;
        }

        LOAD_LIB_SYMBOL(handle, void, start, void*)

        start(cufetch_handle);
    }

    LOAD_LIB_SYMBOL(cufetch_handle, const std::vector<module_t>&, cfGetModules)

    const std::vector<module_t>& modules = cfGetModules();
    moduleMap_t                  moduleMap;

    debug("modules count: {}", modules.size());
    for (const module_t& module : modules)
    {
        debug("adding module {} (has handler: {})", module.name, module.handler != NULL);
        if (!module.handler)
            continue;

        moduleMap.emplace(module.name, module);
    }

    is_live_mode = (config.loop_ms > 50);

    if (config.source_path.empty() || config.source_path == "off")
        config.args_disable_source = true;

    config.m_display_distro = (config.source_path == "os");

    std::string path = config.m_display_distro ? Display::detect_distro(config) : config.source_path;

    if (!config.ascii_logo_type.empty() && config.m_display_distro)
    {
        std::string  logo_type_path{ path };
        const size_t pos = path.rfind('.');

        if (pos != std::string::npos)
            logo_type_path.insert(pos, "_" + config.ascii_logo_type);
        else
            logo_type_path += "_" + config.ascii_logo_type;

        if (std::filesystem::exists(logo_type_path))
            path = logo_type_path;
    }

    debug("{} path = {}", __PRETTY_FUNCTION__, path);

    if (!std::filesystem::exists(path) && !config.args_disable_source)
    {
        path                   = std::filesystem::temp_directory_path() / "customfetch_ascii_logo-XXXXXX";
        Display::ascii_logo_fd = mkstemp(path.data());
        if (Display::ascii_logo_fd < 0)
            die("Failed to create temp path at {}: {}", path, strerror(errno));
        write(Display::ascii_logo_fd, ascii_logo.data(), ascii_logo.size());
    }

#if GUI_APP
    const auto& app = Gtk::Application::create("org.toni.customfetch");
    GUI::Window window(config, colors, path, moduleMap);
    return app->run(window);
#endif  // GUI_APP

    if (!config.wrap_lines)
    {
        // https://en.cppreference.com/w/c/program/exit
        // if something goes wrong like a segfault, then re-enable the cursor again
        std::atexit(enable_cursor);

        // hide cursor and disable line wrapping
        fmt::print("\x1B[?25l\x1B[?7l");
    }

    if (is_live_mode)
    {
        const std::chrono::milliseconds sleep_ms{ config.loop_ms };

        while (true)
        {
            // clear screen and go to position 0, 0
            write(STDOUT_FILENO, "\33[H\33[2J", 7);
            fmt::print("\033[0;0H");

            Display::display(Display::render(config, colors, false, path, moduleMap));
            std::this_thread::sleep_for(sleep_ms);
        }
    }
    else
    {
        Display::display(Display::render(config, colors, false, path, moduleMap));
    }

    // enable both of them again
    if (!config.wrap_lines)
        enable_cursor();

#if CF_LINUX
    if (os_release) fclose(os_release);
    if (cpuinfo) fclose(cpuinfo);
    if (meminfo) fclose(meminfo);
#endif

    UNLOAD_LIBRARY(cufetch_handle);

    return 0;
}
