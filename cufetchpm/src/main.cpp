#include <cstdlib>

#include "libcufetch/common.hh"
#include "pluginManager.hpp"
#include "stateManager.hpp"

#if (!__has_include("version.h"))
#error "version.h not found, please generate it with ./scripts/generateVersion.sh"
#else
#include "version.h"
#endif

#include "getopt_port/getopt.h"

enum Ops
{
    NONE,
    INSTALL,
    LIST
} op = NONE;

static std::vector<std::string> arguments;

void version()
{
    fmt::print(
        "cufetchpm {} built from branch '{}' at {} commit '{}' ({}).\n"
        "Date: {}\n"
        "Tag: {}\n",
        VERSION, GIT_BRANCH, GIT_DIRTY, GIT_COMMIT_HASH, GIT_COMMIT_MESSAGE, GIT_COMMIT_DATE, GIT_TAG);

    // if only everyone would not return error when querying the program version :(
    std::exit(EXIT_SUCCESS);
}

void help(int invalid_opt = false)
{
    fmt::print(R"(Usage: cufetchpm <command> [options]
Manage plugins for customfetch.
NOTE: the operations must be the first argument to pass

Commands:
    install [options] <repo>    Install a new plugin repository. Takes as an argument the git url to be cloned.

Global options:
    -h, --help          Print this help menu.
    -V, --version       Print version and other infos about the build.

)");

    std::exit(invalid_opt);
}

void help_install(int invalid_opt = false)
{
    fmt::print(R"(Usage: cufetchpm install [options] <repo>

Options:
  -f, --force        Force installation
  -h, --help         Show help for install
)");

    std::exit(invalid_opt);
}

void help_list(int invalid_opt = false)
{
    fmt::print(R"(Usage: cufetchpm list [options]

Options:
  -v, --verbose      Show detailed info
  -h, --help         Show help for list
)");

    std::exit(invalid_opt);
}

bool parse_install_args(int argc, char* argv[])
{
    bool force = false;

    // clang-format off
    const struct option long_opts[] = {
        {"force", no_argument, nullptr, 'f'},
        {"help",  no_argument, nullptr, 'h'},
        {0, 0, 0, 0}
    };
    // clang-format on

    int opt;
    while ((opt = getopt_long(argc, argv, "-fh", long_opts, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'f': force = true; break;
            case 'h': help_install(EXIT_SUCCESS); break;
            case '?': help_install(EXIT_FAILURE); break;
        }
    }

    for (int i = optind; i < argc; ++i)
        arguments.emplace_back(argv[i]);

    if (arguments.empty())
        die("install: no repositories given");

    return true;
}

bool parse_list_args(int argc, char* argv[])
{
    bool verbose = false;

    // clang-format off
    const struct option long_opts[] = {
        {"verbose", no_argument, nullptr, 'v'},
        {"help",    no_argument, nullptr, 'h'},
        {0, 0, 0, 0}
    };
    // clang-format on

    int opt;
    while ((opt = getopt_long(argc, argv, "-vh", long_opts, nullptr)) != -1)
    {
        switch (opt)
        {
            case 'v': verbose = true; break;
            case 'h': help_list(EXIT_SUCCESS); break;
            case '?': help_list(EXIT_FAILURE); break;
        }
    }

    return true;
}

static bool parseargs(int argc, char* argv[])
{
    // clang-format off
    int opt = 0;
    int option_index = 0;
    const char *optstring = "-Vh";
    static const struct option opts[] = {
        {"version", no_argument, 0, 'V'},
        {"help",    no_argument, 0, 'h'},

        {0,0,0,0}
    };

    // clang-format on
    optind = 1;
    while ((opt = getopt_long(argc, argv, optstring, opts, &option_index)) != -1)
    {
        switch (opt)
        {
            case 0:   break;
            case '?': help(EXIT_FAILURE); break;

            case 'V': version(); break;
            case 'h': help(); break;
            default:  return false;
        }
    }

    if (optind >= argc)
        help(EXIT_FAILURE);  // no subcommand

    std::string_view cmd      = argv[optind];
    int              sub_argc = argc - optind - 1;
    char**           sub_argv = argv + optind + 1;
    if (cmd == "install" || cmd == "i")
    {
        op     = INSTALL;
        optind = 0;
        return parse_install_args(sub_argc, sub_argv);
    }
    else if (cmd == "list" || cmd == "l")
    {
        op     = LIST;
        optind = 0;
        return parse_list_args(sub_argc, sub_argv);
    }
    else if (cmd == "help")
    {
        if (sub_argc >= 1)
        {
            std::string_view target = sub_argv[0];
            if (target == "install")
                help_install();
            else if (target == "list")
                help_list();
            else
                die("Unknown subcommand '{}'", cmd);
        }
        else
        {
            help(EXIT_FAILURE);
        }
    }

    return true;
}

int main(int argc, char* argv[])
{
    if (!parseargs(argc, argv))
        return -1;

    fs::create_directories({ getHomeCacheDir() / "cufetchpm" / "plugins" });
    switch (op)
    {
        case INSTALL:
        {
            if (arguments.size() < 1)
                die("Please provide a singular git url repository");
            StateManager  state;
            PluginManager plugin_manager(std::move(state));
            plugin_manager.add_repo_plugins(arguments[0]);
            break;
        }
        default: warn("uh?");
    }

    return 0;
}
