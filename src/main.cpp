/*
 * Copyright 2024 Toni500git
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

#include <cstdlib>
#include <filesystem>

#include "config.hpp"
#include "display.hpp"
#include "gui.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

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

static void version()
{
    fmt::println("customfetch {} branch {}", VERSION, BRANCH);

#ifdef GUI_MODE
    fmt::print("GUI mode enabled\n\n");
#else
    fmt::print("GUI mode IS NOT enabled\n\n");
#endif

#if !(USE_DCONF)
    fmt::println("NO flags were set");
#else
    fmt::println("set flags:");
#if USE_DCONF
    fmt::println("USE_DCONF");
#endif
#endif

    // if only everyone would not return error when querying the program version :(
    std::exit(EXIT_SUCCESS);
}

static void help(bool invalid_opt = false)
{
    fmt::println("Usage: customfetch [OPTIONS]...");
    fmt::println(R"(
A command-line system information tool (or neofetch like program), which its focus point is customizability and perfomance

    -n, --no-display		Do not display the logo
    -N, --no-color              Do not output and parse colors. Useful for stdout or pipe operations
    --enable-colors             Inverse of --no-color
    -s, --source-path <path>	Path to the ascii art or image file to display
    -C, --config <path>		Path to the config file to use
    -a, --ascii-logo-type [<name>]
                                The type of ASCII art to apply ("small" or "old").
                                Basically will add "_<type>" to the logo filename.
                                It will return the regular linux ascii art if it doesn't exist.
                                Leave it empty for regular.

    -D, --data-dir <path>       Path to the data dir where we'll taking the distros ascii arts (must contain subdirectory called "ascii")
    -d, --distro <name>         Print a custom distro logo (must be the same name, uppercase or lowercase, e.g "windows 11" or "ArCh")
    -f, --font <name>           The font to be used in GUI mode (syntax must be "[FAMILY-LIST] [STYLE-OPTIONS] [SIZE]" without the double quotes and [])
                                An example: [Liberation Mono] [Normal] [12], which can be "Liberation Mono Normal 12"

    -i, --image-backend	<name>	(EXPERIMENTAL) Image backend tool for displaying images in terminal.
                                Right now only 'kitty' and 'viu' are supported
				It's recommended to use GUI mode for the moment if something doesn't work

    -m, --layout-line           Will replace the config layout, with a layout you specify in the arguments
				Example: `customfetch -m "${{auto}}OS: $<os.name>" -m "${{auto}}CPU: $<cpu.cpu>"`
				Will only print the logo (if not disabled), along side the parsed OS and CPU

    -g, --gui                   Use GUI mode instead of priting in the terminal (use --version to check if it was enabled)
    -p, --logo-position <value> Position of the logo ("top" or "left")
    -o, --offset <num>          Offset between the ascii art and the layout
    -l. --list-modules  	Print the list of the modules and its members
    -h, --help			Print this help menu
    -L, --logo-only             Print only the logo
    -V, --version		Print the version along with the git branch it was built

    --bg-image <path>           Path to image to be used in the background in GUI (put "disable" for disabling in the config)
    --wrap-lines [<0,1>]	Disable (0) or Enable (1) wrapping lines when printing in terminal
    --logo-padding-top	<num>	Padding of the logo from the top
    --logo-padding-left	<num>	Padding of the logo from the left
    --layout-padding-top <num>  Padding of the layout from the top
    --title-sep <string>        A char (or string) to use in $<builtin.title_sep>
    --sep-reset <string>        A separator (or string) that when ecountered, will automatically reset color
    --sep-reset-after [<num>]   Reset color either before of after 'sep-reset' (1 = after && 0 = before)
    --gen-config [<path>]       Generate default config file to config folder (if path, it will generate to the path)
                                Will ask for confirmation if file exists already

    --color <string>            Replace instances of a color with another value.
                                Syntax MUST be "name=value" with no space beetween "=", example: --color "foo=#444333".
				Thus replaces any instance of foo with #444333. Can be done with multiple colors separetly.

Read the manual "customfetch.1" or the autogenerated config file for more infos about customfetch and how it works
)"sv);
    std::exit(invalid_opt);
}

static void modules_list()
{
    fmt::println(R"(
Syntax:
# maybe comments of the module
module
  member	: description [example of what it prints, maybe another]

Should be used in the config as like as $<module.member>
NOTE: there are modules such as "user.de_version" that may slow down customfetch because of querying things like the DE version
      customfetch is still fast tho :)

os
  name		: OS name (pretty name) [Ubuntu 22.04.4 LTS, Arch Linux]
  name_id	: OS name id [ubuntu, arch]
  kernel	: kernel name and version [Linux 6.9.3-zen1-1-zen]
  kernel_name	: kernel name [Linux]
  kernel_version: kernel version [6.9.3-zen1-1-zen]
  version_id	: OS version id [22.04.4, 20240101.0.204074]
  version_codename: OS version codename [jammy]
  pkgs		: the count of the installed packages by a package manager [1869 (pacman), 4 (flatpak)]
  uptime	: (auto) uptime of the system [36 mins, 3 hours, 23 days]
  uptime_secs	: uptime of the system in seconds (should be used along with others uptime_ members) [45]
  uptime_mins   : uptime of the system in minutes (should be used along with others uptime_ members) [12]
  uptime_hours  : uptime of the system in hours   (should be used along with others uptime_ members) [34]
  uptime_days	: uptime of the system in days    (should be used along with others uptime_ members) [2]
  hostname	: hostname of the OS [mymainPC]
  initsys_name	: Init system name [systemd]
  initsys_version: Init system version [256.5-1-arch]

user
  name		: name you are currently logged in (not real name) [toni69]
  shell		: login shell name and version [zsh 5.9]
  shell_name	: login shell [zsh]
  shell_path	: login shell (with path) [/bin/zsh]
  shell_version : login shell version (may be not correct) [5.9]
  de_name	: Desktop Enviroment current session name [Plasma]
  wm_name	: Window manager current session name [dwm, xfwm4]
  wm_version	: Window manager version (may not working correctly) [6.2, 4.18.0]
  terminal	: Terminal name and version [alacritty 0.13.2]
  terminal_name	: Terminal name [alacritty]
  terminal_version: Terminal version [0.13.2]

# with `symb` I mean a symbol to be used for the
# view of the color palette
builtin
  title     	: user and hostname colored with ${{auto2}} [toni@arch2]
  title_sep     : separator between the title and the system infos (with the title lenght) [--------]
  colors	: color palette with background spaces
  colors_light  : light color palette with background spaces
  colors_symbol(symb)	   : color palette with specific symbol
  colors_light_symbol(symb): light color palette with specific symbol

# this module is just for generic theme stuff
# such as indeed cursor
# because it is not GTK-Qt specific
theme
  cursor	: cursor name with its size (auto add the size if queried) [Bibata-Modern-Ice (16px)]
  cursor_name	: cursor name [Bibata-Modern-Ice]
  cursor_size	: cursor size [16]

# If USE_DCONF flag is set, then we're going to use
# dconf, else backing up to gsettings
theme-gsettings
  name          : gsettings theme name [Decay-Green]
  icons         : gsettings icons theme name [Papirus-Dark]
  font          : gsettings font theme name [Cantarell 10]
  cursor        : gsettings cursor name with its size (auto add the size if queried) [Bibata-Modern-Ice (16px)]
  cursor_name   : gsettings cursor name [Bibata-Modern-Ice]
  cursor_size   : gsettings cursor size [16]

# the N stands for the gtk version number to query
# so for example if you want to query the gtk3 theme name
# write it like "theme-gtk3.name"
# note: may be slow because of calling "gsettings" if couldn't read from configs.
#       Read theme-gsettings module comments
theme-gtkN
  name		: gtk theme name [Arc-Dark]
  icons		: gtk icons theme name [Qogir-Dark]
  font		: gtk font theme name [Noto Sans 10]

# basically as like as the "theme-gtkN" module above
# but with gtk{{2,3,4}} and auto format gkt version
# note: may be slow because of calling "gsettings" if couldn't read from configs.
# 	Read theme-gsettings module comments
theme-gtk-all
  name          : gtk theme name [Arc-Dark [GTK2/3/4]]
  icons         : gtk icons theme name [Papirus-Dark [GTK2/3], Qogir [GTK4]]
  font          : gtk font theme name [Hack Nerd Font 13 [GTK2], Noto Sans 10 [GTK3/4]]

# note: these members are auto displayed in from B to YB (depending if using SI byte unit or not(IEC)).
# they all (except those that has the same name as the module or that ends with "_perc")
# have variants from -B to -YB and -B to -YiB
# example: if you want to show your 512MiB of used RAM in GiB
# use the `used-GiB` variant (they don't print the unit tho)
ram
  ram           : used and total amount of RAM (auto) with used percentage [2.81 GiB / 15.88 GiB (5.34%)]
  used		: used amount of RAM (auto) [2.81 GiB]
  free		: available amount of RAM (auto) [10.46 GiB]
  total		: total amount of RAM (auto) [15.88 GiB]
  used_perc	: percentage of used amount of RAM in total [17.69%]
  free_perc	: percentage of available amount of RAM in total [82.31%]

# same comments as RAM (above)
swap
  swap          : used and total amount of the swapfile (auto) with used percentage [477.68 MiB / 512.00 MiB (88.45%)]
  free		: available amount of the swapfile (auto) [34.32 MiB]
  total		: total amount of the swapfile (auto) [512.00 MiB]
  used		: used amount of the swapfile (auto) [477.68 MiB]
  used_perc     : percentage of used amount of the swapfile in total [93.29%]
  free_perc     : percentage of available amount of the swapfile in total [6.71%]

# same comments as RAM (above)
# note: the module can have either a device path
#	or a filesystem path
#	e.g disk(/) or disk(/dev/sda5)
disk(/path/to/fs)
  disk		: used and total amount of disk space (auto) with type of filesystem and used percentage [379.83 GiB / 438.08 GiB (86.70%) - ext4]
  used          : used amount of disk space (auto) [360.02 GiB]
  free          : available amount of disk space (auto) [438.08 GiB]
  total         : total amount of disk space (auto) [100.08 GiB]
  used_perc     : percentage of used amount of the disk in total [82.18%]
  free_perc     : percentage of available amount of the disk in total [17.82%]
  fs            : type of filesystem [ext4]
  device	: path to device [/dev/sda5]
  mountdir	: path to the device mount point [/]

# usually people have 1 GPU in their PC,
# but if you got more than 1 and want to query it,
# you should call gpu module with a number, e.g gpu1 (default gpu0).
# Infos are gotten from `/sys/class/drm/` and on each cardN directory
gpu
  name		: GPU model name [GeForce GTX 1650]
  vendor	: GPU short vendor name [NVIDIA]
  vendor_long   : GPU vendor name [NVIDIA Corporation]

cpu
  cpu		: CPU model name with number of virtual proccessors and max freq [AMD Ryzen 5 5500 (12) @ 4.90 GHz]
  name		: CPU model name [AMD Ryzen 5 5500]
  nproc         : CPU number of virtual proccessors [12]
  freq_bios_limit: CPU freq (limited by bios, in GHz) [4.32]
  freq_cur	: CPU freq (current, in GHz) [3.42]
  freq_min	: CPU freq (mininum, in GHz) [2.45]
  freq_max	: CPU freq (maxinum, in GHz) [4.90]

system
  host		: Host (aka. Motherboard) model name with vendor and version [Micro-Star International Co., Ltd. PRO B550M-P GEN3 (MS-7D95) 1.0]
  host_name	: Host (aka. Motherboard) model name [PRO B550M-P GEN3 (MS-7D95)]
  host_version	: Host (aka. Motherboard) model version [1.0]
  host_vendor	: Host (aka. Motherboard) model vendor [Micro-Star International Co., Ltd.]
  arch          : the architecture of the machine [x86_64, aarch64]

)"sv);
    std::exit(EXIT_SUCCESS);
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
                    die("config file '{}' doesn't exist", optarg);
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
    const char *optstring = "-VhnLlgNa::f:o:C:i:d:D:p:s:m:";
    static const struct option opts[] = {
        {"version",          no_argument,       0, 'V'},
        {"help",             no_argument,       0, 'h'},
        {"no-display",       no_argument,       0, 'n'},
        {"list-modules",     no_argument,       0, 'l'},
        {"logo-only",        no_argument,       0, 'L'},
        {"gui",              no_argument,       0, 'g'},
        {"no-color",         no_argument,       0, 'N'},
        {"ascii-logo-type",  optional_argument, 0, 'a'},
        {"offset",           required_argument, 0, 'o'},
        {"font",             required_argument, 0, 'f'},
        {"config",           required_argument, 0, 'C'},
        {"layout-line",      required_argument, 0, 'm'},
        {"logo-position",    required_argument, 0, 'p'},
        {"data-dir",         required_argument, 0, 'D'},
        {"distro",           required_argument, 0, 'd'},
        {"source-path",      required_argument, 0, 's'},
        {"image-backend",    required_argument, 0, 'i'},
        
        {"enable-colors",      no_argument,       0, "enable-colors"_fnv1a16},
        {"wrap-lines",         optional_argument, 0, "wrap-lines"_fnv1a16},
        {"sep-reset",          required_argument, 0, "sep-reset"_fnv1a16},
        {"title-sep",          required_argument, 0, "title-sep"_fnv1a16},
        {"sep-reset-after",    optional_argument, 0, "sep-reset-after"_fnv1a16},
        {"logo-padding-top",   required_argument, 0, "logo-padding-top"_fnv1a16},
        {"logo-padding-left",  required_argument, 0, "logo-padding-left"_fnv1a16},
        {"layout-padding-top", required_argument, 0, "layout-padding-top"_fnv1a16},
        {"bg-image",           required_argument, 0, "bg-image"_fnv1a16},
        {"color",              required_argument, 0, "color"_fnv1a16},
        {"gen-config",         optional_argument, 0, "gen-config"_fnv1a16},

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
            case 'n':
                config.m_disable_source = true; break;
            case 'l':
                modules_list(); break;
            case 'f':
                config.font = optarg; break;
            case 'L':
                config.m_print_logo_only = true; break;
            case 'g':
                config.gui = true; break;
            case 'o':
                config.offset = std::stoi(optarg); break;
            case 'C': // we have already did it in parse_config_path()
                break;
            case 'D':
                config.data_dir = optarg; break;
            case 'd':
                config.m_custom_distro = str_tolower(optarg); break;
            case 'm':
                config.m_args_layout.push_back(optarg); break;
            case 'p':
                config.logo_position = optarg; break;
            case 's':
                config.source_path = optarg; break;
            case 'i':
                config.m_image_backend = optarg; break;
            case 'N':
                config.m_disable_colors = true; break;
            case 'a':
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.ascii_logo_type = optarg;
                else
                    config.ascii_logo_type.clear();
                break;

            case "logo-padding-top"_fnv1a16:
                config.logo_padding_top = std::stoi(optarg); break;

            case "logo-padding-left"_fnv1a16:
                config.logo_padding_left = std::stoi(optarg); break;

            case "layout-padding-top"_fnv1a16:
                config.layout_padding_top = std::stoi(optarg); break;

            case "bg-image"_fnv1a16:
                config.gui_bg_image = optarg; break;

            case "wrap-lines"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.wrap_lines = static_cast<bool>(std::stoi(optarg));
                else
                    config.wrap_lines = true;
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
                config.sep_reset = optarg; break;

            case "title-sep"_fnv1a16:
                config.builtin_title_sep = optarg; break;

            case "enable-colors"_fnv1a16:
                config.m_disable_colors = false; break;

            case "sep-reset-after"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.sep_reset_after = static_cast<bool>(std::stoi(optarg));
                else
                    config.sep_reset_after = true;
                break;

            default:
                return false;
        }
    }

    return true;
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

    Config config(configFile, configDir, colors);

    if (!parseargs(argc, argv, config, configFile))
        return 1;

    if (config.source_path.empty() || config.source_path == "off")
        config.m_disable_source = true;

    config.m_display_distro = (config.source_path == "os");

    std::string path = config.m_display_distro ? Display::detect_distro(config) : config.source_path;

    if (!config.ascii_logo_type.empty() && config.m_display_distro)
    {
        std::string logo_type_path{path};
        const size_t pos = path.rfind('.');

        if (pos != std::string::npos)
            logo_type_path.insert(pos, "_" + config.ascii_logo_type);
        else
            logo_type_path += "_" + config.ascii_logo_type;

        if (std::filesystem::exists(logo_type_path))
            path = logo_type_path;
    }

    debug("{} path = {}", __PRETTY_FUNCTION__, path);

    if (!std::filesystem::exists(path) && !config.m_disable_source)
        die("'{}' doesn't exist. Can't load image/text file", path);

#ifdef GUI_MODE
    if (config.gui)
    {
        const auto  app = Gtk::Application::create("org.toni.customfetch");
        GUI::Window window(config, colors, path);
        return app->run(window);
    }
#else
    if (config.gui)
        die("Can't run in GUI mode because it got disabled at compile time\n"
            "Compile customfetch with GUI_MODE=1 or contact your distro to enable it");
#endif

    if (config.wrap_lines)
    {
        // hide cursor and disable line wrapping
        fmt::print("\x1B[?25l\x1B[?7l");
        
        Display::display(Display::render(config, colors, false, path));

        // enable both of them again
        fmt::print("\x1B[?25h\x1B[?7h");
    }
    else
    {
        Display::display(Display::render(config, colors, false, path));
    }

    return 0;
}
