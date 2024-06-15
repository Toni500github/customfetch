#include "display.hpp"
#include "util.hpp"
#include "query.hpp"
#include "config.hpp"
#include "gui.hpp"

#include <getopt.h>

static void version() {
    fmt::println("customfetch v{} branch {}", VERSION, BRANCH);
    std::exit(0);
}

static void help(bool invalid_opt = false) {
    fmt::println("Usage: cufetch [OPTION]...");
    fmt::println(R"(
A command-line system information tool (or neofetch like program), which its focus point is customizability and perfomance

    -n, --no-ascii-art		Do not dispay the ascii art
    -a, --ascii-art <path>	Path to the ascii art file to display
    -C, --config <path>		Path to the config file to use
    -g, --gui                   Use GUI mode instead of priting in the terminal (customfetch needs GUI_SUPPORT to be enabled at compile time)
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
  member	: description [e.g example of what it prints]

Should be used in the config as like as $<component.member>

os
  name		: OS name [e.g Windows, Arch]
  kernel_name	: kernel name [e.g Linux]
  kernel_version: kernel version [e.g 6.9.3-zen1-1-zen]
  username	: the user name you are currently logged in (not real name) [e.g toni69]
  uptime_secs	: uptime of the system in seconds (should be used along with uptime_mins and/or uptime_hours) [e.g 45]
  uptime_mins   : uptime of the system in minutes (should be used along with uptime_secs and/or uptime_hours) [e.g 12]
  uptime_hours  : uptime of the system in hours   (should be used along with uptime_mins and/or uptime_secs)  [e.g 34]
  hostname	: hostname of the OS [e.g mymainPC]
  arch		: the architecture of the machine [e.g x86_64, aarch64]

ram
  used		: used amount of RAM (in MB) [e.g 2815]
  free		: available amount of RAM (in MB) [e.g 10456]
  total		: total amount of RAM (in MB) [e.g 15881]

gpu
  name		: GPU model name [e.g NVIDIA GeForce GTX 1650]
  vendor	: GPU vendor [e.g NVIDIA Corporation]

cpu
  name		: CPU model name [e.g AMD Ryzen 5 5500]
)");
    std::exit(0);
}

static bool parseargs(int argc, char* argv[]) {
    int opt = 0;
    int option_index = 0;
    const char *optstring = "VhnlgC:a:";
    static const struct option opts[] =
    {
        {"version",         no_argument,       0, 'V'},
        {"help",            no_argument,       0, 'h'},
        {"no-ascii-art",    no_argument,       0, 'n'},
        {"list-components", no_argument,       0, 'l'},
        {"gui",             no_argument,       0, 'g'},
        {"config",          required_argument, 0, 'C'},
        {"ascii-art",       required_argument, 0, 'a'},
        {0,0,0,0}
    };

    /* parse operation */
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
                config.disable_source = true; break;
            case 'l':
                components_list(); break;
            case 'g':
                config.overrides["config.gui"] = {BOOL, "", true}; break;
            case 'C':
                configFile = strndup(optarg, PATH_MAX); break;
            case 'a':
                config.overrides["config.ascii-art-path"] = {STR, strndup(optarg, PATH_MAX)}; break;
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

    fmt::print("Useless string (input: {}): ", test_1);
    parse(test_1);
    fmt::println("{}", test_1);
    fmt::print("Exec string (input: {}): ", test_2);
    parse(test_2);
    fmt::println("{}", test_2);
    fmt::print("Bypassed exec string #1 (input: {}): ", test_3);
    parse(test_3);
    fmt::println("{}", test_3);
    fmt::print("Bypassed exec string #2 (input: {}): ", test_4);
    parse(test_4);
    fmt::println("{}", test_4);
    fmt::print("Escaped backslash before exec string (input: {}): ", test_5);
    parse(test_5);
    fmt::println("{}", test_5);
    fmt::print("Exec string at start of the string (input: {}): ", test_6);
    parse(test_6);
    fmt::println("{}", test_6);
#endif

#ifdef VENDOR_TEST
    // test
    fmt::println("=== VENDOR TEST! ===");
    
    fmt::println("Intel: {}", binarySearchPCIArray("8086"));
    fmt::println("AMD: {}", binarySearchPCIArray("1002"));
    fmt::println("NVIDIA: {}", binarySearchPCIArray("10de"));
#endif

    std::string configDir = getConfigDir();
    configFile = getConfigDir() + "/config.toml";

    if (!parseargs(argc, argv))
        return 1;
    
    config.init(configFile, configDir);

    if (config.source_path.empty())
        config.disable_source = true;

    pci_init(pac.get());

#ifdef GUI_SUPPORT
    if (config.gui) {
        auto app = Gtk::Application::create("org.toni.customfetch");
        GUI::Window window;
        return app->run(window);
    }
    else 
        Display::display(Display::render());
#else
    if (config.gui) die("Can't run in GUI mode because it got disabled at compile time\nCompile customfetch with GUI_SUPPORT=1 or contact your distro to enable it");
    Display::display(Display::render());
#endif

    return 0;
}
