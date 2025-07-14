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
#include <termios.h>
#include <stdlib.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "core-modules.hh"
#include "display.hpp"
#include "fmt/base.h"
#include "fmt/ranges.h"
#include "gui.hpp"
#include "platform.hpp"
#include "switch_fnv1a.hpp"
#include "util.hpp"

#if (!__has_include("version.h"))
#error "version.h not found, please generate it with ./scripts/generateVersion.sh"
#else
#include "version.h"
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

bool display_modules    = false;
bool display_list_logos = false;

struct termios orig_termios;

static void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);  // Disable echo and canonical mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static int kbhit()
{
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

// Print the version and some other infos, then exit successfully
static void version()
{
    std::string version{ fmt::format(
        "customfetch {} built from branch '{}' at {} commit '{}' ({}).\n"
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

    fmt::print("{}", version);
    fmt::print("\n");

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

    fmt::print("{}", help);
    fmt::print("\n");
    std::exit(invalid_opt);
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

    Syntax: $<module.submodule.sub...> or $<module>

    Examples:
    - $<user.name>       # Displays login username
    - $<os.kernel.name>  # Shows kernel name only
    - $<ram>             # Shows formatted RAM usage

    Use `--list-modules` to see all available modules and members.

2. Bash Command Tag ($())
    Executes shell commands and outputs the result.
    Supports full shell syntax including pipes and redirection.

    Syntax: $(command)

    Examples:
    - $(echo "hello")             # Outputs: hello
    - $(date +%F)                 # Shows current date
    - $(uname -r | cut -d'-' -f1) # Shows kernel version number only

3. Conditional Tag ($[])
    Displays different outputs based on conditions.
    
    Syntax: $[condition,comparison,true_output,false_output]
    
    Examples:
    - $[$<user.name>,toni,Welcome back!,Access denied]
    - $[$(date +%m-%d),12-25,Merry Christmas!,]
    - $[$<os.name.id>,arch,${green}I use arch btw,${red}Non-arch user]

4. Color Tag (${})
    Applies colors and text formatting.
    
    Basic syntax: ${color} or ${modifiers#RRGGBB}

    Color options:
    - Named colors from config
    - Hex colors: ${#ff00cc}
    - Special colors: ${auto} (uses logo colors)
    - Reset styles: ${0} (normal), ${1} (bold reset)

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

// Print a modules list of ascii logos you can use at a "data-dir"
// @param data_dir The data directory
static void list_logos(const std::string& data_dir)
{
    debug("data-dir = {}", data_dir);
    if (access(data_dir.c_str(), F_OK) != 0)
        die("failed to access data directory '{}'", data_dir);

    std::vector<std::string> list;
    for (const auto& logo : std::filesystem::directory_iterator{ data_dir })
    {
        if (logo.is_regular_file())
            list.push_back(logo.path().stem());
    }

    std::sort(list.begin(), list.end());

    fmt::print("{}", fmt::join(list, "\n"));
    fmt::print("\n");
}

// Print all info modules you can put in $<>, then exit successfully
static void modules_list()
{
    for (const module_t& module : cfGetModules())
    {
        std::vector<std::string> parts;

        // Split name into parts (e.g., "os.name.pretty" -> ["os", "name", "pretty"])
        size_t start = 0, end = module.name.find('.');
        bool   new_module = true;
        while (end != std::string::npos)
        {
            new_module = false;
            parts.push_back(module.name.substr(start, end - start));
            start = end + 1;
            end   = module.name.find('.', start);
        }
        parts.push_back(module.name.substr(start));
        if (new_module)
            fmt::print("\n");

        // Generate indentation
        for (size_t depth = 0; depth < parts.size(); ++depth)
        {
            if (depth == parts.size() - 1)
            {
                if (new_module)
                    fmt::print("{} - {}", parts[depth], module.description);
                else
                    fmt::print("{:<6} \t- {}", parts[depth], module.description);
            }
            else
            {
                fmt::print("  ");
            }
        }

        fmt::print("\n");
    }
}

// clang-format off
// Return true if optarg says something true
static bool str_to_bool(const std::string_view str)
{
    return (str == "true" || str == "1" || str == "enable");
}

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

static bool parseargs(int argc, char* argv[], Config& config, const std::filesystem::path& configFile)
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
        {"gtk-css",              required_argument, 0, "gtk-css"_fnv1a16},
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
                display_modules = true; break;
            case 'w':
                explain_how_this_works(); break;
            case "list-logos"_fnv1a16:
                display_list_logos = true; break;
            case 'f':
                config.overrideOption("gui.font", optarg); break;
            case 'o':
                config.overrideOption("config.offset", optarg); break;
            case 'C': // we have already did it in parse_config_path()
                break;
            case 'D':
                config.overrideOption("config.data-dir", optarg); break;
            case 'd':
                config.args_custom_distro = str_tolower(optarg); break;
            case 'm':
                config.args_layout.push_back(optarg); break;
            case 'p':
                config.overrideOption("config.logo-position", optarg); break;
            case 's':
                config.overrideOption("config.source-path", optarg); break;
            case 'i':
                config.args_image_backend = optarg; break;
            case 'O':
                config.overrideOption(optarg); break;
            case 'N':
                config.args_disable_colors = true; break;
            case 'a':
                config.overrideOption("config.ascii-logo-type", optarg); break;
            case 'n':
                config.args_disable_source = true; break;
            case 'L':
                config.args_print_logo_only = true; break;

            case "disallow-command-tag"_fnv1a16:
                config.args_disallow_commands = true; break;

            case "logo-padding-top"_fnv1a16:
                config.overrideOption("config.logo-padding-top", std::stoi(optarg)); break;

            case "logo-padding-left"_fnv1a16:
                config.overrideOption("config.logo-padding-left", std::stoi(optarg)); break;

            case "layout-padding-top"_fnv1a16:
                config.overrideOption("config.layout-padding-top", std::stoi(optarg)); break;

            case "loop-ms"_fnv1a16:
                config.loop_ms = std::stoul(optarg); break;

            case "color"_fnv1a16:
                config.addAliasColors(optarg); break;

            case "sep-reset"_fnv1a16:
                config.overrideOption("config.sep-reset", optarg); break;

            case "title-sep"_fnv1a16:
                config.overrideOption("config.title-sep", optarg); break;

            case "bg-image"_fnv1a16:
                config.overrideOption("gui.bg-image", optarg); break;

            case "gtk-css"_fnv1a16:
                config.overrideOption("gui.gtk-css", optarg); break;

            case "wrap-lines"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.overrideOption("config.wrap-lines", str_to_bool(optarg));
                else
                    config.overrideOption("config.wrap-lines", true);
                break;

            case "debug"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    debug_print = str_to_bool(optarg);
                else
                    debug_print = true;
                break;

            case "gen-config"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.generateConfig(optarg);
                else
                    config.generateConfig(configFile);
                exit(EXIT_SUCCESS);

            case "sep-reset-after"_fnv1a16:
                if (OPTIONAL_ARGUMENT_IS_PRESENT)
                    config.overrideOption("config.sep-reset-after", str_to_bool(optarg));
                else
                    config.overrideOption("config.sep-reset-after", true);
                break;

            default:
                return false;
        }
    }

    config.overrideOption("intern.args.print-logo-only",      config.args_print_logo_only);
    config.overrideOption("intern.args.disable-logo",         config.args_disable_source);
    config.overrideOption("intern.args.disallow-commands",    config.args_disallow_commands);
    config.overrideOption("intern.args.custom-distro",        config.args_custom_distro);
    config.overrideOption("intern.args.image-backend",        config.args_image_backend);
    config.overrideOption("intern.args.disable-colors",       config.args_disable_colors);

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

// clang-format on
int main(int argc, char* argv[])
{
    const std::filesystem::path& configDir  = getConfigDir();
    const std::filesystem::path& configFile = parse_config_path(argc, argv, configDir);

    localize();

    Config config(configFile, configDir);
    if (!parseargs(argc, argv, config, configFile))
        return 1;
    config.loadConfigFile(configFile);
    std::vector<void*> plugins_handle;

    /* TODO(burntranch): track each library and unload them. */
    core_plugins_start(config);
    const std::filesystem::path plguinDir = configDir / "plugins";
    std::filesystem::create_directories(plguinDir);
    for (const auto& entry : std::filesystem::directory_iterator{ plguinDir })
    {
        debug("loading plugin at {}!", entry.path().string());

        void* handle = LOAD_LIBRARY(std::filesystem::absolute(entry.path()).c_str());
        if (!handle)
        {
            // dlerror() is pretty formatted
            warn("Failed to load plugin at {}: {}", entry.path().string(), dlerror());
            dlerror();
            continue;
        }

        LOAD_LIB_SYMBOL(handle, void, start, void*, const ConfigBase&);
        if (dlerror())
        {
            warn("Failed to load plugin at {}: Missing function 'start'", entry.path().string());
            dlclose(handle);
            continue;
        }

        start(handle, config);
        plugins_handle.push_back(handle);
    }

    if (display_modules)
    {
        modules_list();
        return 0;
    }
    else if (display_list_logos)
    {
        list_logos(config.data_dir + "/ascii");
        return 0;
    }

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

    is_live_mode = (config.loop_ms >= 200);

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
    GUI::Window window(config, path, moduleMap);
    return app->run(window);
#endif  // GUI_APP

    if (!config.wrap_lines)
    {
        // https://en.cppreference.com/w/c/program/exit
        std::atexit(enable_cursor);

        // hide cursor and disable line wrapping
        fmt::print("\x1B[?25l\x1B[?7l");
    }

    if (is_live_mode)
    {
        enable_raw_mode();
        const std::chrono::milliseconds sleep_ms{ config.loop_ms };

        while (true)
        {
            if (kbhit())
            {
                char c;
                read(STDIN_FILENO, &c, 1);
                if (c == 'q' || c == 'Q')
                {
                    info("exiting...\n");
                    disable_raw_mode();
                    break;
                }
            }

            // clear screen and go to position 0, 0
            write(STDOUT_FILENO, "\33[H\33[2J", 7);
            fmt::print("\033[0;0H");

            Display::display(Display::render(config, false, path, moduleMap));
            std::this_thread::sleep_for(sleep_ms);
        }
    }
    else
    {
        Display::display(Display::render(config, false, path, moduleMap));
    }

    // enable both of them again
    if (!config.wrap_lines)
        enable_cursor();

    core_plugins_finish();
    for (void* handle : plugins_handle)
    {
        LOAD_LIB_SYMBOL(handle, void, finish, void*);
        if (dlerror())
        {
            dlclose(handle);
            continue;
        }

        finish(handle);
        UNLOAD_LIBRARY(handle);
    }
    plugins_handle.clear();

    return 0;
}
