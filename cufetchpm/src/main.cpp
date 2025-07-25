#include <cstring>
#include "libcufetch/common.hh"
#include "libs/switch_fnv1a.hpp"
#include "pluginManager.hpp"
#include "stateManager.hpp"

#if (!__has_include("version.h"))
#error "version.h not found, please generate it with ./scripts/generateVersion.sh"
#else
#include "version.h"
#endif

#include "getopt_port/getopt.h"

enum {
    INSTALL,
    REMOVE,
    LIST
};

static struct operations_t
{
    int name;
    std::vector<std::string> args;
} op;

static void version()
{
    fmt::print(
        "cufetchpm {} built from branch '{}' at {} commit '{}' ({}).\n"
        "Date: {}\n"
        "Tag: {}\n",
        VERSION, GIT_BRANCH, GIT_DIRTY, GIT_COMMIT_HASH, GIT_COMMIT_MESSAGE, GIT_COMMIT_DATE, GIT_TAG);

    // if only everyone would not return error when querying the program version :(
    std::exit(EXIT_SUCCESS);
}

static void help(int invalid_opt = false)
{
    constexpr std::string_view help(
        R"(Usage: cufetchpm [OPERATION] [ARGUMENTS] [OPTIONS]...
Manage plugins for customfetch.
NOTE: the operations must be the first argument to pass

OPERATIONS:
    install - Install a new plugin repository. Takes as an argument the git url to be cloned.

GENERAL OPTIONS
    -h, --help          Print this help menu.
    -V, --version       Print version and other infos about the build.

)");

    fmt::print("{}", help);
    fmt::print("\n");
    std::exit(invalid_opt);
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
    for (int i = 1; i < argc; ++i)
    {
        if (strncmp(argv[i], "install", 7) == 0 || strncmp(argv[i], "i", 1) == 0)
        {
            op.name = INSTALL;
            optind++;
            break;
        }
        if (strncmp(argv[i], "list", 4) == 0|| strncmp(argv[i], "l", 1) == 0)
        {
            op.name = LIST;
            optind++;
            break;
        }
        if (strncmp(argv[i], "remove", 6) == 0 || strncmp(argv[i], "r", 1) == 0)
        {
            op.name = REMOVE;
            optind++;
            break;
        }
    }

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

    for (int i = optind; i < argc; ++i)
        op.args.push_back(argv[i]);

    return true;
}

int main(int argc, char* argv[])
{
    if (!parseargs(argc, argv))
        return -1;

    fs::create_directories({ getHomeCacheDir() / "cufetchpm" / "plugins" });
    StateManager  state;
    PluginManager plugin_manager(std::move(state));
    switch (op.name)
    {
        case INSTALL: 
            {
                if (op.args.size() != 1)
                    die("Please provide a singular git url repository");
                plugin_manager.add_repo_plugins(op.args[0]); break;
            }
        default:
            warn("Not yet implemented");
    }

    return 0;
}
