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

#include <getopt.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>

#include "config.hpp"
#include "display.hpp"
#include "fmt/ranges.h"
#include "gui.hpp"
#include "query.hpp"
#include "platform.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

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
    std::string version{ fmt::format("customfetch {} built from branch {} at {} commit {} ({}).\n"
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
A command-line, GUI app, android widget system information tool (or neofetch like program), which its focus point is customizability and performance.

NOTE: Arguments that takes [<bool>] values, the values can be either: "true", 1, "enable" or leave it empty. Any other value will be treated as false.

    -n, --no-logo               Do not display the logo
    -N, --no-color              Do not output and parse colors. Useful for stdout or pipe operations
    -L, --logo-only             Print only the logo
    -s, --source-path <path>    Path to the ascii art or image file to display
    -C, --config <path>         Path to the config file to use
    -a, --ascii-logo-type <type>
                                The type of ASCII art to apply ("small" or "old").
                                Basically will add "_<type>" to the logo filename.
                                It will return the regular OS ascii art if it doesn't exist.
                                Make it empty for regular.

    -D, --data-dir <path>       Path to the data dir where we'll taking the distros ascii arts (must contain subdirectory called "ascii")
    -d, --distro <name>         Print a custom logo from the given `data-dir` (must be the same name, uppercase or lowercase, e.g "windows 11" or "ArCh")
    -f, --font <name>           The font to be used in the GUI app (syntax must be "[FAMILY-LIST] [STYLE-OPTIONS] [SIZE]" without the double quotes and [])
                                An example: [Liberation Mono] [Normal] [12], which can be "Liberation Mono Normal 12"

    -i, --image-backend	<name>  (EXPERIMENTAL) Image backend tool for displaying images in terminal.
                                Right now only 'kitty' and 'viu' are supported
                                It's recommended to use the GUI app for the moment if something doesn't work

    -m, --layout-line <string>  Will replace the config layout, with a layout you specify in the arguments
                                Example: `customfetch -m "${auto}OS: $<os.name>" -m "${auto}CPU: $<cpu.cpu>"`
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
    --title-sep <string>        A character (or string) to use in $<builtin.title_sep>
    --sep-reset <string>        A character (or string) that when encountered, will automatically reset color
    --sep-reset-after [<bool>]  Reset color either before or after 'sep-reset'
    --gen-config [<path>]       Generate default config file to config folder (if path, it will generate to the path)
                                Will ask for confirmation if file exists already

    --color <string>            Replace instances of a color with another value.
                                Syntax MUST be "name=value" with no space between "=", example: --color "foo=#444333".
                                Thus replaces any instance of foo with #444333. Can be done with multiple colors separately.

Read the manual "customfetch.1" or --how-it-works for more infos about customfetch and how it works
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

    fmt::print("{}\n", list);
    std::exit(EXIT_SUCCESS);
}

// Print how customfetch works, then exit successfully
static void explain_how_this_works()
{
    constexpr std::string_view str(
R"(
customfetch is designed with customizability in mind
here is how it works:
the variable "layout" is used for showing the infos as like as the user want, no limitation.
inside here there are 5 "tags": $<> $() ${} $[] $%%

The Info tag $<> lets you print the value of a member of a module.
e.g $<user.name> will print the username, $<os.kernel_version> will print the kernel version and so on.

There are variants who you only need the module name,
such as $<ram> or $<title>
All the modules and their members are listed in the `--list-modules` argument

The Bash command tag $() will execute bash commands and print the output.
e.g $(echo \"hello world\") will indeed echo out hello world.
you can even use pipes
e.g $(echo \"hello world\" | cut -d' ' -f2) will only print world

The Conditional tag $[] is used to display different outputs based on the comparison.
syntax MUST be $[something,equalToSomethingElse,iftrue,ifalse]
note: putting spaces between commas, could change the expected result

Each part can have a tag or anything else.
e.g $[$<user.name>,$(echo $USER),the name is correct,the name is NOT correct]

This is useful when on some terminal or WM the detection can be different than others
Or maybe even on holidays for printing special texts
e.g $[$(date +%d-%m),25-12,${red}Happy ${white}Christmas!,]

The Color tag ${} is used for printing the text of a certain color.
e.g "${red}hello world" will indeed print "hello world" in red (or the color you set in the variable)
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

Alternatively, ANSI escape codes can be used, e.g ${\e[1;32m} or ${\e[38;2;34;255;11m}.

To reset colors, use ${0} for a normal reset or ${1} for a bold reset.

To use the colors that the ascii art logo uses, use ${auto} for getting the 1st color, ${auto4} for the 4th one and so on.
If you're using the GUI app and wants to display a custom source that's an image, all the auto colors will be the same colors as the distro ones

The Percentage tag $%% is used for displaying the percentage between 2 numbers.\
It **Must** contain a comma for separating the 2. They can be either be taken from a tag or it put yourself.\
For example: $%50,100%
For inverting colors of bad and great (red and green), before the first '%' put '!'
without quotes ofc

################################################################
# Little FAQ
# Q: Why when I use & or < in the config or ASCII art, it won't work on the GUI app?
# A: replace "<" with "\\<" in the config, or "\<" in the ascii art. Same goes for &
#    It won't affect the printing in terminal
#
# Q: I want to use `cbonsai` as ASCII art, how do I use it?
# A: First off, create a text file and there put only `$(!cbonsai -p)`
#    Save the file and use `-s "/path/to/text/file"`.
#    Use `--offset` (`-o`) for aligning and put it under the bonsai.
#
#    Read the manual customfetch.1 for more infos with $() tag
#
# Q: Can I use recursive tags?
# A: If "$<disk($<disk($[1,1,$(echo -n $<disk(/).mountdir>),23]).mountdir>)>" works,
#    Then I guess yeah
################################################################

)");

    fmt::print("{}\n", str);
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
    for (const auto& logo : std::filesystem::directory_iterator{data_dir})
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
static std::string parse_config_path(int argc, char* argv[], const std::string& configDir)
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

    return configDir + "/config.toml";
}

static bool parseargs(int argc, char* argv[], Config& config, const std::string_view configFile)
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

        {"list-logos",         no_argument,       0, "list-logos"_fnv1a16},
        {"sep-reset-after",    optional_argument, 0, "sep-reset-after"_fnv1a16},
        {"debug",              optional_argument, 0, "debug"_fnv1a16},
        {"wrap-lines",         optional_argument, 0, "wrap-lines"_fnv1a16},
        {"gen-config",         optional_argument, 0, "gen-config"_fnv1a16},
        {"sep-reset",          required_argument, 0, "sep-reset"_fnv1a16},
        {"title-sep",          required_argument, 0, "title-sep"_fnv1a16},
        {"logo-padding-top",   required_argument, 0, "logo-padding-top"_fnv1a16},
        {"logo-padding-left",  required_argument, 0, "logo-padding-left"_fnv1a16},
        {"layout-padding-top", required_argument, 0, "layout-padding-top"_fnv1a16},
        {"loop-ms",            required_argument, 0, "loop-ms"_fnv1a16},
        {"bg-image",           required_argument, 0, "bg-image"_fnv1a16},
        {"color",              required_argument, 0, "color"_fnv1a16},

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

    const std::string& configDir  = getConfigDir();
    const std::string& configFile = parse_config_path(argc, argv, configDir);

    localize();

    Config config(configFile, configDir, colors);
    if (!parseargs(argc, argv, config, configFile))
        return 1;
    config.loadConfigFile(configFile, colors);

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
        path = std::filesystem::temp_directory_path() / "customfetch_ascii_logo-XXXXXX";
        Display::ascii_logo_fd = mkstemp(path.data());
        if (Display::ascii_logo_fd < 0)
            die("Failed to create temp path at {}: {}", path, strerror(errno));
        write(Display::ascii_logo_fd, ascii_logo.data(), ascii_logo.size());
    }

#if GUI_APP
    config.gui = true;
    const auto& app = Gtk::Application::create("org.toni.customfetch");
    GUI::Window window(config, colors, path);
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
        const std::chrono::milliseconds sleep_ms {config.loop_ms};

        while (true)
        {
            // clear screen and go to position 0, 0
            write(STDOUT_FILENO, "\33[H\33[2J", 7);
            fmt::print("\033[0;0H");

            Display::display(Display::render(config, colors, false, path));
            std::this_thread::sleep_for(sleep_ms);
        }
    }
    else
    {
        Display::display(Display::render(config, colors, false, path));
    }

    // enable both of them again
    if (!config.wrap_lines)
        enable_cursor();

    return 0;
}
