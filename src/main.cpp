#include "display.hpp"
#include "util.hpp"
#include "query.hpp"
#include "config.hpp"

#include <unordered_map>
#include <chrono>
#include <getopt.h>

/** Sets up gettext localization. Safe to call multiple times.
 */
/* Inspired by the monotone function localize_monotone. */
#if defined(ENABLE_NLS)
static void localize(void) {
    static int init = 0;
    if (!init) {
        setlocale(LC_ALL, "");
        bindtextdomain("taur", LOCALEDIR);
        textdomain("taur");
        init = 1;
    }
}
#endif

static void version() {
    fmt::println("customfetch v{} branch {}", VERSION, BRANCH);
    std::exit(0);
}

static void help() {
    fmt::println("here is the help. nah later");
    std::exit(1);
}

static bool parseargs(int argc, char* argv[]) {
    int opt = 0;
    int option_index = 0;
    const char *optstring = "Vhc:C:a";
    static const struct option opts[] =
    {
        {"version",    no_argument,       0, 'V'},
        {"help",       no_argument,       0, 'h'},
        {"colors",     no_argument,       0, 'c'},
        {"config",     required_argument, 0, 'C'},
        {"ascii-art",  required_argument, 0, 'a'},
        {0,0,0,0}
    };

    /* parse operation */
    while ((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1) {
        if (opt == 0)
            continue;
        else if (opt == '?')
            return 1;
        
        switch (opt) {
            case 'V':
                version(); break;
            case 'h':
                help(); break;
            case 'c':
                fmt::disable_colors = true; break;
            case 'C':
                configFile = strndup(optarg, PATH_MAX); break;
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

#if defined(ENABLE_NLS)
    localize();
#endif
    
    std::string configDir = getConfigDir();
    configFile = getConfigDir() + "/config.toml";

    if (!parseargs(argc, argv))
        return 1;
    
    config.init(configFile, configDir);
    
    std::string vendor_id = query_gpu.vendor_id();
    
    std::chrono::seconds uptime_secs(query_system.uptime());
    auto uptime_mins = std::chrono::duration_cast<std::chrono::minutes>(uptime_secs);
    auto uptime_hours = std::chrono::duration_cast<std::chrono::hours>(uptime_secs);

    // json looking ahh
    static systemInfo_t systemInfo = 
    {
        {"os",
            {
                {"name",         VARIANT(query_system.os_name())},
                {"uptime_secs",  VARIANT((size_t)uptime_secs.count()%60)},
                {"uptime_mins",  VARIANT((size_t)uptime_mins.count()%60)},
                {"uptime_hours", VARIANT((size_t)uptime_hours.count()%24)},
                {"kernel_name",  VARIANT(query_system.kernel_name())},
                {"kernel_version", VARIANT(query_system.kernel_version())},
                {"hostname",     VARIANT(query_system.hostname())},
                {"arch",         VARIANT(query_system.arch())},
            }
        },

        {"user",
            {
                {"name", VARIANT(query_system.username())}
            }
        },
        
        {"cpu", 
            {
                {"name", VARIANT(query_cpu.name())},
            }
        }, 
        
        {"gpu", 
            {
                {"name",   VARIANT(query_gpu.name(vendor_id))},
                {"vendor", VARIANT(query_gpu.vendor(vendor_id))}
            }
        },

        {"ram", 
            {
                {"used",  VARIANT(query_ram.used_amount())},
                {"total", VARIANT(query_ram.total_amount())},
                {"free",  VARIANT(query_ram.free_amount())}
            }
        }
    };

    Display::display(Display::render(systemInfo), systemInfo);
    return 0;
}
