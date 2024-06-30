#include "display.hpp"
#include "util.hpp"
#include "parse.hpp"
#include "config.hpp"
#include "gui.hpp"

#include <getopt.h>

static void version() {
    fmt::println("customfetch v{} branch {}", VERSION, BRANCH);

#ifdef GUI_SUPPORT
    fmt::println("GUI support enabled");
#else
    fmt::println("GUI support IS NOT enabled");
#endif

    std::exit(0);
}

static void help(bool invalid_opt = false) {
    fmt::println("Usage: cufetch [OPTION]...");
    fmt::println(R"(
A command-line system information tool (or neofetch like program), which its focus point is customizability and perfomance

    -n, --no-display		Do not display the ascii art
    -s, --source-path <path>	Path to the ascii art file to display
    -C, --config <path>		Path to the config file to use
    -d, --distro <name>         Print a custom distro logo (must be the same name, uppercase or lowercase)
    -g, --gui                   Use GUI mode instead of priting in the terminal (use -V to check if it's enabled)
    -l. --list-components	Print the list of the components and its members
    -h, --help			Print this help menu
    -V, --version		Print the version along with the git branch it was built

Read the "README.md" file for more infos about customfetch and how it works
)");
    std::exit(invalid_opt);
}

static void components_list() {
    fmt::println(R"(
Syntax:
component
  member	: description [example of what it prints, maybe another]

Should be used in the config as like as $<component.member>

os
  name		: OS name (pretty_name) [Ubuntu 22.04.4 LTS, Arch Linux]
  kernel_name	: kernel name [Linux]
  kernel_version: kernel version [6.9.3-zen1-1-zen]
  version_id	: OS version id [22.04.4, 20240101.0.204074]
  version_codename: OS version codename [jammy]
  username	: the user name you are currently logged in (not real name) [toni69]
  uptime_secs	: uptime of the system in seconds (should be used along with uptime_mins and/or uptime_hours) [45]
  uptime_mins   : uptime of the system in minutes (should be used along with uptime_secs and/or uptime_hours) [12]
  uptime_hours  : uptime of the system in hours   (should be used along with uptime_mins and/or uptime_secs)  [34]
  hostname	: hostname of the OS [mymainPC]

user
  name		: name you are currently logged in (not real name) [toni69]
  shell_name	: login shell [zsh]
  shell_path	: login shell (with path) [/bin/zsh]
  shell_version : login shell version (may be not correct) [5.9]

ram
  used		: used amount of RAM (in MB) [2815]
  free		: available amount of RAM (in MB) [10456]
  total		: total amount of RAM (in MB) [15881]

disk(/path/to/fs)
  used          : used amount of disk space (in GB) [360.02]
  free          : available amount of disk space (in GB) [438.08]
  total         : total amount of disk space (in GB) [100.08]
  fs            : type of filesystem [ext4]

gpu
  name		: GPU model name [NVIDIA GeForce GTX 1650]
  vendor	: GPU vendor [NVIDIA Corporation]

cpu
  name		: CPU model name [AMD Ryzen 5 5500]
  nproc         : CPU number of virtual proccessors [12]
  freq_bios_limit: CPU freq (limited by bios, in GHz) [4.32]
  freq_cur	: CPU freq (current, in GHz) [3.42]
  freq_min	: CPU freq (mininum, in GHz) [2.45]
  freq_max	: CPU freq (maxinum, in GHz) [4.90]

system
  host_name	: Host (aka. Motherboard) model name [PRO B550M-P GEN3 (MS-7D95)]
  host_version	: Host (aka. Motherboard) model version [1.0]
  host_vendor	: Host (aka. Motherboard) model vendor [Micro-Star International Co., Ltd.]
  arch          : the architecture of the machine [x86_64, aarch64]

)");
    std::exit(0);
}

// parseargs() but only for parsing the user config path trough args
// and so we can directly construct Config
static bool parse_config_path(int argc, char* argv[], std::string& configFile) {
    int opt = 0;
    int option_index = 0;
    opterr = 0;
    const char *optstring = "C:";
    static const struct option opts[] =
    {
        {"config", required_argument, 0, 'C'},
        {0,0,0,0}
    };
    
    while ((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1) {
        if (opt == 0 || opt == '?')
            continue;

        switch (opt) {
            case 'C':
                configFile = strndup(optarg, PATH_MAX); 
                if (!std::filesystem::exists(configFile))
                    die("config file '{}' doesn't exist", configFile);

                break;
            default:
                return false;
        }
    }

    return true;
}

static bool parseargs(int argc, char* argv[], Config& config) {
    int opt = 0;
    int option_index = 0;
    opterr = 1; // re-enable since before we disabled for "invalid option" error
    const char *optstring = "VhnlgC:d:s:";
    static const struct option opts[] =
    {
        {"version",         no_argument,       0, 'V'},
        {"help",            no_argument,       0, 'h'},
        {"no-display",      no_argument,       0, 'n'},
        {"list-components", no_argument,       0, 'l'},
        {"gui",             no_argument,       0, 'g'},
        {"config",          required_argument, 0, 'C'},
        {"distro",          required_argument, 0, 'd'},
        {"source-path",     required_argument, 0, 's'},
        {0,0,0,0}
    };

    /* parse operation */
    optind = 0;
    while ((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1) {
        if (opt == 0)
            continue;
        else if (opt == '?')
            help(1);

        switch (opt) {
            case 'V':
                version(); break;
            case 'h':
                help(); break;
            case 'n':
                config.m_disable_source = true; break;
            case 'l':
                components_list(); break;
            case 'g':
                config.gui = true; break;
            case 'C': // we have already did it in parse_config_path()
                continue;
            case 'd':
                config.m_custom_distro = str_tolower(optarg); break;
            case 's':
                config.source_path = optarg; break;
            default:
                return false;
        }
    }

    return true;
}

int main (int argc, char *argv[]) {
#ifdef PARSER_TEST
    // test
    fmt::println("=== PARSER TEST! ===");

    std::string test_1 = "Hello, World!";
    std::string test_2 = "Hello, $(echo \"World\")!";
    std::string test_3 = "Hello, \\$(echo \"World\")!";
    std::string test_4 = "Hello, $\\(echo \"World\")!";
    std::string test_5 = "Hello, \\\\$(echo \"World\")!";
    std::string test_6 = "$(echo \"World\")!";
    systemInfo_t systemInfo;
    std::unique_ptr<std::string> pureOutput = std::make_unique<std::string>();
    std::string clr = "#d3dae3";

    fmt::print("Useless string (input: {}): ", test_1);
    parse(test_1, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_1);
    
    fmt::print("Exec string (input: {}): ", test_2);
    parse(test_2, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_2);
    
    fmt::print("Bypassed exec string #1 (input: {}): ", test_3);
    parse(test_3, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_3);
    
    fmt::print("Bypassed exec string #2 (input: {}): ", test_4);
    parse(test_4, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_4);
    
    fmt::print("Escaped backslash before exec string (input: {}): ", test_5);
    parse(test_5, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_5);
    
    fmt::print("Exec string at start of the string (input: {}): ", test_6);
    parse(test_6, systemInfo, pureOutput, clr);
    fmt::println("\t{}", test_6);
#endif

#ifdef VENDOR_TEST
    // test
    fmt::println("=== VENDOR TEST! ===");
    
    fmt::println("Intel: {}", binarySearchPCIArray("8086"));
    fmt::println("AMD: {}", binarySearchPCIArray("1002"));
    fmt::println("NVIDIA: {}", binarySearchPCIArray("10de"));
#endif
    
    struct colors_t colors;

    std::string configDir = getConfigDir();
    std::string configFile = configDir + "/config.toml";    
    parse_config_path(argc, argv, configFile);
    
    Config config(configFile, configDir, colors);

    if (!parseargs(argc, argv, config))
        return 1;

    if ( config.source_path.empty() || config.source_path == "off" )
        config.m_disable_source = true;
    
    if (config.source_path == "os")
        config.m_display_distro = true;
    else
        config.m_display_distro = false;

#ifdef GUI_SUPPORT
    if (config.gui) {
        auto app = Gtk::Application::create("org.toni.customfetch");
        GUI::Window window(config, colors);
        return app->run(window);
    }
#else
    if (config.gui) 
        die("Can't run in GUI mode because it got disabled at compile time\nCompile customfetch with GUI_SUPPORT=1 or contact your distro to enable it");
#endif

    Display::display(Display::render(config, colors));

    return 0;
}
