#include <getopt.h>

#include "pluginManager.hpp"
#include "stateManager.hpp"
#if (!__has_include("version.h"))
#error "version.h not found, please generate it with ./scripts/generateVersion.sh"
#else
#include "version.h"
#endif

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

    -h, --help          Print this help menu.
    -V, --version       Print version and other infos about the build.

OPERATIONS:
    add - Add a new plugin repository. Takes as an argument the git url to be cloned.
)");

    fmt::print("{}", help);
    fmt::print("\n");
    std::exit(invalid_opt);
}

static bool parseargs(int argc, char* argv[])
{
    int opt = 0;
    int option_index = 0;
    const char *optstring = "-Vh";
    static const struct option opts[] = {
        {"version", no_argument, 0, 'V'},
        {"help",    no_argument, 0, 'h'},

        {0,0,0,0}
    };

    /* parse operation */
    optind = 0;
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

    return true;
}

int main(int argc, char* argv[])
{
    if (!parseargs(argc, argv))
        return -1;

    std::filesystem::create_directories({ getHomeCacheDir() / "cufetchpm" / "plugins" });
    StateManager  state;
    PluginManager man(std::move(state));
    man.add_repo_plugins(argv[2]);
    return 0;
}
